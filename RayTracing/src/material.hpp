#pragma once

#include "hittable.h"
#include "common.hpp"

#include "Walnut/Random.h"

struct HitRecord;

class Material
{
public:
	virtual bool scatter(const Ray& rIn, const HitRecord& hitRecord, color& attenuation, Ray& scattered, bool hemisphereRandom) const = 0;
};

class Lambertian : public Material
{
public:
	Lambertian(const color& a) : m_albedo(a) {}

	bool scatter(const Ray& rIn, const HitRecord& hitRecord, color& attenuation, Ray& scattered, bool hemisphereRandom) const override
	{
		auto scatterDirection = hitRecord.m_normal + (hemisphereRandom ? randomInHemisphere(hitRecord.m_normal) : Walnut::Random::InUnitSphere());
		
		const auto eps = 1e-8f;
		if (fabs(scatterDirection.x) < eps && fabs(scatterDirection.y) < eps && fabs(scatterDirection.z) < eps)
		{
			scatterDirection = hitRecord.m_normal;
		}
		
		scattered = Ray(hitRecord.m_point, scatterDirection);
		attenuation = m_albedo;

		return true;
	}

private:
	color m_albedo;
};

class Metal : public Material
{
public:
	Metal(const color& a) : m_albedo(a) {}

	bool scatter(const Ray& rIn, const HitRecord& hitRecord, color& attenuation, Ray& scattered, bool hemisphereRandom) const override
	{
		auto reflected = glm::reflect(glm::normalize(rIn.direction()), hitRecord.m_normal);
		scattered = Ray(hitRecord.m_point, reflected + (FUZZY * Walnut::Random::InUnitSphere()));
		attenuation = m_albedo;

		return glm::dot(scattered.direction(), hitRecord.m_normal) > 0.f;
	}

private:
	color m_albedo;
};