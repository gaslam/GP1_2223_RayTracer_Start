#pragma once

#include <cstdint>
#include <vector>

struct SDL_Window;
struct SDL_Surface;

namespace dae
{
	class Scene;
	struct Camera;
	struct Light;
	class Material;

	class Renderer final
	{
	public:
		Renderer(SDL_Window* pWindow);
		~Renderer() = default;

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void Render(Scene* pScene) const;
		bool SaveBufferToImage() const;

		void CycleLightningMode();
		void ToggleShadows() { m_ShadowsEnabled = !m_ShadowsEnabled; };

		void RenderPixel(Scene* pScene, uint32_t pixelIndex, float fov, float aspectRatio, const Camera& camera, const std::vector<Light>& lights, const std::vector<Material*>& materials) const;

	private:
		enum class LightningMode
		{
			ObservedArea,
			Radiance,
			BDRF,
			Combined
		};

		const int lightningModeMaxIndex{ 3 };
		LightningMode m_CurrentLightningMode{ LightningMode::Combined };

		bool m_ShadowsEnabled{ true };

		SDL_Window* m_pWindow{};

		SDL_Surface* m_pBuffer{};
		uint32_t* m_pBufferPixels{};

		int m_Width{};
		int m_Height{};;
	};
}
