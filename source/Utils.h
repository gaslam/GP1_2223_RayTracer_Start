#pragma once
#include <cassert>
#include <fstream>
#include "Math.h"
#include "DataTypes.h"

namespace dae
{
	namespace GeometryUtils
	{
#pragma region Sphere HitTest
		//SPHERE HIT-TESTS
		inline bool HitTest_Sphere(const Sphere& sphere, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{

			const float a{ Vector3::Dot(ray.direction, ray.direction) };
			const Vector3 rayMinusSphere{ ray.origin - sphere.origin };
			const float b{ Vector3::Dot(2 * ray.direction, rayMinusSphere) };
			const float c{ (Vector3::Dot(rayMinusSphere, rayMinusSphere)) - (sphere.radius * sphere.radius) };

			const float D{ (b * b) - (4 * a * c) };
			if (D < 0)
			{
				hitRecord.didHit = false;
				return false;
			}

			float t{ (-b - sqrtf(D)) / (2 * a) };

			if (t > ray.max || t < ray.min)
			{
				t = (-b + sqrtf(D)) / (2 * a);
				if (t > ray.max || t < ray.min)
				{
					hitRecord.didHit = false;
					return false;
				}
			}


			hitRecord.origin = ray.origin + t * ray.direction;
			hitRecord.materialIndex = sphere.materialIndex;
			hitRecord.normal = (hitRecord.origin - sphere.origin).Normalized();
			hitRecord.didHit = true;
			hitRecord.t = t;
			return hitRecord.didHit;
		}

		inline bool HitTest_Sphere(const Sphere& sphere, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_Sphere(sphere, ray, temp, true);
		}
#pragma endregion
#pragma region Plane HitTest
		//PLANE HIT-TESTS
		inline bool HitTest_Plane(const Plane& plane, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			const float t{ (Vector3::Dot((plane.origin - ray.origin),plane.normal)) / (Vector3::Dot(ray.direction, plane.normal)) };
			if (t > ray.min && t < ray.max)
			{
				hitRecord.origin = ray.origin + t * ray.direction;
				hitRecord.didHit = true;
				hitRecord.materialIndex = plane.materialIndex;
				hitRecord.normal = plane.normal;
				hitRecord.t = t;
				return true;
			}
			hitRecord.didHit = false;
			return false;
		}

		inline bool HitTest_Plane(const Plane& plane, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_Plane(plane, ray, temp, true);
		}
#pragma endregion
#pragma region Triangle HitTest

		//TRIANGLE HIT-TESTS
		inline bool HitTest_Triangle(const Triangle& triangle, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{

			auto a{ triangle.v1 - triangle.v0 };
			auto b{ triangle.v2 - triangle.v0 };

			auto triangleCenter{ (triangle.v0 + triangle.v1 + triangle.v2) / 3.f };

			if (dae::AreEqual(Vector3::Dot(triangle.normal, ray.direction), .0f))
			{
				hitRecord.didHit = false;
				return false;
			}

			if (triangle.cullMode == TriangleCullMode::FrontFaceCulling)
			{
				if (Vector3::Dot(triangle.normal, ray.direction) > 0.f)
				{
					hitRecord.didHit = false;
					return false;
				}
			}
			else if (triangle.cullMode == TriangleCullMode::BackFaceCulling)
			{
				if (Vector3::Dot(triangle.normal, ray.direction) < 0.f)
				{
					hitRecord.didHit = false;
					return false;
				}
			}

			auto t{ Vector3::Dot(triangleCenter - ray.origin, triangle.normal) / Vector3::Dot(ray.direction, triangle.normal) };

			if (t < ray.min || t > ray.max)
			{
				hitRecord.didHit = false;
				return false;
			}

			Vector3 intersection{ ray.origin + t * ray.direction };
			Vector3 side{ triangle.v1 - triangle.v0 };
			Vector3 pointToSide{ intersection - triangle.v0 };

			if (Vector3::Dot(triangle.normal, Vector3::Cross(side, pointToSide)) < 0.f)
			{
				return false;
			}
			side = triangle.v2 - triangle.v1;
			pointToSide = intersection - triangle.v1;

			if (Vector3::Dot(triangle.normal, Vector3::Cross(side, pointToSide)) < 0.f)
			{
				return false;
			}

			side = triangle.v0 - triangle.v2;
			pointToSide = intersection - triangle.v2;

			if (Vector3::Dot(triangle.normal, Vector3::Cross(side, pointToSide)) < 0.f)
			{
				return false;
			}

			if (!ignoreHitRecord)
			{
				hitRecord.origin = intersection;
				hitRecord.didHit = true;
				hitRecord.materialIndex = triangle.materialIndex;
				hitRecord.normal = triangle.normal;
				hitRecord.t = t;
			}
			return true;
		}


		inline bool HitTest_Triangle(const Triangle& triangle, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_Triangle(triangle, ray, temp, true);
		}
#pragma endregion
#pragma region TriangeMesh HitTest
		inline bool HitTest_TriangleMesh(const TriangleMesh& mesh, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{
			hitRecord.t = FLT_MAX;
			int currentIndices{};
			int timesToLoop{ static_cast<int>(mesh.indices.size()) / 3 };
			std::vector<Vector3> transformedPositions{ mesh.transformedPositions };
			bool isHit{ false };
			for (int i{}; i < timesToLoop; ++i)
			{
				int startIndex{ i * 3 };
				std::vector<int> range{ mesh.indices[startIndex],mesh.indices[startIndex + 1],mesh.indices[startIndex + 2] };
				Triangle triangle{ transformedPositions[range[0]],transformedPositions[range[1]],transformedPositions[range[2]] };
				Vector3 a{ triangle.v1 - triangle.v0 };
				Vector3 b{ triangle.v2 - triangle.v0 };
				triangle.normal = triangle.normal;
				triangle.cullMode = mesh.cullMode;
				triangle.materialIndex = mesh.materialIndex;
				HitRecord newHit{};
				if (GeometryUtils::HitTest_Triangle(triangle, ray, newHit))
				{
					if (hitRecord.t > newHit.t && !ignoreHitRecord)
					{
						hitRecord = newHit;
					}
					isHit = true;
				}
			}
			return isHit;
		}

		inline bool HitTest_TriangleMesh(const TriangleMesh& mesh, const Ray& ray)
		{
			HitRecord temp{};
			return HitTest_TriangleMesh(mesh, ray, temp, true);
		}
#pragma endregion
	}

	namespace LightUtils
	{
		//Direction from target to light
		inline Vector3 GetDirectionToLight(const Light& light, const Vector3& origin)
		{
			if (light.type == LightType::Directional)
			{
				return -light.direction;
			}
			Vector3 originToLight{ light.origin - origin };
			return originToLight;
		}

		inline ColorRGB GetRadiance(const Light& light, const Vector3& target)
		{
			if (light.type == LightType::Directional)
			{
				return light.color * light.intensity;
			}
			const Vector3 line{ light.origin - target };
			const ColorRGB distance{ light.color * (light.intensity / line.SqrMagnitude()) };
			return distance;
		}
	}

	namespace Utils
	{
		//Just parses vertices and indices
#pragma warning(push)
#pragma warning(disable : 4505) //Warning unreferenced local function
		static bool ParseOBJ(const std::string& filename, std::vector<Vector3>& positions, std::vector<Vector3>& normals, std::vector<int>& indices)
		{
			std::ifstream file(filename);
			if (!file)
				return false;

			std::string sCommand;
			// start a while iteration ending when the end of file is reached (ios::eof)
			while (!file.eof())
			{
				//read the first word of the string, use the >> operator (istream::operator>>) 
				file >> sCommand;
				//use conditional statements to process the different commands	
				if (sCommand == "#")
				{
					// Ignore Comment
				}
				else if (sCommand == "v")
				{
					//Vertex
					float x, y, z;
					file >> x >> y >> z;
					positions.push_back({ x, y, z });
				}
				else if (sCommand == "f")
				{
					float i0, i1, i2;
					file >> i0 >> i1 >> i2;

					indices.push_back((int)i0 - 1);
					indices.push_back((int)i1 - 1);
					indices.push_back((int)i2 - 1);
				}
				//read till end of line and ignore all remaining chars
				file.ignore(1000, '\n');

				if (file.eof())
					break;
			}

			//Precompute normals
			for (uint64_t index = 0; index < indices.size(); index += 3)
			{
				uint32_t i0 = indices[index];
				uint32_t i1 = indices[index + 1];
				uint32_t i2 = indices[index + 2];

				Vector3 edgeV0V1 = positions[i1] - positions[i0];
				Vector3 edgeV0V2 = positions[i2] - positions[i0];
				Vector3 normal = Vector3::Cross(edgeV0V1, edgeV0V2);

				if (isnan(normal.x))
				{
					int k = 0;
				}

				normal.Normalize();
				if (isnan(normal.x))
				{
					int k = 0;
				}

				normals.push_back(normal);
			}

			return true;
		}
#pragma warning(pop)
	}
}