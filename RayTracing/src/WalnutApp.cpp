#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"

#include "Walnut/Image.h"
#include "Walnut/Random.h"
#include "Walnut/Timer.h"

#include <glm/glm.hpp>

#include <iostream>
#include <fstream>
#include <string_view>

using namespace Walnut;

namespace
{
constexpr std::string_view C_IMAGE_PATH = "image.ppm";
}

class ExampleLayer : public Walnut::Layer
{
public:
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

		if (ImGui::Button("Output Image"))
		{
			outputImage();
		}

		ImGui::End();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, {0.f, 0.f});

		ImGui::Begin("Viewport");
		
		m_viewportWidth = static_cast<uint32_t>(ImGui::GetContentRegionAvail().x);
		m_viewportHeight = static_cast<uint32_t>(ImGui::GetContentRegionAvail().y);

		if (m_image)
			ImGui::Image(m_image->GetDescriptorSet(), { (float)m_image->GetWidth(), (float)m_image->GetHeight()});

		ImGui::End();

		ImGui::PopStyleVar();
	}

	void Render()
	{
		Timer timer;

		glm::vec3 v;

		if (!m_image || m_viewportWidth != m_image->GetWidth() || m_viewportHeight != m_image->GetHeight())
		{
			renderImageToFile(C_IMAGE_PATH);
			readImageFromFile(C_IMAGE_PATH);
		}

		m_lastRenderTime = timer.ElapsedMillis();
	}

private:
	void outputImage()
	{
		const int width = 256;
		const int height = 256;

		std::cout << "P3\n" << width << ' ' << height << "\n255\n";

		for (int j = height - 1; j >= 0; --j)
		{
			for (int i = 0; i < width; ++i)
			{
				auto r = double(i) / (width - 1);
				auto g = double(j) / (height - 1);
				auto b = 0.25;

				int ir = static_cast<int>(255.999 * r);
				int ig = static_cast<int>(255.999 * g);
				int ib = static_cast<int>(255.999 * b);

				std::cout << ir << ' ' << ig << ' ' << ib << '\n';
			}
		}
	}

	void renderImageToFile(std::string_view path)
	{
		Timer timer;

		std::ofstream out;
		out.open(std::string(path), std::ofstream::out | std::ofstream::trunc);

		if (out.is_open())
		{
			const int width = m_viewportWidth;
			const int height = m_viewportHeight;

			out << "P3\n" << width << ' ' << height << "\n255\n";

			for (int j = height - 1; j >= 0; --j)
			{
				for (int i = 0; i < width; ++i)
				{
					auto r = double(i) / (width - 1);
					auto g = double(j) / (height - 1);
					auto b = 0.25;

					int ir = static_cast<int>(255.999 * r);
					int ig = static_cast<int>(255.999 * g);
					int ib = static_cast<int>(255.999 * b);

					out << ir << ' ' << ig << ' ' << ib << '\n';
				}
			}

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

					m_imageData[j * width + i] = (255 << 24) + (b << 16) + (g << 8) + r;
				}
			}

			m_image->SetData(m_imageData);

			in.close();
		}

		m_readTime = timer.ElapsedMillis();
	}

private:
	std::shared_ptr<Image> m_image;
	uint32_t* m_imageData = nullptr;
	uint32_t m_viewportWidth = 0;
	uint32_t m_viewportHeight = 0;

	float m_writeTime = 0.f;
	float m_readTime = 0.f;
	float m_lastRenderTime = 0.f;
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