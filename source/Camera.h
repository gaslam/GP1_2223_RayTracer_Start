#pragma once
#include <cassert>
#include <iostream>
#include <SDL_keyboard.h>
#include <SDL_mouse.h>

#include "Math.h"
#include "Timer.h"

namespace dae
{
	struct Camera
	{
		Camera() = default;

		Camera(const Vector3& _origin, float _fovAngle) :
			origin{ _origin },
			fovAngle{ _fovAngle }
		{
		}


		Vector3 origin{};
		float fovAngle{};

		Vector3 forward{ Vector3::UnitZ };
		Vector3 up{ Vector3::UnitY };
		Vector3 right{ Vector3::UnitX };

		float totalPitch{ 0.f };
		float totalYaw{ 0.f };

		Matrix cameraToWorld{};


		Matrix CalculateCameraToWorld()
		{
			right = Vector3::Cross(Vector3::UnitY, forward).Normalized();
			up = Vector3::Cross(forward, right).Normalized();

			const Matrix ONB{
				{right,0},
				{up, 0},
				{forward, 0},
				{origin,1}
			};
			return ONB;
		}

		void Update(Timer* pTimer)
		{
			const float speed{ 10.f };
			const float deltaTime = pTimer->GetElapsed();

			//Keyboard Input
			const uint8_t* pKeyboardState = SDL_GetKeyboardState(nullptr);

			HandleKeyMovement(pKeyboardState, speed, deltaTime);

			//Mouse Input
			int mouseX{}, mouseY{};
			const uint32_t mouseState = SDL_GetRelativeMouseState(&mouseX, &mouseY);

			HandleMouseRotation(mouseState, mouseX, mouseY, speed, deltaTime);

		}

		void HandleMouseRotation(const uint32_t& mouseState, const int mouseX, const int mouseY, const float speed, const float deltaTime)
		{
			if (mouseState & SDL_BUTTON(SDL_BUTTON_LEFT))
			{
				totalYaw += (mouseX / speed) * deltaTime;
				//if (mouseY != 0)
				//{
				//	Vector3 dir{ forward * (deltaTime * speed) };
				//	origin += mouseY > 0 ? dir : -dir;
				//}
			}

			if (mouseState & SDL_BUTTON(SDL_BUTTON_RIGHT))
			{
				totalYaw += (mouseX / speed) * deltaTime;
				totalPitch += (mouseY / speed) * deltaTime;
			}

			Matrix Mc{ Matrix::CreateRotation(Vector3(-totalPitch,totalYaw,0.f))};

			forward = Mc.TransformVector(Vector3::UnitZ);
			forward.Normalize();
		}


		void HandleKeyMovement(const uint8_t* pKeyboardState, const float speed, const float deltaTime)
		{
			if (pKeyboardState[SDL_SCANCODE_D])
			{
				float lengthSquared{ forward.SqrMagnitude() };
				if (forward.z >= 0.f)
				{
					origin.x += (speed * deltaTime);
				}
				else
				{
					origin.x -= (speed * deltaTime);
				}
				origin.z += forward.x >= 0.f ? (sinf(totalYaw) * sinf(totalYaw)) * -lengthSquared : (sinf(totalYaw) * sinf(totalYaw)) * lengthSquared;
			}

			if (pKeyboardState[SDL_SCANCODE_A])
			{
				float lengthSquared{ forward.SqrMagnitude() };
				if (forward.z < 0.f)
				{
					origin.x += (speed * deltaTime);
				}
				else
				{
					origin.x -= (speed * deltaTime);
				}
				origin.z += forward.x < 0.f ? (sinf(totalYaw) * sinf(totalYaw)) * -lengthSquared : (sinf(totalYaw) * sinf(totalYaw)) * lengthSquared;
			}

			if (pKeyboardState[SDL_SCANCODE_W])
			{

				origin += forward * (speed * deltaTime);
			}

			if (pKeyboardState[SDL_SCANCODE_S])
			{
				origin -= forward * (speed * deltaTime);
			}
		}
	};
}
