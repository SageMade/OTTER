#include "ParticleLayer.h"
#include "Gameplay/Components/ParticleSystem.h"
#include "Application/Application.h"

ParticleLayer::ParticleLayer() :
	ApplicationLayer()
{
	Name = "Particles";
	Overrides = AppLayerFunctions::OnUpdate | AppLayerFunctions::OnRender;
}

ParticleLayer::~ParticleLayer()
{ }

void ParticleLayer::OnUpdate()
{
	Application& app = Application::Get();

	if (app.CurrentScene()->IsPlaying) {
		app.CurrentScene()->Components().Each<ParticleSystem>([](const ParticleSystem::Sptr& system) {
			if (system->IsEnabled) {
				system->Update();
			}
		});
	}
}

void ParticleLayer::OnRender(const Framebuffer::Sptr& prevLayer)
{
	Application::Get().CurrentScene()->Components().Each<ParticleSystem>([](const ParticleSystem::Sptr& system) {
		if (system->IsEnabled) {
			system->Render();
		}
	});
}
