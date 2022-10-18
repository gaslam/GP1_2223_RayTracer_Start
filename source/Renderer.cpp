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

using namespace dae;

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


void Renderer::Render(Scene* pScene) const
{
	Camera& camera = pScene->GetCamera();
	auto& materials = pScene->GetMaterials();
	auto& lights = pScene->GetLights();

	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{
			//float gradient = px / static_cast<float>(m_Width);
			//gradient += py / static_cast<float>(m_Width);
			//gradient /= 2.0f;
			const float aspectRatio{ static_cast<float>(m_Width) / static_cast<float>(m_Height) };
			const float fov{ tanf((camera.fovAngle * TO_RADIANS) / 2.f) };

			const float cx{ (((2.f * (px + 0.5f)) / static_cast<float>(m_Width)) - 1.f) * aspectRatio * fov };
			const float cy{static_cast<float>(1.f - ((2.f * (py + 0.5f)) / static_cast<float>(m_Height))) * fov };
			Vector3 rayDirection{ cx, cy,1.f };
			rayDirection.Normalize();
			Matrix camToWorldView{ camera.CalculateCameraToWorld() };
			rayDirection = camToWorldView.TransformVector(rayDirection);

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

					if(m_ShadowsEnabled)
					{
						Ray invDirectionLight{ originOffset, LightUtils::GetDirectionToLight(light, originOffset).Normalized(), 0.0001f, lightDirectionMag };
						if (pScene->DoesHit(invDirectionLight))
						{
							continue;
						}
					}

					const float observedArea{ Vector3::Dot(lightDirection, closestHit.normal) };

					if(observedArea < 0.f)
						continue;

					switch(m_CurrentLightningMode)
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
	}

	//@END
	//Update SDL Surface
	SDL_UpdateWindowSurface(m_pWindow);
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBuffer, "RayTracing_Buffer.bmp");
}