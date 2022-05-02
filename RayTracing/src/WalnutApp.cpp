#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"

#include "Walnut/Image.h"
#include "Walnut/Random.h"
#include "Walnut/Timer.h"

#include "common.hpp"
#include "ray.hpp"
#include "sphere.hpp"
#include "hittable_list.hpp"
#include "camera.hpp"

#include <glm/glm.hpp>

#include <iostream>
#include <fstream>
#include <string_view>

using namespace Walnut;

namespace
{
constexpr std::string_view C_IMAGE_PATH = "image.ppm";

void write_color(std::ostream& out, color pixelColor, int samplesPerPixel) {

	auto r = pixelColor.r;
	auto g = pixelColor.g;
	auto b = pixelColor.b;

	auto scale = 1.f / samplesPerPixel;

	r = sqrt(r * scale);
	g = sqrt(g * scale);
	b = sqrt(b * scale);

	out << static_cast<int>(255.999 * std::clamp(r, 0.f, 0.999f)) << ' '
		<< static_cast<int>(255.999 * std::clamp(g, 0.f, 0.999f)) << ' '
		<< static_cast<int>(255.999 * std::clamp(b, 0.f, 0.999f)) << '\n';
}

}

class ExampleLayer : public Walnut::Layer
{
public:
	virtual void OnAttach() override
	{
		m_world.add(std::make_shared<Sphere>(point3(-0.5f, 0.f, -1.f), 0.5f));
		m_world.add(std::make_shared<Sphere>(point3(0.5f, -0.5f, -1.f), 0.25f));
		m_world.add(std::make_shared<Sphere>(point3(0.5f, 0.0f, -1.f), 0.25f));
		m_world.add(std::make_shared<Sphere>(point3(0.f, -100.5f, -1.f), 100.f));
	}

	virtual void OnUIRender() override
	{
		ImGui::Begin("Settings");

		ImGui::Text("Last Render: %.3fms", m_lastRenderTime);
		ImGui::Text("Write: %.3fms", m_writeTime);
		ImGui::Text("Read: %.3fms", m_readTime);

		ImGui::Checkbox("Random in Hemisphere", &m_randomInHemisphere);

		if (ImGui::Button("Render"))
		{
			Render();
		}

		ImGui::End();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.f, 0.f});

		ImGui::Begin("Viewport");
		
		m_imageWidth = static_cast<uint32_t>(ImGui::GetContentRegionAvail().x);
		m_imageHeight = static_cast<uint32_t>(ImGui::GetContentRegionAvail().y);
		m_aspectRatio = (float) m_imageWidth / m_imageHeight;

		if (m_image)
			ImGui::Image(m_image->GetDescriptorSet(), { (float)m_image->GetWidth(), (float)m_image->GetHeight()});

		ImGui::End();

		ImGui::PopStyleVar();
	}

	void Render()
	{
		Timer timer;

		renderImageToFile(C_IMAGE_PATH);
		readImageFromFile(C_IMAGE_PATH);

		m_lastRenderTime = timer.ElapsedMillis();
	}

private:
	void renderImageToFile(std::string_view path)
	{
		Timer timer;

		std::ofstream out;
		out.open(std::string(path), std::ofstream::out | std::ofstream::trunc);

		if (out.is_open())
		{
			m_camera = std::make_shared<Camera>(m_aspectRatio);

			const int width = m_imageWidth;
			const int height = m_imageHeight;

			out << "P3\n" << width << ' ' << height << "\n255\n";

			for (int j = height - 1; j >= 0; --j)
			{
				std::cerr << "\rScanlines remaining: " << j << ' ' << std::flush;
				for (int i = 0; i < width; ++i)
				{
					color pixelColor(0.f, 0.f, 0.f);

					for (int s = 0; s < m_samplesPerPixel; s++)
					{
						auto u = float(i + fabs(Random::Float())) / (width - 1); //-- 0 -> 1
						auto v = float(j + fabs(Random::Float())) / (height - 1); //-- 0 -> 1	
						Ray r = m_camera->getRay(u, v);
						pixelColor += rayColor(r, m_maxDepth);
					}
					
					write_color(out, pixelColor, m_samplesPerPixel);
				}
			}

			std::cerr << "\nDone.\n";
			out.close();
		}

		m_writeTime = timer.ElapsedMillis();
	}
	void readImageFromFile(std::string_view path)
	{
		Timer timer;

		std::ifstream in;
		in.open(std::string(path));
		if (in.is_open())
		{
			std::string buffer;
			in >> buffer; // P3
			
			in >> buffer; // width
			int width = stoi(buffer);

			in >> buffer; // height
			int height = stoi(buffer);
			
			in >> buffer; // max color value

			int r, g, b = 0;

			m_image = std::make_shared<Image>(width, height, ImageFormat::RGBA);
			delete[] m_imageData;
			m_imageData = new uint32_t[width * height];

			// read pixels
			for (int i = 0; i < width; i++)
			{
				for (int j = 0; j < height; j++)
				{
					in >> buffer;
					r = stoi(buffer);
					in >> buffer;
					g = stoi(buffer);
					in >> buffer;
					b = stoi(buffer);

					m_imageData[i * height + j] = (255 << 24) + (b << 16) + (g << 8) + r;
				}
			}

			m_image->SetData(m_imageData);

			in.close();
		}

		m_readTime = timer.ElapsedMillis();
	}

	color rayColor(const Ray& ray, int depth)
	{
		HitRecord hitRecord;

		if (depth <= 0)
			return color(0.f, 0.f, 0.f);

		if (m_world.hit(ray, 0.001f, C_INFINITY, hitRecord))
		{
			point3 target = hitRecord.m_point + hitRecord.m_normal + (m_randomInHemisphere ? randomInHemisphere(hitRecord.m_normal) : Random::InUnitSphere());
			return 0.5f * rayColor(Ray(hitRecord.m_point, target - hitRecord.m_point), depth - 1);
		}

		vec3 unitDirection = glm::normalize(ray.direction());
		auto t = 0.5f * (unitDirection.y + 1.f);
		return (1.f - t) * color(1.f, 1.f, 1.f) + t * color(0.3f, 0.5f, 1.f);
	}

	vec3 randomInHemisphere(const vec3& normal)
	{
		vec3 inUnitSphere = Random::InUnitSphere();
		if (glm::dot(inUnitSphere, normal) > 0)
			return inUnitSphere;
		else
			return -inUnitSphere;
	}

private:
	std::shared_ptr<Image> m_image;
	uint32_t* m_imageData = nullptr;
	uint32_t m_imageWidth = 0;
	uint32_t m_imageHeight = 0;

	int m_samplesPerPixel = 16;
	int m_maxDepth = 100;

	float m_writeTime = 0.f;
	float m_readTime = 0.f;
	float m_lastRenderTime = 0.f;

	//-- camera
	float m_aspectRatio;

	bool m_randomInHemisphere = false;

	std::shared_ptr<Camera> m_camera;

	HittableList m_world;
};

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	Walnut::ApplicationSpecification spec;
	spec.Name = "Walnut Example";

	Walnut::Application* app = new Walnut::Application(spec);
	app->PushLayer<ExampleLayer>();
	app->SetMenubarCallback([app]()
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Exit"))
			{
				app->Close();
			}
			ImGui::EndMenu();
		}
	});
	return app;
}