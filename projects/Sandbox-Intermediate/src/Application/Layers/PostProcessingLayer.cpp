#include "Application/Layers/PostProcessingLayer.h"

#include "Application/Application.h"
#include "RenderLayer.h"

#include "PostProcessing/ColorCorrectionEffect.h"

PostProcessingLayer::PostProcessingLayer() :
	ApplicationLayer()
{
	Name = "Post Processing";
	Overrides =
		AppLayerFunctions::OnAppLoad |
		AppLayerFunctions::OnSceneLoad | AppLayerFunctions::OnSceneUnload | 
		AppLayerFunctions::OnPostRender |
		AppLayerFunctions::OnWindowResize;
}

PostProcessingLayer::~PostProcessingLayer() = default;

void PostProcessingLayer::OnAppLoad(const nlohmann::json& config)
{
	// TODO: Load effects
	_effects.push_back(std::make_shared<ColorCorrectionEffect>());

	Application& app = Application::Get();
	const glm::uvec4& viewport = app.GetPrimaryViewport();

	for (const auto& effect : _effects) {
		FramebufferDescriptor fboDesc = FramebufferDescriptor();
		fboDesc.Width  = viewport.z * effect->_outputScale.x;
		fboDesc.Height = viewport.w * effect->_outputScale.y;
		fboDesc.RenderTargets[RenderTargetAttachment::Color0] = RenderTargetDescriptor(effect->_format);

		effect->_output = std::make_shared<Framebuffer>(fboDesc);
	}

	// We need a mesh for drawing fullscreen quads

	glm::vec2 positions[6] = {
		{ -1.0f,  1.0f }, { -1.0f, -1.0f }, { 1.0f, 1.0f },
		{ -1.0f, -1.0f }, {  1.0f, -1.0f }, { 1.0f, 1.0f }
	};

	VertexBuffer::Sptr vbo = std::make_shared<VertexBuffer>();
	vbo->LoadData(positions, 6);

	_quadVAO = VertexArrayObject::Create();
	_quadVAO->AddVertexBuffer(vbo, {
		BufferAttribute(0, 2, AttributeType::Float, sizeof(glm::vec2), 0, AttribUsage::Position)
	});
}

void PostProcessingLayer::OnPostRender()
{
	Application& app = Application::Get();
	const glm::uvec4& viewport = app.GetPrimaryViewport();


	const RenderLayer::Sptr& renderer = app.GetLayer<RenderLayer>();
	const Framebuffer::Sptr& output = renderer->GetRenderOutput();
	const Framebuffer::Sptr& gBuffer = renderer->GetGBuffer();

	Framebuffer::Sptr current = output;

	glDisable(GL_DEPTH_TEST);
	glDepthMask(false);
	glDisable(GL_BLEND);

	_quadVAO->Bind();
	for (const auto& effect : _effects) {
		if (effect->Enabled) {
			effect->_output->Bind();
			glViewport(0, 0, effect->_output->GetWidth(), effect->_output->GetHeight());

			current->BindAttachment(RenderTargetAttachment::Color0, 0);

			effect->Apply(gBuffer);
			_quadVAO->Draw();

			current->Unbind();
			current = effect->_output;
		}
	}
	_quadVAO->Unbind();

	// Restore viewport to game viewport
	glViewport(viewport.x, viewport.y, viewport.z, viewport.w);

	current->Bind(FramebufferBinding::Read);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	current->Blit(
		{ 0, 0, current->GetWidth(), current->GetHeight() },
		{ viewport.x, viewport.y, viewport.x + viewport.z, viewport.y + viewport.w },
		BufferFlags::Color,
		MagFilter::Linear
	);

	current->Unbind();
}

void PostProcessingLayer::OnSceneLoad()
{
	for (const auto& effect : _effects) {
		effect->OnSceneLoad();
	}
}

void PostProcessingLayer::OnSceneUnload()
{
	for (const auto& effect : _effects) {
		effect->OnSceneUnload();
	}
}

void PostProcessingLayer::OnWindowResize(const glm::ivec2& oldSize, const glm::ivec2& newSize)
{
	for (const auto& effect : _effects) {
		effect->OnWindowResize(oldSize, newSize);
		effect->_output->Resize(newSize.x, newSize.y);
	}
}

const std::vector<PostProcessingLayer::Effect::Sptr>& PostProcessingLayer::GetEffects() const
{
	return _effects;
}

void PostProcessingLayer::Effect::DrawFullscreen()
{
	glDrawArrays(GL_TRIANGLES, 0, 6);
}
