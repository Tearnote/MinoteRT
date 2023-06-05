export module minote.samplers;

import <vuk/Image.hpp>;

// Commonly used sampler presets.

export constexpr auto NearestClamp = vuk::SamplerCreateInfo{
	.magFilter = vuk::Filter::eNearest,
	.minFilter = vuk::Filter::eNearest,
	.addressModeU = vuk::SamplerAddressMode::eClampToEdge,
	.addressModeV = vuk::SamplerAddressMode::eClampToEdge,
};

export constexpr auto LinearClamp = vuk::SamplerCreateInfo{
	.magFilter = vuk::Filter::eLinear,
	.minFilter = vuk::Filter::eLinear,
	.addressModeU = vuk::SamplerAddressMode::eClampToEdge,
	.addressModeV = vuk::SamplerAddressMode::eClampToEdge,
};

export constexpr auto LinearRepeat = vuk::SamplerCreateInfo{
	.magFilter = vuk::Filter::eLinear,
	.minFilter = vuk::Filter::eLinear,
	.addressModeU = vuk::SamplerAddressMode::eRepeat,
	.addressModeV = vuk::SamplerAddressMode::eRepeat,
};

export constexpr auto TrilinearClamp = vuk::SamplerCreateInfo{
	.magFilter = vuk::Filter::eLinear,
	.minFilter = vuk::Filter::eLinear,
	.mipmapMode = vuk::SamplerMipmapMode::eLinear,
	.addressModeU = vuk::SamplerAddressMode::eClampToEdge,
	.addressModeV = vuk::SamplerAddressMode::eClampToEdge,
};

export constexpr auto TrilinearRepeat = vuk::SamplerCreateInfo{
	.magFilter = vuk::Filter::eLinear,
	.minFilter = vuk::Filter::eLinear,
	.mipmapMode = vuk::SamplerMipmapMode::eLinear,
	.addressModeU = vuk::SamplerAddressMode::eRepeat,
	.addressModeV = vuk::SamplerAddressMode::eRepeat,
};
