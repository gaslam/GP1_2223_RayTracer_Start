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
			const float aspectRatio{ float(m_Width) / m_Height };
			const float fov{ tanf((camera.fovAngle * TO_RADIANS)  / 2.f)  };

			const float cx{ (((2.f * (px + 0.5f)) / m_Width) - 1.f) *aspectRatio  * fov };
			const float cy{ float(1.f - ((2.f * (py + 0.5f)) / m_Height)) * fov  };
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
				finalColor = materials[closestHit.materialIndex]->Shade();
				for (const Light& light : lights)
				{
					Vector3 lightDirection{ LightUtils::GetDirectionToLight(light, closestHit.origin + (closestHit.normal * 0.001f)) };
					const float rayMag{ lightDirection.Normalize()};
					Ray direction{ closestHit.origin + (closestHit.normal * 0.001f),lightDirection };
					direction.max = rayMag;
					direction.min = 0.0001f;
					if (pScene->DoesHit(direction))
					{
						finalColor *= 0.5f;
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