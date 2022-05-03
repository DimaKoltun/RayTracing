#pragma once

#include "common.hpp"

class Camera
{
public:
	Camera(const point3& lookFrom, const point3& lookAt, vec3 up, float vfov, float aspectRatio, float aperture, float focusDist)
	{
		auto theta = degreesToRadians(vfov);
		auto h = tan(theta/2.f);

		auto viewportHeight = 2.f * h;
		auto viewportWidth = aspectRatio * viewportHeight;
		auto focalLength = 1.f;

		m_w = glm::normalize(lookFrom - lookAt);
		m_u = glm::normalize(glm::cross(up, m_w));
		m_v = cross(m_w, m_u);
	
		m_origin = lookFrom;
		m_horizontal = focusDist * viewportWidth * m_u;
		m_vertical = focusDist * viewportHeight * m_v;
		m_lowerLeftCorner = m_origin - m_horizontal / 2.f - m_vertical / 2.f - m_w * focusDist;

		m_lensRadius = aperture / 2.f;
	}

	Ray getRay(float s, float t) const
	{
		vec3 rd = m_lensRadius * randomInUnitDisk();
		vec3 offset = rd.x * m_u + rd.y * m_v;

		return Ray(m_origin + offset, m_lowerLeftCorner + s * m_horizontal + t * m_vertical - m_origin - offset);
	}

private:
	point3 m_origin;
	point3 m_lowerLeftCorner;
	vec3 m_horizontal;
	vec3 m_vertical;

	vec3 m_u, m_v, m_w;
	float m_lensRadius;
};