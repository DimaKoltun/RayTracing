#pragma once

#include "common.hpp"
#include "ray.hpp"

class Material;

struct HitRecord
{
	point3 m_point;
	vec3 m_normal;
	float m_t;
	bool m_frontFace;

	std::shared_ptr<Material> m_materialPtr;

	void setFaceNormal(const Ray& r, const vec3& outwardNormal)
	{
		m_frontFace = glm::dot(r.direction(), outwardNormal) < 0.f;
		m_normal = m_frontFace ? outwardNormal : -outwardNormal;
	}
};

class Hittable
{
public:
	virtual bool hit(const Ray& r, float tMin, float tMax, HitRecord& hitRecord) = 0;
};
