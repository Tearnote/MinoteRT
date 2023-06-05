export module minote.resource;

import <vuk/Future.hpp>;

// Some weak typedefs to add semantics to gfx function params

export template<typename T>
using Texture2D = vuk::Future;

export template<typename T>
using Buffer = vuk::Future;
