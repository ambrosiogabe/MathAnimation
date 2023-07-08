#include "editor/panels/InspectorPanel.h"
#include "editor/panels/SceneHierarchyPanel.h"
#include "editor/imgui/ImGuiExtended.h"
#include "editor/imgui/ImGuiLayer.h"
#include "editor/UndoSystem.h"
#include "animation/AnimationManager.h"
#include "animation/Animation.h"
#include "animation/TextAnimations.h"
#include "svg/Svg.h"
#include "svg/SvgParser.h"
#include "renderer/Fonts.h"
#include "renderer/Colors.h"
#include "scripting/LuauLayer.h"
#include "utils/IconsFontAwesome5.h"
#include "platform/Platform.h"
#include "core/Application.h"

#include <imgui.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

#include <nfd.h>

namespace MathAnim
{
	namespace InspectorPanel
	{
		// ------- Private variables --------
		static AnimObjId activeAnimObjectId = NULL_ANIM_OBJECT;
		static AnimId activeAnimationId = NULL_ANIM;
		static std::vector<AnimObjId> allActiveObjects = {};

		static constexpr float slowDragSpeed = 0.02f;
		static constexpr float indentationDepth = 25.0f;
		static bool svgErrorPopupOpen = false;
		static const char* ERROR_IMPORTING_SVG_POPUP = "Error Importing SVG##ERROR_IMPORTING_SVG";
		static const char* ANIM_OBJECT_DROP_TARGET_ID = "ANIM_OBJECT_DROP_TARGET_ID";

		// ---------------------- Internal Functions ----------------------
		static void checkSvgErrorPopup();
		static bool applySettingToChildren(const char* id);

		// ---------------------- Inspector functions ----------------------
		static void handleAnimObjectInspector(AnimationManagerData* am, AnimObjId animObjectId);
		static void handleAnimationInspector(AnimationManagerData* am, AnimId animationId);
		static void handleTextObjectInspector(AnimationManagerData* am, AnimObject* object);
		static void handleCodeBlockInspector(AnimationManagerData* am, AnimObject* object);
		static void handleLaTexObjectInspector(AnimObject* object);
		static void handleSvgFileObjectInspector(AnimationManagerData* am, AnimObject* object);
		static void handleCameraObjectInspector(AnimationManagerData* am, AnimObject* object);
		static void handleMoveToAnimationInspector(AnimationManagerData* am, Animation* animation);
		static void handleAnimateScaleInspector(AnimationManagerData* am, Animation* animation);
		static void handleTransformAnimation(AnimationManagerData* am, Animation* animation);
		static void handleShiftInspector(Animation* animation);
		static void handleRotateToAnimationInspector(Animation* animation);
		static void handleAnimateStrokeColorAnimationInspector(Animation* animation);
		static void handleAnimateFillColorAnimationInspector(Animation* animation);
		static void handleCircumscribeInspector(AnimationManagerData* am, Animation* animation);
		static void handleSquareInspector(AnimObject* object);
		static void handleCircleInspector(AnimObject* object);
		static void handleArrowInspector(AnimObject* object);
		static void handleCubeInspector(AnimationManagerData* am, AnimObject* object);
		static void handleAxisInspector(AnimObject* object);
		static void handleScriptObjectInspector(AnimationManagerData* am, AnimObject* object);
		static void handleImageObjectInspector(AnimationManagerData* am, AnimObject* object);

		void update(AnimationManagerData* am)
		{
			ImGui::Begin(ICON_FA_LIST " Inspector");
			if (ImGui::CollapsingHeader("Animation Object Inspector", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Indent(indentationDepth);
				if (!isNull(activeAnimObjectId))
				{
					handleAnimObjectInspector(am, activeAnimObjectId);
					// NOTE: Don't update object state while the animation is playing because
					//       it messes up the playback
					// TODO: Investigate the root cause of this issue and fix that instead (why don't ya?)
					if (Application::getEditorPlayState() == AnimState::Pause)
					{
						// TODO: This slows stuff down a lot. Optimize the heck out of it
						AnimationManager::updateObjectState(am, activeAnimObjectId);
					}
				}
				ImGui::Unindent(indentationDepth);
			}

			ImGui::Separator();
			if (ImGui::CollapsingHeader("Animation Inspector", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Indent(indentationDepth);
				if (!isNull(activeAnimationId))
				{
					handleAnimationInspector(am, activeAnimationId);
				}
				ImGui::Unindent(indentationDepth);
			}

			ImGui::End();

			checkSvgErrorPopup();
		}

		void free()
		{
			// TODO: Serialize this
			activeAnimationId = NULL_ANIM;
			activeAnimObjectId = NULL_ANIM_OBJECT;
		}

		void setActiveAnimation(AnimId animationId)
		{
			activeAnimationId = animationId;
		}

		void setActiveAnimObject(const AnimationManagerData* am, AnimObjId animObjectId)
		{
			activeAnimObjectId = animObjectId;
			allActiveObjects.clear();

			const AnimObject* obj = AnimationManager::getObject(am, animObjectId);
			if (obj)
			{
				// Push animObjectId and all children to vector
				allActiveObjects.push_back(animObjectId);
				for (auto iter = obj->beginBreadthFirst(am); iter != obj->end(); ++iter)
				{
					allActiveObjects.push_back(*iter);
				}
			}
		}

		AnimObjId getActiveAnimObject()
		{
			return activeAnimObjectId;
		}

		AnimId getActiveAnimation()
		{
			return activeAnimationId;
		}

		const std::vector<AnimObjId>& getAllActiveAnimObjects()
		{
			return allActiveObjects;
		}

		const char* getAnimObjectPayloadId()
		{
			return ANIM_OBJECT_DROP_TARGET_ID;
		}

		// ---------------------- Internal Functions ----------------------
		static void checkSvgErrorPopup()
		{
			if (svgErrorPopupOpen)
			{
				ImGui::OpenPopup(ERROR_IMPORTING_SVG_POPUP);
				svgErrorPopupOpen = false;
			}

			// Always center this window when appearing
			ImVec2 center = ImGui::GetMainViewport()->GetCenter();
			ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

			if (ImGui::BeginPopupModal(ERROR_IMPORTING_SVG_POPUP, NULL, ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::TextColored(Colors::AccentRed[3], "Error:");
				ImGui::SameLine();
				ImGui::Text("%s", SvgParser::getLastError());
				ImGui::NewLine();
				ImGui::Separator();

				if (ImGui::Button("OK", ImVec2(120, 0)))
				{
					ImGui::CloseCurrentPopup();
				}
				ImGui::SetItemDefaultFocus();
				ImGui::EndPopup();
			}
		}

		static bool applySettingToChildren(const char* id)
		{
			bool res = false;
			if (ImGui::BeginPopupContextItem(id))
			{
				if (ImGui::Selectable("Apply to children"))
				{
					res = true;
				}

				ImGui::EndPopup();
			}

			return res;
		}

		// ---------------------- Inspector functions ----------------------
		static void handleAnimObjectInspector(AnimationManagerData* am, AnimObjId animObjectId)
		{
			AnimObject* animObject = AnimationManager::getMutableObject(am, animObjectId);
			if (!animObject)
			{
				g_logger_error("No anim object with id '{}' exists", animObject);
				activeAnimObjectId = NULL_ANIM_OBJECT;
				return;
			}

			std::string transformComponentName = "Transform##" + std::to_string(animObjectId);
			if (ImGui::CollapsingHeader(transformComponentName.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
			{
				constexpr int scratchLength = 256;
				char scratch[scratchLength] = {};
				if (animObject->nameLength < scratchLength - 1)
				{
					g_memory_copyMem(scratch, animObject->name, animObject->nameLength * sizeof(char));
					scratch[animObject->nameLength] = '\0';
					if (auto res = ImGuiExtended::InputTextEx(": Name", scratch, scratchLength * sizeof(char));
						res.editState != EditState::NotEditing)
					{
						if (res.editState == EditState::FinishedEditing)
						{
							UndoSystem::setStringProp(
								Application::getUndoSystem(),
								animObject->id,
								res.ogData,
								(char*)animObject->name,
								StringPropType::Name
							);
						}
						else
						{
							animObject->setName(scratch);
						}
					}
				}
				else
				{
					g_logger_error("Anim Object name has more 256 characters. Tell Gabe to increase scratch length for Anim Object names.");
				}

				// Position
				if (auto res = ImGuiExtended::DragFloat3Ex(": Position", &animObject->_positionStart, slowDragSpeed);
					res.editState == EditState::FinishedEditing)
				{
					UndoSystem::setVec3Prop(
						Application::getUndoSystem(),
						animObject->id,
						res.ogData,
						animObject->_positionStart,
						Vec3PropType::Position
					);
				}

				// Rotation
				if (auto res = ImGuiExtended::DragFloat3Ex(": Rotation", &animObject->_rotationStart);
					res.editState == EditState::FinishedEditing)
				{
					UndoSystem::setVec3Prop(
						Application::getUndoSystem(),
						animObject->id,
						res.ogData,
						animObject->_rotationStart,
						Vec3PropType::Rotation
					);
				}

				// Scale
				if (auto res = ImGuiExtended::DragFloat3Ex(": Scale", &animObject->_scaleStart, slowDragSpeed);
					res.editState == EditState::FinishedEditing)
				{
					UndoSystem::setVec3Prop(
						Application::getUndoSystem(),
						animObject->id,
						res.ogData,
						animObject->_scaleStart,
						Vec3PropType::Scale
					);
				}
			}

			std::string svgPropsComponentName = "Svg Properties##" + std::to_string(animObjectId);
			if (ImGui::CollapsingHeader(svgPropsComponentName.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
			{
				// TODO: Remove this once you get auto SVG stuff working good
				if (ImGui::DragFloat(": SVG Scale", &animObject->svgScale, slowDragSpeed))
				{
				}

				// NanoVG only allows stroke width between [0-200] so we reflect that here
				if (auto res = ImGuiExtended::DragFloatEx(": Stroke Width", (float*)&animObject->_strokeWidthStart, 1.0f, 0.0f, 200.0f);
					res.editState == EditState::FinishedEditing)
				{
					UndoSystem::setFloatProp(
						Application::getUndoSystem(),
						animObject->id,
						res.ogData,
						animObject->_strokeWidthStart,
						FloatPropType::StrokeWidth
					);
				}
				if (applySettingToChildren("##StrokeWidthChildrenApply"))
				{

				}

				// Stroke Color
				if (auto res = ImGuiExtended::ColorEdit4Ex(": Stroke Color", &animObject->_strokeColorStart);
					res.editState == EditState::FinishedEditing)
				{
					UndoSystem::setU8Vec4Prop(
						Application::getUndoSystem(),
						animObject->id,
						res.ogData,
						animObject->_strokeColorStart,
						U8Vec4PropType::StrokeColor
					);
				}

				if (applySettingToChildren("##StrokeColorChildrenApply"))
				{
					UndoSystem::applyU8Vec4ToChildren(
						Application::getUndoSystem(),
						animObject->id,
						U8Vec4PropType::StrokeColor
					);
				}

				// Fill Color
				if (auto res = ImGuiExtended::ColorEdit4Ex(": Fill Color", &animObject->_fillColorStart);
					res.editState == EditState::FinishedEditing)
				{
					UndoSystem::setU8Vec4Prop(
						Application::getUndoSystem(),
						animObject->id,
						res.ogData,
						animObject->_fillColorStart,
						U8Vec4PropType::FillColor
					);
				}

				if (applySettingToChildren("##FillColorChildrenApply"))
				{
					UndoSystem::applyU8Vec4ToChildren(
						Application::getUndoSystem(),
						animObject->id,
						U8Vec4PropType::FillColor
					);
				}

				static bool showAdvancedStuff = false;
				ImGui::Checkbox(": Advanced", &showAdvancedStuff);
				if (showAdvancedStuff)
				{
					ImGui::Checkbox(": Draw Debug Boxes", &animObject->drawDebugBoxes);
					if (animObject->drawDebugBoxes)
					{
						ImGui::Checkbox(": Draw Curve Debug Boxes", &animObject->drawCurveDebugBoxes);
					}
					ImGui::Checkbox(": Draw Curves", &animObject->drawCurves);
					ImGui::Checkbox(": Draw Control Points", &animObject->drawControlPoints);
				}
			}

			std::string componentName = std::string(_animationObjectTypeNames[(uint8)animObject->objectType])
				+ "##" + std::to_string(animObjectId);

			if (bool shouldShow = !_isInternalObjectOnly[(uint8)animObject->objectType];
				shouldShow && ImGui::CollapsingHeader(componentName.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
			{
				switch (animObject->objectType)
				{
				case AnimObjectTypeV1::TextObject:
					handleTextObjectInspector(am, animObject);
					break;
				case AnimObjectTypeV1::CodeBlock:
					handleCodeBlockInspector(am, animObject);
					break;
				case AnimObjectTypeV1::LaTexObject:
					handleLaTexObjectInspector(animObject);
					break;
				case AnimObjectTypeV1::Square:
					handleSquareInspector(animObject);
					break;
				case AnimObjectTypeV1::Circle:
					handleCircleInspector(animObject);
					break;
				case AnimObjectTypeV1::Cube:
					handleCubeInspector(am, animObject);
					break;
				case AnimObjectTypeV1::Axis:
					handleAxisInspector(animObject);
					break;
				case AnimObjectTypeV1::SvgFileObject:
					handleSvgFileObjectInspector(am, animObject);
					break;
				case AnimObjectTypeV1::Camera:
					handleCameraObjectInspector(am, animObject);
					break;
				case AnimObjectTypeV1::SvgObject:
					// NOP
					break;
				case AnimObjectTypeV1::ScriptObject:
					handleScriptObjectInspector(am, animObject);
					break;
				case AnimObjectTypeV1::Image:
					handleImageObjectInspector(am, animObject);
					break;
				case AnimObjectTypeV1::Arrow:
					handleArrowInspector(animObject);
					break;
				case AnimObjectTypeV1::Length:
				case AnimObjectTypeV1::None:
					break;
				}
			}
		}

		static void handleAnimationInspector(AnimationManagerData* am, AnimId animationId)
		{
			Animation* animation = AnimationManager::getMutableAnimation(am, animationId);
			if (!animation)
			{
				g_logger_error("No animation with id '{}' exists", animationId);
				activeAnimationId = NULL_ANIM;
				return;
			}

			std::string animPropsComponentName = "Animation Properties##" + std::to_string(animationId);
			if (ImGui::CollapsingHeader(animPropsComponentName.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
			{
				int currentType = (int)(animation->easeType) - 1;
				if (auto res = ImGuiExtended::ComboEx(": Ease Type", &currentType, &easeTypeNames[1], (int)EaseType::Length - 1);
					res.editState == EditState::FinishedEditing)
				{
					UndoSystem::setEnumProp(
						Application::getUndoSystem(),
						animation->id,
						res.ogData + 1,
						currentType + 1,
						EnumPropType::EaseType
					);
				}

				int currentDirection = (int)(animation->easeDirection) - 1;
				if (auto res = ImGuiExtended::ComboEx(": Ease Direction", &currentDirection, &easeDirectionNames[1], (int)EaseDirection::Length - 1);
					res.editState == EditState::FinishedEditing)
				{
					UndoSystem::setEnumProp(
						Application::getUndoSystem(),
						animation->id,
						res.ogData + 1,
						currentDirection + 1,
						EnumPropType::EaseDirection
					);
				}

				int currentPlaybackType = (int)(animation->playbackType) - 1;
				if (auto res = ImGuiExtended::ComboEx(": Playback Type", &currentPlaybackType, &_playbackTypeNames[1], (int)PlaybackType::Length - 1);
					res.editState == EditState::FinishedEditing)
				{
					UndoSystem::setEnumProp(
						Application::getUndoSystem(),
						animation->id,
						res.ogData + 1,
						currentPlaybackType + 1,
						EnumPropType::PlaybackType
					);
				}

				ImGui::BeginDisabled(animation->playbackType == PlaybackType::Synchronous);
				if (auto res = ImGuiExtended::DragFloatEx(": Lag Ratio", &animation->lagRatio, slowDragSpeed, 0.0f, 1.0f);
					res.editState == EditState::FinishedEditing)
				{
					UndoSystem::setFloatProp(
						Application::getUndoSystem(),
						animation->id,
						res.ogData,
						animation->lagRatio,
						FloatPropType::LagRatio
					);
				}
				ImGui::EndDisabled();


				if (Animation::isAnimationGroup(animation->type))
				{
					static bool isAddingAnimObject = false;
					ImGui::PushStyleColor(ImGuiCol_FrameBg, Colors::Neutral[7]);

					// TODO: UI needs some work
					if (ImGui::BeginListBox(": Anim Objects", ImVec2(0.0f, 5 * ImGui::GetTextLineHeightWithSpacing())))
					{
						std::unordered_set<AnimObjId> objectIdsCopy = animation->animObjectIds;
						for (auto animObjectIdIter = objectIdsCopy.begin(); animObjectIdIter != objectIdsCopy.end(); animObjectIdIter++)
						{
							const AnimObject* obj = AnimationManager::getObject(am, *animObjectIdIter);
							if (obj)
							{
								static ImVec2 buttonSize = ImVec2(0.0f, 0.0f);

								// Draw custom rect for background of textbox
								ImVec2 framePadding = ImGui::GetStyle().FramePadding;
								framePadding.y *= 0.7f;
								ImVec2 pMin = ImGui::GetCursorScreenPos() - framePadding;
								ImVec2 pMax = pMin + ImVec2(ImGui::GetContentRegionAvail().x, buttonSize.y) + (framePadding * 2.0f);
								ImGui::GetWindowDrawList()->AddRectFilled(
									pMin, pMax, ImColor(Colors::Neutral[5])
								);

								// Treat the uint64 as a pointer ID so ImGui hashes it into an int
								ImGui::PushID((const void*)*animObjectIdIter);
								float oldCursorPosY = ImGui::GetCursorPosY();
								ImVec2 textSize = ImGui::CalcTextSize((char*)obj->name);
								ImGui::SetCursorPosY(oldCursorPosY + (buttonSize.y / 2.0f - textSize.y / 2.0f));
								ImGui::Text((char*)obj->name);
								ImGui::SameLine();
								ImGui::SetCursorPosY(oldCursorPosY);

								ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x - buttonSize.x);
								if (ImGui::Button(ICON_FA_MINUS "##RemoveAnimObjectFromAnim"))
								{
									AnimationManager::removeObjectFromAnim(am, *animObjectIdIter, animation->id);
								}
								buttonSize = ImGui::GetItemRectSize();

								ImGui::PopID();

								ImGui::SetCursorScreenPos(
									ImVec2(ImGui::GetCursorScreenPos().x, pMax.y + framePadding.y * 2.0f)
								);
							}
						}

						if (isAddingAnimObject)
						{
							const char dummyInputText[] = "Drag Object Here";
							size_t dummyInputTextSize = sizeof(dummyInputText);
							ImGui::BeginDisabled();
							ImGui::InputText("##AnimObjectDropTarget", (char*)dummyInputText, dummyInputTextSize, ImGuiInputTextFlags_ReadOnly);
							ImGui::EndDisabled();

							if (auto objPayload = ImGuiExtended::AnimObjectDragDropTarget(); objPayload != nullptr)
							{
								bool exists =
									std::find(
										animation->animObjectIds.begin(),
										animation->animObjectIds.end(),
										objPayload->animObjectId
									) != animation->animObjectIds.end();

								if (!exists)
								{
									AnimationManager::addObjectToAnim(am, objPayload->animObjectId, animation->id);
								}
								isAddingAnimObject = false;
							}
						}

						ImGui::EndListBox();
					}

					ImGui::PopStyleColor();

					if (ImGui::Button(ICON_FA_PLUS " Add Anim Object"))
					{
						isAddingAnimObject = true;
					}
				}
			}

			std::string componentName = std::string(_animationTypeNames[(uint8)animation->type])
				+ "##" + std::to_string(animationId);

			if (ImGui::CollapsingHeader(componentName.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
			{
				switch (animation->type)
				{
				case AnimTypeV1::Create:
				case AnimTypeV1::UnCreate:
				case AnimTypeV1::FadeIn:
				case AnimTypeV1::FadeOut:
					// NOP
					break;
				case AnimTypeV1::Transform:
					handleTransformAnimation(am, animation);
					break;
				case AnimTypeV1::MoveTo:
					handleMoveToAnimationInspector(am, animation);
					break;
				case AnimTypeV1::AnimateScale:
					handleAnimateScaleInspector(am, animation);
					break;
				case AnimTypeV1::Shift:
					handleShiftInspector(animation);
					break;
				case AnimTypeV1::RotateTo:
					handleRotateToAnimationInspector(animation);
					break;
				case AnimTypeV1::AnimateFillColor:
					handleAnimateFillColorAnimationInspector(animation);
					break;
				case AnimTypeV1::AnimateStrokeColor:
					handleAnimateStrokeColorAnimationInspector(animation);
					break;
				case AnimTypeV1::AnimateStrokeWidth:
					//handleAnimateStrokeWidthInspector(animation);
					g_logger_warning("TODO: Implement me.");
					break;
				case AnimTypeV1::Circumscribe:
					handleCircumscribeInspector(am, animation);
					break;
				case AnimTypeV1::Length:
				case AnimTypeV1::None:
					break;
				}
			}
		}

		static void handleTextObjectInspector(AnimationManagerData* am, AnimObject* object)
		{
			bool shouldRegenerate = false;

			const std::vector<std::string>& fonts = Platform::getAvailableFonts();
			int fontIndex = -1;
			if (object->as.textObject.font != nullptr)
			{
				std::filesystem::path fontFilepath = object->as.textObject.font->fontFilepath;
				fontFilepath.make_preferred();
				int index = 0;
				for (const auto& font : fonts)
				{
					std::filesystem::path f1 = std::filesystem::path(font);
					f1.make_preferred();
					if (fontFilepath == f1)
					{
						fontIndex = index;
						break;
					}
					index++;
				}
			}
			const char* previewValue = "No Font Selected";
			if (fontIndex != -1)
			{
				previewValue = fonts[fontIndex].c_str();
			}
			else
			{
				if (object->as.textObject.font != nullptr)
				{
					g_logger_warning("Could not find font '{}'", object->as.textObject.font->fontFilepath);
				}
			}

			if (ImGui::BeginCombo(": Font", previewValue))
			{
				for (int n = 0; n < fonts.size(); n++)
				{
					const bool is_selected = (fontIndex == n);
					if (ImGui::Selectable(fonts[n].c_str(), is_selected))
					{
						if (object->as.textObject.font)
						{
							Fonts::unloadFont(object->as.textObject.font);
						}
						object->as.textObject.font = Fonts::loadFont(fonts[n].c_str());
						shouldRegenerate = true;
					}

					// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
					if (is_selected)
					{
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}

			constexpr int scratchLength = 256;
			char scratch[scratchLength] = {};
			if (object->as.textObject.textLength >= scratchLength)
			{
				g_logger_error("Text object has more than 256 characters. Tell Gabe to increase scratch length for text objects.");
				return;
			}
			g_memory_copyMem(scratch, object->as.textObject.text, object->as.textObject.textLength * sizeof(char));
			scratch[object->as.textObject.textLength] = '\0';
			if (ImGui::InputTextMultiline(": Text", scratch, scratchLength * sizeof(char)))
			{
				size_t newLength = std::strlen(scratch);
				object->as.textObject.text = (char*)g_memory_realloc(object->as.textObject.text, sizeof(char) * (newLength + 1));
				object->as.textObject.textLength = (int32_t)newLength;
				g_memory_copyMem(object->as.textObject.text, scratch, newLength * sizeof(char));
				object->as.textObject.text[newLength] = '\0';
				shouldRegenerate = true;
			}

			if (shouldRegenerate)
			{
				object->as.textObject.reInit(am, object);
			}
		}

		static void handleCodeBlockInspector(AnimationManagerData* am, AnimObject* object)
		{
			bool shouldRegenerate = false;

			constexpr int scratchLength = 512;
			char scratch[scratchLength] = {};
			if (object->as.codeBlock.textLength >= scratchLength)
			{
				g_logger_error("Text object has more than 512 characters. Tell Gabe to increase scratch length for code blocks.");
				return;
			}

			int currentLang = (int)object->as.codeBlock.language - 1;
			if (ImGui::Combo(": Language", &currentLang, _highlighterLanguageNames.data() + 1, (int)HighlighterLanguage::Length - 1))
			{
				g_logger_assert(currentLang >= 0 && currentLang < (int)HighlighterLanguage::Length - 1, "How did this happen?");
				object->as.codeBlock.language = (HighlighterLanguage)(currentLang + 1);
				shouldRegenerate = true;
			}

			int currentTheme = (int)object->as.codeBlock.theme - 1;
			if (ImGui::Combo(": Theme", &currentTheme, _highlighterThemeNames.data() + 1, (int)HighlighterTheme::Length - 1))
			{
				g_logger_assert(currentTheme >= 0 && currentTheme < (int)HighlighterTheme::Length - 1, "How did this happen?");
				object->as.codeBlock.theme = (HighlighterTheme)(currentTheme + 1);
				shouldRegenerate = true;
			}

			g_memory_copyMem(scratch, object->as.codeBlock.text, object->as.codeBlock.textLength * sizeof(char));
			scratch[object->as.codeBlock.textLength] = '\0';
			if (ImGui::InputTextMultiline(": Code", scratch, scratchLength * sizeof(char)))
			{
				size_t newLength = std::strlen(scratch);
				object->as.codeBlock.text = (char*)g_memory_realloc(object->as.codeBlock.text, sizeof(char) * (newLength + 1));
				object->as.codeBlock.textLength = (int32_t)newLength;
				g_memory_copyMem(object->as.codeBlock.text, scratch, newLength * sizeof(char));
				object->as.codeBlock.text[newLength] = '\0';
				shouldRegenerate = true;
			}

			if (shouldRegenerate)
			{
				object->as.codeBlock.reInit(am, object);
			}
		}

		static void handleLaTexObjectInspector(AnimObject* object)
		{
			constexpr int scratchLength = 2048;
			char scratch[scratchLength] = {};
			if (object->as.laTexObject.textLength >= scratchLength)
			{
				g_logger_error("Text object has more than '{}' characters. Tell Gabe to increase scratch length for LaTex objects.", scratchLength);
				return;
			}
			g_memory_copyMem(scratch, object->as.laTexObject.text, object->as.laTexObject.textLength * sizeof(char));
			scratch[object->as.laTexObject.textLength] = '\0';
			if (ImGui::InputTextMultiline(": LaTeX", scratch, scratchLength * sizeof(char)))
			{
				size_t newLength = std::strlen(scratch);
				object->as.laTexObject.text = (char*)g_memory_realloc(object->as.laTexObject.text, sizeof(char) * (newLength + 1));
				object->as.laTexObject.textLength = (int32_t)newLength;
				g_memory_copyMem(object->as.laTexObject.text, scratch, newLength * sizeof(char));
				object->as.laTexObject.text[newLength] = '\0';
			}

			ImGui::Checkbox(": Is Equation", &object->as.laTexObject.isEquation);

			// Disable the generate button if it's currently parsing some SVG
			bool disableButton = object->as.laTexObject.isParsingLaTex;
			if (disableButton)
			{
				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
			}
			if (ImGui::Button("Generate LaTeX"))
			{
				object->as.laTexObject.parseLaTex();
			}
			if (disableButton)
			{
				ImGui::PopItemFlag();
				ImGui::PopStyleVar();
			}
		}

		static void handleSvgFileObjectInspector(AnimationManagerData* am, AnimObject* object)
		{
			ImGui::BeginDisabled();
			char* filepathStr = object->as.svgFile.filepath;
			size_t filepathStrLength = object->as.svgFile.filepathLength * sizeof(char);
			if (!filepathStr)
			{
				// This string will never be changed so no need to worry about this... Hopefully :sweat_smile:
				filepathStr = (char*)"Select a File";
				filepathStrLength = sizeof("Select a File");
			}
			ImGui::InputText(
				": Filepath",
				filepathStr,
				filepathStrLength,
				ImGuiInputTextFlags_ReadOnly
			);
			ImGui::EndDisabled();
			ImGui::SameLine();

			bool shouldRegenerate = false;

			if (ImGui::Button(ICON_FA_FILE_UPLOAD))
			{
				nfdchar_t* outPath = NULL;
				nfdresult_t result = NFD_OpenDialog("svg", NULL, &outPath);

				if (result == NFD_OKAY)
				{
					if (!object->as.svgFile.setFilepath(outPath))
					{
						g_logger_info("Opening error popup");
						svgErrorPopupOpen = true;
					}
					else
					{
						shouldRegenerate = true;
					}
					std::free(outPath);
				}
				else if (result == NFD_CANCEL)
				{
					// NOP;
				}
				else
				{
					g_logger_error("Error opening SVGFileObject:\n\t'{}'", NFD_GetError());
				}
			}

			if (shouldRegenerate)
			{
				object->as.svgFile.reInit(am, object);
			}
		}

		static void handleCameraObjectInspector(AnimationManagerData*, AnimObject* object)
		{
			int currentMode = (int)object->as.camera.mode;
			if (ImGui::Combo(": Projection", &currentMode, _cameraModeNames.data(), (int)CameraMode::Length))
			{
				g_logger_assert(currentMode >= 0 && currentMode < (int)CameraMode::Length, "How did this happen?");
				object->as.camera.mode = (CameraMode)currentMode;
			}

			ImGui::DragFloat(": Field Of View", &object->as.camera.fov, 1.0f, 360.0f);
			ImGui::DragFloat3(": Position", &object->as.camera.position.x);
			ImGui::DragFloat3(": Orientation", &object->as.camera.orientation.x);
			ImGui::DragInt2(": Aspect Ratio", &object->as.camera.aspectRatioFraction.x);
			ImGui::DragFloat(": Near", &object->as.camera.nearFarRange.min);
			ImGui::DragFloat(": Far", &object->as.camera.nearFarRange.max);
			ImGui::DragFloat(": Focal Distance", &object->as.camera.focalDistance);
			ImGui::BeginDisabled(object->as.camera.mode != CameraMode::Orthographic);
			ImGui::DragFloat(": Ortho Zoom Level", &object->as.camera.orthoZoomLevel);
			ImGui::EndDisabled();

			ImGui::ColorEdit4(": Background Color", &object->as.camera.fillColor.r);
		}

		static void handleTransformAnimation(AnimationManagerData* am, Animation* animation)
		{
			ImGuiExtended::AnimObjDragDropInputBox(": Source##ReplacementTransformSrcTarget", am, &animation->as.replacementTransform.srcAnimObjectId, animation->id);
			ImGuiExtended::AnimObjDragDropInputBox(": Replace To##ReplacementTransformDstTarget", am, &animation->as.replacementTransform.dstAnimObjectId, animation->id);
		}

		static void handleMoveToAnimationInspector(AnimationManagerData* am, Animation* animation)
		{
			ImGuiExtended::AnimObjDragDropInputBox(": Object##MoveToObjectTarget", am, &animation->as.moveTo.object, animation->id);
			ImGui::DragFloat2(": Target Position", &animation->as.moveTo.target.x, slowDragSpeed);
		}

		static void handleAnimateScaleInspector(AnimationManagerData* am, Animation* animation)
		{
			ImGuiExtended::AnimObjDragDropInputBox(": Object##AnimateScaleObjectTarget", am, &animation->as.animateScale.object, animation->id);
			ImGui::DragFloat2(": Target Scale", &animation->as.animateScale.target.x, slowDragSpeed);
		}

		static void handleShiftInspector(Animation* animation)
		{
			ImGui::DragFloat3(": Shift Amount", &animation->as.modifyVec3.target.x, slowDragSpeed);
		}

		static void handleRotateToAnimationInspector(Animation* animation)
		{
			ImGui::DragFloat3(": Target Rotation", &animation->as.modifyVec3.target.x);
		}


		static void handleAnimateStrokeColorAnimationInspector(Animation* animation)
		{
			float strokeColor[4] = {
				(float)animation->as.modifyU8Vec4.target.r / 255.0f,
				(float)animation->as.modifyU8Vec4.target.g / 255.0f,
				(float)animation->as.modifyU8Vec4.target.b / 255.0f,
				(float)animation->as.modifyU8Vec4.target.a / 255.0f,
			};
			if (ImGui::ColorEdit4(": Stroke Color", strokeColor))
			{
				animation->as.modifyU8Vec4.target.r = (uint8)(strokeColor[0] * 255.0f);
				animation->as.modifyU8Vec4.target.g = (uint8)(strokeColor[1] * 255.0f);
				animation->as.modifyU8Vec4.target.b = (uint8)(strokeColor[2] * 255.0f);
				animation->as.modifyU8Vec4.target.a = (uint8)(strokeColor[3] * 255.0f);
			}
		}

		static void handleAnimateFillColorAnimationInspector(Animation* animation)
		{
			float strokeColor[4] = {
				(float)animation->as.modifyU8Vec4.target.r / 255.0f,
				(float)animation->as.modifyU8Vec4.target.g / 255.0f,
				(float)animation->as.modifyU8Vec4.target.b / 255.0f,
				(float)animation->as.modifyU8Vec4.target.a / 255.0f,
			};
			if (ImGui::ColorEdit4(": Fill Color", strokeColor))
			{
				animation->as.modifyU8Vec4.target.r = (uint8)(strokeColor[0] * 255.0f);
				animation->as.modifyU8Vec4.target.g = (uint8)(strokeColor[1] * 255.0f);
				animation->as.modifyU8Vec4.target.b = (uint8)(strokeColor[2] * 255.0f);
				animation->as.modifyU8Vec4.target.a = (uint8)(strokeColor[3] * 255.0f);
			}
		}

		static void handleCircumscribeInspector(AnimationManagerData* am, Animation* animation)
		{
			ImGuiExtended::AnimObjDragDropInputBox(": Object", am, &animation->as.circumscribe.obj, animation->id);

			int currentShape = (int)animation->as.circumscribe.shape;
			if (ImGui::Combo(": Shape", &currentShape, _circumscribeShapeNames.data(), (int)CircumscribeShape::Length))
			{
				g_logger_assert(currentShape >= 0 && currentShape < (int)CircumscribeShape::Length, "How did this happen?");
				animation->as.circumscribe.shape = (CircumscribeShape)currentShape;
			}

			int currentFade = (int)animation->as.circumscribe.fade;
			if (ImGui::Combo(": Fade Type", &currentFade, _circumscribeFadeNames.data(), (int)CircumscribeFade::Length))
			{
				g_logger_assert(currentFade >= 0 && currentFade < (int)CircumscribeFade::Length, "How did this happen?");
				animation->as.circumscribe.fade = (CircumscribeFade)currentFade;
			}
			ImGui::BeginDisabled(animation->as.circumscribe.fade != CircumscribeFade::FadeNone);
			ImGui::DragFloat(": Time Width", &animation->as.circumscribe.timeWidth, slowDragSpeed, 0.1f, 1.0f, "%2.3f");
			ImGui::EndDisabled();
			ImGui::ColorEdit4(": Color", &animation->as.circumscribe.color.x);
			ImGui::DragFloat(": Buffer Size", &animation->as.circumscribe.bufferSize, slowDragSpeed, 0.0f, 10.0f, "%2.3f");
		}

		static void handleSquareInspector(AnimObject* object)
		{
			if (ImGui::DragFloat(": Side Length", &object->as.square.sideLength, slowDragSpeed))
			{
				object->as.square.reInit(object);
			}
		}

		static void handleCircleInspector(AnimObject* object)
		{
			if (ImGui::DragFloat(": Radius", &object->as.circle.radius, slowDragSpeed))
			{
				object->as.circle.reInit(object);
			}
		}

		static void handleArrowInspector(AnimObject* object)
		{
			bool shouldRegenerate = false;
			if (ImGui::DragFloat(": Stem Length##ArrowShape", &object->as.arrow.stemLength, slowDragSpeed))
			{
				shouldRegenerate = true;
			}

			if (ImGui::DragFloat(": Stem Width##ArrowShape", &object->as.arrow.stemWidth, slowDragSpeed))
			{
				shouldRegenerate = true;
			}

			if (ImGui::DragFloat(": Tip Length##ArrowShape", &object->as.arrow.tipLength, slowDragSpeed))
			{
				shouldRegenerate = true;
			}

			if (ImGui::DragFloat(": Tip Width##ArrowShape", &object->as.arrow.tipWidth, slowDragSpeed))
			{
				shouldRegenerate = true;
			}

			if (shouldRegenerate)
			{
				object->as.arrow.reInit(object);
			}
		}

		static void handleCubeInspector(AnimationManagerData* am, AnimObject* object)
		{
			if (ImGui::DragFloat(": Side Length", &object->as.cube.sideLength, slowDragSpeed))
			{
				object->as.cube.reInit(am, object);
			}
		}

		static void handleAxisInspector(AnimObject* object)
		{
			bool reInitObject = false;

			reInitObject = ImGui::DragFloat3(": Axes Length", object->as.axis.axesLength.values, slowDragSpeed) || reInitObject;

			int xVals[2] = { object->as.axis.xRange.min, object->as.axis.xRange.max };
			if (ImGui::DragInt2(": X-Range", xVals, slowDragSpeed))
			{
				// Make sure it's in strictly increasing order
				if (xVals[0] < xVals[1])
				{
					object->as.axis.xRange.min = xVals[0];
					object->as.axis.xRange.max = xVals[1];
					reInitObject = true;
				}
			}

			int yVals[2] = { object->as.axis.yRange.min, object->as.axis.yRange.max };
			if (ImGui::DragInt2(": Y-Range", yVals, slowDragSpeed))
			{
				// Make sure it's in strictly increasing order
				if (yVals[0] < yVals[1])
				{
					object->as.axis.yRange.min = yVals[0];
					object->as.axis.yRange.max = yVals[1];
					reInitObject = true;
				}
			}

			int zVals[2] = { object->as.axis.zRange.min, object->as.axis.zRange.max };
			if (ImGui::DragInt2(": Z-Range", zVals, slowDragSpeed))
			{
				// Make sure it's in strictly increasing order
				if (zVals[0] < zVals[1])
				{
					object->as.axis.zRange.min = zVals[0];
					object->as.axis.zRange.max = zVals[1];
					reInitObject = true;
				}
			}

			reInitObject = ImGui::DragFloat(": X-Increment", &object->as.axis.xStep, slowDragSpeed) || reInitObject;
			reInitObject = ImGui::DragFloat(": Y-Increment", &object->as.axis.yStep, slowDragSpeed) || reInitObject;
			reInitObject = ImGui::DragFloat(": Z-Increment", &object->as.axis.zStep, slowDragSpeed) || reInitObject;
			reInitObject = ImGui::DragFloat(": Tick Width", &object->as.axis.tickWidth, slowDragSpeed) || reInitObject;
			reInitObject = ImGui::Checkbox(": Draw Labels", &object->as.axis.drawNumbers) || reInitObject;
			if (object->as.axis.drawNumbers)
			{
				reInitObject = ImGui::DragFloat(": Font Size Pixels", &object->as.axis.fontSizePixels, slowDragSpeed, 0.0f, 300.0f) || reInitObject;
				reInitObject = ImGui::DragFloat(": Label Padding", &object->as.axis.labelPadding, slowDragSpeed, 0.0f, 500.0f) || reInitObject;
				reInitObject = ImGui::DragFloat(": Label Stroke Width", &object->as.axis.labelStrokeWidth, slowDragSpeed, 0.0f, 200.0f) || reInitObject;
			}

			//if (ImGui::Checkbox(": Is 3D", &object->as.axis.is3D))
			//{
			//	// Reset to default values if we toggle 3D on or off
			//	if (object->as.axis.is3D)
			//	{
			//		object->_positionStart = Vec3{ 0.0f, 0.0f, 0.0f };
			//		object->as.axis.axesLength = Vec3{ 8.0f, 5.0f, 8.0f };
			//		object->as.axis.xRange = { 0, 8 };
			//		object->as.axis.yRange = { 0, 5 };
			//		object->as.axis.zRange = { 0, 8 };
			//		object->as.axis.tickWidth = 0.2f;
			//		object->_strokeWidthStart = 0.05f;
			//	}
			//	else
			//	{
			//		glm::vec2 outputSize = Application::getOutputSize();
			//		object->_positionStart = Vec3{ outputSize.x / 2.0f, outputSize.y / 2.0f, 0.0f };
			//		object->as.axis.axesLength = Vec3{ 3'000.0f, 1'700.0f, 1.0f };
			//		object->as.axis.xRange = { 0, 18 };
			//		object->as.axis.yRange = { 0, 10 };
			//		object->as.axis.zRange = { 0, 10 };
			//		object->as.axis.tickWidth = 75.0f;
			//		object->_strokeWidthStart = 7.5f;
			//	}
			//	reInitObject = true;
			//}

			if (reInitObject)
			{
				//for (int i = 0; i < object->children.size(); i++)
				//{
				//	object->children[i].free();
				//}
				//object->children.clear();
				g_logger_warning("TODO: Fix me");

				object->as.axis.init(object);
			}
		}

		static void handleScriptObjectInspector(AnimationManagerData* am, AnimObject* obj)
		{
			ScriptObject& script = obj->as.script;

			constexpr size_t bufferSize = 512;
			char buffer[bufferSize] = "Drag File Here";
			if (bufferSize >= script.scriptFilepathLength + 1 && script.scriptFilepathLength > 0)
			{
				g_memory_copyMem((void*)buffer, script.scriptFilepath, sizeof(char) * script.scriptFilepathLength);
				buffer[script.scriptFilepathLength] = '\0';
			}

			if (ImGuiExtended::FileDragDropInputBox(": Script File##ScriptFileTarget", buffer, bufferSize))
			{
				size_t newFilepathLength = std::strlen(buffer);
				script.scriptFilepath = (char*)g_memory_realloc(script.scriptFilepath, sizeof(char) * (newFilepathLength + 1));
				script.scriptFilepathLength = newFilepathLength;
				g_memory_copyMem((void*)script.scriptFilepath, buffer, sizeof(char) * (newFilepathLength + 1));
			}

			if (ImGui::Button("Generate"))
			{
				if (script.scriptFilepathLength > 0 && Platform::fileExists(script.scriptFilepath))
				{
					// First remove all generated children, which were generated as a result
					// of this object (presumably)
					for (int i = 0; i < obj->generatedChildrenIds.size(); i++)
					{
						AnimObject* child = AnimationManager::getMutableObject(am, obj->generatedChildrenIds[i]);
						if (child)
						{
							SceneHierarchyPanel::deleteAnimObject(*child);
							AnimationManager::removeAnimObject(am, obj->generatedChildrenIds[i]);
						}
					}
					obj->generatedChildrenIds.clear();

					// Next init again which should regenerate the children
					if (LuauLayer::compile(script.scriptFilepath))
					{
						if (!LuauLayer::executeOnAnimObj(script.scriptFilepath, "generate", am, obj->id))
						{
							// If execution fails, delete any objects that may have been created prematurely
							for (int i = 0; i < obj->generatedChildrenIds.size(); i++)
							{
								AnimObject* child = AnimationManager::getMutableObject(am, obj->generatedChildrenIds[i]);
								if (child)
								{
									SceneHierarchyPanel::deleteAnimObject(*child);
									AnimationManager::removeAnimObject(am, obj->generatedChildrenIds[i]);
								}
							}
							obj->generatedChildrenIds.clear();
						}
					}

					// Copy the svgObjectStart to all the svgObjects to any generated children
					// to make sure that they render properly
					for (auto childId : obj->generatedChildrenIds)
					{
						AnimObject* child = AnimationManager::getMutableObject(am, childId);
						if (child)
						{
							if (child->_svgObjectStart)
							{
								Svg::copy(child->svgObject, child->_svgObjectStart);
							}
						}
					}
				}
			}
		}

		static void handleImageObjectInspector(AnimationManagerData* am, AnimObject* object)
		{
			ImGui::BeginDisabled();
			char* filepathStr = object->as.image.imageFilepath;
			size_t filepathStrLength = object->as.image.imageFilepathLength * sizeof(char);
			if (!filepathStr)
			{
				// This string will never be changed so no need to worry about this... Hopefully :sweat_smile:
				filepathStr = (char*)"Select a File";
				filepathStrLength = sizeof("Select a File");
			}
			ImGui::InputText(
				": Filepath",
				filepathStr,
				filepathStrLength,
				ImGuiInputTextFlags_ReadOnly
			);
			ImGui::EndDisabled();
			ImGui::SameLine();

			bool shouldRegenerate = false;

			if (ImGui::Button(ICON_FA_FILE_UPLOAD))
			{
				nfdchar_t* outPath = NULL;
				nfdresult_t result = NFD_OpenDialog("png,jpeg,jpg,tga,bmp,psd,gif,hdr,pic,pnm", NULL, &outPath);

				if (result == NFD_OKAY)
				{
					size_t strLength = std::strlen(outPath);
					object->as.image.setFilepath(outPath, strLength);
					shouldRegenerate = true;
					std::free(outPath);
				}
				else if (result == NFD_CANCEL)
				{
					// NOP;
				}
				else
				{
					g_logger_error("Error opening Image:\n\t'{}'", NFD_GetError());
				}
			}

			int currentFilterMode = (int)object->as.image.filterMode;
			if (ImGui::Combo(": Sample Mode", &currentFilterMode, _imageFilterModeNames.data(), (int)ImageFilterMode::Length))
			{
				g_logger_assert(currentFilterMode >= 0 && currentFilterMode < (int)ImageFilterMode::Length, "How did this happen?");
				object->as.image.filterMode = (ImageFilterMode)currentFilterMode;
				shouldRegenerate = true;
			}

			int currentRepeatMode = (int)object->as.image.repeatMode;
			if (ImGui::Combo(": Repeat", &currentRepeatMode, _imageRepeatModeNames.data(), (int)ImageRepeatMode::Length))
			{
				g_logger_assert(currentRepeatMode >= 0 && currentRepeatMode < (int)ImageRepeatMode::Length, "How did this happen?");
				object->as.image.repeatMode = (ImageRepeatMode)currentFilterMode;
				shouldRegenerate = true;
			}

			if (shouldRegenerate)
			{
				object->as.image.reInit(am, object, true);
			}
		}
	}
}
