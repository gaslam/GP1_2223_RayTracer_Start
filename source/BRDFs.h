#pragma once
#include <cassert>
#include "Math.h"

namespace dae
{
	namespace BRDF
	{
		/**
		 * \param kd Diffuse Reflection Coefficient
		 * \param cd Diffuse Color
		 * \return Lambert Diffuse Color
		 */
		static ColorRGB Lambert(float kd, const ColorRGB& cd)
		{
			auto p{  cd * kd };

			return p  / PI;
		}

		static ColorRGB Lambert(const ColorRGB& kd, const ColorRGB& cd)
		{
			auto p{ cd * kd };

			return p / PI;
		}

		/**
		 * \brief todo
		 * \param ks Specular Reflection Coefficient
		 * \param exp Phong Exponent
		 * \param l Incoming (incident) Light Direction
		 * \param v View Direction
		 * \param n Normal of the Surface
		 * \return Phong Specular Color
		 */
		static ColorRGB Phong(float ks, float exp, const Vector3& l, const Vector3& v, const Vector3& n)
		{
			const Vector3 reflect{ l - 2.f * Vector3::Dot(n,l) * n };

			float cosReflect{ks * powf(std::max(0.0f,Vector3::Dot(reflect, v)), exp) };
			return { cosReflect, cosReflect, cosReflect };
		}

		/**
		 * \brief BRDF Fresnel Function >> Schlick
		 * \param h Normalized Halfvector between View and Light directions
		 * \param v Normalized View direction
		 * \param f0 Base reflectivity of a surface based on IOR (Indices Of Refrection), this is different for Dielectrics (Non-Metal) and Conductors (Metal)
		 * \return
		 */
		static ColorRGB FresnelFunction_Schlick(const Vector3& h, const Vector3& v, const ColorRGB& f0)
		{
			const float dot{ Vector3::Dot(v,h) };
			return f0*dot;
		}

		/**
		 * \brief BRDF NormalDistribution >> Trowbridge-Reitz GGX (UE4 implemetation - squared(roughness))
		 * \param n Surface normal
		 * \param h Normalized half vector
		 * \param roughness Roughness of the material
		 * \return BRDF Normal Distribution Term using Trowbridge-Reitz GGX
		 */
		static float NormalDistribution_GGX(const Vector3& n, const Vector3& h, float roughness)
		{
			const float roughnessSquared{ roughness * roughness }, dotNH{ Vector3::Dot(n,h) };
			const float dotNHSquared{ dotNH * dotNH };
			const float denominator{ PI * ((dotNHSquared * (roughnessSquared - 1.f) + 1.f) * (dotNHSquared * (roughnessSquared - 1.f) + 1.f)) };
			const float result{ roughnessSquared / denominator };
			return result;
		}


		/**
		 * \brief BRDF Geometry Function >> Schlick GGX (Direct Lighting + UE4 implementation - squared(roughness))
		 * \param n Normal of the surface
		 * \param v Normalized view direction
		 * \param roughness Roughness of the material
		 * \return BRDF Geometry Term using SchlickGGX
		 */
		static float GeometryFunction_SchlickGGX(const Vector3& n, const Vector3& v, float roughness)
		{
			const float remappedRoughness{ ((roughness + 1.f) * (roughness + 1.f)) / 8.f };
			const float dotNV{ Vector3::Dot(n,v) };
			if (dotNV < 0.f)
				return 0.f;

			const float result{ dotNV / (dotNV * (1.f - remappedRoughness) + remappedRoughness) };
			return result;
		}

		/**
		 * \brief BRDF Geometry Function >> Smith (Direct Lighting)
		 * \param n Normal of the surface
		 * \param v Normalized view direction
		 * \param l Normalized light direction
		 * \param roughness Roughness of the material
		 * \return BRDF Geometry Term using Smith (> SchlickGGX(n,v,roughness) * SchlickGGX(n,l,roughness))
		 */
		static float GeometryFunction_Smith(const Vector3& n, const Vector3& v, const Vector3& l, float roughness)
		{
			float shadowing{ GeometryFunction_SchlickGGX(n,v,roughness) };
			float masking{ GeometryFunction_SchlickGGX(n,l,roughness) };
			return shadowing * masking;
		}

	}
}