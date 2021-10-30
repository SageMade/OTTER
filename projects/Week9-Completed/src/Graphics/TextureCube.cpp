#include "Graphics/TextureCube.h"
#include <filesystem>
#include "stb_image.h"
#include "../../Week8-Completed/src/Utils/JsonGlmHelpers.h"

TextureCube::TextureCube(const std::string& baseFilename) :
	ITexture(TextureType::Cubemap),
	_description(TextureCubeDescription())
{
	_description.Filename = baseFilename;
	_LoadFromDescription();
}

TextureCube::TextureCube(const std::unordered_map<CubeMapFace, std::string>& faceFilenames) :
	ITexture(TextureType::Cubemap),
	_description(TextureCubeDescription())
{
	_description.FaceFileNames = faceFilenames;
	_LoadFromDescription();
}

TextureCube::TextureCube(const TextureCubeDescription& description) :
	ITexture(TextureType::Cubemap),
	_description(description)
{
	_LoadFromDescription();
}

nlohmann::json TextureCube::ToJson() const
{
	nlohmann::json result;
	result["filter_min"] = ~_description.MinificationFilter;
	result["filter_mag"] = ~_description.MagnificationFilter;
	
	if (!_description.FaceFileNames.empty()) {
		result["face_filenames"] = nlohmann::json();
		for (int ix = 0; ix < 6; ix++) {
			CubeMapFace face = (CubeMapFace)ix;
			result["face_filenames"][~face] = _description.FaceFileNames.at(face);
		}
	} else {
		result["base_filename"] = _description.Filename;
	}
	return result;
}

TextureCube::Sptr TextureCube::FromJson(const nlohmann::json& data)
{
	TextureCubeDescription descr = TextureCubeDescription();
	descr.MinificationFilter  = JsonParseEnum(MinFilter, data, "filter_min", MinFilter::NearestMipNearest);
	descr.MagnificationFilter = JsonParseEnum(MagFilter, data, "filter_mag", MagFilter::Linear);
	descr.Filename       = JsonGet<std::string>(data, "base_filename", "");
	if (data.contains("face_filenames") && data["face_filenames"].is_object()) {
		for (auto& [key, value] : data["face_filenames"].items()) {
			CubeMapFace face = ParseCubeMapFace(key, CubeMapFace::Unknown);
			if (face != CubeMapFace::Unknown) {
				descr.FaceFileNames[face] = value;
			}
		}
	}
	return std::make_shared<TextureCube>(descr);
}

void TextureCube::_LoadFromDescription()
{
	// If we weren't passed face filenames but WERE passed a base filename, try and get the 6 face files
	if (_description.FaceFileNames.empty() && !_description.Filename.empty()) {
		// Get the file path and it's directory to extract the root file name w/o extension
		std::filesystem::path baseName = std::filesystem::absolute(std::filesystem::path(_description.Filename));
		std::filesystem::path directory = baseName.parent_path();
		std::filesystem::path rootFileName = directory / baseName.stem();

		// Iterate over all 6 faces of the cube
		for (int ix = 0; ix < 6; ix++) {
			// We convert the index to a CubeMapFace so we can convert it to a string
			CubeMapFace face = (CubeMapFace)ix;

			// Make a path that consists of the base path + FaceName + extension
			// EX: foo/bar/Skybox_PosX.png
			std::filesystem::path targetPath = rootFileName;
			targetPath += "_" + ~face;
			targetPath += baseName.extension();

			// If the file exists, store it in the description
			if (std::filesystem::exists(targetPath)) {
				_description.FaceFileNames[face] = targetPath.string();
			}
		}
	}

	// If we don't have 6 faces for our cube, something has gone horribly wrong (or the files don't exist)
	if (_description.FaceFileNames.size() != 6) {
		LOG_ERROR("TextureCube was not given 6 faces, aborting load");
		return;
	}

	// Load all the images into the texture
	_LoadImages(_description.FaceFileNames);
}

void TextureCube::_LoadImages(const std::unordered_map<CubeMapFace, std::string>& faceFilenames)
{
	uint8_t* datastore = nullptr;

	size_t textureDataSize = 0;

	int numChannels = 0;
	for (int ix = 0; ix < 6; ix++) {
		CubeMapFace face = (CubeMapFace)ix;
		
		const std::string& filename = _description.FaceFileNames[face];
		int fileWidth, fileHeight, fileNumChannels;

		// Use STBI to load the image
		stbi_set_flip_vertically_on_load(true);
		uint8_t* data = stbi_load(filename.c_str(), &fileWidth, &fileHeight, &fileNumChannels, 0);

		// If we could not load any data, warn and return null
		if (data == nullptr) {
			if (data != nullptr) { delete[] datastore; }
			LOG_ERROR("STBI Failed to load image from \"{}\"", filename);
			return;
		}
		if (fileWidth != fileHeight) {
			if (data != nullptr) { delete[] datastore; }
			LOG_ERROR("Image loaded from \"{}\" was not square", filename);
			stbi_image_free(data);
			return;
		}
		if (datastore == nullptr) {
			_description.Size = fileWidth;
			numChannels = fileNumChannels;
			_description.Format = GetInternalFormatForChannels8(numChannels);
			_description.FormatHint = GetPixelFormatForChannels(numChannels);
			textureDataSize = ((size_t)_description.Size * _description.Size * GetTexelSize(_description.FormatHint, PixelType::Byte));

			// This is one of those poorly documented things in OpenGL
			if ((GetTexelSize(_description.FormatHint, PixelType::Byte) * _description.Size) % 4 != 0) {
				LOG_WARN("The alignment of a horizontal line is not a multiple of 4, this will require a call to glPixelStorei(GL_PACK_ALIGNMENT)");
			}

			datastore = new uint8_t[textureDataSize * 6];
		} else if (fileWidth != _description.Size || fileNumChannels != numChannels) {
			delete[] datastore;
			LOG_WARN("Image \"{}\" did not match size or format of texture cube", filename);
			stbi_image_free(data);
			return;
		}

		memcpy(datastore + textureDataSize * ix, data, textureDataSize);
		stbi_image_free(data);
	}

	_SetTextureParams();

	int componentSize = (GLint)GetTexelComponentSize(PixelType::Byte);
	glPixelStorei(GL_PACK_ALIGNMENT, componentSize);

	// Upload our data to our image (note that the custom enum tools let us convert to base type [GLenum] with the * operator)
	glTextureSubImage3D(_handle, 0, 0, 0, 0, _description.Size, _description.Size, 6, *_description.FormatHint, *PixelType::UByte, datastore);
	delete[] datastore;
}

void TextureCube::_SetTextureParams(){
	// Make sure the size is greater than zero and that we have a format specified before trying to set parameters
	if (_description.Size > 0 && _description.Format != InternalFormat::Unknown) {
		// Allocates the memory for our texture
		glTextureStorage2D(_handle, 1, (GLenum)_description.Format, _description.Size, _description.Size);

		glTextureParameteri(_handle, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(_handle, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTextureParameteri(_handle, GL_TEXTURE_MIN_FILTER, (GLenum)_description.MinificationFilter);
		glTextureParameteri(_handle, GL_TEXTURE_MAG_FILTER, (GLenum)_description.MagnificationFilter);
	}
}
