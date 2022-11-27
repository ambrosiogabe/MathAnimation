#include "editor/DebugPanel.h"
#include "core.h"
#include "core/Application.h"
#include "svg/Svg.h"
#include "svg/SvgCache.h"
#include "renderer/Texture.h"

namespace MathAnim
{
	namespace DebugPanel
	{
		// Internal variables
		static constexpr int previousFrameTimesLength = 100;
		static float previousFrameTimes[previousFrameTimesLength];
		static int previousFrameTimesIndex = 0;

		void init()
		{

		}

		void update()
		{
			ImGui::Begin("Debug");

			ImGuiIO& io = ImGui::GetIO();

			// Display SVG cache
			{
				ImGui::Text("Svg Cache");
				ImGui::Separator();

				SvgCache* svgCache = Application::getSvgCache();
				const Framebuffer& framebuffer = svgCache->getFramebuffer();
				for (size_t i = 0; i < framebuffer.colorAttachments.size(); i++)
				{
					ImTextureID texId = (ImTextureID)framebuffer.colorAttachments[i].graphicsId;
					ImVec2 pos = ImGui::GetCursorScreenPos();
					ImVec4 tintCol = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);   // No tint
					ImVec4 borderCol = ImVec4(1.0f, 1.0f, 1.0f, 0.5f); // 50% opaque white
					float textureWidth = 512.0f;
					float textureHeight = 512.0f * ((float)framebuffer.width / (float)framebuffer.height);
					ImGui::Image(
						texId,
						ImVec2(textureWidth, textureHeight),
						ImVec2(0, 1),
						ImVec2(1, 0),
						tintCol,
						borderCol
					);

					if (ImGui::IsItemHovered() && io.KeyCtrl)
					{
						ImGui::BeginTooltip();
						float region_sz = 64.0f;
						float region_x = io.MousePos.x - pos.x - region_sz * 0.5f;
						float region_y = io.MousePos.y - pos.y - region_sz * 0.5f;
						float zoom = 6.0f;
						if (region_x < 0.0f) { region_x = 0.0f; }
						else if (region_x > textureWidth - region_sz) { region_x = textureWidth - region_sz; }
						if (region_y < 0.0f) { region_y = 0.0f; }
						else if (region_y > textureHeight - region_sz) { region_y = textureHeight - region_sz; }
						ImGui::Text("Min: (%.2f, %.2f)", region_x, region_y);
						ImGui::Text("Max: (%.2f, %.2f)", region_x + region_sz, region_y + region_sz);
						ImVec2 uv0 = ImVec2((region_x) / textureWidth, 1.0f - (region_y) / textureHeight);
						ImVec2 uv1 = ImVec2((region_x + region_sz) / textureWidth, 1.0f - (region_y + region_sz) / textureHeight);
						ImGui::Image(texId, ImVec2(region_sz * zoom, region_sz * zoom), uv0, uv1, tintCol, borderCol);
						ImGui::EndTooltip();
					}
				}
			}

			ImGui::End();

			ImGui::Begin("App Metrics");

			float avgFrameTime = 0.0f;
			for (int i = 0; i < previousFrameTimesLength; i++)
			{
				avgFrameTime += previousFrameTimes[i];
			}
			avgFrameTime /= (float)previousFrameTimesLength;
			ImGui::Text("Average Frame Time: %2.3fms (%2.3f FPS)", avgFrameTime, 1.0f / avgFrameTime);
			previousFrameTimes[previousFrameTimesIndex] = Application::getDeltaTime();
			previousFrameTimesIndex = (previousFrameTimesIndex + 1) % previousFrameTimesLength;

			ImGui::End();
		}

		void free()
		{

		}
	}
}