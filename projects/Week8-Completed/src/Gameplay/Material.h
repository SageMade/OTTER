#pragma once
#include <memory>
#include "Graphics/Texture2D.h"
#include "Graphics/Shader.h"

// Helper structure for material parameters
// to our shader
struct Material : public IResource {
public:
	typedef std::shared_ptr<Material> Sptr;
	// A human readable name for the material
	std::string     Name;
	// The shader that the material is using
	Shader::Sptr    MatShader;

	// Material shader parameters
	Texture2D::Sptr Texture;
	float           Shininess;

	/// <summary>
	/// Handles applying this material's state to the OpenGL pipeline
	/// Will bind the shader, update material uniforms, and bind textures
	/// </summary>
	virtual void Apply();

	/// <summary>
	/// Loads a material from a JSON blob
	/// </summary>
	static Material::Sptr FromJson(const nlohmann::json& data);

	/// <summary>
	/// Converts this material into it's JSON representation for storage
	/// </summary>
	nlohmann::json ToJson() const;
};