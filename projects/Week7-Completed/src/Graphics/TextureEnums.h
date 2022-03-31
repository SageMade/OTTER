#pragma once

#include <EnumToString.h>

#include "Logging.h"
#include "glad/glad.h"

/// <summary>
/// The types of texture we will support in our framework
/// </summary>
/// <see>https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glCreateTextures.xhtml</see>
enum class TextureType : GLenum {
	_1D = GL_TEXTURE_1D,
	_2D = GL_TEXTURE_2D,
	_3D = GL_TEXTURE_3D,
	Cubemap = GL_TEXTURE_CUBE_MAP,
	_2DMultisample = GL_TEXTURE_2D_MULTISAMPLE
};

// https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexImage2D.xhtml
// These are some of our more common available internal formats
enum class InternalFormat : GLint {
	Unknown      = GL_NONE,
	Depth16      = GL_DEPTH_COMPONENT16,
	Depth24      = GL_DEPTH_COMPONENT24,
	Depth32      = GL_DEPTH_COMPONENT32,
	DepthStencil = GL_DEPTH_STENCIL,
	R8           = GL_R8,
	R16          = GL_R16,
	RG8          = GL_RG8,
	RGB8         = GL_RGB8,
	SRGB         = GL_SRGB8,
	RGB10        = GL_RGB10,
	RGB16        = GL_RGB16,
	RGB32F       = GL_RGB32F,
	RGBA8        = GL_RGBA8,
	SRGBA        = GL_SRGB8_ALPHA8,
	RGBA16       = GL_RGBA16,
	RGB32AF      = GL_RGBA32F
	// Note: There are sized internal formats but there is a LOT of them
};

// The layout of the input pixel data
enum class PixelFormat : GLint {
	Red          = GL_RED,
	RG           = GL_RG,
	RGB          = GL_RGB,
	SRGB         = GL_SRGB,
	BGR          = GL_BGR,
	RGBA         = GL_RGBA,
	BGRA         = GL_BGRA,
	Depth        = GL_DEPTH_COMPONENT,
	DepthStencil = GL_DEPTH_STENCIL
};

// The type for each component of the pixel data
enum class PixelType : GLint{
	UByte  = GL_UNSIGNED_BYTE,
	Byte   = GL_BYTE,
	UShort = GL_UNSIGNED_SHORT,
	Short  = GL_SHORT,
	UInt   = GL_UNSIGNED_INT,
	Int    = GL_INT,
	Float  = GL_FLOAT
};

// These are our options for GL_TEXTURE_WRAP_T, GL_TEXTURE_WRAP_S and GL_TEXTURE_WRAP_R
enum class WrapMode : GLint {
	Unknown           = GL_NONE,
	ClampToEdge       = GL_CLAMP_TO_EDGE,
	ClampToBorder     = GL_CLAMP_TO_BORDER,
	MirroredRepeat    = GL_MIRRORED_REPEAT,
	Repeat            = GL_REPEAT, // Default
	MirrorClampToEdge = GL_MIRROR_CLAMP_TO_EDGE
};

// These are our available options for the GL_TEXTURE_MIN_FILTER setting
enum class MinFilter : GLint {
	Nearest           = GL_NEAREST,
	Linear            = GL_LINEAR,
	NearestMipNearest = GL_NEAREST_MIPMAP_NEAREST,
	LinearMipNearest  = GL_LINEAR_MIPMAP_NEAREST,
	NearestMipLinear  = GL_NEAREST_MIPMAP_LINEAR, // This is the default setting
	LinearMipLinear   = GL_LINEAR_MIPMAP_LINEAR
};

// These are our available options for the GL_TEXTURE_MAG_FILTER setting
enum class MagFilter : GLint {
	Nearest           = GL_NEAREST,
	Linear            = GL_LINEAR  // This is the default setting
};

/*
 * Gets the size of a single component in the given format, in bytes.
 */
constexpr size_t GetTexelComponentSize(PixelType type)
{
	switch (type) {
	case PixelType::UByte:
	case PixelType::Byte:
		return 1;
	case PixelType::UShort:
	case PixelType::Short:
		return 2;
	case PixelType::Int:
	case PixelType::UInt:
		return 4;
	default:
		LOG_ASSERT(false, "Unknown type: {}", type);
		return 0;
	}
}

/*
 * Gets the number of components in a given pixel format
 */
constexpr GLint GetTexelComponentCount(PixelFormat format)
{
	switch (format)
	{
		case PixelFormat::Depth:
		case PixelFormat::DepthStencil:
		case PixelFormat::Red:
			return 1;
		case PixelFormat::RG:
			return 2;
		case PixelFormat::RGB:
		case PixelFormat::BGR:
			return 3;
		case PixelFormat::RGBA:
		case PixelFormat::BGRA:
			return 4;
		default:
			LOG_ASSERT(false, "Unknown format: {}", format);
			return 0;
	}
}

/*
 * Gets the number of bytes needed to represent a single texel of the given format and type
 * @param format The format of the texel
 * @param type The data type of the texel
 * @returns The size of a single texel of the given format and type, in bytes
 */
constexpr size_t GetTexelSize(PixelFormat format, PixelType type) {
	return GetTexelComponentSize(type) * GetTexelComponentCount(format);
}