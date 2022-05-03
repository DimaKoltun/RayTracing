#pragma once

#include "hittable.h"

class Sphere : public Hittable
{
public:
	Sphere(const point3& center, float radius, std::shared_ptr<Material> materialPtr)
		: m_center(center)
		, m_radius(radius)
		, m_materialPtr(materialPtr) {}

	bool hit(const Ray& r, float tMin, float tMax, HitRecord& hitRecord) override
	{
		vec3 oc = r.origin() - m_center;
		auto a = glm::dot(r.direction(), r.direction());
		auto b = 2 * glm::dot(r.direction(), oc);
		auto c = glm::dot(oc, oc) - m_radius * m_radius;
		auto discriminant = b * b - 4 * a * c;

		if (discriminant < 0)
		{
			return false;
		}
		
		auto sqrtd = sqrt(discriminant);

		// Find the nearest root that lies in the acceptable range.
		auto root = (-b - sqrtd) /(2.f * a);
		if (root < tMin || tMax < root)
		{
			root = (-b + sqrtd) / (2.f * a);
			if (root < tMin || tMax < root)
				return false;
		}

		hitRecord.m_t = root;
		hitRecord.m_point = r.at(hitRecord.m_t);
		auto outwardNormal = (hitRecord.m_point - m_center) / m_radius; //-- normalize
		hitRecord.setFaceNormal(r, outwardNormal);
		hitRecord.m_materialPtr = m_materialPtr;

		return true;
	}

private:
	point3 m_center;
	float m_radius;

	std::shared_ptr<Material> m_materialPtr;
};
