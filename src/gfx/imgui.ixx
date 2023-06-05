export module minote.imgui;

import <algorithm>;
import <cstring>;
import <utility>;
import <memory>;
import <array>;
import <cmath>;
import <span>;
import <backends/imgui_impl_glfw.h>;
import <imgui.h>;
import <vuk/CommandBuffer.hpp>;
import <vuk/RenderGraph.hpp>;
import <vuk/Allocator.hpp>;
import <vuk/Pipeline.hpp>;
import <vuk/Partials.hpp>;
import <vuk/Future.hpp>;
import <vuk/Image.hpp>;
import minote.samplers;
import minote.vulkan;
import minote.window;
import minote.types;
import minote.math;

export class Imgui:
	Window,
	Vulkan
{
public:
	explicit Imgui(vuk::Allocator& allocator) {
		// Create Imgui basic context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui::StyleColorsDark();
		ImGui_ImplGlfw_InitForVulkan(Window::serv->handle(), true);

		// Configure imgui renderer
		auto& io = ImGui::GetIO();
		io.BackendRendererName = "imgui_impl_vuk";
		io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset; // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.

		// Retrieve font bitmap
		auto* fontPixels = static_cast<unsigned char*>(nullptr);
		auto fontSize = ivec2();
		io.Fonts->GetTexDataAsRGBA32(&fontPixels, &fontSize.x(), &fontSize.y());

		// Create font texture
		auto [fontTex, fontFut] = vuk::create_texture(allocator,
			vuk::Format::eR8G8B8A8Srgb,
			vuk::Extent3D{uint(fontSize.x()), uint(fontSize.y()), 1u},
			fontPixels, false
		);
		font = std::move(fontTex);
		auto comp = vuk::Compiler();
		fontFut.wait(allocator, comp);
		Vulkan::serv->context.set_name(font, "imgui/font");
		// io.Fonts->TexID = static_cast<ImTextureID>(&m_font.view.get());

		// Compile shader
		auto imguiPci = vuk::PipelineBaseCreateInfo();
		constexpr auto imguiVertSource = std::to_array<uint>({
#include "spv/imgui.vert.spv"
			});
		constexpr auto imguiFragSource = std::to_array<uint>({
#include "spv/imgui.frag.spv"
			});
		imguiPci.add_static_spirv(imguiVertSource.data(), imguiVertSource.size(), "imgui.vert");
		imguiPci.add_static_spirv(imguiFragSource.data(), imguiFragSource.size(), "imgui.frag");
		Vulkan::serv->context.create_named_pipeline("imgui", imguiPci);
	}

	~Imgui() { ImGui_ImplGlfw_Shutdown(); }

	void begin(uvec2 size) {
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}

	auto render(vuk::Allocator& allocator, vuk::Future target) -> vuk::Future {
		// Finalize the frame
		ImGui::Render();
		auto* drawdata = ImGui::GetDrawData();

		// Fill up the vertex and index buffers
		auto vertexSize = drawdata->TotalVtxCount * sizeof(ImDrawVert);
		auto  indexSize = drawdata->TotalIdxCount * sizeof(ImDrawIdx);
		auto imvert = *vuk::allocate_buffer(allocator, {vuk::MemoryUsage::eCPUtoGPU, vertexSize});
		auto imind  = *vuk::allocate_buffer(allocator, {vuk::MemoryUsage::eCPUtoGPU, indexSize});

		auto vtxDst = 0_zu;
		auto idxDst = 0_zu;
		for (auto* list: std::span(drawdata->CmdLists, drawdata->CmdListsCount)) {
			auto imverto = imvert->add_offset(vtxDst * sizeof(ImDrawVert));
			auto  imindo =  imind->add_offset(idxDst * sizeof(ImDrawIdx));
			std::memcpy(imverto.mapped_ptr, list->VtxBuffer.Data, list->VtxBuffer.Size * sizeof(ImDrawVert));
			std::memcpy(imindo.mapped_ptr, list->IdxBuffer.Data, list->IdxBuffer.Size * sizeof(ImDrawIdx));
			vtxDst += list->VtxBuffer.Size;
			idxDst += list->IdxBuffer.Size;
		}

		auto rg = std::make_shared<vuk::RenderGraph>("imgui");
		rg->attach_in("input", std::move(target));

		// Execute all imgui draws in this pass
		rg->add_pass(vuk::Pass{
			.name = "imgui",
			.resources = {
				"input"_image >> vuk::eColorRW >> "output",
			},
			.execute = [this, imvert = imvert.get(), imind = imind.get(), drawdata](auto& cmd) {

				cmd.bind_graphics_pipeline("imgui")
					.set_dynamic_state(vuk::DynamicStateFlagBits::eViewport | vuk::DynamicStateFlagBits::eScissor)
					.set_viewport(0, vuk::Rect2D::framebuffer())
					.set_rasterization({})
					.broadcast_color_blend(vuk::BlendPreset::eAlphaBlend);

				cmd.bind_image(0, 0, *font.view).bind_sampler(0, 0, TrilinearRepeat);
				constexpr auto ImguiIndexSize =
					sizeof(ImDrawIdx) == 2?
					vuk::IndexType::eUint16 :
					vuk::IndexType::eUint32;
				if (imind.size > 0)
					cmd.bind_index_buffer(imind, ImguiIndexSize);
				cmd.bind_vertex_buffer(0, imvert, 0, vuk::Packed{
					vuk::Format::eR32G32Sfloat,
					vuk::Format::eR32G32Sfloat,
					vuk::Format::eR8G8B8A8Unorm,
				});

				struct Constants {
					vec2 scale;
					vec2 translate;
				};
				auto scale = vec2{
					2.0f / drawdata->DisplaySize.x,
					2.0f / drawdata->DisplaySize.y,
				};
				auto constants = Constants{
					.scale = scale,
					.translate = vec2{-1.0f, -1.0f} - vec2{drawdata->DisplayPos.x, drawdata->DisplayPos.y} * scale,
				};
				cmd.push_constants(vuk::ShaderStageFlagBits::eVertex, 0, constants);

				// Will project scissor/clipping rectangles into framebuffer space
				auto clipOff = drawdata->DisplayPos;         // (0,0) unless using multi-viewports
				auto clipScale = drawdata->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

				// Render command lists
				// (Because we merged all buffers into a single one, we maintain our own offset into them)
				auto globalVtxOffset = 0;
				auto globalIdxOffset = 0;
				for (auto* list: std::span(drawdata->CmdLists, drawdata->CmdListsCount)) {
					for (auto& pcmd: list->CmdBuffer) {
						// Project scissor/clipping rectangles into framebuffer space
						auto clipRect = vec4{
							(pcmd.ClipRect.x - clipOff.x) * clipScale.x,
							(pcmd.ClipRect.y - clipOff.y) * clipScale.y,
							(pcmd.ClipRect.z - clipOff.x) * clipScale.x,
							(pcmd.ClipRect.w - clipOff.y) * clipScale.y,
						};
						auto fbWidth  = cmd.get_ongoing_renderpass().extent.width;
						auto fbHeight = cmd.get_ongoing_renderpass().extent.height;
						if (clipRect.x() >= fbWidth || clipRect.y() >= fbHeight ||
							clipRect.z() < 0.0f || clipRect.w() < 0.0f) continue;

						// Negative offsets are illegal for vkCmdSetScissor
						clipRect.x() = std::max(clipRect.x(), 0.0f);
						clipRect.y() = std::max(clipRect.y(), 0.0f);
						cmd.set_scissor(0, vuk::Rect2D{
							.offset = {int(clipRect.x()), int(clipRect.y())},
							.extent = {uint(clipRect.z() - clipRect.x()), uint(clipRect.w() - clipRect.y())},
						});

						// Draw
						cmd.draw_indexed(pcmd.ElemCount, 1, pcmd.IdxOffset + globalIdxOffset, pcmd.VtxOffset + globalVtxOffset, 0);
					}
					globalIdxOffset += list->IdxBuffer.Size;
					globalVtxOffset += list->VtxBuffer.Size;
				}
			}
		});

		return vuk::Future(std::move(rg), "output");
	}

private:
	vuk::Texture font;
};
