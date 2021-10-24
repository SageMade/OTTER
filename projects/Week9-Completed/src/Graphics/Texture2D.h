#pragma once
#include "ITexture.h"

/// <summary>
/// Describes all parameters we can manipulate with our 2D Textures
/// </summary>
struct Texture2DDescription {
	/// <summary>
	/// The number of texels in this image along the x axis
	/// </summary>
	uint32_t       Width;
	/// <summary>
	/// The number of texels in this image along the y axis
	/// </summary>
	uint32_t       Height;
	/// <summary>
	/// The internal format that OpenGL should use when storing this texture
	/// </summary>
	InternalFormat Format;
	/// <summary>
	/// The wrap mode to use when a UV coordinate is outside the 0-1 range on the x axis
	/// </summary>
	WrapMode       HorizontalWrap;
	/// <summary>
	/// The wrap mode to use when a UV coordinate is outside the 0-1 range on the y axis
	/// </summary>
	WrapMode       VerticalWrap;
	/// <summary>
	/// The filter to use when multiple texels will map to a single pixel
	/// </summary>
	MinFilter      MinificationFilter;
	/// <summary>
	/// The filter to use when one texel will map to multiple pixels
	/// </summary>
	MagFilter      MagnificationFilter;
	/// <summary>
	/// The level of anisotropic filtering to use when this texture is viewed at an oblique angle
	/// </summary>
	/// <see>https://en.wikipedia.org/wiki/Anisotropic_filtering</see>
	float          MaxAnisotropic;
	/// <summary>
	/// True if this texture should generate mip maps (smaller copies of the image with filtering pre-applied)
	/// </summary>
	bool           GenerateMipMaps;

	Texture2DDescription() :
		Width(0), Height(0),
		Format(InternalFormat::Unknown),
		HorizontalWrap(WrapMode::Repeat),
		VerticalWrap(WrapMode::Repeat),
		MinificationFilter(MinFilter::NearestMipLinear),
		MagnificationFilter(MagFilter::Linear),
		MaxAnisotropic(-1.0f),
		GenerateMipMaps(true)
	{ }
};

class Texture2D : public ITexture {
public:
	typedef std::shared_ptr<Texture2D> Sptr;

	// Remove the copy and and assignment operators
	Texture2D(const Texture2D& other) = delete;
	Texture2D(Texture2D&& other) = delete;
	Texture2D& operator=(const Texture2D& other) = delete;
	Texture2D& operator=(Texture2D&& other) = delete;

	// Make sure we mark our destructor as virtual so base class is called
	virtual ~Texture2D() = default;

public:
	Texture2D(const Texture2DDescription& description);

	uint32_t GetWidth() const { return _description.Width; }
	uint32_t GetHeight() const { return _description.Height; }
	InternalFormat GetFormat() const { return _description.Format; }
	MinFilter GetMinFilter() const { return _description.MinificationFilter; }
	MagFilter GetMagFilter() const { return _description.MagnificationFilter; }
	WrapMode GetWrapS() const { return _description.HorizontalWrap; }
	WrapMode GetWrapT() const { return _description.VerticalWrap; }

	void LoadData(uint32_t width, uint32_t height, PixelFormat format, PixelType type, void* data, uint32_t offsetX = 0, uint32_t offsetY = 0);

	const Texture2DDescription& GetDescription() const { return _description; }

protected:
	Texture2DDescription _description;

	void _SetTextureParams();

public:
	static Texture2D::Sptr LoadFromFile(const std::string& path, const Texture2DDescription& description = Texture2DDescription(), bool forceRgba = true);
};
