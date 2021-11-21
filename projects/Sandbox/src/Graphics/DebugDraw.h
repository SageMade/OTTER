#pragma once
#include <GLM/glm.hpp>
#include <stack>
#include "Graphics/VertexTypes.h"
#include "Graphics/Shader.h"

class DebugDrawer
{
public:
	inline static const size_t LINE_BATCH_SIZE = 8192;
	inline static const size_t TRI_BATCH_SIZE = 4096;

	DebugDrawer(const DebugDrawer& other) = delete;
	DebugDrawer(DebugDrawer&& other) = delete;
	DebugDrawer& operator =(const DebugDrawer& other) = delete;
	DebugDrawer& operator =(DebugDrawer&& other) = delete;

	DebugDrawer();
	virtual ~DebugDrawer() = default;

	static DebugDrawer& Get();
	static void Uninitialize();

	void PushColor(const glm::vec3& color);
	glm::vec3 PopColor();

	void PushWorldMatrix(const glm::mat4& world);
	void PopWorldMatrix();

	void DrawLine(const glm::vec3& p1, const glm::vec3& p2);
	void DrawLine(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& color = glm::vec3(1.0f));
	void DrawLine(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& color1, const glm::vec3& color2);
	void FlushLines();

	void DrawTri(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3);
	void DrawTri(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, const glm::vec3& color = glm::vec3(1.0f));
	void DrawTri(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, const glm::vec3& c1, const glm::vec3& c2, const glm::vec3& c3);
	void FlushTris();

	void FlushAll();

	void SetViewProjection(const glm::mat4& viewProjection);

protected:
	std::stack<glm::vec3> _colorStack;
	std::stack<glm::mat4> _transformStack;
	glm::mat4    _viewProjection;
	glm::mat4    _worldMatrix;

	size_t       _lineOffset;
	VertexPosCol _lineBuffer[LINE_BATCH_SIZE * 2];
	size_t       _triangleOffset;
	VertexPosCol _triBuffer[TRI_BATCH_SIZE * 3];

	VertexBuffer::Sptr _linesVBO;
	VertexArrayObject::Sptr _linesVAO;
	VertexBuffer::Sptr _trisVBO;
	VertexArrayObject::Sptr _trisVAO;

	inline static DebugDrawer* __Instance = nullptr;
	inline static Shader::Sptr __Shader = nullptr;
};