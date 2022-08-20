#include "editor/EditorGui.h"
#include "editor/Timeline.h"
#include "editor/AnimObjectPanel.h"
#include "editor/DebugPanel.h"
#include "editor/ExportPanel.h"
#include "core/Application.h"

#include "imgui.h"

namespace MathAnim
{
	namespace EditorGui
	{
		// ------------- Internal Functions -------------
		static void getLargestSizeForViewport(ImVec2* imageSize, ImVec2* offset);
		static void checkHotKeys();

		void init()
		{
			Timeline::init();
			AnimObjectPanel::init();
			ExportPanel::init();
		}

		void update(uint32 sceneTextureId)
		{
			// TODO: Do this in a central file
			checkHotKeys();

			ImGui::Begin("Animation View", nullptr, ImGuiWindowFlags_MenuBar);
			if (ImGui::BeginMenuBar())
			{
				ImVec2 contentRegion = ImGui::GetContentRegionAvail();
				static ImVec2 playButtonSize = ImVec2(0, 0);
				static ImVec2 pauseButtonSize = ImVec2(0, 0);
				ImVec2 playOffset = ImGui::GetCursorPos();
				ImGuiStyle& style = ImGui::GetStyle();
				float paddingX = style.ItemSpacing.x + style.ItemInnerSpacing.x;
				playOffset.x += (contentRegion.x - playButtonSize.x - pauseButtonSize.x - paddingX * 4.0f) / 2.0f;
				ImGui::SetCursorPos(playOffset);

				if (ImGui::Button("Play"))
				{
					Application::setEditorPlayState(AnimState::PlayForward);
				}
				playButtonSize = ImGui::GetItemRectSize();
				if (ImGui::Button("Pause"))
				{
					Application::setEditorPlayState(AnimState::Pause);
				}
				pauseButtonSize = ImGui::GetItemRectSize();

				ImGui::EndMenuBar();
			}

			ImVec2 viewportSize, viewportOffset;
			getLargestSizeForViewport(&viewportSize, &viewportOffset);
			ImGui::SetCursorPos(viewportOffset);
			ImTextureID textureId = (void*)(uintptr_t)sceneTextureId;
			ImGui::Image(textureId, viewportSize, ImVec2(0, 0), ImVec2(1, 1));

			ImGui::End();

			Timeline::update();
			AnimObjectPanel::update();
			DebugPanel::update();
			ExportPanel::update();
		}

		void free()
		{
			ExportPanel::free();
			AnimObjectPanel::free();
			Timeline::free();
		}

		// ------------- Internal Functions -------------
		static void checkHotKeys()
		{
			ImGuiIO& io = ImGui::GetIO();
			if (io.KeyCtrl)
			{
				if (ImGui::IsKeyPressed(ImGuiKey_S, false))
				{
					Application::saveProject();
					g_logger_info("Saving project.");
				}
			}
		}

		static void getLargestSizeForViewport(ImVec2* imageSize, ImVec2* offset)
		{
			float targetAspectRatio = Application::getOutputTargetAspectRatio();
			ImVec2 contentRegion = ImGui::GetContentRegionAvail();
			float scrollAmount = ImGui::GetScrollY();
			ImVec2 res = contentRegion;

			float width = res.x;
			float height = res.x / targetAspectRatio;
			if (height > contentRegion.y)
			{
				height = contentRegion.y;
				width = targetAspectRatio * height;
			}

			res.x = width;
			res.y = height;

			*imageSize = res;

			ImVec2 padding = ImGui::GetCursorPos();
			*offset = padding;


			if (res.y < contentRegion.y)
			{
				float yOffset = (contentRegion.y - res.y) / 2.0f;
				offset->y += yOffset;
			}

			if (res.x < contentRegion.x)
			{
				float xOffset = (contentRegion.x - res.x - (padding.x * 2.0f)) / 2.0f;
				offset->x += xOffset;
			}
		}
	}
}