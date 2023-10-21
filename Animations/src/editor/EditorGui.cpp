#include "editor/EditorGui.h"
#include "editor/panels/AnimObjectPanel.h"
#include "editor/panels/AssetManagerPanel.h"
#include "editor/panels/CodeEditorPanelManager.h"
#include "editor/panels/ConsoleLog.h"
#include "editor/panels/DebugPanel.h"
#include "editor/panels/ErrorPopups.h"
#include "editor/panels/ExportPanel.h"
#include "editor/panels/InspectorPanel.h"
#include "editor/panels/RegexTester.h"
#include "editor/panels/SceneHierarchyPanel.h"
#include "editor/timeline/Timeline.h"
#include "editor/imgui/ImGuiLayer.h"
#include "editor/imgui/ImGuiExtended.h"
#include "editor/Clipboard.h"
#include "editor/Gizmos.h"
#include "editor/EditorSettings.h"
#include "editor/EditorLayout.h"
#include "editor/UndoSystem.h"
#include "animation/AnimationManager.h"
#include "core/Application.h"
#include "core/Input.h"
#include "renderer/Colors.h"
#include "renderer/Texture.h"
#include "renderer/Framebuffer.h"
#include "core/Profiling.h"
#include "utils/FontAwesome.h"

#include <imgui.h>

namespace MathAnim
{
	struct ActionText
	{
		std::string text;
		float displayTimeLeft;
	};

	namespace EditorGui
	{
		// ------------- Internal Functions -------------
		static void handleNewCodeEditor();
		static void drawEditorViewport(const Framebuffer& editorFramebuffer, float deltaTime);
		static void getLargestSizeForViewport(ImVec2* imageSize, ImVec2* offset);
		static void checkHotKeys(AnimationManagerData* am);
		static void checkForMousePicking(const AnimationManagerData* am, const Framebuffer& mainFramebuffer);

		static void showActiveObjectSelctionCtxMenu(AnimationManagerData* am);

		// ------------- Internal data -------------
		static TimelineData timeline;
		static bool timelineLoaded = false;
		static ImVec2 viewportOffset;
		static ImVec2 viewportSize;
		static bool mouseHoveringViewport;
		static bool mainViewportIsActive;
		static bool editorViewportIsActive;
		static std::vector<ActionText> actionTextQueue;
		static ClipboardData* clipboard;
		static Texture gizmoPreviewTexture;
		static ImGuiID editorViewportDockId = {};

		static bool openActiveObjectSelectionContextMenu = false;
		static const char* openActiveObjectSelectionContextMenuId = "##ACTIVE_OBJECT_SELECTION_CTX_MENU";

		void init(AnimationManagerData* am, const std::filesystem::path& projectRoot, uint32 outputWidth, uint32 outputHeight)
		{
			actionTextQueue = {};
			viewportOffset = { 0, 0 };
			viewportSize = { 0, 0 };

			clipboard = {};

			if (!timelineLoaded)
			{
				timeline = Timeline::initInstance();
				Application::setFrameIndex(0);
			}
			Timeline::init(am);

			AnimObjectPanel::init();
			ExportPanel::init(outputWidth, outputHeight);
			SceneHierarchyPanel::init(am);
			AssetManagerPanel::init(projectRoot);
			EditorLayout::init(projectRoot);
			CodeEditorPanelManager::init();
			timelineLoaded = true;

			gizmoPreviewTexture = TextureBuilder()
				.setFilepath("./assets/images/gizmoPreviews.png")
				.generateFromFile();

			clipboard = Clipboard::create();
		}

		void update(const Framebuffer& mainFramebuffer, const Framebuffer& editorFramebuffer, AnimationManagerData* am, float deltaTime)
		{
			MP_PROFILE_EVENT("EditorGui_Update");

			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

			// NOTE: Begin will return whether the window is visible
			mainViewportIsActive = ImGui::Begin("Animation View", nullptr, ImGuiWindowFlags_MenuBar);
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
			ImGui::Image(textureId, mainViewportSize, ImVec2(0, 1), ImVec2(1, 0));
			ImGui::End();

			drawEditorViewport(editorFramebuffer, deltaTime);

			ImGui::PopStyleVar();

			AnimObjectPanel::update();
			DebugPanel::update(am);
			ExportPanel::update(am);
			SceneHierarchyPanel::update(am);
			AssetManagerPanel::update();
			EditorSettings::imgui(am);
			ConsoleLog::update();
			Timeline::update(timeline, am);
			InspectorPanel::update(am);
			CodeEditorPanelManager::update(am, editorViewportDockId);
			ErrorPopups::update(am);
			RegexTester::update();

			// TODO: Do this in a central file
			checkHotKeys(am);
			checkForMousePicking(am, editorFramebuffer);
		}

		void onGizmo(AnimationManagerData* am)
		{
			AnimObjId activeAnimObjectId = InspectorPanel::getActiveAnimObject();
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

			AnimId activeAnimationId = InspectorPanel::getActiveAnimation();
			Animation* activeAnimation = AnimationManager::getMutableAnimation(am, activeAnimationId);
			if (activeAnimation)
			{
				activeAnimation->onGizmo();
			}
		}

		Vec2 toNormalizedViewportCoords(const Vec2& screenCoords)
		{
			Vec2 viewportPos = toViewportCoords(screenCoords);
			viewportPos.x = (viewportPos.x / viewportSize.x);
			viewportPos.y = 1.0f - (viewportPos.y / viewportSize.y);
			return viewportPos;
		}

		Vec2 mouseToNormalizedViewport()
		{
			return toNormalizedViewportCoords(Vec2{ Input::mouseX, Input::mouseY });
		}

		Vec2 mouseToViewportCoords()
		{
			return toViewportCoords(Vec2{ Input::mouseX, Input::mouseY });
		}

		Vec2 toViewportCoords(const Vec2& screenCoords)
		{
			return screenCoords - Vec2{ viewportOffset.x, viewportOffset.y };
		}

		void free(AnimationManagerData* am)
		{
			gizmoPreviewTexture.destroy();

			AssetManagerPanel::free();
			SceneHierarchyPanel::free();
			ExportPanel::free();
			AnimObjectPanel::free();
			Timeline::freeInstance(timeline);
			Timeline::free(am);
			CodeEditorPanelManager::free();
			timelineLoaded = false;

			Clipboard::free(clipboard);
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

		void displayActionText(const std::string& actionText)
		{
			constexpr float timeToDisplaySeconds = 3.0f;
			actionTextQueue.emplace_back(ActionText{ actionText, timeToDisplaySeconds });
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

		bool anyEditorItemActive()
		{
			return ImGui::IsAnyItemActive();
		}

		BBox getViewportBounds()
		{
			BBox res;
			res.min = viewportOffset;
			res.max = viewportOffset + viewportSize;
			return res;
		}

		void copyObjectToClipboard(const AnimationManagerData* am, AnimObjId obj)
		{
			Clipboard::copyObjectTo(clipboard, am, obj);
		}

		AnimObjId duplicateObject(AnimationManagerData* am, AnimObjId obj)
		{
			return Clipboard::duplicateObject(clipboard, am, obj);
		}

		AnimObjId pasteObjectFromClipboardToParent(AnimationManagerData* am, AnimObjId newParent)
		{
			return Clipboard::pasteObjectToParent(clipboard, am, newParent);
		}

		AnimObjId pasteObjectFromClipboard(AnimationManagerData* am)
		{
			return Clipboard::pasteObjectToScene(clipboard, am);
		}

		// ------------- Internal Functions -------------
		static void drawEditorViewport(const Framebuffer& editorFramebuffer, float deltaTime)
		{
			editorViewportIsActive = ImGui::Begin("Animation Editor View", nullptr);
			editorViewportDockId = ImGui::GetWindowDockID();

			ImVec2 editorViewportRelativeOffset;
			getLargestSizeForViewport(&viewportSize, &editorViewportRelativeOffset);
			ImGui::SetCursorPos(editorViewportRelativeOffset);
			viewportOffset = ImGui::GetCursorScreenPos() - ImGui::GetMainViewport()->Pos;

			const Texture& editorColorTexture = editorFramebuffer.getColorAttachment(0);
			ImTextureID editorTextureId = (void*)(uintptr_t)editorColorTexture.graphicsId;
			ImGui::Image(editorTextureId, viewportSize, ImVec2(0, 1), ImVec2(1, 0));
			mouseHoveringViewport = ImGui::IsItemHovered();

			// Draw action text displayed in the bottom left corner of the viewport
			{
				ImGui::PushFont(ImGuiLayer::getMediumFont());

				ImDrawList* drawList = ImGui::GetWindowDrawList();
				float fontHeight = ImGui::GetFontSize();
				ImVec2 position = ImGui::GetWindowPos() + ImVec2(0.0f, ImGui::GetWindowSize().y);
				position.y -= fontHeight * (float)actionTextQueue.size();
				for (auto iter = actionTextQueue.begin(); iter != actionTextQueue.end(); iter++)
				{
					constexpr float fadeTime = 0.5f;
					float opacity = 1.0f;
					if (iter->displayTimeLeft <= fadeTime)
					{
						opacity = 1.0f - ((fadeTime - iter->displayTimeLeft) / fadeTime);
					}

					drawList->AddText(position, ImColor(1.0f, 1.0f, 1.0f, opacity), iter->text.c_str());
					position.y += fontHeight;

					iter->displayTimeLeft -= deltaTime;

					if (iter->displayTimeLeft <= 0.0f)
					{
						iter = actionTextQueue.erase(iter);
						if (actionTextQueue.size() == 0)
						{
							break;
						}
					}
				}

				ImGui::PopFont();
			}

			// Draw translate/scale/rotate mode dropdown at the top-right corner of the viewport
			static bool showCameraOrientationGizmo = true;
			{
				const char* chooseGizmoModePopupId = "CHOOSE_GIZMO_MODE_POPUP";

				ImDrawList* drawList = ImGui::GetCurrentWindow()->DrawList;

				constexpr ImVec2 comboSize = ImVec2(80.f, 40.f);
				constexpr ImVec2 comboHalfSize = ImVec2(40.f, 40.f);
				constexpr float comboHzPadding = 15.f;
				constexpr float comboVtPadding = 15.f;

				ImVec2 topLeft = editorViewportRelativeOffset;
				topLeft.x += viewportSize.x - comboSize.x - comboHzPadding;
				topLeft.y += comboVtPadding;
				ImGui::SetCursorPos(topLeft);

				ImVec2 previewUvMin = ImVec2(0.0f / (float)gizmoPreviewTexture.width, 1.0f);
				ImVec2 previewUvMax = ImVec2(77.0f / (float)gizmoPreviewTexture.width, 1.0f - (77.0f / (float)gizmoPreviewTexture.height));

				GizmoType gizmoType = GizmoManager::getVisualMode();

				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

				if (ImGui::InvisibleButton(
					"GizmoPreviewButton",
					comboHalfSize
				))
				{
					if (gizmoType == GizmoType::None)
					{
						GizmoManager::changeVisualMode(GizmoType::Translation);
					}
					else
					{
						GizmoManager::changeVisualMode(GizmoType::None);
					}
				}

				// Draw the actual button
				ImVec2 windowPos = ImGui::GetCurrentWindow()->Pos;
				bool enableButtonHovered = ImGui::IsItemHovered();
				ImVec4 bgColor = gizmoType != GizmoType::None
					? enableButtonHovered ? Colors::Primary[1] : Colors::Primary[2]
					: enableButtonHovered ? Colors::Neutral[4] : Colors::Neutral[5];
				constexpr float rounding = 4.f;
				drawList->AddRectFilled(
					topLeft + windowPos,
					topLeft + comboHalfSize + windowPos,
					ImColor(bgColor),
					rounding
				);
				drawList->AddImage(
					(void*)(uintptr_t)gizmoPreviewTexture.graphicsId,
					topLeft + windowPos + ImVec2(rounding, rounding),
					topLeft + comboHalfSize + windowPos - ImVec2(rounding, rounding),
					previewUvMin,
					previewUvMax
				);

				ImGui::SetCursorPos(topLeft + ImVec2(comboHalfSize.x, 0.0f));
				std::string chevronLabel = ICON_FA_CHEVRON_DOWN + std::string("##GizmoPreviewDropdown");
				if (ImGui::InvisibleButton(
					chevronLabel.c_str(),
					comboHalfSize
				))
				{
					ImGui::OpenPopup(chooseGizmoModePopupId);
				}

				ImColor dropdownArrowBgColor = ImGui::IsItemHovered()
					? Colors::Neutral[5]
					: Colors::Neutral[7];
				drawList->AddRectFilled(
					topLeft + ImVec2(comboHalfSize.x, 0.f) + windowPos,
					topLeft + comboSize + windowPos,
					ImColor(dropdownArrowBgColor),
					rounding
				);

				ImColor chevronColor = Colors::Neutral[2];
				ImVec2 chevronSize = ImGui::CalcTextSize(ICON_FA_ANGLE_DOWN);
				// Center the chevron on the button
				ImVec2 chevronPos = topLeft + ImVec2(comboHalfSize.x, 0.f) + windowPos +
					(comboHalfSize / 2.0f) - (chevronSize / 2.0f);
				drawList->AddText(chevronPos, chevronColor, ICON_FA_ANGLE_DOWN);

				ImGui::PopStyleVar();

				ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, 10));
				if (ImGui::BeginPopupContextItem(chooseGizmoModePopupId))
				{
					ImGui::TextColored(ImColor(Colors::Neutral[3]), "Object Gizmos");

					if (ImGui::Selectable("Translate"))
					{
						GizmoManager::changeVisualMode(GizmoType::Translation);
					}

					if (ImGui::Selectable("Rotate"))
					{
						GizmoManager::changeVisualMode(GizmoType::Rotation);
					}

					if (ImGui::Selectable("Scale"))
					{
						GizmoManager::changeVisualMode(GizmoType::Scaling);
					}

					if (ImGui::Selectable("Toggle Camera Orientation"))
					{
						showCameraOrientationGizmo = !showCameraOrientationGizmo;
					}

					ImGui::EndPopup();
				}
				ImGui::PopStyleVar();

				// Set cursor for next drawing thing
				ImVec2 bottomRight = topLeft + comboSize + ImVec2(0.f, comboVtPadding);
				ImGui::SetCursorPos(bottomRight);
			}

			// Composite camera orientation texture in top right corner of the window
			if (showCameraOrientationGizmo)
			{
				constexpr float cameraOrientationGizmoVtPadding = 15.f;

				const Texture& cameraOrientationTexture = GizmoManager::getCameraOrientationTexture();

				ImGui::SetCursorPosX(ImGui::GetCursorPosX() - cameraOrientationTexture.width);
				ImGui::SetCursorPosY(ImGui::GetCursorPosY() + cameraOrientationGizmoVtPadding);

				ImTextureID cameraOrientationTextureId = (void*)(uintptr_t)cameraOrientationTexture.graphicsId;
				ImGui::Image(
					cameraOrientationTextureId,
					ImVec2((float)cameraOrientationTexture.width, (float)cameraOrientationTexture.height),
					ImVec2(0, 1),
					ImVec2(1, 0)
				);
			}

			ImGui::End();
		}

		static void checkHotKeys(AnimationManagerData* am)
		{
			AnimObjId activeAnimObj = InspectorPanel::getActiveAnimObject();
			const AnimObject* activeObject = AnimationManager::getObject(am, activeAnimObj);
			bool mouseHoveringViewportOrScenePanel = mouseHoveringViewport || SceneHierarchyPanel::mouseIsHovered();
			ImGuiIO& io = ImGui::GetIO();

			// TODO: Should undo+redo be global or should we only do them when hovering over
			//       certain panels?
			if (!io.WantTextInput && Input::keyPressed(GLFW_KEY_Z, KeyMods::Ctrl))
			{
				UndoSystem::undo(Application::getUndoSystem());
			}

			if (!io.WantTextInput && Input::keyPressed(GLFW_KEY_Z, KeyMods::Ctrl | KeyMods::Shift))
			{
				UndoSystem::redo(Application::getUndoSystem());
			}

			// Ctrl+S (Save Project)
			if (Input::keyPressed(GLFW_KEY_S, KeyMods::Ctrl))
			{
				Application::saveProject();
			}

			// Ctrl+C and mouse hovering viewport/scene heirarchy panel and active object
			// Copy object to clipboard
			if (mouseHoveringViewportOrScenePanel && !isNull(activeAnimObj) && activeObject && Input::keyPressed(GLFW_KEY_C, KeyMods::Ctrl))
			{
				copyObjectToClipboard(am, activeAnimObj);
			}

			// Ctrl+V and mouse hovering viewport/scene heirarchy panel and active object
			// Paste object from clipboard to scene
			if (mouseHoveringViewportOrScenePanel && Clipboard::getType(clipboard) == ClipboardContents::GameObject &&
				Input::keyPressed(GLFW_KEY_V, KeyMods::Ctrl))
			{
				activeAnimObj = pasteObjectFromClipboard(am);
			}

			// Ctrl+D and mouse hovering viewport/scene heirarchy panel and duplicate the active object
			// Duplicate active object
			if (mouseHoveringViewportOrScenePanel && !isNull(activeAnimObj) && Input::keyPressed(GLFW_KEY_D, KeyMods::Ctrl))
			{
				activeAnimObj = duplicateObject(am, activeAnimObj);
			}

			// Shift+G (Open Group Menu pressed)
			if (mouseHoveringViewport && !isNull(activeAnimObj) && Input::keyPressed(GLFW_KEY_G, KeyMods::Shift))
			{
				Input::keyPressed(GLFW_KEY_G, KeyMods::Shift);
				openActiveObjectSelectionContextMenu = true;
			}

			// Space is pressed, handle Pause/Play of timeline
			if (!io.WantTextInput && Input::keyPressed(GLFW_KEY_SPACE))
			{
				AnimState currentPlayState = Application::getEditorPlayState();
				AnimState newState = currentPlayState == AnimState::PlayForward
					? AnimState::Pause
					: AnimState::PlayForward;
				Application::setEditorPlayState(newState);
			}

			// Handle Shift+G Popup
			if (openActiveObjectSelectionContextMenu)
			{
				ImGui::OpenPopup(openActiveObjectSelectionContextMenuId);
				openActiveObjectSelectionContextMenu = false;
			}

			showActiveObjectSelctionCtxMenu(am);
		}

		static void checkForMousePicking(const AnimationManagerData* am, const Framebuffer& mainFramebuffer)
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
						InspectorPanel::setActiveAnimObject(am, objId);
					}
					else
					{
						InspectorPanel::setActiveAnimObject(am, NULL_ANIM_OBJECT);
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

		static void showActiveObjectSelctionCtxMenu(AnimationManagerData* am)
		{
			if (ImGui::BeginPopupContextItem(openActiveObjectSelectionContextMenuId))
			{
				ImGui::PushStyleColor(ImGuiCol_Text, Colors::Neutral[2]);
				ImGui::PushStyleColor(ImGuiCol_Separator, Colors::Neutral[2]);
				ImGui::Text("Select Grouped");
				ImGui::Separator();
				ImGui::PopStyleColor(2);

				AnimObjId activeObject = InspectorPanel::getActiveAnimObject();
				const AnimObject* obj = AnimationManager::getObject(am, activeObject);
				if (ImGui::Selectable("Root"))
				{
					// Find the parent
					AnimObjId newActiveObj = activeObject;
					AnimObjId parent = NULL_ANIM_OBJECT;
					if (obj)
					{
						parent = obj->parentId;
					}
					while (!isNull(parent))
					{
						obj = AnimationManager::getObject(am, parent);
						if (obj)
						{
							parent = obj->parentId;
							newActiveObj = obj->id;
						}
						else
						{
							parent = NULL_ANIM_OBJECT;
						}
					}

					InspectorPanel::setActiveAnimObject(am, newActiveObj);
				}
				if (ImGui::Selectable("Parent"))
				{
					// Find the parent
					if (obj && !isNull(obj->parentId))
					{
						InspectorPanel::setActiveAnimObject(am, obj->parentId);
					}
				}
				if (ImGui::Selectable("First Child"))
				{
					std::vector<AnimObjId> children = AnimationManager::getChildren(am, activeObject);
					if (children.size() > 0)
					{
						InspectorPanel::setActiveAnimObject(am, children[0]);
					}
				}
				if (ImGui::Selectable("Next Sibling"))
				{
					AnimObjId sibling = AnimationManager::getNextSibling(am, activeObject);
					InspectorPanel::setActiveAnimObject(am, sibling);
				}
				ImGui::EndPopup();
			}
		}
	}
}