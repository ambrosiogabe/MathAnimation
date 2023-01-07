#include "editor/DebugPanel.h"
#include "core.h"
#include "core/Application.h"
#include "core/Colors.h"
#include "svg/Svg.h"
#include "svg/SvgCache.h"
#include "renderer/Texture.h"
#include "renderer/Renderer.h"

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
			if (ImGui::BeginTabBar("SVG Cache"))
			{
				SvgCache* svgCache = Application::getSvgCache();
				const Framebuffer& framebuffer = svgCache->getFramebuffer();
				for (size_t i = 0; i < framebuffer.colorAttachments.size(); i++)
				{
					std::string tabName = "CacheEntry_" + std::to_string(i);
					if (ImGui::BeginTabItem(tabName.c_str()))
					{
						ImTextureID texId = (ImTextureID)(uint64)framebuffer.colorAttachments[i].graphicsId;
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

						ImGui::EndTabItem();
					}
				}

				ImGui::EndTabBar();
			}

			ImGui::End();

			ImGui::Begin("App Metrics");

			// Draw the FPS and color it according to how fast/slow it's going
			{
				float avgFrameTime = 0.0f;
				for (int i = 0; i < previousFrameTimesLength; i++)
				{
					avgFrameTime += previousFrameTimes[i];
				}
				avgFrameTime /= (float)previousFrameTimesLength;
				ImGui::Text("Average Frame Time: ");
				ImGui::SameLine();
				Vec4 fpsIndicatorColor = Colors::AccentGreen[3];
				if ((1.0f / avgFrameTime) < 31.0f)
				{
					fpsIndicatorColor = Colors::AccentRed[3];
				}
				else if ((1.0f / avgFrameTime) < 45.0f)
				{
					fpsIndicatorColor = Colors::AccentYellow[3];
				}
				ImGui::TextColored(fpsIndicatorColor, "%2.3fms (%2.3f FPS)", avgFrameTime, 1.0f / avgFrameTime);

				// Update the array of previous frame times
				previousFrameTimes[previousFrameTimesIndex] = Application::getDeltaTime();
				previousFrameTimesIndex = (previousFrameTimesIndex + 1) % previousFrameTimesLength;
			}

			// Draw call breakdown
			if (ImGui::TreeNodeEx("###DrawCallBreakdown_Tab", ImGuiTreeNodeFlags_FramePadding, "Num Draw Calls: %d", Renderer::getTotalNumDrawCalls()))
			{
				if (ImGui::BeginTable("##DrawCallBreakdown", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders))
				{
					ImGui::TableSetupColumn("Draw List Type");
					ImGui::TableSetupColumn("# of Draw Calls");
					ImGui::TableHeadersRow();

					ImGui::TableNextColumn();
					ImGui::Text("Draw List 2D:");
					ImGui::TableNextColumn();
					ImGui::Text("%d", Renderer::getDrawList2DNumDrawCalls());
					ImGui::TableNextRow();

					ImGui::TableNextColumn();
					ImGui::Text("Draw List Font 2D:");
					ImGui::TableNextColumn();
					ImGui::Text("%d", Renderer::getDrawListFont2DNumDrawCalls());
					ImGui::TableNextRow();

					ImGui::TableNextColumn();
					ImGui::Text("Draw List 3D:");
					ImGui::TableNextColumn();
					ImGui::Text("%d", Renderer::getDrawList3DNumDrawCalls());

					ImGui::EndTable();
				}
				ImGui::TreePop();
			}

			// Number of triangles breakdown
			if (ImGui::TreeNodeEx("###TriBreakdown_Tab", ImGuiTreeNodeFlags_FramePadding, "Num Tris: %d", Renderer::getTotalNumTris()))
			{
				if (ImGui::BeginTable("##DrawCallBreakdown", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_Borders))
				{
					ImGui::TableSetupColumn("Draw List Type");
					ImGui::TableSetupColumn("# of Tris");
					ImGui::TableHeadersRow();

					ImGui::TableNextColumn();
					ImGui::Text("Draw List 2D:");
					ImGui::TableNextColumn();
					ImGui::Text("%d", Renderer::getDrawList2DNumTris());
					ImGui::TableNextRow();

					ImGui::TableNextColumn();
					ImGui::Text("Draw List Font 2D:");
					ImGui::TableNextColumn();
					ImGui::Text("%d", Renderer::getDrawListFont2DNumTris());
					ImGui::TableNextRow();

					ImGui::TableNextColumn();
					ImGui::Text("Draw List 3D:");
					ImGui::TableNextColumn();
					ImGui::Text("%d", Renderer::getDrawList3DNumTris());

					ImGui::EndTable();
				}

				ImGui::TreePop();
			}

			ImGui::End();
		}

		void free()
		{

		}
	}
}