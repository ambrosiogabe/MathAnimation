#include "editor/EditorGui.h"
#include "editor/Timeline.h"
#include "editor/AnimObjectPanel.h"
#include "editor/DebugPanel.h"
#include "editor/ExportPanel.h"
#include "editor/SceneHierarchyPanel.h"
#include "editor/Gizmos.h"
#include "editor/EditorSettings.h"
#include "editor/AssetManagerPanel.h"
#include "editor/ConsoleLog.h"
#include "animation/AnimationManager.h"
#include "core/Application.h"
#include "core/Input.h"
#include "core/Colors.h"
#include "renderer/Texture.h"
#include "renderer/Framebuffer.h"

#include "imgui.h"

namespace MathAnim
{
	namespace EditorGui
	{
		// ------------- Internal Functions -------------
		static void getLargestSizeForViewport(ImVec2* imageSize, ImVec2* offset);
		static void checkHotKeys();
		static void checkForMousePicking(const Framebuffer& mainFramebuffer);
		static TimelineData timeline;
		static bool timelineLoaded = false;
		static ImVec2 viewportOffset;
		static ImVec2 viewportSize;
		static bool mouseHoveringViewport;
		static bool mainViewportIsActive;
		static bool editorViewportIsActive;

		void init(AnimationManagerData* am, const std::string& assetsRoot)
		{
			viewportOffset = { 0, 0 };
			viewportSize = { 0, 0 };

			if (!timelineLoaded)
			{
				timeline = Timeline::initInstance();
				Application::setFrameIndex(0);
			}
			Timeline::init(am);

			AnimObjectPanel::init();
			ExportPanel::init();
			SceneHierarchyPanel::init(am);
			AssetManagerPanel::init(assetsRoot);
			timelineLoaded = true;
		}

		void update(const Framebuffer& mainFramebuffer, const Framebuffer& editorFramebuffer, AnimationManagerData* am)
		{
			// TODO: Do this in a central file
			checkHotKeys();
			checkForMousePicking(editorFramebuffer);

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

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

			ImVec2 mainViewportOffset, mainViewportSize;
			getLargestSizeForViewport(&mainViewportSize, &mainViewportOffset);
			ImGui::SetCursorPos(mainViewportOffset);

			const Texture& mainColorTexture = mainFramebuffer.getColorAttachment(0);
			ImTextureID textureId = (void*)(uintptr_t)mainColorTexture.graphicsId;
			ImGui::Image(textureId, mainViewportSize, ImVec2(0, 0), ImVec2(1, 1));
			mainViewportIsActive = ImGui::IsItemVisible();
			ImGui::End();

			// Draw the editor framebuffer viewport
			{
				ImGui::Begin("Animation Editor View", nullptr);

				ImVec2 editorViewportRelativeOffset;
				getLargestSizeForViewport(&viewportSize, &editorViewportRelativeOffset);
				ImGui::SetCursorPos(editorViewportRelativeOffset);
				viewportOffset = ImGui::GetCursorScreenPos() - ImGui::GetMainViewport()->Pos;

				const Texture& editorColorTexture = editorFramebuffer.getColorAttachment(0);
				ImTextureID editorTextureId = (void*)(uintptr_t)editorColorTexture.graphicsId;
				ImGui::Image(editorTextureId, viewportSize, ImVec2(0, 0), ImVec2(1, 1));
				mouseHoveringViewport = ImGui::IsItemHovered();
				editorViewportIsActive = ImGui::IsItemVisible();

				ImGui::End();
			}

			ImGui::PopStyleVar();

			Timeline::update(timeline, am);
			AnimObjectPanel::update();
			DebugPanel::update();
			ExportPanel::update();
			SceneHierarchyPanel::update(am);
			AssetManagerPanel::update();
			EditorSettings::imgui();
			ConsoleLog::update();
		}

		void onGizmo(AnimationManagerData* am)
		{
			AnimObjId activeAnimObjectId = Timeline::getActiveAnimObject();
			AnimObject* activeAnimObject = AnimationManager::getMutableObject(am, activeAnimObjectId);
			if (activeAnimObject)
			{
				activeAnimObject->onGizmo(am);

				// Render any animations that contain this object
				std::vector<AnimId> animations = AnimationManager::getAssociatedAnimations(am, activeAnimObject->id);
				for (AnimId animId : animations)
				{
					Animation* animation = AnimationManager::getMutableAnimation(am, animId);
					if (animation)
					{
						animation->onGizmo();
					}
				}
			}

			AnimId activeAnimationId = Timeline::getActiveAnimation();
			Animation* activeAnimation = AnimationManager::getMutableAnimation(am, activeAnimationId);
			if (activeAnimation)
			{
				activeAnimation->onGizmo();
			}
		}

		Vec2 mouseToNormalizedViewport()
		{
			Vec2 mousePos = mouseToViewportCoords();
			mousePos.x = (mousePos.x / viewportSize.x);
			mousePos.y = (mousePos.y / viewportSize.y);
			return mousePos;
		}

		Vec2 mouseToViewportCoords()
		{
			Vec2 mousePos = Vec2{ Input::mouseX, Input::mouseY };
			mousePos -= viewportOffset;
			return mousePos;
		}

		void free(AnimationManagerData* am)
		{
			AssetManagerPanel::free();
			SceneHierarchyPanel::free();
			ExportPanel::free();
			AnimObjectPanel::free();
			Timeline::freeInstance(timeline);
			Timeline::free(am);
			timelineLoaded = false;
		}

		const TimelineData& getTimelineData()
		{
			return timeline;
		}

		void setTimelineData(const TimelineData& data)
		{
			Timeline::freeInstance(timeline);
			timeline = data;
			timelineLoaded = true;
		}

		bool mainViewportActive()
		{
			return mainViewportIsActive;
		}

		bool editorViewportActive()
		{
			return editorViewportIsActive;
		}

		bool mouseHoveredEditorViewport()
		{
			return mouseHoveringViewport;
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

			if (!io.WantTextInput)
			{
				if (ImGui::IsKeyPressed(ImGuiKey_Space, false))
				{
					AnimState currentPlayState = Application::getEditorPlayState();
					AnimState newState = currentPlayState == AnimState::PlayForward
						? AnimState::Pause
						: AnimState::PlayForward;
					Application::setEditorPlayState(newState);
				}
			}
		}
		static void checkForMousePicking(const Framebuffer& mainFramebuffer)
		{
			if (mouseHoveringViewport && !GizmoManager::anyGizmoActive())
			{
				if (Input::mouseClicked(MouseButton::Left))
				{
					const Texture& pickingTexture = mainFramebuffer.getColorAttachment(3);
					// Get the mouse pos in normalized coords
					Vec2 normalizedMousePos = mouseToNormalizedViewport();
					Vec2 mousePixelPos = Vec2{
						normalizedMousePos.x * (float)pickingTexture.width,
						normalizedMousePos.y * (float)pickingTexture.height
					};
					AnimObjId objId = mainFramebuffer.readPixelUint64(3, (int)mousePixelPos.x, (int)mousePixelPos.y);
					if (objId != NULL_ANIM_OBJECT)
					{
						Timeline::setActiveAnimObject((int)objId);
					}
					else
					{
						Timeline::setActiveAnimObject(NULL_ANIM_OBJECT);
					}
				}
			}
		}

		static void getLargestSizeForViewport(ImVec2* imageSize, ImVec2* offset)
		{
			float targetAspectRatio = Application::getOutputTargetAspectRatio();
			ImVec2 contentRegion = ImGui::GetContentRegionAvail();
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