#pragma once

#include "Application/ApplicationLayer.h"
#include "Utils/Macros.h"
#include "Graphics/VertexArrayObject.h"

class PostProcessingLayer final : public ApplicationLayer {
public:
	MAKE_PTRS(PostProcessingLayer);

	/**
	 * Base class for post processing effects, we extend this to create new effects
	 */
	class Effect : public IResource {
	public:
		MAKE_PTRS(Effect);
		bool Enabled = true;
		std::string Name;
		virtual ~Effect() = default;

		virtual void Apply(const Framebuffer::Sptr& gBuffer) = 0;
		virtual void OnSceneLoad() {}
		virtual void OnSceneUnload() {}
		virtual void OnWindowResize(const glm::ivec2& oldSize, const glm::ivec2& newSize) {}
		virtual void RenderImGui() {}

		void DrawFullscreen();

	protected:
		friend class PostProcessingLayer;

		Framebuffer::Sptr _output = nullptr;
		glm::vec2 _outputScale = glm::vec2(1);
		RenderTargetType _format = RenderTargetType::ColorRgba8;
		
		Effect() = default;
	};

	PostProcessingLayer();
	virtual ~PostProcessingLayer();

	template <typename T, typename = typename std::enable_if<std::is_base_of<Effect, T>::value>::type>
	std::shared_ptr<T> GetEffect() {
		// Iterate over all the pointers in the binding list
		for (const auto& ptr : _effects) {
			// If the pointer type matches T, we return that behaviour, making sure to cast it back to the requested type
			if (std::type_index(typeid(*ptr.get())) == std::type_index(typeid(T))) {
				return std::dynamic_pointer_cast<T>(ptr);
			}
		}
		return nullptr;
	}

	const std::vector<Effect::Sptr>& GetEffects() const;

	// Inherited from ApplicationLayer

	virtual void OnAppLoad(const nlohmann::json& config) override;
	virtual void OnPostRender() override;
	virtual void OnSceneLoad() override;
	virtual void OnSceneUnload() override;
	virtual void OnWindowResize(const glm::ivec2& oldSize, const glm::ivec2& newSize) override;

protected:
	friend class Effect;
	std::vector<Effect::Sptr> _effects;
	VertexArrayObject::Sptr _quadVAO;
};