#pragma once
#include "Gameplay/Components/IComponent.h"
#include "Graphics/ShaderProgram.h"
#include "Graphics/Textures/Texture2DArray.h"

ENUM(ParticleType, uint32_t,
	StreamEmitter = 0,
	SphereEmitter = 1,
	BoxEmitter    = 2,
	ConeEmitter   = 3,
	Particle      = 1 << 17
);

class ParticleSystem : public Gameplay::IComponent{
public:
	MAKE_PTRS(ParticleSystem);

	ParticleSystem();
	~ParticleSystem();

	void Update();
	void Render();

	Texture2DArray::Sptr Atlas;

	void AddEmitter(const glm::vec3& position, const glm::vec3& direction, float emitRate = 1.0f, const glm::vec4& color = glm::vec4(1.0f), float size = 0.5f);

	// Inherited from IComponent

	virtual void RenderImGui() override;
	virtual void Awake() override;
	virtual nlohmann::json ToJson() const override;
	static ParticleSystem::Sptr FromJson(const nlohmann::json& blob);
	MAKE_TYPENAME(ParticleSystem);

protected:
	struct ParticleData {
		ParticleType Type;     // uint32_t, lower 16 bits for emitters, upper 16 for particles
		uint32_t     TexID;
		glm::vec3    Position;
		glm::vec3    Velocity; // For emitters, this is initial velocity
		glm::vec4    Color;
		float        Lifetime; // For emitters, this is the time to next particle spawn

		// For emitters, x is time to next particle, y is max deviation from direction in radians, z-w is lifetime range
		glm::vec4    Metadata;
		glm::vec4    Metadata2;
	};

	bool _hasInit;
	bool _needsUpload;

	uint32_t _maxParticles;
	GLuint _numParticles;

	uint32_t _particleBuffers[2];
	uint32_t _feedbackBuffers[2];
	uint32_t _updateVaos[2];
	uint32_t _renderVaos[2];
	uint32_t _query;

	uint32_t _currentVertexBuffer;
	uint32_t _currentFeedbackBuffer;

	ShaderProgram::Sptr _updateShader;
	ShaderProgram::Sptr _renderShader;
	glm::vec3           _gravity;

	std::vector<ParticleData> _emitters;
};