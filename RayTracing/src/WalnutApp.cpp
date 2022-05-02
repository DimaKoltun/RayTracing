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

void write_color(std::ostream& out, color pixel_color) {
	// Write the translated [0,255] value of each color component.
	out << static_cast<int>(255.999 * pixel_color.x) << ' '
		<< static_cast<int>(255.999 * pixel_color.y) << ' '
		<< static_cast<int>(255.999 * pixel_color.z) << '\n';
}

}


class ExampleLayer : public Walnut::Layer
{
public:
	virtual void OnAttach() override
	{
		m_world.add(std::make_shared<Sphere>(point3(0.f, 0.f, -1.f), 0.5f));
		m_world.add(std::make_shared<Sphere>(point3(0.f, -100.5f, -1.f), 100.f));
	}

	virtual void OnUIRender() override
	{
		ImGui::Begin("Settings");

		ImGui::Text("Last Render: %.3fms", m_lastRenderTime);
		ImGui::Text("Write: %.3fms", m_writeTime);
		ImGui::Text("Read: %.3fms", m_readTime);

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

		glm::vec3 v;

		if (!m_image || m_imageWidth != m_image->GetWidth() || m_imageHeight != m_image->GetHeight())
		{
			renderImageToFile(C_IMAGE_PATH);
			readImageFromFile(C_IMAGE_PATH);
		}

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
						pixelColor += rayColor(r);
					}
					
					write_color(out, pixelColor * (1.f / m_samplesPerPixel));
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

	color rayColor(const Ray& ray)
	{
		HitRecord hitRecord;

		if (m_world.hit(ray, 0, C_INFINITY, hitRecord))
		{
			return 0.5f * (hitRecord.m_normal + color(1.f, 1.f, 1.f));
		}

		vec3 unitDirection = glm::normalize(ray.direction());
		auto t = 0.5f * (unitDirection.y + 1.f);
		return (1.f - t) * color(1.f, 1.f, 1.f) + t * color(0.5f, 0.7f, 1.f);
	}

private:
	std::shared_ptr<Image> m_image;
	uint32_t* m_imageData = nullptr;
	uint32_t m_imageWidth = 0;
	uint32_t m_imageHeight = 0;

	int m_samplesPerPixel = 8;

	float m_writeTime = 0.f;
	float m_readTime = 0.f;
	float m_lastRenderTime = 0.f;

	//-- camera
	float m_aspectRatio;
	float m_viewportWidth;
	float m_viewportHeight;
	float m_focalLength;
	vec3 m_origin;
	vec3 m_horizontal;
	vec3 m_vertical;
	vec3 m_lowerLeftCorner;

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