#pragma once

#include <glm/glm.hpp>

class Ray
{
public:
	Ray() {}
	
	Ray(const glm::vec3& origin, const glm::vec3& direction)
		: m_origin(origin)
		, m_direction(direction) {}


	const glm::vec3& origin() const { return m_origin; }
	const glm::vec3& direction() const { return m_direction; }

	glm::vec3 at(float t) const { return m_origin + m_direction * t; }

private:
	glm::vec3 m_origin;
	glm::vec3 m_direction;
};