#include "Texture3D.h"
#include "Utils/Base64.h"
#include "Utils/JsonGlmHelpers.h"
#include "Utils/StringUtils.h"
#include <Logging.h>
#include <stb_image.h>
#include <iostream>
#include <fstream>

inline int CalcRequiredMipLevels(int width, int height, int depth) {
	return (1 + floor(log2(std::max(width, std::max(height, depth)))));
}

Texture3D::Texture3D(const std::string& filePath) : 
	ITexture(TextureType::_3D),
	_description(Texture3DDescription())
{
	_description.Filename = filePath;
	_LoadDataFromFile();
}

Texture3D::Texture3D(const Texture3DDescription& description) :
	ITexture(TextureType::_3D),
	_description(description)
{
	_SetTextureParams();
	if (!description.Filename.empty()) {
		_LoadDataFromFile();
	}
}

void Texture3D::SetMinFilter(MinFilter value)
{
	_description.MinificationFilter = value;
	glTextureParameteri(_rendererId, GL_TEXTURE_MIN_FILTER, *_description.MinificationFilter);
}

void Texture3D::SetMagFilter(MagFilter value)
{
	_description.MagnificationFilter = value;
	glTextureParameteri(_rendererId, GL_TEXTURE_MAG_FILTER, *_description.MagnificationFilter);
}

void Texture3D::LoadData(uint32_t width, uint32_t height, uint32_t depth, PixelFormat format, PixelType type, void* data, uint32_t offsetX /*= 0*/, uint32_t offsetY /*= 0*/, uint32_t offsetZ /*= 0*/)
{
	LOG_ASSERT(((width + offsetX) <= _description.Width) && ((height + offsetY) <= _description.Height) && ((depth + offsetZ) <= _description.Depth), "Pixel bounds are outside of the extents of the image!");

	_description.FormatHint = format; 

	// Align the data store to the size of a single component to ensure we don't get weirdness with images that aren't RGBA
	// See https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glPixelStore.xhtml
	int componentSize = (GLint)GetTexelComponentSize(type);
	glPixelStorei(GL_PACK_ALIGNMENT, componentSize);

	// Upload our data to our image
	glTextureSubImage3D(_rendererId, 0, offsetX, offsetY, offsetZ, width, height, depth, (GLenum)format, (GLenum)type, data);

	// If requested, generate mip-maps for our texture
	if (_description.GenerateMipMaps) {
		glGenerateTextureMipmap(_rendererId); 
	}
}

nlohmann::json Texture3D::ToJson() const
{
	nlohmann::json result = {
		{ "wrap_s",  ~_description.WrapS },
		{ "wrap_t",  ~_description.WrapT },
		{ "wrap_r",  ~_description.WrapR },
		{ "filter_min",       ~_description.MinificationFilter },
		{ "filter_mag",       ~_description.MagnificationFilter },
		{ "generate_mipmaps",  _description.GenerateMipMaps },
	};

	if (!_description.Filename.empty()) {
		result["filename"] = _description.Filename;
	}
	else {
		result["size_x"] = _description.Width;
		result["size_y"] = _description.Height;
		result["size_z"] = _description.Depth;

		result["format"] = ~_description.FormatHint;
		result["pixel_type"] = ~PixelType::UByte;

		if ((_description.Width * _description.Height * _description.Depth) > 0 && _description.FormatHint != PixelFormat::Unknown) {
			size_t dataSize = GetTexelSize(_description.FormatHint, PixelType::UByte) * _description.Width * _description.Height * _description.Depth;
			uint8_t* dataStore = new uint8_t[dataSize];
			glGetTextureImage(_rendererId, 0, *_description.Format, *PixelType::UByte, dataSize, dataStore);
			result["data"] = Base64::Encode(dataStore, dataSize);
		}
	}
	return result;
}

Texture3D::Sptr Texture3D::FromJson(const nlohmann::json& data)
{
	Texture3DDescription description = Texture3DDescription();
	description.Filename = JsonGet<std::string>(data, "filename", "");

	description.Width  = JsonGet(data, "size_x", description.Width);
	description.Height = JsonGet(data, "size_y", description.Height);
	description.Depth  = JsonGet(data, "size_z", description.Depth);

	description.WrapS  = JsonParseEnum(WrapMode, data, "wrap_s", description.WrapS);
	description.WrapT  = JsonParseEnum(WrapMode, data, "wrap_t", description.WrapT);
	description.WrapR  = JsonParseEnum(WrapMode, data, "wrap_r", description.WrapR);

	description.MinificationFilter = JsonParseEnum(MinFilter, data, "filter_min", MinFilter::NearestMipNearest);
	description.MagnificationFilter = JsonParseEnum(MagFilter, data, "filter_mag", MagFilter::Linear);
	description.GenerateMipMaps = JsonGet(data, "generate_mipmaps", false);
	description.FormatHint = JsonParseEnum(PixelFormat, data, "format", PixelFormat::Unknown);

	Texture3D::Sptr result = std::make_shared<Texture3D>(description);

	// If we embedded data into the JSON, load it now
	if (description.Filename.empty() && data.contains("data") && data["data"].is_string()) {
		PixelType type = JsonParseEnum(PixelType, data, "pixel_type", PixelType::Unknown);
		try {
			std::string rawData = Base64::Decode(data['data'].get<std::string>());
			result->LoadData(description.Width, description.Height, description.Depth, description.FormatHint, type, rawData.data());
		}
		catch (std::runtime_error()) {
			LOG_WARN("JSON blob had data, but failed to load to texture");
		}
	}

	return result;
}

void Texture3D::_LoadDataFromFile()
{
	LOG_ASSERT(_description.Width + _description.Height + _description.Depth == 0, "This texture has already been configured with a size! Cannot re-allocate memory!");

	if (!_description.Filename.empty()) {
		if (StringTools::EndsWith(_description.Filename, ".cube")) {
			_LoadCubeFile();
		}
	}
}

void Texture3D::_LoadCubeFile()
{
	std::ifstream inFile(_description.Filename);

	if (!inFile.is_open()) {
		LOG_WARN("Failed to open file .cube file: {}", _description.Filename);
		return;
	}

	glm::u8vec3* textureData = nullptr;
	uint32_t lutSize{ 0 };
	uint32_t ix{ 0 };
	float r{ 0 }, g{ 0 }, b{ 0 };

	std::string line;
	while (std::getline(inFile, line)) {
		// Handle sizing the LUT
		if (line.find("LUT_3D_SIZE") != std::string::npos) {
			std::stringstream lReader(line.substr(12));
			lReader >> lutSize;
			_description.Width = _description.Height = _description.Depth = lutSize;

			if (lutSize > 0) {
				if (textureData != nullptr) {
					delete[] textureData;
				} 
				textureData = new glm::u8vec3[(size_t)lutSize * lutSize * lutSize];
			}
		}
		// We'll grab the title for our debug name, nice lil use of it
		else if (line.find("TITLE") != std::string::npos) {
			std::string name = line.substr(6);
			StringTools::Trim(name);
			SetDebugName(name);
		}
		// Reading data lines
		else {
			StringTools::Trim(line);

			if (ix >= (lutSize * lutSize * lutSize) && lutSize > 0) {
				LOG_ASSERT(false, "nani?");
			}

			if (!line.empty() && textureData != nullptr) {
				std::stringstream lReader(line);
				lReader >> r >> g >> b;
				textureData[ix].r = static_cast<uint8_t>(r * 255);
				textureData[ix].g = static_cast<uint8_t>(g * 255);
				textureData[ix].b = static_cast<uint8_t>(b * 255);
				ix++;
			}
		}
	}

	if (textureData != nullptr) {
		// Set the pixel format
		_description.Format = InternalFormat::RGB8;
		// We need to clamp to edge for LUTS
		_description.WrapS = _description.WrapT = _description.WrapR = WrapMode::ClampToEdge;

		// Allocate data and configure params
		_SetTextureParams();
		// Load data
		LoadData(lutSize, lutSize, lutSize, PixelFormat::RGB, PixelType::UByte, textureData);

		// Clean up after ourselves
		delete[] textureData;
	}
}

void Texture3D::_SetTextureParams()
{
	// Calculate how many layers of storage to allocate based on whether mipmaps are enabled or not
	int layers = _description.GenerateMipMaps ? CalcRequiredMipLevels(_description.Width, _description.Height, _description.Depth) : 1;
	// Allocates the memory for our texture
	glTextureStorage3D(_rendererId, layers, (GLenum)_description.Format, _description.Width, _description.Height, _description.Depth);

	glTextureParameteri(_rendererId, GL_TEXTURE_MIN_FILTER, (GLenum)_description.MinificationFilter);
	glTextureParameteri(_rendererId, GL_TEXTURE_MAG_FILTER, (GLenum)_description.MagnificationFilter);
	glTextureParameteri(_rendererId, GL_TEXTURE_WRAP_S, (GLenum)_description.WrapS);
	glTextureParameteri(_rendererId, GL_TEXTURE_WRAP_T, (GLenum)_description.WrapT);
	glTextureParameteri(_rendererId, GL_TEXTURE_WRAP_R, (GLenum)_description.WrapR);
}

Texture3D::Sptr Texture3D::LoadFromFile(const std::string& path, const Texture3DDescription& description /*= Texture3DDescription()*/, bool forceRgba /*= true*/)
{
	// Create a copy of the description and change filename to the path
	Texture3DDescription desc = description;
	desc.Filename = path;

	// Create a texture from the description (it'll load the file)
	Texture3D::Sptr result = std::make_shared<Texture3D>(desc);

	return result;
}