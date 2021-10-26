#pragma once
#include "json.hpp"
#include "Utils/JsonGlmHelpers.h"

// Helper structure for our light data
struct Light {
	glm::vec3 Position;
	glm::vec3 Color;
	// The approximate range of our light
	float Range = 4.0f;

	/// <summary>
	/// Loads a light from a JSON blob
	/// </summary>
	inline static Light FromJson(const nlohmann::json& data) {
		Light result;
		result.Position = ParseJsonVec3(data["position"]);
		result.Color = ParseJsonVec3(data["color"]);
		result.Range = data["range"].get<float>();
		return result;
	}

	/// <summary>
	/// Converts this object into it's JSON representation for storage
	/// </summary>
	inline nlohmann::json ToJson() const {
		return {
			{ "position", GlmToJson(Position) },
			{ "color", GlmToJson(Color) },
			{ "range", Range },
		};
	}

};
