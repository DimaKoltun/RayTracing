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
	Metal(const color& a, float fuzzy = 0.f) : m_albedo(a), m_fuzzy(fuzzy) {}

	bool scatter(const Ray& rIn, const HitRecord& hitRecord, color& attenuation, Ray& scattered, bool hemisphereRandom) const override
	{
		auto reflected = glm::reflect(glm::normalize(rIn.direction()), hitRecord.m_normal);
		scattered = Ray(hitRecord.m_point, reflected + (m_fuzzy * Walnut::Random::InUnitSphere()));
		attenuation = m_albedo;

		return glm::dot(scattered.direction(), hitRecord.m_normal) > 0.f;
	}

private:
	color m_albedo;
	float m_fuzzy;
};

class Dialectric : public Material
{
public:
	Dialectric(float indexOfRefraction) : m_indexOfRefraction(indexOfRefraction) {}

	bool scatter(const Ray& rIn, const HitRecord& hitRecord, color& attenuation, Ray& scattered, bool hemisphereRandom) const override
	{
		attenuation = color(1.f, 1.f, 1.f);
		float refractionRatio = hitRecord.m_frontFace ? (1.f / m_indexOfRefraction) : m_indexOfRefraction;
		vec3 unitDirection = glm::normalize(rIn.direction());

		float cosTheta = fminf(glm::dot(-unitDirection, hitRecord.m_normal), 1.f);
		float sinTheta = sqrt(1 - cosTheta * cosTheta);

		bool cannotRefract = refractionRatio * sinTheta > 1.f;
		vec3 direction;

		if (cannotRefract || reflectance(cosTheta, refractionRatio) > Walnut::Random::Float())
		{
			direction = glm::reflect(unitDirection, hitRecord.m_normal);
		}
		else
		{
			direction = glm::refract(unitDirection, hitRecord.m_normal, refractionRatio);
		}

		scattered = Ray(hitRecord.m_point, direction);
		return true;
	}

private:
	static float reflectance(float cosine, float refIndex)
	{
		// Use Schlick's approximation for reflectance.
		auto r0 = (1.f - refIndex) / (1.f + refIndex);
		r0 = r0 * r0;
		return r0 + (1 - r0) * pow((1 - cosine), 5);
	}

private:
	float m_indexOfRefraction;
};