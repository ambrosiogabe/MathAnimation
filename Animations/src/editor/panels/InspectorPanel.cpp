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
#include "parsers/SyntaxTheme.h"

#include <imgui.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

#include <nfd.h>

#define NO_EDIT res.editState == EditState::NotEditing
#define BEING_EDITED res.editState == EditState::BeingEdited
#define FINISHED_EDITING res.editState == EditState::FinishedEditing
#define ANY_EDIT res.editState != EditState::NotEditing

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
		static const char* ANIM_OBJECT_DROP_TARGET_ID = "ANIM_OBJECT_DROP_TARGET_ID";

		// ---------------------- Internal Functions ----------------------
		static bool applySettingToChildren(const char* id);

		// ---------------------- Inspector functions ----------------------
		static bool handleAnimObjectInspector(AnimationManagerData* am, AnimObjId animObjectId);
		static bool handleAnimationInspector(AnimationManagerData* am, AnimId animationId);
		static bool handleTextObjectInspector(AnimationManagerData* am, AnimObject* object);
		static bool handleCodeBlockInspector(AnimationManagerData* am, AnimObject* object);
		static bool handleLaTexObjectInspector(AnimObject* object);
		static bool handleSvgFileObjectInspector(AnimObject* object);
		static bool handleCameraObjectInspector(AnimationManagerData* am, AnimObject* object);
		static bool handleMoveToAnimationInspector(AnimationManagerData* am, Animation* animation);
		static bool handleAnimateScaleInspector(AnimationManagerData* am, Animation* animation);
		static bool handleTransformAnimation(AnimationManagerData* am, Animation* animation);
		static bool handleShiftInspector(Animation* animation);
		static bool handleRotateToAnimationInspector(Animation* animation);
		static bool handleAnimateStrokeColorAnimationInspector(Animation* animation);
		static bool handleAnimateFillColorAnimationInspector(Animation* animation);
		static bool handleCircumscribeInspector(AnimationManagerData* am, Animation* animation);
		static bool handleSquareInspector(AnimObject* object);
		static bool handleCircleInspector(AnimObject* object);
		static bool handleArrowInspector(AnimObject* object);
		static bool handleCubeInspector(AnimationManagerData* am, AnimObject* object);
		static bool handleAxisInspector(AnimationManagerData* am, AnimObject* object);
		static bool handleScriptObjectInspector(AnimationManagerData* am, AnimObject* object);
		static bool handleImageObjectInspector(AnimationManagerData* am, AnimObject* object);

		void update(AnimationManagerData* am)
		{
			ImGui::Begin(ICON_FA_LIST " Animation Object Inspector");
			{
				ImGui::Indent(indentationDepth);
				if (!isNull(activeAnimObjectId))
				{
					bool anyAnimObjectPropertyChanged = handleAnimObjectInspector(am, activeAnimObjectId);
					// NOTE: Don't update object state while the animation is playing because
					//       it messes up the playback
					// TODO: Investigate the root cause of this issue and fix that instead (why don't ya?)
					if (anyAnimObjectPropertyChanged && Application::getEditorPlayState() == AnimState::Pause)
					{
						// TODO: This slows stuff down a lot. Optimize the heck out of it
						AnimationManager::updateObjectState(am, activeAnimObjectId);
					}
				}
				ImGui::Unindent(indentationDepth);
			}
			ImGui::End();

			ImGui::Begin(ICON_FA_LIST " Animation Inspector");
			{
				ImGui::Indent(indentationDepth);
				if (!isNull(activeAnimationId))
				{
					handleAnimationInspector(am, activeAnimationId);
				}
				ImGui::Unindent(indentationDepth);
			}

			ImGui::End();
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
		static bool handleAnimObjectInspector(AnimationManagerData* am, AnimObjId animObjectId)
		{
			AnimObject* animObject = AnimationManager::getMutableObject(am, animObjectId);
			if (!animObject)
			{
				g_logger_error("No anim object with id '{}' exists", animObjectId);
				activeAnimObjectId = NULL_ANIM_OBJECT;
				return false;
			}

			bool anyPropertyChanged = false;

			std::string transformComponentName = "Transform##" + std::to_string(animObjectId);
			if (ImGui::CollapsingHeader(transformComponentName.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
			{
				constexpr int scratchLength = 256;
				char scratch[scratchLength] = {};
				if (animObject->nameLength < scratchLength - 1)
				{
					g_memory_copyMem(scratch, scratchLength, animObject->name, animObject->nameLength * sizeof(char));
					scratch[animObject->nameLength] = '\0';
					if (auto res = ImGuiExtended::InputTextEx(": Name", scratch, scratchLength * sizeof(char));
						ANY_EDIT)
					{
						anyPropertyChanged = true;

						if (FINISHED_EDITING)
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
					ANY_EDIT)
				{
					anyPropertyChanged = true;

					if (FINISHED_EDITING)
					{
						UndoSystem::setVec3Prop(
							Application::getUndoSystem(),
							animObject->id,
							res.ogData,
							animObject->_positionStart,
							Vec3PropType::Position
						);
					}
				}

				// Rotation
				if (auto res = ImGuiExtended::DragFloat3Ex(": Rotation", &animObject->_rotationStart);
					ANY_EDIT)
				{
					anyPropertyChanged = true;

					if (FINISHED_EDITING)
					{
						UndoSystem::setVec3Prop(
							Application::getUndoSystem(),
							animObject->id,
							res.ogData,
							animObject->_rotationStart,
							Vec3PropType::Rotation
						);
					}
				}

				// Scale
				if (auto res = ImGuiExtended::DragFloat3Ex(": Scale", &animObject->_scaleStart, slowDragSpeed);
					ANY_EDIT)
				{
					anyPropertyChanged = true;

					if (FINISHED_EDITING)
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
			}

			std::string svgPropsComponentName = "Svg Properties##" + std::to_string(animObjectId);
			if (ImGui::CollapsingHeader(svgPropsComponentName.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
			{
				// TODO: Remove this once you get auto SVG stuff working good
				if (ImGui::DragFloat(": SVG Scale", &animObject->svgScale, slowDragSpeed))
				{
					anyPropertyChanged = true;
				}

				// NanoVG only allows stroke width between [0-200] so we reflect that here
				if (auto res = ImGuiExtended::DragFloatEx(": Stroke Width", (float*)&animObject->_strokeWidthStart, 1.0f, 0.0f, 200.0f);
					ANY_EDIT)
				{
					anyPropertyChanged = true;

					if (FINISHED_EDITING)
					{
						UndoSystem::setFloatProp(
							Application::getUndoSystem(),
							animObject->id,
							res.ogData,
							animObject->_strokeWidthStart,
							FloatPropType::StrokeWidth
						);
					}
				}
				if (applySettingToChildren("##StrokeWidthChildrenApply"))
				{
					anyPropertyChanged = true;
				}

				// Stroke Color
				if (auto res = ImGuiExtended::ColorEdit4Ex(": Stroke Color", &animObject->_strokeColorStart);
					ANY_EDIT)
				{
					anyPropertyChanged = true;

					if (FINISHED_EDITING)
					{
						UndoSystem::setU8Vec4Prop(
							Application::getUndoSystem(),
							animObject->id,
							res.ogData,
							animObject->_strokeColorStart,
							U8Vec4PropType::StrokeColor
						);
					}
				}

				if (applySettingToChildren("##StrokeColorChildrenApply"))
				{
					anyPropertyChanged = true;

					UndoSystem::applyU8Vec4ToChildren(
						Application::getUndoSystem(),
						animObject->id,
						U8Vec4PropType::StrokeColor
					);
				}

				// Fill Color
				if (auto res = ImGuiExtended::ColorEdit4Ex(": Fill Color", &animObject->_fillColorStart);
					ANY_EDIT)
				{
					anyPropertyChanged = true;

					if (FINISHED_EDITING)
					{
						UndoSystem::setU8Vec4Prop(
							Application::getUndoSystem(),
							animObject->id,
							res.ogData,
							animObject->_fillColorStart,
							U8Vec4PropType::FillColor
						);
					}
				}

				if (applySettingToChildren("##FillColorChildrenApply"))
				{
					anyPropertyChanged = true;

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
					anyPropertyChanged = anyPropertyChanged || handleTextObjectInspector(am, animObject);
					break;
				case AnimObjectTypeV1::CodeBlock:
					anyPropertyChanged = anyPropertyChanged || handleCodeBlockInspector(am, animObject);
					break;
				case AnimObjectTypeV1::LaTexObject:
					anyPropertyChanged = anyPropertyChanged || handleLaTexObjectInspector(animObject);
					break;
				case AnimObjectTypeV1::Square:
					anyPropertyChanged = anyPropertyChanged || handleSquareInspector(animObject);
					break;
				case AnimObjectTypeV1::Circle:
					anyPropertyChanged = anyPropertyChanged || handleCircleInspector(animObject);
					break;
				case AnimObjectTypeV1::Cube:
					anyPropertyChanged = anyPropertyChanged || handleCubeInspector(am, animObject);
					break;
				case AnimObjectTypeV1::Axis:
					anyPropertyChanged = anyPropertyChanged || handleAxisInspector(am, animObject);
					break;
				case AnimObjectTypeV1::SvgFileObject:
					anyPropertyChanged = anyPropertyChanged || handleSvgFileObjectInspector(animObject);
					break;
				case AnimObjectTypeV1::Camera:
					anyPropertyChanged = anyPropertyChanged || handleCameraObjectInspector(am, animObject);
					break;
				case AnimObjectTypeV1::SvgObject:
					// NOP
					break;
				case AnimObjectTypeV1::ScriptObject:
					anyPropertyChanged = anyPropertyChanged || handleScriptObjectInspector(am, animObject);
					break;
				case AnimObjectTypeV1::Image:
					anyPropertyChanged = anyPropertyChanged || handleImageObjectInspector(am, animObject);
					break;
				case AnimObjectTypeV1::Arrow:
					anyPropertyChanged = anyPropertyChanged || handleArrowInspector(animObject);
					break;
				case AnimObjectTypeV1::Length:
				case AnimObjectTypeV1::None:
					break;
				}
			}

			return anyPropertyChanged;
		}

		static bool handleAnimationInspector(AnimationManagerData* am, AnimId animationId)
		{
			Animation* animation = AnimationManager::getMutableAnimation(am, animationId);
			if (!animation)
			{
				g_logger_error("No animation with id '{}' exists", animationId);
				activeAnimationId = NULL_ANIM;
				return false;
			}

			bool anyPropertyChanged = false;

			std::string animPropsComponentName = "Animation Properties##" + std::to_string(animationId);
			if (ImGui::CollapsingHeader(animPropsComponentName.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
			{
				int currentType = (int)(animation->easeType) - 1;
				if (auto res = ImGuiExtended::ComboEx(": Ease Type", &currentType, &easeTypeNames[1], (int)EaseType::Length - 1);
					ANY_EDIT)
				{
					anyPropertyChanged = true;

					if (FINISHED_EDITING)
					{
						UndoSystem::setEnumProp(
							Application::getUndoSystem(),
							animation->id,
							res.ogData + 1,
							currentType + 1,
							EnumPropType::EaseType
						);
					}
				}

				int currentDirection = (int)(animation->easeDirection) - 1;
				if (auto res = ImGuiExtended::ComboEx(": Ease Direction", &currentDirection, &easeDirectionNames[1], (int)EaseDirection::Length - 1);
					ANY_EDIT)
				{
					anyPropertyChanged = true;

					if (FINISHED_EDITING)
					{
						UndoSystem::setEnumProp(
							Application::getUndoSystem(),
							animation->id,
							res.ogData + 1,
							currentDirection + 1,
							EnumPropType::EaseDirection
						);
					}
				}

				int currentPlaybackType = (int)(animation->playbackType) - 1;
				if (auto res = ImGuiExtended::ComboEx(": Playback Type", &currentPlaybackType, &_playbackTypeNames[1], (int)PlaybackType::Length - 1);
					ANY_EDIT)
				{
					anyPropertyChanged = true;

					if (FINISHED_EDITING)
					{
						UndoSystem::setEnumProp(
							Application::getUndoSystem(),
							animation->id,
							res.ogData + 1,
							currentPlaybackType + 1,
							EnumPropType::PlaybackType
						);
					}
				}

				ImGui::BeginDisabled(animation->playbackType == PlaybackType::Synchronous);
				if (auto res = ImGuiExtended::DragFloatEx(": Lag Ratio", &animation->lagRatio, slowDragSpeed, 0.0f, 1.0f);
					ANY_EDIT)
				{
					anyPropertyChanged = true;
					if (FINISHED_EDITING)
					{
						UndoSystem::setFloatProp(
							Application::getUndoSystem(),
							animation->id,
							res.ogData,
							animation->lagRatio,
							FloatPropType::LagRatio
						);
					}
				}
				ImGui::EndDisabled();


				if (Animation::isAnimationGroup(animation->type))
				{
					ImGui::PushStyleColor(ImGuiCol_FrameBg, Colors::Neutral[7]);

					// TODO: UI needs some work
					ImGui::Text("Animation Objects:");
					if (ImGui::BeginListBox("##: Anim Objects", ImVec2(0.0f, 5 * ImGui::GetTextLineHeightWithSpacing())))
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
									anyPropertyChanged = true;

									// This will invalidate our iterator, so instead of continuing to loop
									// we break out. This will cause the GUI to break for a single frame,
									// but it's worth it in order to avoid the undefined behavior otherwise.
									UndoSystem::removeObjectFromAnim(
										Application::getUndoSystem(),
										*animObjectIdIter,
										animation->id
									);
									ImGui::PopID();
									break;
								}
								buttonSize = ImGui::GetItemRectSize();

								ImGui::PopID();

								ImGui::SetCursorScreenPos(
									ImVec2(ImGui::GetCursorScreenPos().x, pMax.y + framePadding.y * 2.0f)
								);
							}
						}

						ImGui::EndListBox();
					}

					// Handle drag drop for anim object
					if (auto objPayload = ImGuiExtended::AnimObjectDragDropTarget(); objPayload != nullptr)
					{
						anyPropertyChanged = true;

						UndoSystem::addObjectToAnim(
							Application::getUndoSystem(),
							objPayload->animObjectId,
							animation->id
						);
					}

					ImGui::PopStyleColor();
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
					anyPropertyChanged = anyPropertyChanged || handleTransformAnimation(am, animation);
					break;
				case AnimTypeV1::MoveTo:
					anyPropertyChanged = anyPropertyChanged || handleMoveToAnimationInspector(am, animation);
					break;
				case AnimTypeV1::AnimateScale:
					anyPropertyChanged = anyPropertyChanged || handleAnimateScaleInspector(am, animation);
					break;
				case AnimTypeV1::Shift:
					anyPropertyChanged = anyPropertyChanged || handleShiftInspector(animation);
					break;
				case AnimTypeV1::RotateTo:
					anyPropertyChanged = anyPropertyChanged || handleRotateToAnimationInspector(animation);
					break;
				case AnimTypeV1::AnimateFillColor:
					anyPropertyChanged = anyPropertyChanged || handleAnimateFillColorAnimationInspector(animation);
					break;
				case AnimTypeV1::AnimateStrokeColor:
					anyPropertyChanged = anyPropertyChanged || handleAnimateStrokeColorAnimationInspector(animation);
					break;
				case AnimTypeV1::AnimateStrokeWidth:
					//handleAnimateStrokeWidthInspector(animation);
					g_logger_warning("TODO: Implement me.");
					break;
				case AnimTypeV1::Circumscribe:
					anyPropertyChanged = anyPropertyChanged || handleCircumscribeInspector(am, animation);
					break;
				case AnimTypeV1::Length:
				case AnimTypeV1::None:
					break;
				}
			}

			return anyPropertyChanged;
		}

		static bool handleTextObjectInspector(AnimationManagerData* am, AnimObject* object)
		{
			bool anyPropertyChanged = false;

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
						std::string oldFont = "";
						if (object->as.textObject.font)
						{
							oldFont = object->as.textObject.font->fontFilepath;
						}

						anyPropertyChanged = true;

						UndoSystem::setFont(
							Application::getUndoSystem(),
							object->id,
							oldFont,
							fonts[n]
						);
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
				return false;
			}
			g_memory_copyMem(scratch, scratchLength, object->as.textObject.text, object->as.textObject.textLength * sizeof(char));
			scratch[object->as.textObject.textLength] = '\0';
			if (auto res = ImGuiExtended::InputTextMultilineEx(": Text##TextObject", scratch, scratchLength * sizeof(char));
				ANY_EDIT)
			{
				anyPropertyChanged = true;

				if (FINISHED_EDITING)
				{
					UndoSystem::setStringProp(
						Application::getUndoSystem(),
						object->id,
						res.ogData,
						scratch,
						StringPropType::TextObjectText
					);
				}
				else
				{
					if (strcmp(scratch, object->as.textObject.text) != 0)
					{
						object->as.textObject.setText(scratch);
						object->as.textObject.reInit(am, object);
					}
				}
			}

			return anyPropertyChanged;
		}

		struct CodeEditUserData
		{
			std::vector<ScopedName> ancestors;
			DebugPackedSyntaxStyle settings;
			HighlighterLanguage language;
			HighlighterTheme theme;
			std::string matchText;
			bool isActive;
			bool usingDefaultSettings;
		};

		static int codeEditCallback(ImGuiInputTextCallbackData* data)
		{
			CodeEditUserData& userData = *(CodeEditUserData*)data->UserData;

			if (!userData.isActive)
			{
				return 0;
			}

			const SyntaxHighlighter* highlighter = Highlighters::getHighlighter(userData.language);
			if (!highlighter)
			{
				return 0;
			}

			const SyntaxTheme* theme = Highlighters::getTheme(userData.theme);
			if (!theme)
			{
				return 0;
			}

			auto parseInfo = highlighter->getAncestorsFor(theme, data->Buf, data->CursorPos);
			userData.ancestors = parseInfo.ancestors;
			userData.settings = parseInfo.settings;
			userData.matchText = parseInfo.matchText;
			userData.usingDefaultSettings = parseInfo.usingDefaultSettings;

			return 0;
		}

		static bool handleCodeBlockInspector(AnimationManagerData* am, AnimObject* object)
		{
			bool anyPropertyChanged = false;

			constexpr int scratchLength = 512;
			char scratch[scratchLength] = {};
			if (object->as.codeBlock.textLength >= scratchLength)
			{
				g_logger_error("Text object has more than 512 characters. Tell Gabe to increase scratch length for code blocks.");
				return false;
			}

			int currentLang = (int)object->as.codeBlock.language - 1;
			if (auto res = ImGuiExtended::ComboEx(": Language", &currentLang, _highlighterLanguageNames.data() + 1, (int)HighlighterLanguage::Length - 1);
				ANY_EDIT)
			{
				anyPropertyChanged = true;

				if (FINISHED_EDITING)
				{
					UndoSystem::setEnumProp(
						Application::getUndoSystem(),
						object->id,
						res.ogData + 1,
						currentLang + 1,
						EnumPropType::HighlighterLanguage
					);
				}
			}

			int currentTheme = (int)object->as.codeBlock.theme - 1;
			if (auto res = ImGuiExtended::ComboEx(": Theme", &currentTheme, _highlighterThemeNames.data() + 1, (int)HighlighterTheme::Length - 1);
				ANY_EDIT)
			{
				anyPropertyChanged = true;

				if (FINISHED_EDITING)
				{
					UndoSystem::setEnumProp(
						Application::getUndoSystem(),
						object->id,
						res.ogData + 1,
						currentTheme + 1,
						EnumPropType::HighlighterTheme
					);
				}
			}

			// Debug struct/info
			static CodeEditUserData codeEditUserData = {};
			if (codeEditUserData.isActive)
			{
				codeEditUserData.language = object->as.codeBlock.language;
				codeEditUserData.theme = object->as.codeBlock.theme;
				codeEditUserData.ancestors = {};
				codeEditUserData.settings = {};
			}

			g_memory_copyMem(scratch, scratchLength, object->as.codeBlock.text, object->as.codeBlock.textLength * sizeof(char));
			scratch[object->as.codeBlock.textLength] = '\0';
			if (auto res = ImGuiExtended::InputTextMultilineEx(": Code##CodeBlockData", scratch, scratchLength * sizeof(char), ImVec2(0, 0), ImGuiInputTextFlags_CallbackAlways, codeEditCallback, &codeEditUserData);
				ANY_EDIT)
			{
				anyPropertyChanged = true;

				if (FINISHED_EDITING)
				{
					UndoSystem::setStringProp(
						Application::getUndoSystem(),
						object->id,
						res.ogData,
						scratch,
						StringPropType::CodeBlockText
					);
				}
				else
				{
					if (strcmp(scratch, object->as.codeBlock.text) != 0)
					{
						object->as.codeBlock.setText(scratch);
						object->as.codeBlock.reInit(am, object);
					}
				}
			}

			// Debug stuff, checking this box allows you to inspect the styles and
			// ancestors that are generated for the codeblock where the text cursor
			// is located.
			ImGui::Checkbox(": Inspect", &codeEditUserData.isActive);
			if (codeEditUserData.isActive)
			{
				ImGui::BeginChild("Code Ancestors", ImVec2(0, 0), true);

				ImGui::PushFont(ImGuiLayer::getMediumFont());
				if (codeEditUserData.matchText.length() > 30)
				{
					std::string firstPart = codeEditUserData.matchText.substr(0, 15);
					std::string lastPart = codeEditUserData.matchText.substr(codeEditUserData.matchText.length() - 15);
					ImGui::Text("%s...%s", firstPart.c_str(), lastPart.c_str());
				}
				else
				{
					ImGui::Text(codeEditUserData.matchText.c_str());
				}
				ImGui::PopFont();

				ImGui::Separator();

				if (ImGui::BeginTable("textmate scopes", 2))
				{
					const SyntaxTheme* theme = Highlighters::getTheme(object->as.codeBlock.theme);

					ImGui::TableNextColumn(); ImGui::Text("language");
					ImGui::TableNextColumn(); ImGui::Text("%d", codeEditUserData.settings.style.getLanguageId());

					ImGui::TableNextColumn(); ImGui::Text("standard token type");
					ImGui::TableNextColumn(); ImGui::Text("%d", codeEditUserData.settings.style.getStandardTokenType());

					ImGui::TableNextColumn(); ImGui::Text("foreground");
					Vec4 foregroundColor = theme->getColor(codeEditUserData.settings.style.getForegroundColor());
					std::string foregroundStr = toHexString(foregroundColor);
					ImGui::TableNextColumn(); ImGui::Text("%s", foregroundStr.c_str()); ImGui::SameLine();
					ImGui::ColorButton("##ForegroundInspector_InspectorPanel", foregroundColor, 
						ImGuiColorEditFlags_NoLabel
					);

					ImGui::TableNextColumn(); ImGui::Text("background");
					Vec4 backgroundColor = theme->getColor(codeEditUserData.settings.style.getBackgroundColor());
					std::string backgroundStr = toHexString(backgroundColor);
					ImGui::TableNextColumn(); ImGui::Text("%s", backgroundStr.c_str()); ImGui::SameLine();
					ImGui::ColorButton("##ForegroundInspector_InspectorPanel", backgroundColor,
						ImGuiColorEditFlags_NoLabel
					);

					ImGui::TableNextColumn(); ImGui::Text("font style");
					ImGui::TableNextColumn(); ImGui::Text("%s", Css::toString(codeEditUserData.settings.style.getFontStyle()).c_str());

					ImGui::Separator();

					ImGui::TableNextColumn(); ImGui::Text("textmate scopes");
					for (size_t i = 0; i < codeEditUserData.ancestors.size(); i++)
					{
						size_t backwardsIndex = codeEditUserData.ancestors.size() - i - 1;
						std::string friendlyName = codeEditUserData.ancestors[backwardsIndex].getFriendlyName();
						ImGui::TableNextColumn(); ImGui::Text(friendlyName.c_str());

						if (i < codeEditUserData.ancestors.size() - 1)
						{
							ImGui::TableNextRow(); ImGui::TableNextColumn();
						}
					}

					ImGui::TableNextRow(); ImGui::TableNextRow();

					ImGui::TableNextColumn(); ImGui::Text("foreground");
					bool foundThemeSelector = false;
					if (codeEditUserData.usingDefaultSettings)
					{
						ImGui::TableNextColumn(); ImGui::Text("No theme selector");
					}
					else 
					{
						Vec4 const& foregroundCssColor = theme->getColor(codeEditUserData.settings.style.getForegroundColor());
						std::string foregroundColorStr = toHexString(foregroundCssColor);
						if (codeEditUserData.settings.style.isForegroundInherited())
						{
							foregroundColorStr = "inherit";
						}

						ImGui::TableNextColumn(); ImGui::Text(codeEditUserData.settings.styleMatched.c_str());
						ImGui::TableNextRow(); ImGui::TableNextColumn();
						ImGui::TableNextColumn(); ImGui::Text("{ \"foreground\": \"%s\"", foregroundColorStr.c_str());
						foundThemeSelector = true;
					}

					ImGui::EndTable();
				}

				ImGui::EndChild();
			}

			return anyPropertyChanged;
		}

		// TODO: LaTeX objects are pretty broken, fix them!
		static bool handleLaTexObjectInspector(AnimObject* object)
		{
			constexpr int scratchLength = 2048;
			char scratch[scratchLength] = {};
			if (object->as.laTexObject.textLength >= scratchLength)
			{
				g_logger_error("Text object has more than '{}' characters. Tell Gabe to increase scratch length for LaTex objects.", scratchLength);
				return false;
			}

			bool anyPropertyChanged = false;

			g_memory_copyMem(scratch, scratchLength, object->as.laTexObject.text, object->as.laTexObject.textLength * sizeof(char));
			scratch[object->as.laTexObject.textLength] = '\0';
			if (auto res = ImGuiExtended::InputTextMultilineEx(": LaTeX##LaTeXEditor", scratch, scratchLength * sizeof(char));
				ANY_EDIT)
			{
				anyPropertyChanged = true;

				if (FINISHED_EDITING)
				{
					UndoSystem::setStringProp(
						Application::getUndoSystem(),
						object->id,
						res.ogData,
						scratch,
						StringPropType::LaTexText
					);
				}
				else
				{
					if (strcmp(scratch, object->as.codeBlock.text) != 0)
					{
						object->as.laTexObject.setText(scratch);
					}
				}
			}

			anyPropertyChanged = anyPropertyChanged
				|| ImGui::Checkbox(": Is Equation", &object->as.laTexObject.isEquation);

			return anyPropertyChanged;
		}

		static bool handleSvgFileObjectInspector(AnimObject* object)
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

			bool anyPropertyChanged = false;

			if (ImGui::Button(ICON_FA_FILE_UPLOAD))
			{
				nfdchar_t* outPath = NULL;
				nfdresult_t result = NFD_OpenDialog("svg", NULL, &outPath);

				if (result == NFD_OKAY)
				{
					anyPropertyChanged = true;

					UndoSystem::setStringProp(
						Application::getUndoSystem(),
						object->id,
						filepathStr ? std::string(filepathStr) : "",
						std::string(outPath),
						StringPropType::SvgFilepath
					);
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

			return anyPropertyChanged;
		}

		static bool handleCameraObjectInspector(AnimationManagerData*, AnimObject* object)
		{
			bool anyPropertyChanged = false;

			int currentMode = (int)object->as.camera.mode;
			if (auto res = ImGuiExtended::ComboEx(": Projection", &currentMode, _cameraModeNames.data(), (int)CameraMode::Length);
				ANY_EDIT)
			{
				anyPropertyChanged = true;

				if (FINISHED_EDITING)
				{
					UndoSystem::setEnumProp(
						Application::getUndoSystem(),
						object->id,
						res.ogData,
						currentMode,
						EnumPropType::CameraMode
					);
				}
			}

			if (auto res = ImGuiExtended::DragFloatEx(": Field Of View", &object->as.camera.fov, 1.0f, 360.0f);
				ANY_EDIT)
			{
				anyPropertyChanged = true;

				if (FINISHED_EDITING)
				{
					UndoSystem::setFloatProp(
						Application::getUndoSystem(),
						object->id,
						res.ogData,
						object->as.camera.fov,
						FloatPropType::CameraFieldOfView
					);
				}
			}

			if (auto res = ImGuiExtended::DragInt2Ex(": Aspect Ratio", &object->as.camera.aspectRatioFraction);
				ANY_EDIT)
			{
				anyPropertyChanged = true;

				if (FINISHED_EDITING)
				{
					UndoSystem::setVec2iProp(
						Application::getUndoSystem(),
						object->id,
						res.ogData,
						object->as.camera.aspectRatioFraction,
						Vec2iPropType::AspectRatio
					);
				}
			}

			if (auto res = ImGuiExtended::DragFloatEx(": Near", &object->as.camera.nearFarRange.min);
				ANY_EDIT)
			{
				anyPropertyChanged = true;

				if (FINISHED_EDITING)
				{
					UndoSystem::setFloatProp(
						Application::getUndoSystem(),
						object->id,
						res.ogData,
						object->as.camera.nearFarRange.min,
						FloatPropType::CameraNearPlane
					);
				}
			}

			if (auto res = ImGuiExtended::DragFloatEx(": Far", &object->as.camera.nearFarRange.max);
				ANY_EDIT)
			{
				anyPropertyChanged = true;

				if (FINISHED_EDITING)
				{
					UndoSystem::setFloatProp(
						Application::getUndoSystem(),
						object->id,
						res.ogData,
						object->as.camera.nearFarRange.max,
						FloatPropType::CameraFarPlane
					);
				}
			}

			if (auto res = ImGuiExtended::DragFloatEx(": Focal Distance", &object->as.camera.focalDistance);
				ANY_EDIT)
			{
				anyPropertyChanged = true;

				if (FINISHED_EDITING)
				{
					UndoSystem::setFloatProp(
						Application::getUndoSystem(),
						object->id,
						res.ogData,
						object->as.camera.focalDistance,
						FloatPropType::CameraFocalDistance
					);
				}
			}

			ImGui::BeginDisabled(object->as.camera.mode != CameraMode::Orthographic);
			if (auto res = ImGuiExtended::DragFloatEx(": Ortho Zoom Level", &object->as.camera.orthoZoomLevel);
				ANY_EDIT)
			{
				anyPropertyChanged = true;

				if (FINISHED_EDITING)
				{
					UndoSystem::setFloatProp(
						Application::getUndoSystem(),
						object->id,
						res.ogData,
						object->as.camera.orthoZoomLevel,
						FloatPropType::CameraOrthoZoomLevel
					);
				}
			}
			ImGui::EndDisabled();

			if (auto res = ImGuiExtended::ColorEdit4Ex(": Background Color", &object->as.camera.fillColor);
				ANY_EDIT)
			{
				anyPropertyChanged = true;

				if (FINISHED_EDITING)
				{
					UndoSystem::setVec4Prop(
						Application::getUndoSystem(),
						object->id,
						res.ogData,
						object->as.camera.fillColor,
						Vec4PropType::CameraBackgroundColor
					);
				}
			}

			return anyPropertyChanged;
		}

		static bool handleTransformAnimation(AnimationManagerData* am, Animation* animation)
		{
			bool anyPropertyChanged = false;

			if (auto res = ImGuiExtended::AnimObjDragDropInputBoxEx(": Source##ReplacementTransformSrcTarget", am, &animation->as.replacementTransform.srcAnimObjectId, animation->id);
				ANY_EDIT)
			{
				anyPropertyChanged = true;

				if (FINISHED_EDITING)
				{
					UndoSystem::animDragDropInput(
						Application::getUndoSystem(),
						res.ogData,
						animation->as.replacementTransform.srcAnimObjectId,
						animation->id,
						AnimDragDropType::ReplacementTransformSrc
					);
				}
			}

			if (auto res = ImGuiExtended::AnimObjDragDropInputBoxEx(": Replace To##ReplacementTransformDstTarget", am, &animation->as.replacementTransform.dstAnimObjectId, animation->id);
				ANY_EDIT)
			{
				anyPropertyChanged = true;

				if (FINISHED_EDITING)
				{
					UndoSystem::animDragDropInput(
						Application::getUndoSystem(),
						res.ogData,
						animation->as.replacementTransform.dstAnimObjectId,
						animation->id,
						AnimDragDropType::ReplacementTransformDst
					);
				}
			}

			return anyPropertyChanged;
		}

		static bool handleMoveToAnimationInspector(AnimationManagerData* am, Animation* animation)
		{
			bool anyPropertyChanged = false;

			if (auto res = ImGuiExtended::AnimObjDragDropInputBoxEx(": Object##MoveToObjectTarget", am, &animation->as.moveTo.object, animation->id);
				ANY_EDIT)
			{
				anyPropertyChanged = true;

				if (FINISHED_EDITING)
				{
					UndoSystem::animDragDropInput(
						Application::getUndoSystem(),
						res.ogData,
						animation->as.moveTo.object,
						animation->id,
						AnimDragDropType::MoveToTarget
					);
				}
			}

			if (auto res = ImGuiExtended::DragFloat3Ex(": Target Position##MoveToAnimation", &animation->as.moveTo.target, slowDragSpeed);
				ANY_EDIT)
			{
				anyPropertyChanged = true;

				if (FINISHED_EDITING)
				{
					UndoSystem::setVec3Prop(
						Application::getUndoSystem(),
						animation->id,
						res.ogData,
						animation->as.moveTo.target,
						Vec3PropType::MoveToTargetPos
					);
				}
			}

			return anyPropertyChanged;
		}

		static bool handleAnimateScaleInspector(AnimationManagerData* am, Animation* animation)
		{
			bool anyPropertyChanged = false;

			if (auto res = ImGuiExtended::AnimObjDragDropInputBoxEx(": Object##AnimateScaleObjectTarget", am, &animation->as.animateScale.object, animation->id);
				ANY_EDIT)
			{
				anyPropertyChanged = true;

				if (FINISHED_EDITING)
				{
					UndoSystem::animDragDropInput(
						Application::getUndoSystem(),
						res.ogData,
						animation->as.animateScale.object,
						animation->id,
						AnimDragDropType::AnimateScaleTarget
					);
				}
			}

			if (auto res = ImGuiExtended::DragFloat2Ex(": Target Scale##ScaleAnimation", &animation->as.animateScale.target, slowDragSpeed);
				ANY_EDIT)
			{
				anyPropertyChanged = true;

				if (FINISHED_EDITING)
				{
					UndoSystem::setVec2Prop(
						Application::getUndoSystem(),
						animation->id,
						res.ogData,
						animation->as.animateScale.target,
						Vec2PropType::AnimateScaleTarget
					);
				}
			}

			return anyPropertyChanged;
		}

		static bool handleShiftInspector(Animation* animation)
		{
			bool anyPropertyChanged = false;

			if (auto res = ImGuiExtended::DragFloat3Ex(": Shift Amount##ShiftAnimation", &animation->as.modifyVec3.target, slowDragSpeed);
				ANY_EDIT)
			{
				anyPropertyChanged = true;

				if (FINISHED_EDITING)
				{
					UndoSystem::setVec3Prop(
						Application::getUndoSystem(),
						animation->id,
						res.ogData,
						animation->as.modifyVec3.target,
						Vec3PropType::ModifyAnimationVec3Target
					);
				}
			}

			return anyPropertyChanged;
		}

		static bool handleRotateToAnimationInspector(Animation* animation)
		{
			bool anyPropertyChanged = false;

			if (auto res = ImGuiExtended::DragFloat3Ex(": Target Rotation##AnimateRotation", &animation->as.modifyVec3.target);
				ANY_EDIT)
			{
				anyPropertyChanged = true;

				if (FINISHED_EDITING)
				{
					UndoSystem::setVec3Prop(
						Application::getUndoSystem(),
						animation->id,
						res.ogData,
						animation->as.modifyVec3.target,
						Vec3PropType::ModifyAnimationVec3Target
					);
				}
			}

			return anyPropertyChanged;
		}


		static bool handleAnimateStrokeColorAnimationInspector(Animation* animation)
		{
			bool anyPropertyChanged = false;

			if (auto res = ImGuiExtended::ColorEdit4Ex(": Stroke Color##AnimateStrokeColor", &animation->as.modifyU8Vec4.target);
				ANY_EDIT)
			{
				anyPropertyChanged = true;

				if (FINISHED_EDITING)
				{
					UndoSystem::setU8Vec4Prop(
						Application::getUndoSystem(),
						animation->id,
						res.ogData,
						animation->as.modifyU8Vec4.target,
						U8Vec4PropType::AnimateU8Vec4Target
					);
				}
			}

			return anyPropertyChanged;
		}

		static bool handleAnimateFillColorAnimationInspector(Animation* animation)
		{
			bool anyPropertyChanged = false;

			if (auto res = ImGuiExtended::ColorEdit4Ex(": Fill Color##AnimateFillColor", &animation->as.modifyU8Vec4.target);
				ANY_EDIT)
			{
				anyPropertyChanged = true;

				if (FINISHED_EDITING)
				{
					UndoSystem::setU8Vec4Prop(
						Application::getUndoSystem(),
						animation->id,
						res.ogData,
						animation->as.modifyU8Vec4.target,
						U8Vec4PropType::AnimateU8Vec4Target
					);
				}
			}

			return anyPropertyChanged;
		}

		static bool handleCircumscribeInspector(AnimationManagerData* am, Animation* animation)
		{
			bool anyPropertyChanged = false;

			if (auto res = ImGuiExtended::AnimObjDragDropInputBoxEx(": Object##CircumscribeAnimationTarget", am, &animation->as.circumscribe.obj, animation->id);
				ANY_EDIT)
			{
				anyPropertyChanged = true;

				if (FINISHED_EDITING)
				{
					UndoSystem::animDragDropInput(
						Application::getUndoSystem(),
						res.ogData,
						animation->as.circumscribe.obj,
						animation->id,
						AnimDragDropType::CircumscribeTarget
					);
				}
			}

			int currentShape = (int)animation->as.circumscribe.shape;
			if (auto res = ImGuiExtended::ComboEx(": Shape##CircumscribeAnimation", &currentShape, _circumscribeShapeNames.data(), (int)CircumscribeShape::Length);
				ANY_EDIT)
			{
				anyPropertyChanged = true;

				if (FINISHED_EDITING)
				{
					UndoSystem::setEnumProp(
						Application::getUndoSystem(),
						animation->id,
						res.ogData,
						currentShape,
						EnumPropType::CircumscribeShape
					);
				}
			}

			int currentFade = (int)animation->as.circumscribe.fade;
			if (auto res = ImGuiExtended::ComboEx(": Fade Type##CircumscribeAnimation", &currentFade, _circumscribeFadeNames.data(), (int)CircumscribeFade::Length);
				ANY_EDIT)
			{
				anyPropertyChanged = true;

				if (FINISHED_EDITING)
				{
					UndoSystem::setEnumProp(
						Application::getUndoSystem(),
						animation->id,
						res.ogData,
						currentFade,
						EnumPropType::CircumscribeFade
					);
				}
			}

			ImGui::BeginDisabled(animation->as.circumscribe.fade != CircumscribeFade::FadeNone);
			if (auto res = ImGuiExtended::DragFloatEx(": Time Width##CircumscribeAnimation", &animation->as.circumscribe.timeWidth, slowDragSpeed, 0.1f, 1.0f, "%2.3f");
				ANY_EDIT)
			{
				anyPropertyChanged = true;

				if (FINISHED_EDITING)
				{
					UndoSystem::setFloatProp(
						Application::getUndoSystem(),
						animation->id,
						res.ogData,
						animation->as.circumscribe.timeWidth,
						FloatPropType::CircumscribeTimeWidth
					);
				}
			}
			ImGui::EndDisabled();

			if (auto res = ImGuiExtended::ColorEdit4Ex(": Color##CircumscribeAnimation", &animation->as.circumscribe.color);
				ANY_EDIT)
			{
				anyPropertyChanged = true;

				if (FINISHED_EDITING)
				{
					UndoSystem::setVec4Prop(
						Application::getUndoSystem(),
						animation->id,
						res.ogData,
						animation->as.circumscribe.color,
						Vec4PropType::CircumscribeColor
					);
				}
			}

			if (auto res = ImGuiExtended::DragFloatEx(": Buffer Size##CircumscribeAnimation", &animation->as.circumscribe.bufferSize, slowDragSpeed, 0.0f, 10.0f, "%2.3f");
				ANY_EDIT)
			{
				anyPropertyChanged = true;

				if (FINISHED_EDITING)
				{
					UndoSystem::setFloatProp(
						Application::getUndoSystem(),
						animation->id,
						res.ogData,
						animation->as.circumscribe.bufferSize,
						FloatPropType::CircumscribeBufferSize
					);
				}
			}

			return anyPropertyChanged;
		}

		static bool handleSquareInspector(AnimObject* object)
		{
			bool anyPropertyChanged = false;

			if (auto res = ImGuiExtended::DragFloatEx(": Side Length", &object->as.square.sideLength, slowDragSpeed);
				ANY_EDIT)
			{
				anyPropertyChanged = true;

				if (FINISHED_EDITING)
				{
					UndoSystem::setFloatProp(
						Application::getUndoSystem(),
						object->id,
						res.ogData,
						object->as.square.sideLength,
						FloatPropType::SquareSideLength
					);
				}
				else
				{
					object->as.square.reInit(object);
				}
			}

			return anyPropertyChanged;
		}

		static bool handleCircleInspector(AnimObject* object)
		{
			bool anyPropertyChanged = false;

			if (auto res = ImGuiExtended::DragFloatEx(": Radius", &object->as.circle.radius, slowDragSpeed);
				ANY_EDIT)
			{
				anyPropertyChanged = true;

				if (FINISHED_EDITING)
				{
					UndoSystem::setFloatProp(
						Application::getUndoSystem(),
						object->id,
						res.ogData,
						object->as.circle.radius,
						FloatPropType::CircleRadius
					);
				}
				else
				{
					object->as.circle.reInit(object);
				}
			}

			return anyPropertyChanged;
		}

		static bool handleArrowInspector(AnimObject* object)
		{
			bool shouldRegenerate = false;
			bool anyPropertyChanged = false;

			if (auto res = ImGuiExtended::DragFloatEx(": Stem Length##ArrowShape", &object->as.arrow.stemLength, slowDragSpeed);
				ANY_EDIT)
			{
				anyPropertyChanged = true;

				if (FINISHED_EDITING)
				{
					UndoSystem::setFloatProp(
						Application::getUndoSystem(),
						object->id,
						res.ogData,
						object->as.arrow.stemLength,
						FloatPropType::ArrowStemLength
					);
				}
				else
				{
					shouldRegenerate = true;
				}
			}

			if (auto res = ImGuiExtended::DragFloatEx(": Stem Width##ArrowShape", &object->as.arrow.stemWidth, slowDragSpeed);
				ANY_EDIT)
			{
				anyPropertyChanged = true;

				if (FINISHED_EDITING)
				{
					UndoSystem::setFloatProp(
						Application::getUndoSystem(),
						object->id,
						res.ogData,
						object->as.arrow.stemWidth,
						FloatPropType::ArrowStemWidth
					);
				}
				else
				{
					shouldRegenerate = true;
				}
			}

			if (auto res = ImGuiExtended::DragFloatEx(": Tip Length##ArrowShape", &object->as.arrow.tipLength, slowDragSpeed);
				ANY_EDIT)
			{
				anyPropertyChanged = true;

				if (FINISHED_EDITING)
				{
					UndoSystem::setFloatProp(
						Application::getUndoSystem(),
						object->id,
						res.ogData,
						object->as.arrow.tipLength,
						FloatPropType::ArrowTipLength
					);
				}
				else
				{
					shouldRegenerate = true;
				}
			}

			if (auto res = ImGuiExtended::DragFloatEx(": Tip Width##ArrowShape", &object->as.arrow.tipWidth, slowDragSpeed);
				ANY_EDIT)
			{
				anyPropertyChanged = true;

				if (FINISHED_EDITING)
				{
					UndoSystem::setFloatProp(
						Application::getUndoSystem(),
						object->id,
						res.ogData,
						object->as.arrow.tipWidth,
						FloatPropType::ArrowTipWidth
					);
				}
				else
				{
					shouldRegenerate = true;
				}
			}

			if (shouldRegenerate)
			{
				object->as.arrow.reInit(object);
			}

			return anyPropertyChanged;
		}

		static bool handleCubeInspector(AnimationManagerData* am, AnimObject* object)
		{
			bool anyPropertyChanged = false;

			if (auto res = ImGuiExtended::DragFloatEx(": Side Length", &object->as.cube.sideLength, slowDragSpeed);
				ANY_EDIT)
			{
				anyPropertyChanged = true;

				if (FINISHED_EDITING)
				{
					UndoSystem::setFloatProp(
						Application::getUndoSystem(),
						object->id,
						res.ogData,
						object->as.cube.sideLength,
						FloatPropType::CubeSideLength
					);
				}
				else
				{
					object->as.cube.reInit(am, object);
				}
			}

			return anyPropertyChanged;
		}

		static bool handleAxisInspector(AnimationManagerData* am, AnimObject* object)
		{
			bool anyPropertyChanged = false;
			bool reInitObject = false;

			if (auto res = ImGuiExtended::DragFloat3Ex(": Axes Length##Axis", &object->as.axis.axesLength, slowDragSpeed);
				ANY_EDIT)
			{
				anyPropertyChanged = true;

				if (FINISHED_EDITING)
				{
					UndoSystem::setVec3Prop(
						Application::getUndoSystem(),
						object->id,
						res.ogData,
						object->as.axis.axesLength,
						Vec3PropType::AxisAxesLength
					);
				}
				else
				{
					reInitObject = true;
				}
			}

			if (auto res = ImGuiExtended::DragInt2Ex(": X-Range##Axis", &object->as.axis.xRange, slowDragSpeed);
				ANY_EDIT)
			{
				anyPropertyChanged = true;

				// Make sure it's in strictly increasing order
				if (object->as.axis.xRange.min > object->as.axis.xRange.max)
				{
					object->as.axis.xRange.min = object->as.axis.xRange.max;
				}

				if (FINISHED_EDITING)
				{
					UndoSystem::setVec2iProp(
						Application::getUndoSystem(),
						object->id,
						res.ogData,
						object->as.axis.xRange,
						Vec2iPropType::AxisXRange
					);
				}
				else
				{
					reInitObject = true;
				}
			}

			if (auto res = ImGuiExtended::DragInt2Ex(": Y-Range##Axis", &object->as.axis.yRange, slowDragSpeed);
				ANY_EDIT)
			{
				anyPropertyChanged = true;

				// Make sure it's in strictly increasing order
				if (object->as.axis.yRange.min > object->as.axis.yRange.max)
				{
					object->as.axis.yRange.min = object->as.axis.yRange.max;
				}

				if (FINISHED_EDITING)
				{
					UndoSystem::setVec2iProp(
						Application::getUndoSystem(),
						object->id,
						res.ogData,
						object->as.axis.yRange,
						Vec2iPropType::AxisYRange
					);
				}
				else
				{
					reInitObject = true;
				}
			}

			if (auto res = ImGuiExtended::DragInt2Ex(": Z-Range##Axis", &object->as.axis.zRange, slowDragSpeed);
				ANY_EDIT)
			{
				anyPropertyChanged = true;

				if (object->as.axis.zRange.min > object->as.axis.zRange.max)
				{
					object->as.axis.zRange.min = object->as.axis.zRange.max;
				}

				if (FINISHED_EDITING)
				{
					UndoSystem::setVec2iProp(
						Application::getUndoSystem(),
						object->id,
						res.ogData,
						object->as.axis.zRange,
						Vec2iPropType::AxisZRange
					);
				}
				else
				{
					reInitObject = true;
				}
			}

			if (auto res = ImGuiExtended::DragFloatEx(": X-Increment##Axis", &object->as.axis.xStep, slowDragSpeed);
				ANY_EDIT)
			{
				anyPropertyChanged = true;

				if (FINISHED_EDITING)
				{
					UndoSystem::setFloatProp(
						Application::getUndoSystem(),
						object->id,
						res.ogData,
						object->as.axis.xStep,
						FloatPropType::AxisXStep
					);
				}
				else
				{
					reInitObject = true;
				}
			}

			if (auto res = ImGuiExtended::DragFloatEx(": Y-Increment##Axis", &object->as.axis.yStep, slowDragSpeed);
				ANY_EDIT)
			{
				anyPropertyChanged = true;

				if (FINISHED_EDITING)
				{
					UndoSystem::setFloatProp(
						Application::getUndoSystem(),
						object->id,
						res.ogData,
						object->as.axis.yStep,
						FloatPropType::AxisYStep
					);
				}
				else
				{
					reInitObject = true;
				}
			}

			if (auto res = ImGuiExtended::DragFloatEx(": Z-Increment##Axis", &object->as.axis.zStep, slowDragSpeed);
				ANY_EDIT)
			{
				anyPropertyChanged = true;

				if (FINISHED_EDITING)
				{
					UndoSystem::setFloatProp(
						Application::getUndoSystem(),
						object->id,
						res.ogData,
						object->as.axis.zStep,
						FloatPropType::AxisZStep
					);
				}
				else
				{
					reInitObject = true;
				}
			}

			if (auto res = ImGuiExtended::DragFloatEx(": Tick Width##Axis", &object->as.axis.tickWidth, slowDragSpeed);
				ANY_EDIT)
			{
				anyPropertyChanged = true;

				if (FINISHED_EDITING)
				{
					UndoSystem::setFloatProp(
						Application::getUndoSystem(),
						object->id,
						res.ogData,
						object->as.axis.tickWidth,
						FloatPropType::AxisTickWidth
					);
				}
				else
				{
					reInitObject = true;
				}
			}

			if (auto res = ImGuiExtended::CheckboxEx(": Draw Labels##Axis", &object->as.axis.drawNumbers);
				ANY_EDIT)
			{
				anyPropertyChanged = true;

				if (FINISHED_EDITING)
				{
					UndoSystem::setBoolProp(
						Application::getUndoSystem(),
						object->id,
						res.ogData,
						object->as.axis.drawNumbers,
						BoolPropType::AxisDrawNumbers
					);
				}
				else
				{
					reInitObject = true;
				}
			}

			if (object->as.axis.drawNumbers)
			{
				if (auto res = ImGuiExtended::DragFloatEx(": Font Size Pixels##Axis", &object->as.axis.fontSizePixels, slowDragSpeed, 0.0f, 300.0f);
					ANY_EDIT)
				{
					anyPropertyChanged = true;

					if (FINISHED_EDITING)
					{
						UndoSystem::setFloatProp(
							Application::getUndoSystem(),
							object->id,
							res.ogData,
							object->as.axis.fontSizePixels,
							FloatPropType::AxisFontSizePixels
						);
					}
					else
					{
						reInitObject = true;
					}
				}

				if (auto res = ImGuiExtended::DragFloatEx(": Label Padding##Axis", &object->as.axis.labelPadding, slowDragSpeed, 0.0f, 500.0f);
					ANY_EDIT)
				{
					anyPropertyChanged = true;

					if (FINISHED_EDITING)
					{
						UndoSystem::setFloatProp(
							Application::getUndoSystem(),
							object->id,
							res.ogData,
							object->as.axis.labelPadding,
							FloatPropType::AxisLabelPadding
						);
					}
					else
					{
						reInitObject = true;
					}
				}

				if (auto res = ImGuiExtended::DragFloatEx(": Label Stroke Width##Axis", &object->as.axis.labelStrokeWidth, slowDragSpeed, 0.0f, 200.0f);
					ANY_EDIT)
				{
					anyPropertyChanged = true;

					if (FINISHED_EDITING)
					{
						UndoSystem::setFloatProp(
							Application::getUndoSystem(),
							object->id,
							res.ogData,
							object->as.axis.labelStrokeWidth,
							FloatPropType::AxisLabelStrokeWidth
						);
					}
					else
					{
						reInitObject = true;
					}
				}
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
				object->as.axis.reInit(am, object);
			}

			return anyPropertyChanged;
		}

		static bool handleScriptObjectInspector(AnimationManagerData* am, AnimObject* obj)
		{
			bool anyPropertyChanged = false;

			ScriptObject& script = obj->as.script;

			constexpr size_t bufferSize = 512;
			char buffer[bufferSize] = "Drag File Here";
			if (bufferSize >= script.scriptFilepathLength + 1 && script.scriptFilepathLength > 0)
			{
				g_memory_copyMem((void*)buffer, bufferSize, script.scriptFilepath, sizeof(char) * script.scriptFilepathLength);
				buffer[script.scriptFilepathLength] = '\0';
			}

			if (auto res = ImGuiExtended::FileDragDropInputBoxEx(": Script File##ScriptFileTarget", buffer, bufferSize);
				ANY_EDIT)
			{
				anyPropertyChanged = true;

				if (FINISHED_EDITING)
				{
					UndoSystem::setStringProp(
						Application::getUndoSystem(),
						obj->id,
						res.ogData,
						std::string(buffer),
						StringPropType::ScriptFile
					);
				}
			}

			if (ImGui::Button("Generate"))
			{
				anyPropertyChanged = true;

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

			return anyPropertyChanged;
		}

		static bool handleImageObjectInspector(AnimationManagerData*, AnimObject* object)
		{
			bool anyPropertyChanged = false;

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
				": Filepath##ImageObject",
				filepathStr,
				filepathStrLength,
				ImGuiInputTextFlags_ReadOnly
			);
			ImGui::EndDisabled();
			ImGui::SameLine();

			if (ImGui::Button(ICON_FA_FILE_UPLOAD))
			{
				nfdchar_t* outPath = NULL;
				nfdresult_t result = NFD_OpenDialog("png,jpeg,jpg,tga,bmp,psd,gif,hdr,pic,pnm", NULL, &outPath);

				if (result == NFD_OKAY)
				{
					anyPropertyChanged = true;

					UndoSystem::setStringProp(
						Application::getUndoSystem(),
						object->id,
						filepathStr ? std::string(filepathStr) : "",
						std::string(outPath),
						StringPropType::ImageFilepath
					);
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
			if (auto res = ImGuiExtended::ComboEx(": Sample Mode##ImageObject", &currentFilterMode, _imageFilterModeNames.data(), (int)ImageFilterMode::Length);
				ANY_EDIT)
			{
				anyPropertyChanged = true;

				if (FINISHED_EDITING)
				{
					UndoSystem::setEnumProp(
						Application::getUndoSystem(),
						object->id,
						res.ogData,
						currentFilterMode,
						EnumPropType::ImageSampleMode
					);
				}
			}

			int currentRepeatMode = (int)object->as.image.repeatMode;
			if (auto res = ImGuiExtended::ComboEx(": Repeat##ImageObject", &currentRepeatMode, _imageRepeatModeNames.data(), (int)ImageRepeatMode::Length);
				ANY_EDIT)
			{
				anyPropertyChanged = true;

				if (FINISHED_EDITING)
				{
					UndoSystem::setEnumProp(
						Application::getUndoSystem(),
						object->id,
						res.ogData,
						currentRepeatMode,
						EnumPropType::ImageRepeat
					);
				}
			}

			return anyPropertyChanged;
		}
	}
}
