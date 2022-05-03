#pragma once

#include "ray.hpp"

#include "Walnut/Random.h"

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>

#include <cmath>
#include <limits>
#include <memory>

using color = glm::vec3;
using point3 = glm::vec3;
using vec3 = glm::vec3;

//-- constants
static const float C_INFINITY = std::numeric_limits<float>::infinity();
static const float C_PI = 3.1415926535897932385f;

float FUZZY = 0.f;

inline float degreesToRadians(float degrees) {
	return degrees * C_PI / 180.0f;
}

vec3 randomInHemisphere(const vec3& normal)
{
	vec3 inUnitSphere = Walnut::Random::InUnitSphere();
	if (glm::dot(inUnitSphere, normal) > 0)
		return inUnitSphere;
	else
		return -inUnitSphere;
}

vec3 randomInUnitDisk()
{
	while (true)
	{
		float x = 2.f * Walnut::Random::Float() - 1.f;
		float y = 2.f * Walnut::Random::Float() - 1.f;
		auto p = vec3(x, y, 0.f);

		if (glm::length2(p) >= 1)
			continue;

		return p;
	}
}