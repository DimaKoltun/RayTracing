#pragma once

#include "hittable.h"

#include <memory>
#include <vector>

class HittableList : public Hittable
{
public:
	using HittablePtr = std::shared_ptr<Hittable>;

	HittableList() {}
	HittableList(HittablePtr object) { add(object); }

	void add(HittablePtr object) { m_objects.push_back(object); }
	void clear() { m_objects.clear(); }
	auto& objects() { return m_objects; }

	bool hit(const Ray& r, float tMin, float tMax, HitRecord& hitRecord) override
	{
		HitRecord tempRecord;
		bool hitAnything = false;
		float closestSoFar = tMax;

		for (const auto& object : m_objects)
		{
			if (object->hit(r, tMin, closestSoFar, tempRecord))
			{
				hitAnything = true;
				closestSoFar = tempRecord.m_t;
				hitRecord = tempRecord;
			}
		}

		return hitAnything;
	}

private:
	std::vector<std::shared_ptr<Hittable>> m_objects;
};
