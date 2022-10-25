//External includes
#include "SDL.h"
#include "SDL_surface.h"

//Project includes
#include "Renderer.h"
#include "Math.h"
#include "Matrix.h"
#include "Material.h"
#include "Scene.h"
#include "Utils.h"
#include <thread>
#include <future>
#include <ppl.h>

using namespace dae;

//#define ASYNC
#define PARALELL_FOR

Renderer::Renderer(SDL_Window* pWindow) :
	m_pWindow(pWindow),
	m_pBuffer(SDL_GetWindowSurface(pWindow))
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);
	m_pBufferPixels = static_cast<uint32_t*>(m_pBuffer->pixels);
}


void Renderer::CycleLightningMode()
{
	int index{ static_cast<int>(m_CurrentLightningMode) };
	if (index >= lightningModeMaxIndex)
	{
		index = 0;
	}
	else
	{
		++index;
	}

	m_CurrentLightningMode = static_cast<LightningMode>(index);
}

void dae::Renderer::RenderPixel(Scene* pScene, uint32_t pixelIndex, float fov, float aspectRatio, const Camera& camera, const std::vector<Light>& lights, const std::vector<Material*>& materials) const
{
	const int px = pixelIndex % m_Width;
	const int py = pixelIndex / m_Width;
	const float cx{ (((2.f * (px + 0.5f)) / static_cast<float>(m_Width)) - 1.f) * aspectRatio * fov };
	const float cy{ static_cast<float>(1.f - ((2.f * (py + 0.5f)) / static_cast<float>(m_Height))) * fov };
	Vector3 rayDirection{ cx, cy,1.f };
	rayDirection.Normalize();
	rayDirection = camera.cameraToWorld.TransformVector(rayDirection);

	//Ray hitRay{ {0,0,0},rayDirection };

	//ColorRGB finalColor{ rayDirection.x, rayDirection.y, rayDirection.z};

	Ray viewRay{ camera.origin,rayDirection };

	HitRecord closestHit{};

	ColorRGB finalColor{};

	pScene->GetClosestHit(viewRay, closestHit);

	if (closestHit.didHit)
	{
		Vector3 originOffset{ closestHit.origin + (closestHit.normal * 0.0001f) };
		for (const Light& light : lights)
		{
			Vector3 lightDirection{ LightUtils::GetDirectionToLight(light, closestHit.origin) };
			Vector3 invRayDirection{ -rayDirection };

			const float lightDirectionMag{ lightDirection.Normalize() };

			if (m_ShadowsEnabled)
			{
				Ray invDirectionLight{ originOffset, LightUtils::GetDirectionToLight(light, originOffset).Normalized(), 0.0001f, lightDirectionMag };
				if (pScene->DoesHit(invDirectionLight))
				{
					continue;
				}
			}

			const float observedArea{ Vector3::Dot(lightDirection, closestHit.normal) };

			if (observedArea < 0.f)
				continue;


			switch (m_CurrentLightningMode)
			{
			case LightningMode::Combined:
				finalColor += LightUtils::GetRadiance(light, closestHit.origin) * materials[closestHit.materialIndex]
					->Shade(closestHit, lightDirection, invRayDirection) * observedArea;
				break;
			case LightningMode::BDRF:
				finalColor += materials[closestHit.materialIndex]->Shade(closestHit, lightDirection, invRayDirection);
				break;
			case LightningMode::ObservedArea:
				finalColor += ColorRGB(observedArea, observedArea, observedArea);
				break;
			case LightningMode::Radiance:
				finalColor += LightUtils::GetRadiance(light, closestHit.origin);
				break;
			}
		}
	}

	//Update Color in Buffer
	finalColor.MaxToOne();

	m_pBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBuffer->format,
		static_cast<uint8_t>(finalColor.r * 255),
		static_cast<uint8_t>(finalColor.g * 255),
		static_cast<uint8_t>(finalColor.b * 255));
}


void Renderer::Render(Scene* pScene) const
{
	Camera& camera = pScene->GetCamera();
	auto& materials = pScene->GetMaterials();
	auto& lights = pScene->GetLights();

	const float aspectRatio{ static_cast<float>(m_Width) / static_cast<float>(m_Height) };
	const float fov{ tanf((camera.fovAngle * TO_RADIANS) / 2.f) };

	const uint32_t numPixels{ static_cast<uint32_t>(m_Width * m_Height) };

#if defined(ASYNC)
	//async
	const uint32_t numCores = std::thread::hardware_concurrency();
	std::vector<std::future<void>> async_futures{};

	const uint32_t numPixelPerTask = numPixels / numCores;
	uint32_t numUnassignedPixels = numPixels % numCores;
	uint32_t currentPixelIndex = 0;

	for (uint32_t coreId{ 0 }; coreId < numCores; coreId++)
	{
		uint32_t taskSize = numPixelPerTask;
		if (numUnassignedPixels > 0)
		{
			++taskSize;
			--numUnassignedPixels;
		}

		async_futures.push_back(
			std::async(std::launch::async, [=, this]
				{
					const uint32_t pixelIndexEnd = currentPixelIndex + taskSize;
					for (uint32_t pixelIndex{ currentPixelIndex }; pixelIndex < pixelIndexEnd; pixelIndex++)
					{
						RenderPixel(pScene, pixelIndex, fov, aspectRatio, camera, lights, materials);
					}
				})
		);
		currentPixelIndex += taskSize;
	}
#elif defined(PARALELL_FOR)
	//paralell

	Concurrency::parallel_for(0u, numPixels, [=, this](int i)
		{
			RenderPixel(pScene, i, fov, aspectRatio, camera, lights, materials);
		});
#else
	//syncronous
	for (uint32_t i{}; i < numPixels; ++i)
	{
		RenderPixel(pScene, i, fov, aspectRatio, camera, lights, materials);
	}
#endif

	//@END
	//Update SDL Surface
	SDL_UpdateWindowSurface(m_pWindow);
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBuffer, "RayTracing_Buffer.bmp");
}