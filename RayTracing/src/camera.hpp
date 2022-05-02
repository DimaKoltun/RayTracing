#pragma once

#include "common.hpp"

class Camera
{
public:
	Camera(float aspectRatio)
	{
		auto viewportHeight = 2.f;
		auto viewportWidth = aspectRatio * viewportHeight;
		auto focalLength = 1.f;
	
		m_origin = point3(0.f, 0.f, 0.f);
		m_horizontal = vec3(viewportWidth, 0.f, 0.f);
		m_vertical = vec3(0.f, viewportHeight, 0.f);
		m_lowerLeftCorner = m_origin - m_horizontal / 2.f - m_vertical / 2.f - vec3(0.f, 0.f, focalLength);
	}

	Ray getRay(float x, float y) const
	{
		return Ray(m_origin, m_lowerLeftCorner + x * m_horizontal + y * m_vertical - m_origin);
	}

private:
	point3 m_origin;
	point3 m_lowerLeftCorner;
	vec3 m_horizontal;
	vec3 m_vertical;
};