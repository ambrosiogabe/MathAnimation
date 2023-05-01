#include "editor/Gizmos.h"
#include "editor/EditorGui.h"
#include "animation/AnimationManager.h"
#include "renderer/Framebuffer.h"
#include "renderer/Renderer.h"
#include "renderer/Camera.h"
#include "renderer/Colors.h"
#include "core/Input.h"
#include "core/Application.h"
#include "core/Profiling.h"
#include "math/CMath.h"
#include "physics/Physics.h"

namespace MathAnim
{
	// -------------- Internal Structures --------------
	enum class FollowMouseMoveMode : uint8
	{
		None = 0,
		HzOnly,
		VtOnly,
		FreeMove
	};

	struct GizmoState
	{
		GizmoType gizmoType;
		GizmoVariant variant;
		uint64 idHash;
		Vec3 position;
		Vec3 positionMoveStart;
		Vec3 mouseDelta;
		FollowMouseMoveMode moveMode;
		bool shouldDraw;

		void render();
	};

	struct GlobalContext
	{
		// Maps from idHash to index in gizmos vector
		std::unordered_map<uint64, uint32> gizmoById;
		std::vector<GizmoState*> gizmos;

		uint64 hoveredGizmo;
		uint64 activeGizmo;
		uint64 lastActiveGizmo; // Gizmo active last frame
		GizmoVariant hotGizmoVariant; // Variant of the above IDs
		Vec3 mouseWorldPos3f;
		Ray mouseRay;
	};

	// -------------- Internal Functions --------------
	static inline Vec2 getGizmoPos(const Vec3& position, const Vec2& offset, float cameraZoom)
	{
		return CMath::vector2From3(position) + offset * cameraZoom;
	}

	static inline Vec3 getGizmoPos3D(const Vec3& position, const Vec3& offset, float cameraZoom)
	{
		return position + offset * cameraZoom;
	}

	namespace GizmoManager
	{
		// -------------- Internal Variables --------------
		static GlobalContext* gGizmoManager;
		static constexpr Vec2 defaultFreeMoveSize = Vec2{ 0.1f, 0.1f };

		static constexpr float defaultArrowHalfLength = 0.3f;
		static constexpr float defaultArrowTipRadius = 0.1f;
		static constexpr float defaultArrowTipLength = 0.25f;

		static constexpr Vec3 defaultVerticalMoveOffset3D = Vec3{ -0.4f, 0.1f, 0.0f };
		static constexpr Vec3 defaultHorizontalMoveOffset3D = Vec3{ 0.1f, -0.4f, 0.0f };
		static constexpr Vec3 defaultForwardMoveOffset3D = Vec3{ -0.4f, -0.4f, -0.1f - defaultArrowHalfLength };

		static constexpr Vec3 defaultVerticalMoveAABBSize = Vec3{ 
			defaultArrowTipRadius, 
			defaultArrowTipLength + defaultArrowHalfLength * 2.0f, 
			0.0f 
		};
		static constexpr Vec3 defaultHorizontalMoveAABBSize = Vec3{
			defaultArrowTipLength + defaultArrowHalfLength * 2.0f,
			defaultArrowTipRadius,
			0.0f
		};
		static constexpr Vec3 defaultForwardMoveAABBSize = Vec3{
			0.0f,
			defaultArrowTipRadius,
			defaultArrowTipLength + defaultArrowHalfLength * 2.0f
		};

		static constexpr int cameraOrientationFramebufferSize = 180;
		static Framebuffer cameraOrientationFramebuffer;
		static Texture cameraOrientationGizmoTexture;

		static constexpr uint64 NullGizmo = UINT64_MAX;

		// -------------- Internal Functions --------------
		static GizmoState* getGizmoByName(const char* name);
		static GizmoState* getGizmoById(uint64 id);
		static uint64 hashName(const char* name);
		static GizmoState* createDefaultGizmoState(const char* name, GizmoType type);
		static bool isMouseHovered(const Vec3& centerPosition, const Vec3& size);
		static Vec3 getMouseWorldPos3f();
		static Ray getMouseRay();
		static void handleActiveCheck(GizmoState* gizmo, const Vec3& offset, const Vec3& gizmoSize, float cameraZoom);

		void init()
		{
			void* gMemory = g_memory_allocate(sizeof(GlobalContext));
			gGizmoManager = new(gMemory)GlobalContext();
			gGizmoManager->hoveredGizmo = NullGizmo;
			gGizmoManager->activeGizmo = NullGizmo;
			gGizmoManager->mouseWorldPos3f = Vec3{ 0.0f, 0.0f, 0.0f };
			gGizmoManager->mouseRay = Physics::createRay(Vec3{ 0, 0, 0 }, Vec3{ 1, 0, 0 });

			cameraOrientationFramebuffer = Renderer::prepareFramebuffer(cameraOrientationFramebufferSize, cameraOrientationFramebufferSize);
			cameraOrientationGizmoTexture = TextureBuilder()
				.setFilepath("./assets/images/cameraOrientationGizmos.png")
				.generate(true);
		}

		void update(AnimationManagerData* am)
		{
			MP_PROFILE_EVENT("Gizmo_Update");
			GlobalContext* g = gGizmoManager;
			g->lastActiveGizmo = g->activeGizmo;
			g->mouseWorldPos3f = getMouseWorldPos3f();
			g->mouseRay = getMouseRay();

			EditorGui::onGizmo(am);
		}

		void render(AnimationManagerData* am)
		{
			MP_PROFILE_EVENT("Gizmo_Render");

			// Render call stuff
			{
				GlobalContext* g = gGizmoManager;
				for (auto iter : g->gizmos)
				{
					if (iter->shouldDraw)
					{
						// Draw the gizmo
						iter->render();
					}
				}

				// End Frame stuff
				for (auto iter : g->gizmos)
				{
					iter->shouldDraw = false;
				}
			}

			// Draw animation manager camera frustum/billboards
			{
				const AnimObject* orthoCameraObj = AnimationManager::getActiveCamera2D(am);
				if (orthoCameraObj)
				{
					const Camera& orthoCamera = orthoCameraObj->as.camera;

					Renderer::pushStrokeWidth(0.05f);
					Renderer::pushColor(Colors::Neutral[0]);
					Vec4 leftRightBottomTop = orthoCamera.getLeftRightBottomTop();
					Vec2 projectionSize = CMath::abs(Vec2{
						leftRightBottomTop.values[1] - leftRightBottomTop.values[0],
						leftRightBottomTop.values[3] - leftRightBottomTop.values[2]
						});
					Renderer::drawSquare(CMath::vector2From3(orthoCamera.position) - projectionSize / 2.0f, projectionSize);
					Renderer::popColor();
					Renderer::popStrokeWidth();
				}
			}
		}

		void renderOrientationGizmo(const Camera& editorCamera)
		{
			// Collect draw calls
			{
				static Camera dummyCamera = Camera::createDefault();
				dummyCamera.mode = CameraMode::Perspective;
				dummyCamera.aspectRatioFraction.x = 1;
				dummyCamera.aspectRatioFraction.y = 1;
				dummyCamera.orientation = editorCamera.orientation;
				dummyCamera.focalDistance = editorCamera.focalDistance;
				dummyCamera.orthoZoomLevel = editorCamera.orthoZoomLevel;
				dummyCamera.calculateMatrices(true);

				dummyCamera.position = -6.0f * dummyCamera.forward;
				dummyCamera.calculateMatrices(true);

				Renderer::pushCamera3D(&dummyCamera);

				constexpr Vec2 size = Vec2{ 2.0f, 2.0f };
				constexpr float radius = 0.8f;

				constexpr const Vec3 xPos = Vec3{ radius, 0.0f, 0.0f };
				float orientationXDp = CMath::dot(xPos, dummyCamera.forward);
				bool orientationIsXPositive = orientationXDp >= -1.0f && orientationXDp < 0.0f;
				Renderer::drawTexturedBillboard3D(
					cameraOrientationGizmoTexture,
					xPos, size,
					Vec2{ 0, 0 }, Vec2{ 0.5f, 0.5f },
					orientationIsXPositive ? Colors::AccentRed[4] : Colors::AccentRed[6]
				);
				Renderer::drawTexturedBillboard3D(
					cameraOrientationGizmoTexture,
					-1.0f * xPos, size,
					Vec2{ 0.5f, 0.5f }, Vec2{ 1, 1 },
					orientationIsXPositive ? Colors::AccentRed[6] : Colors::AccentRed[4]
				);

				constexpr const Vec3 yPos = Vec3{ 0.0f, radius, 0.0f };
				float orientationYDp = CMath::dot(yPos, dummyCamera.forward);
				bool orientationIsYPositive = orientationYDp >= -1.0f && orientationYDp < 0.0f;
				Renderer::drawTexturedBillboard3D(
					cameraOrientationGizmoTexture,
					yPos, size,
					Vec2{ 0.5f, 0.0f }, Vec2{ 1.0f, 0.5f },
					orientationIsYPositive ? Colors::Primary[4] : Colors::Primary[7]
				);
				Renderer::drawTexturedBillboard3D(
					cameraOrientationGizmoTexture,
					-1.0f * yPos, size,
					Vec2{ 0.5f, 0.5f }, Vec2{ 1, 1 },
					orientationIsYPositive ? Colors::Primary[7] : Colors::Primary[4]
				);

				constexpr const Vec3 zPos = Vec3{ 0, 0, radius };
				float orientationZDp = CMath::dot(zPos, dummyCamera.forward);
				bool orientationIsZPositive = orientationZDp >= -1.0f && orientationZDp < 0.0f;
				Renderer::drawTexturedBillboard3D(
					cameraOrientationGizmoTexture,
					zPos, size,
					Vec2{ 0.0f, 0.5f }, Vec2{ 0.5f, 1.0f },
					orientationIsZPositive ? Colors::AccentGreen[4] : Colors::AccentGreen[6]
				);
				Renderer::drawTexturedBillboard3D(
					cameraOrientationGizmoTexture,
					-1.0f * zPos, size,
					Vec2{ 0.5f, 0.5f }, Vec2{ 1, 1 },
					orientationIsZPositive ? Colors::AccentGreen[6] : Colors::AccentGreen[4]
				);

				constexpr const Vec3 center = Vec3{ 0, 0, 0 };
				constexpr float lineWidth = 0.4f;
				Renderer::drawLine3D(
					center, xPos, lineWidth,
					orientationIsXPositive ? Colors::AccentRed[4] : Colors::AccentRed[6]
				);
				Renderer::drawLine3D(
					center, yPos, lineWidth,
					orientationIsYPositive ? Colors::Primary[4] : Colors::Primary[7]
				);
				Renderer::drawLine3D(
					center, zPos, lineWidth,
					orientationIsZPositive ? Colors::AccentGreen[4] : Colors::AccentGreen[6]
				);

				Renderer::popCamera3D();
			}

			// Draw the output to the framebuffer
			Renderer::bindAndUpdateViewportForFramebuffer(cameraOrientationFramebuffer);
			Renderer::clearFramebuffer(cameraOrientationFramebuffer, Vec4{ 0, 0, 0, 0 });
			Renderer::renderToFramebuffer(cameraOrientationFramebuffer, "Gizmos_CameraOrientationGizmo");
		}

		void free()
		{
			if (gGizmoManager)
			{
				auto iter = gGizmoManager->gizmos.begin();
				while (iter != gGizmoManager->gizmos.end())
				{
					g_memory_free(*iter);
					iter = gGizmoManager->gizmos.erase(iter);
				}

				gGizmoManager->~GlobalContext();
				g_memory_free(gGizmoManager);
				gGizmoManager = nullptr;
			}

			cameraOrientationFramebuffer.destroy();
			cameraOrientationGizmoTexture.destroy();
		}

		const Texture& getCameraOrientationTexture()
		{
			return cameraOrientationFramebuffer.getColorAttachment(0);
		}

		bool anyGizmoActive()
		{
			// Return whether any gizmo was active this frame or last frame 
			// so that it reflects the state of queries accurately
			return gGizmoManager->activeGizmo != NullGizmo || gGizmoManager->lastActiveGizmo != NullGizmo;
		}

		bool translateGizmo(const char* gizmoName, Vec3* position, GizmoVariant variant)
		{
			GlobalContext* g = gGizmoManager;

			// Find or create the gizmo
			GizmoState* gizmo = getGizmoByName(gizmoName);
			if (!gizmo)
			{
				gizmo = createDefaultGizmoState(gizmoName, GizmoType::Translation);
				// Initialize to something sensible
				gizmo->positionMoveStart = *position;
			}
			gizmo->position = *position;
			gizmo->variant = variant;

			if (Input::keyPressed(GLFW_KEY_G))
			{
				switch (gizmo->moveMode)
				{
				case FollowMouseMoveMode::None:
					gizmo->positionMoveStart = *position;
					gizmo->mouseDelta = gizmo->positionMoveStart - g->mouseWorldPos3f;
					gizmo->moveMode = FollowMouseMoveMode::FreeMove;
					break;
				case FollowMouseMoveMode::HzOnly:
				case FollowMouseMoveMode::VtOnly:
				case FollowMouseMoveMode::FreeMove:
					break;
				}
			}

			if (gizmo->moveMode != FollowMouseMoveMode::None)
			{
				if (Input::keyPressed(GLFW_KEY_X))
				{
					switch (gizmo->moveMode)
					{
					case FollowMouseMoveMode::VtOnly:
					case FollowMouseMoveMode::FreeMove:
						gizmo->moveMode = FollowMouseMoveMode::HzOnly;
						break;
					case FollowMouseMoveMode::HzOnly:
					case FollowMouseMoveMode::None:
						break;
					}
					*position = gizmo->positionMoveStart;
				}
				if (Input::keyPressed(GLFW_KEY_Y))
				{
					switch (gizmo->moveMode)
					{
					case FollowMouseMoveMode::HzOnly:
					case FollowMouseMoveMode::FreeMove:
						gizmo->moveMode = FollowMouseMoveMode::VtOnly;
						break;
					case FollowMouseMoveMode::VtOnly:
					case FollowMouseMoveMode::None:
						break;
					}
					*position = gizmo->positionMoveStart;
				}
				if (Input::keyPressed(GLFW_KEY_ESCAPE))
				{
					// We can return immediately from a cancel operation
					*position = gizmo->positionMoveStart;
					gizmo->moveMode = FollowMouseMoveMode::None;
					return true;
				}
			}

			const Camera* camera = Application::getEditorCamera();
			float zoom = camera->orthoZoomLevel;
			if (gizmo->moveMode != FollowMouseMoveMode::None)
			{
				Vec2 mousePos = EditorGui::mouseToNormalizedViewport();
				Vec3 unprojectedMousePos = camera->reverseProject(mousePos);
				switch (gizmo->moveMode)
				{
				case FollowMouseMoveMode::VtOnly:
					position->y = unprojectedMousePos.y + gizmo->mouseDelta.y;
					gizmo->shouldDraw = true;
					break;
				case FollowMouseMoveMode::HzOnly:
					position->x = unprojectedMousePos.x + gizmo->mouseDelta.x;
					gizmo->shouldDraw = true;
					break;
				case FollowMouseMoveMode::FreeMove:
					position->x = unprojectedMousePos.x + gizmo->mouseDelta.x;
					position->y = unprojectedMousePos.y + gizmo->mouseDelta.y;
					break;
				case FollowMouseMoveMode::None:
					break;
				}

				g->activeGizmo = gizmo->idHash;
				if (Input::mouseClicked(MouseButton::Left))
				{
					gizmo->moveMode = FollowMouseMoveMode::None;
					g->activeGizmo = NullGizmo;
				}

				// If we're in gizmo follow mouse mode, then every frame results in a change operation.
				// So, we return true here every time.
				return true;
			}

			// Otherwise, we continue on to the regular gizmo logic

			gizmo->shouldDraw = true;
			if (g->hoveredGizmo == NullGizmo && g->activeGizmo == NullGizmo)
			{
				// Check if free move variant is hovered
				//if (isMouseHovered(gizmo->position, defaultFreeMoveSize * zoom))
				//{
				//	g->hoveredGizmo = gizmo->idHash;
				//	g->hotGizmoVariant = GizmoVariant::Free;
				//}
				if (isMouseHovered(
					getGizmoPos3D(gizmo->position, defaultVerticalMoveOffset3D, zoom), 
					defaultVerticalMoveAABBSize * zoom
				))
				{
					g->hoveredGizmo = gizmo->idHash;
					g->hotGizmoVariant = GizmoVariant::Vertical;
				}
				else if (isMouseHovered(
					getGizmoPos3D(gizmo->position, defaultHorizontalMoveOffset3D, zoom), 
					defaultHorizontalMoveAABBSize * zoom
				))
				{
					g->hoveredGizmo = gizmo->idHash;
					g->hotGizmoVariant = GizmoVariant::Horizontal;
				}
				else if (isMouseHovered(
					getGizmoPos3D(gizmo->position, defaultForwardMoveOffset3D, zoom),
					defaultForwardMoveAABBSize * zoom
				))
				{
					g->hoveredGizmo = gizmo->idHash;
					g->hotGizmoVariant = GizmoVariant::Forward;
				}
			}

			if (g->hoveredGizmo == gizmo->idHash)
			{
				// Gizmo is "active" if the user is holding the mouse button down on the gizmo
				// Check if free move variant is changed to active
				//if (g->hotGizmoVariant == GizmoVariant::Free)
				//{
				//	handleActiveCheck(gizmo, Vec2{ 0, 0 }, defaultFreeMoveSize, zoom);
				//}
				// Check if vertical move is changed to active
				if (g->hotGizmoVariant == GizmoVariant::Vertical)
				{
					handleActiveCheck(gizmo, defaultVerticalMoveOffset3D, defaultVerticalMoveAABBSize, zoom);
				}
				// Check if horizontal move is changed to active
				else if (g->hotGizmoVariant == GizmoVariant::Horizontal)
				{
					handleActiveCheck(gizmo, defaultHorizontalMoveOffset3D, defaultHorizontalMoveAABBSize, zoom);
				}
				// Check if forward move is changed to active
				else if (g->hotGizmoVariant == GizmoVariant::Forward)
				{
					handleActiveCheck(gizmo, defaultForwardMoveOffset3D, defaultForwardMoveAABBSize, zoom);
				}
			}

			bool modified = false;
			if (g->activeGizmo == gizmo->idHash)
			{
				if (Input::mouseUp(MouseButton::Left))
				{
					g->activeGizmo = NullGizmo;
				}
				else if (Input::keyPressed(GLFW_KEY_ESCAPE))
				{
					// Cancel move operation
					g->activeGizmo = NullGizmo;
					*position = gizmo->positionMoveStart;
					modified = true;
				}
				else
				{
					// Handle mouse dragging
					// Transform the position passed in to the current mouse position
					switch (g->hotGizmoVariant)
					{
					case GizmoVariant::Free:
						*position = g->mouseWorldPos3f;
						break;
					case GizmoVariant::Vertical:
						// NOTE: We subtract mouseDelta here to make sure it's offset properly from the mouse
						// and then when it gets added back in below it cancels the operation
						*position = Vec3{ position->x - gizmo->mouseDelta.x, g->mouseWorldPos3f.y, position->z - gizmo->mouseDelta.z };
						break;
					case GizmoVariant::Horizontal:
						// NOTE: Same note as above
						*position = Vec3{ g->mouseWorldPos3f.x, position->y - gizmo->mouseDelta.y, position->z - gizmo->mouseDelta.z };
						break;
					case GizmoVariant::Forward:
						// NOTE: Same note as above
						*position = Vec3{ position->x - gizmo->mouseDelta.x, position->y - gizmo->mouseDelta.y, g->mouseWorldPos3f.z };
						break;
					case GizmoVariant::None:
					case GizmoVariant::All:
						break;
					}

					// Add back in mouse delta to make sure we maintain original distance
					// when we started dragging the object
					*position += gizmo->mouseDelta;
					gizmo->position = *position;
					modified = true;
				}
			}

			return modified;
		}

		// -------------- Internal Functions --------------
		static GizmoState* getGizmoByName(const char* name)
		{
			uint64 gizmoId = hashName(name);
			return getGizmoById(gizmoId);
		}

		static GizmoState* getGizmoById(uint64 id)
		{
			GlobalContext* g = gGizmoManager;
			auto iter = g->gizmoById.find(id);
			if (iter == g->gizmoById.end())
			{
				return nullptr;
			}

			return g->gizmos[iter->second];
		}

		static uint64 hashName(const char* name)
		{
			constexpr uint64 FNVOffsetBasis = 0xcbf29ce48422232ULL;
			constexpr uint64 FNVPrime = 0x00000100000001B3ULL;

			uint64 hash = FNVOffsetBasis;
			size_t strLen = std::strlen(name);
			for (size_t i = 0; i < strLen; i++)
			{
				hash = hash * FNVPrime;
				hash = hash ^ name[i];
			}

			return hash;
		}

		static GizmoState* createDefaultGizmoState(const char* name, GizmoType type)
		{
			GizmoState* gizmoState = (GizmoState*)g_memory_allocate(sizeof(GizmoState));
			gizmoState->gizmoType = type;
			uint64 hash = hashName(name);
			gizmoState->idHash = hash;
			gizmoState->shouldDraw = false;
			gizmoState->moveMode = FollowMouseMoveMode::None;
			gizmoState->mouseDelta = Vec3{ 0.0f, 0.0f, 0.0f };

			GlobalContext* g = gGizmoManager;
			g->gizmos.push_back(gizmoState);
			g->gizmoById[hash] = (uint32)g->gizmos.size() - 1;

			return gizmoState;
		}

		static bool isMouseHovered(const Vec3& centerPosition, const Vec3& size)
		{
			const Ray& ray = gGizmoManager->mouseRay;
			AABB aabb = Physics::createAABB(centerPosition, size);
			RaycastResult res = Physics::rayIntersectsAABB(ray, aabb);
			return res.hit();
		}

		static void handleActiveCheck(GizmoState* gizmo, const Vec3& offset, const Vec3& gizmoSize, float cameraZoom)
		{
			GlobalContext* g = gGizmoManager;
			if (Input::mouseDown(MouseButton::Left) && isMouseHovered(getGizmoPos3D(gizmo->position, offset, cameraZoom), gizmoSize * cameraZoom))
			{
				gizmo->mouseDelta = gizmo->position - g->mouseWorldPos3f;
				g->activeGizmo = gizmo->idHash;
				g->hoveredGizmo = NullGizmo;
			}
			else if (!isMouseHovered(getGizmoPos3D(gizmo->position, offset, cameraZoom), gizmoSize * cameraZoom))
			{
				g->hoveredGizmo = NullGizmo;
			}
		}

		static Vec3 getMouseWorldPos3f()
		{
			Vec2 normalizedMousePos = EditorGui::mouseToNormalizedViewport();
			Camera* camera = Application::getEditorCamera();
			return camera->reverseProject(normalizedMousePos);
		}

		static Ray getMouseRay()
		{
			Vec2 normalizedMousePos = EditorGui::mouseToNormalizedViewport();
			Camera* camera = Application::getEditorCamera();
			Vec3 origin = camera->reverseProject(normalizedMousePos, camera->nearFarRange.min);
			Vec3 end = camera->reverseProject(normalizedMousePos, camera->nearFarRange.max);
			return Physics::createRay(origin, end);
		}
	}

	// -------------- Interal Gizmo Functions --------------

	// -------------- Gizmo Functions --------------
	void GizmoState::render()
	{
		GlobalContext* g = GizmoManager::gGizmoManager;
		const Camera* camera = Application::getEditorCamera();
		const float zoom = camera->orthoZoomLevel;
		Vec4 leftRightBottomTop = camera->getLeftRightBottomTop();
		Vec2 projectionSize = Vec2{
			leftRightBottomTop.values[1] - leftRightBottomTop.values[0],
			leftRightBottomTop.values[3] - leftRightBottomTop.values[2]
		};

		// If it's in free move mode, render guidelines
		if (moveMode != FollowMouseMoveMode::None)
		{
			const Vec2& cameraProjectionSize = projectionSize * zoom;
			Vec3 leftGuideline = this->positionMoveStart - Vec3{ cameraProjectionSize.x, 0.0f, 0.0f };
			Vec3 rightGuideline = this->positionMoveStart + Vec3{ cameraProjectionSize.x, 0.0f, 0.0f };
			Vec3 bottomGuideline = this->positionMoveStart - Vec3{ 0.0f, cameraProjectionSize.y, 0.0f };
			Vec3 topGuideline = this->positionMoveStart + Vec3{ 0.0f, cameraProjectionSize.y, 0.0f };

			float hzGuidelineWidth = cameraProjectionSize.x / 1'000.0f;
			float vtGuidelineWidth = cameraProjectionSize.y / 400.0f;
			Vec2 hzGuidelineSize = Vec2{ rightGuideline.x - leftGuideline.x, hzGuidelineWidth };
			Vec2 vtGuidelineSize = Vec2{ vtGuidelineWidth, topGuideline.y - bottomGuideline.y };

			switch (moveMode)
			{
			case FollowMouseMoveMode::HzOnly:
				Renderer::pushColor(Colors::AccentRed[4]);
				Renderer::drawFilledQuad(CMath::vector2From3(this->positionMoveStart), hzGuidelineSize);
				Renderer::popColor();
				break;
			case FollowMouseMoveMode::VtOnly:
				Renderer::pushColor(Colors::AccentGreen[4]);
				Renderer::drawFilledQuad(CMath::vector2From3(this->positionMoveStart), vtGuidelineSize);
				Renderer::popColor();
				break;
			case FollowMouseMoveMode::None:
			case FollowMouseMoveMode::FreeMove:
				break;
			}

			return;
		}

		// Otherwise render the gizmo shapes
		if ((uint8)variant & (uint8)GizmoVariant::Free)
		{
			int colorIndex = 4;
			if ((idHash != g->hoveredGizmo && idHash != g->activeGizmo) || g->hotGizmoVariant != GizmoVariant::Free)
			{
				colorIndex = 4;
			}
			else if (idHash == g->hoveredGizmo)
			{
				colorIndex = 5;
			}
			else if (idHash == g->activeGizmo)
			{
				colorIndex = 6;
			}

			Renderer::pushColor(Colors::AccentRed[colorIndex]);
			Renderer::drawFilledQuad3D(
				this->position,
				GizmoManager::defaultFreeMoveSize * zoom,
				Vector3::Forward,
				Vector3::Up
			);
			Renderer::popColor();

			Vec3 xArrowCenter = getGizmoPos3D(this->position, GizmoManager::defaultHorizontalMoveOffset3D, zoom);
			Vec3 zArrowCenter = getGizmoPos3D(this->position, GizmoManager::defaultForwardMoveOffset3D, zoom);
			Vec3 yArrowCenter = getGizmoPos3D(this->position, GizmoManager::defaultVerticalMoveOffset3D, zoom);

			Renderer::pushColor(Colors::Primary[colorIndex]);
			Renderer::drawFilledQuad3D(
				Vec3{ this->position.x, xArrowCenter.y, zArrowCenter.z },
				GizmoManager::defaultFreeMoveSize * zoom,
				Vector3::Up,
				Vector3::Right
			);
			Renderer::popColor();

			Renderer::pushColor(Colors::AccentGreen[colorIndex]);
			Renderer::drawFilledQuad3D(
				Vec3{ yArrowCenter.x, this->position.y, zArrowCenter.z },
				GizmoManager::defaultFreeMoveSize * zoom,
				Vector3::Right,
				Vector3::Up
			);
			Renderer::popColor();
		}

		if ((uint8)variant & (uint8)GizmoVariant::Horizontal)
		{
			Vec3 pos3D = getGizmoPos3D(this->position, GizmoManager::defaultHorizontalMoveOffset3D, zoom);
			const Vec4* color = nullptr;
			if ((idHash != g->hoveredGizmo && idHash != g->activeGizmo) || g->hotGizmoVariant != GizmoVariant::Horizontal)
			{
				color = &Colors::AccentRed[4];
			}
			else if (idHash == g->hoveredGizmo)
			{
				color = &Colors::AccentRed[5];
			}
			else if (idHash == g->activeGizmo)
			{
				color = &Colors::AccentRed[6];
			}

			if (color != nullptr)
			{
				Renderer::pushColor(*color);
				Vec3 halfSizeZoom = Vec3{ GizmoManager::defaultArrowHalfLength, 0.0f, 0.0f } *zoom;
				Renderer::drawCylinder(
					pos3D - halfSizeZoom,
					pos3D + halfSizeZoom,
					Vector3::Up,
					GizmoManager::defaultArrowTipRadius / 3.0f * zoom
				);
				Vec3 baseCenter = pos3D + halfSizeZoom;
				Renderer::drawCone3D(
					baseCenter,
					Vector3::Right,
					Vector3::Up,
					GizmoManager::defaultArrowTipRadius * zoom,
					GizmoManager::defaultArrowTipLength * zoom
				);
				Renderer::popColor();
			}
		}

		if ((uint8)variant & (uint8)GizmoVariant::Vertical)
		{
			Vec3 pos3D = getGizmoPos3D(this->position, GizmoManager::defaultVerticalMoveOffset3D, zoom);
			const Vec4* color = nullptr;
			if ((idHash != g->hoveredGizmo && idHash != g->activeGizmo) || g->hotGizmoVariant != GizmoVariant::Vertical)
			{
				color = &Colors::Primary[4];
			}
			else if (idHash == g->hoveredGizmo)
			{
				color = &Colors::Primary[5];
			}
			else if (idHash == g->activeGizmo)
			{
				color = &Colors::Primary[6];
			}

			if (color != nullptr)
			{
				Renderer::pushColor(*color);
				Vec3 halfSizeZoom = Vec3{ 0.0f, GizmoManager::defaultArrowHalfLength, 0.0f } *zoom;
				Renderer::drawCylinder(
					pos3D - halfSizeZoom,
					pos3D + halfSizeZoom,
					Vector3::Right,
					GizmoManager::defaultArrowTipRadius / 3.0f * zoom
				);
				Vec3 baseCenter = pos3D + halfSizeZoom;
				Renderer::drawCone3D(
					baseCenter,
					Vector3::Up,
					Vector3::Right,
					GizmoManager::defaultArrowTipRadius * zoom,
					GizmoManager::defaultArrowTipLength * zoom
				);
				Renderer::popColor();
			}
		}

		if ((uint8)variant & (uint8)GizmoVariant::Forward)
		{
			Vec3 pos3D = getGizmoPos3D(this->position, GizmoManager::defaultForwardMoveOffset3D, zoom);
			const Vec4* color = nullptr;
			if ((idHash != g->hoveredGizmo && idHash != g->activeGizmo) || g->hotGizmoVariant != GizmoVariant::Forward)
			{
				color = &Colors::AccentGreen[4];
			}
			else if (idHash == g->hoveredGizmo)
			{
				color = &Colors::AccentGreen[5];
			}
			else if (idHash == g->activeGizmo)
			{
				color = &Colors::AccentGreen[6];
			}

			if (color != nullptr)
			{
				Renderer::pushColor(*color);
				Vec3 halfSizeZoom = Vec3{ 0.0f, 0.0f, GizmoManager::defaultArrowHalfLength } *zoom;
				Renderer::drawCylinder(
					pos3D - halfSizeZoom,
					pos3D + halfSizeZoom,
					Vector3::Up,
					GizmoManager::defaultArrowTipRadius / 3.0f * zoom
				);
				Vec3 baseCenter = pos3D - halfSizeZoom;
				Renderer::drawCone3D(
					baseCenter,
					Vector3::Back,
					Vector3::Up,
					GizmoManager::defaultArrowTipRadius * zoom,
					GizmoManager::defaultArrowTipLength * zoom
				);
				Renderer::popColor();
			}
		}
	}

	// -------------- Interal Gizmo Functions --------------
}