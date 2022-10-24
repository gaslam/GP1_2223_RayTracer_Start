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
			const float b{ Vector3::Dot(2 * ray.direction, rayMinusSphere ) };
			const float c{ (Vector3::Dot(rayMinusSphere, rayMinusSphere)) - (sphere.radius * sphere.radius)};

			const float D{ (b * b) - (4 * a * c) };
			if(D < 0)
			{
				hitRecord.didHit = false;
				return false;
			}

			float t{ (-b - sqrtf(D)) / (2 * a)};

			if(t > ray.max || t < ray.min)
			{
				t = (-b + sqrtf(D)) / (2 * a);
				if(t > ray.max || t < ray.min)
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
			const float t{ (Vector3::Dot((plane.origin - ray.origin),plane.normal))/ (Vector3::Dot(ray.direction, plane.normal))};
			if(t > ray.min && t < ray.max)
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

		inline bool SameSide(Vector3 p1, Vector3 p2, Vector3 A, Vector3 B)
		{
			Vector3 cp1 = Vector3::Cross(B - A, p1 - A);
			Vector3 cp2 = Vector3::Cross(B - A, p2 - A);
			if (Vector3::Dot(cp1, cp2) >= 0.f) return true;
			return false;

		}

		inline bool PointInTriangle(Triangle triangle, Vector3 P)
		{
			if (SameSide(P, triangle.v0, triangle.v1, triangle.v2) && SameSide(P, triangle.v1, triangle.v0, triangle.v2) && SameSide(P, triangle.v2, triangle.v0, triangle.v1))
			{
				Vector3 vc1 = Vector3::Cross(triangle.v0 - triangle.v1, triangle.v0 - triangle.v2);
				if (Vector3::Dot(triangle.v0 - P, vc1) <= .01f)
					return true;
			}

			return false;
		}

		//TRIANGLE HIT-TESTS
		inline bool HitTest_Triangle(const Triangle& triangle, const Ray& ray, HitRecord& hitRecord, bool ignoreHitRecord = false)
		{

			auto a{ triangle.v1 - triangle.v0 };
			auto b{ triangle.v2 - triangle.v0 };

			auto triangleCenter{ (triangle.v0 + triangle.v1 + triangle.v2) / 3.f };
			auto normal{ Vector3::Cross(a,b) };

			if (Vector3::Dot(triangle.normal, ray.direction) == .0f)
			{
				hitRecord.didHit = false;
				return false;
			}

			if (triangle.cullMode != TriangleCullMode::NoCulling)
			{
				if (triangle.cullMode == TriangleCullMode::FrontFaceCulling)
				{
					if (Vector3::Dot(normal, ray.direction) < 0.f)
					{
						hitRecord.didHit = false;
						return false;
					}
				}
				else if (triangle.cullMode == TriangleCullMode::BackFaceCulling)
				{
					if (Vector3::Dot(normal, ray.direction) > 0.f)
					{
						hitRecord.didHit = false;
						return false;
					}
				}
			}

			auto t{ Vector3::Dot(triangleCenter - ray.origin, normal) / Vector3::Dot(ray.direction, normal) };

			if (t < ray.min || t > ray.max)
			{
				hitRecord.didHit = false;
				return false;
			}

			Vector3 intersection{ ray.origin + t * ray.direction };
			Vector3 edgeV1V0{ triangle.v1 - triangle.v0 };
			Vector3 pointToSide{ intersection - triangle.v0 };

			if (Vector3::Dot(triangle.normal, Vector3::Cross(edgeV1V0, pointToSide)) < 0.f)
			{
				hitRecord.didHit = false;
				return false;
			}

			if (PointInTriangle(triangle, intersection))
			{
				if (!ignoreHitRecord)
				{
					hitRecord.origin = intersection;
					hitRecord.didHit = true;
					hitRecord.materialIndex = triangle.materialIndex;
					hitRecord.normal = normal;
					hitRecord.t = t;
				}
				return true;
			}
			return false;
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
			int currentIndices{};
			int timesToLoop{ static_cast<int>(mesh.indices.size()) / 3 };
			std::vector<Vector3> transformedPositions{ mesh.transformedPositions };
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
				if (GeometryUtils::HitTest_Triangle(triangle, ray, hitRecord))
				{
					return true;
				}
			}
			return false;
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
			if (light.type == LightType::Directional )
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
			const ColorRGB distance{ light.color * (light.intensity/line.SqrMagnitude())};
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

				if(isnan(normal.x))
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