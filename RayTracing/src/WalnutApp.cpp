#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"

#include "Walnut/Image.h"
#include "Walnut/Random.h"
#include "Walnut/Timer.h"

#include "common.hpp"
#include "ray.hpp"
#include "sphere.hpp"
#include "material.hpp"
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
		generateRandomScene();
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
			initCamera();

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
			Ray scattered;
			color attenuation;
			
			if (hitRecord.m_materialPtr->scatter(ray, hitRecord, attenuation, scattered, m_randomInHemisphere))
			{
				return attenuation * rayColor(scattered, depth - 1);
			}

			return color(0.f, 0.f, 0.f);
		}

		vec3 unitDirection = glm::normalize(ray.direction());
		auto t = 0.5f * (unitDirection.y + 1.f);
		return (1.f - t) * color(1.f, 1.f, 1.f) + t * color(0.3f, 0.5f, 1.f);
	}
	
	void generateRandomScene()
	{
		auto groundMaterial = std::make_shared<Lambertian>(color(0.5f, 0.5f, 0.5f));
		m_world.add(std::make_shared<Sphere>(vec3(0.f, -1000.f, 0.f), 1000.f, groundMaterial));

		for (int a = -11; a < 11; a++)
		{
			for (int b = -11; b < 11; b++)
			{
				auto chooseMat = Random::Float();
				point3 center(a + 0.9f * Random::Float(), 0.2f, b + 0.9f * Random::Float());

				if (glm::length(center - point3(4.f, 0.2f, 0.f)) > 0.9f)
				{
					std::shared_ptr<Material> sphereMaterial;

					if (chooseMat < 0.8f)
					{
						auto albedo = Random::Vec3() * Random::Vec3();
						sphereMaterial = std::make_shared<Lambertian>(albedo);
						m_world.add(std::make_shared<Sphere>(center, 0.2f, sphereMaterial));
					}
					else if (chooseMat < 0.95f)
					{
						auto albedo = Random::Vec3(0.5f, 1.f);
						auto fuzz = Random::Float() / 2.f;
						sphereMaterial = std::make_shared<Metal>(albedo, fuzz);
						m_world.add(std::make_shared<Sphere>(center, 0.2f, sphereMaterial));
					}
					else
					{
						sphereMaterial = std::make_shared<Dialectric>(1.5f);
						m_world.add(std::make_shared<Sphere>(center, 0.2f, sphereMaterial));
					}
				}
			}
		}

		auto material1 = std::make_shared<Dialectric>(1.5f);
		m_world.add(std::make_shared<Sphere>(point3(0.f, 1.f, 0.f), 1.0f, material1));

		auto material2 = std::make_shared<Lambertian>(color(0.4f, 0.2f, 0.1f));
		m_world.add(std::make_shared<Sphere>(point3(-4.f, 1.f, 0.f), 1.0f, material2));

		auto material3 = std::make_shared<Metal>(color(0.7f, 0.6f, 0.5f), 0.0f);
		m_world.add(std::make_shared<Sphere>(point3(4.f, 1.f, 0.f), 1.0f, material3));
	}
	
	void initCamera()
	{
		point3 lookFrom(13.f, 2.f, 3.f);
		point3 lookAt(0.f, 0.f, 0.f);
		vec3 vup(0.f, 1.f, 0.f);
		float distToFocus = 10.f;
		float aperture = 0.1f;

		m_camera = std::make_shared<Camera>(lookFrom, lookAt, vup, 25.f, m_aspectRatio, aperture, distToFocus);
	}

private:
	std::shared_ptr<Image> m_image;
	uint32_t* m_imageData = nullptr;
	uint32_t m_imageWidth = 0;
	uint32_t m_imageHeight = 0;

	int m_samplesPerPixel = 64;
	int m_maxDepth = 200;

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