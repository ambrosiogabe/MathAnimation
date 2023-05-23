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
	enum class FollowMouseConstraint : uint8
	{
		None = 0,
		XOnly,
		YOnly,
		ZOnly,
		XZOnly,
		YZOnly,
		XYOnly,
		FreeMove
	};
}

CppUtils::Stream& operator<<(CppUtils::Stream& stream, const MathAnim::FollowMouseConstraint& c)
{
	switch (c)
	{
	case MathAnim::FollowMouseConstraint::None: stream << "FollowMouseConstraint::None"; break;
	case MathAnim::FollowMouseConstraint::XOnly: stream << "FollowMouseConstraint::XOnly"; break;
	case MathAnim::FollowMouseConstraint::YOnly: stream << "FollowMouseConstraint::YOnly"; break;
	case MathAnim::FollowMouseConstraint::ZOnly: stream << "FollowMouseConstraint::ZOnly"; break;
	case MathAnim::FollowMouseConstraint::XZOnly: stream << "FollowMouseConstraint::XZOnly"; break;
	case MathAnim::FollowMouseConstraint::YZOnly: stream << "FollowMouseConstraint::YZOnly"; break;
	case MathAnim::FollowMouseConstraint::XYOnly: stream << "FollowMouseConstraint::XYOnly"; break;
	case MathAnim::FollowMouseConstraint::FreeMove: stream << "FollowMouseConstraint::FreeMove"; break;
	}

	return stream;
}

namespace MathAnim
{

	enum class GizmoSubComponent : uint8
	{
		None = 0,

		XTranslate,
		YTranslate,
		ZTranslate,
		XZPlaneTranslate,
		YZPlaneTranslate,
		XYPlaneTranslate,

		XRotate,
		YRotate,
		ZRotate,

		XScale,
		YScale,
		ZScale,
		XZPlaneScale,
		YZPlaneScale,
		XYPlaneScale,

		Length
	};

	struct GizmoState
	{
		GizmoType gizmoType;
		uint64 idHash;
		Vec3 position;
		Vec3 positionMoveStart;
		Vec3 scaleStart;
		Vec3 mouseDelta;
		FollowMouseConstraint moveMode;
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
		GizmoSubComponent hotGizmoComponent; // SubComponent of the above IDs
		Vec3 mouseWorldPos3f;
		Ray mouseRay;
		GizmoType visualMode; // This is the gizmo's that are visually shown to the user
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

		static constexpr float translatePlaneSideLength = 0.1f;
		static constexpr float translateArrowHalfLength = 0.3f;
		static constexpr float translateArrowTipRadius = 0.1f;
		static constexpr float translateArrowTipLength = 0.25f;

		static constexpr float scaleCubeSideLength = 0.15f;
		static constexpr Vec3 scaleCubeSize = Vec3{ scaleCubeSideLength, scaleCubeSideLength , scaleCubeSideLength };
		static const Vec3 scaleCubeHalfSize = scaleCubeSize / 2.0f;

		// Translation arrow offsets
		static constexpr Vec3 yTranslateOffset = Vec3{ -0.4f, 0.1f, 0.0f };
		static constexpr Vec3 xTranslateOffset = Vec3{ 0.1f, -0.4f, 0.0f };
		static constexpr Vec3 zTranslateOffset = Vec3{ -0.4f, -0.4f, -0.1f - translateArrowHalfLength };

		// Plane movement offsets
		static const Vec3 xyTranslateOffset = Vec3{ 0.0f, 0.0f, 0.0f };
		static const Vec3 xzTranslateOffset = Vec3{
			0.0f,
			xTranslateOffset.y,
			zTranslateOffset.z
		};
		static const Vec3 zyTranslateOffset = Vec3{
			yTranslateOffset.x,
			0.0f,
			zTranslateOffset.z
		};

		// Arrow translation AABBs
		static constexpr Vec3 yTranslateAABBSize = Vec3{
			translateArrowTipRadius,
			translateArrowTipLength + translateArrowHalfLength * 2.0f,
			translateArrowTipRadius
		};
		static constexpr Vec3 xTranslateAABBSize = Vec3{
			translateArrowTipLength + translateArrowHalfLength * 2.0f,
			translateArrowTipRadius,
			translateArrowTipRadius
		};
		static constexpr Vec3 zTranslateAABBSize = Vec3{
			translateArrowTipRadius,
			translateArrowTipRadius,
			translateArrowTipLength + translateArrowHalfLength * 2.0f
		};

		// Plane translation AABBs
		static constexpr Vec2 translationPlaneSize = Vec2{ translatePlaneSideLength, translatePlaneSideLength };
		static constexpr Vec3 xyTranslateAABBSize = Vec3{
			translatePlaneSideLength,
			translatePlaneSideLength,
			0.0f
		};
		static constexpr Vec3 zyTranslateAABBSize = Vec3{
			0.0f,
			translatePlaneSideLength,
			translatePlaneSideLength
		};
		static constexpr Vec3 xzTranslateAABBSize = Vec3{
			translatePlaneSideLength,
			0.0f,
			translatePlaneSideLength
		};

		// Scale arrow offsets
		static constexpr Vec3 yScaleOffset = Vec3{ -0.4f, 0.1f, 0.0f };
		static constexpr Vec3 xScaleOffset = Vec3{ 0.1f, -0.4f, 0.0f };
		static constexpr Vec3 zScaleOffset = Vec3{ -0.4f, -0.4f, -0.1f - translateArrowHalfLength };

		// Plane scale offsets
		static const Vec3 xyScaleOffset = xyTranslateOffset;
		static const Vec3 xzScaleOffset = xzTranslateOffset;
		static const Vec3 zyScaleOffset = zyTranslateOffset;

		// Arrow scale AABBs
		static constexpr Vec3 yScaleAABBSize = Vec3{
			translateArrowTipRadius,
			translateArrowTipLength + translateArrowHalfLength * 2.0f,
			translateArrowTipRadius
		};
		static constexpr Vec3 xScaleAABBSize = Vec3{
			translateArrowTipLength + translateArrowHalfLength * 2.0f,
			translateArrowTipRadius,
			translateArrowTipRadius
		};
		static constexpr Vec3 zScaleAABBSize = Vec3{
			translateArrowTipRadius,
			translateArrowTipRadius,
			translateArrowTipLength + translateArrowHalfLength * 2.0f
		};

		// Plane scale AABBs
		static constexpr Vec2 scalePlaneSize = translationPlaneSize;
		static constexpr Vec3 xyScaleAABBSize = xyTranslateAABBSize;
		static constexpr Vec3 zyScaleAABBSize = zyTranslateAABBSize;
		static constexpr Vec3 xzScaleAABBSize = xzTranslateAABBSize;

		static constexpr float rotationGizmoInnerRadius = 1.0f;
		static constexpr float rotationGizmoOuterRadius = 1.1f;

		static const auto translateGizmoOffsets = fixedSizeArray<Vec3, (uint8)GizmoSubComponent::Length>(
			Vec3{ 0.f, 0.f, 0.f },

			// Translate
			xTranslateOffset,
			yTranslateOffset,
			zTranslateOffset,
			xzTranslateOffset,
			zyTranslateOffset,
			xyTranslateOffset,

			// Rotate
			Vec3{ 0.f, 0.f, 0.f },
			Vec3{ 0.f, 0.f, 0.f },
			Vec3{ 0.f, 0.f, 0.f },

			// Scale
			xScaleOffset,
			yScaleOffset,
			zScaleOffset,
			xzScaleOffset,
			zyScaleOffset,
			xyScaleOffset
			);

		static const auto translateGizmoAABBs = fixedSizeArray<Vec3, (uint8)GizmoSubComponent::Length>(
			Vec3{ 0.f, 0.f, 0.f },

			// Translate
			xTranslateAABBSize,
			yTranslateAABBSize,
			zTranslateAABBSize,
			xzTranslateAABBSize,
			zyTranslateAABBSize,
			xyTranslateAABBSize,

			// Rotate
			Vec3{ 0.f, 0.f, 0.f },
			Vec3{ 0.f, 0.f, 0.f },
			Vec3{ 0.f, 0.f, 0.f },

			// Scale
			xScaleAABBSize,
			yScaleAABBSize,
			zScaleAABBSize,
			xzScaleAABBSize,
			zyScaleAABBSize,
			xyScaleAABBSize
			);

		static const auto rotationGizmosForwardVec = fixedSizeArray<Vec3, (uint8)GizmoSubComponent::Length>(
			Vec3{ 0.f, 0.f, 0.f },

			// Translate
			Vec3{ 0.f, 0.f, 0.f },
			Vec3{ 0.f, 0.f, 0.f },
			Vec3{ 0.f, 0.f, 0.f },
			Vec3{ 0.f, 0.f, 0.f },
			Vec3{ 0.f, 0.f, 0.f },
			Vec3{ 0.f, 0.f, 0.f },

			// Rotate
			Vector3::Right,
			Vector3::Up,
			Vector3::Back,

			// Scale
			Vec3{ 0.f, 0.f, 0.f },
			Vec3{ 0.f, 0.f, 0.f },
			Vec3{ 0.f, 0.f, 0.f },
			Vec3{ 0.f, 0.f, 0.f },
			Vec3{ 0.f, 0.f, 0.f },
			Vec3{ 0.f, 0.f, 0.f }
			);

		static const auto rotationGizmosUpVec = fixedSizeArray<Vec3, (uint8)GizmoSubComponent::Length>(
			Vec3{ 0.f, 0.f, 0.f },

			// Translate
			Vec3{ 0.f, 0.f, 0.f },
			Vec3{ 0.f, 0.f, 0.f },
			Vec3{ 0.f, 0.f, 0.f },
			Vec3{ 0.f, 0.f, 0.f },
			Vec3{ 0.f, 0.f, 0.f },
			Vec3{ 0.f, 0.f, 0.f },

			// Rotate
			Vector3::Up,
			Vector3::Right,
			Vector3::Up,

			// Scale
			Vec3{ 0.f, 0.f, 0.f },
			Vec3{ 0.f, 0.f, 0.f },
			Vec3{ 0.f, 0.f, 0.f },
			Vec3{ 0.f, 0.f, 0.f },
			Vec3{ 0.f, 0.f, 0.f },
			Vec3{ 0.f, 0.f, 0.f }
		);

		static constexpr int cameraOrientationFramebufferSize = 180;
		static Framebuffer cameraOrientationFramebuffer;
		static Texture cameraOrientationGizmoTexture;

		static constexpr uint64 NullGizmo = UINT64_MAX;

		// -------------- Internal Functions --------------
		static bool handleLinearGizmoMouseEvents(GizmoState* gizmo, const Vec3& gizmoPosition, Vec3* delta, GizmoSubComponent start, GizmoSubComponent end);
		static bool handleLinearGizmoKeyEvents(GizmoState* gizmo, const Vec3& gizmoPosition, Vec3* delta, int hotKeyStart);
		static void resetGlobalContext();

		static GizmoState* getGizmoByName(const char* name);
		static GizmoState* getGizmoById(uint64 id);
		static uint64 hashName(const char* name);
		static GizmoState* createDefaultGizmoState(const char* name, GizmoType type);
		static RaycastResult mouseHoveredRaycast(const Vec3& centerPosition, const Vec3& size);
		static bool isMouseHovered(const Vec3& centerPosition, const Vec3& size);
		static RaycastResult mouseHoveredRotationGizmoRaycast(const Vec3& center, const Vec3& forward, const Vec3& up, float innerRadius, float outerRadius);
		static bool isMouseHoveredRotationGizmo(const Vec3& center, const Vec3& forward, const Vec3& up, float innerRadius, float outerRadius);
		static Vec3 getMouseWorldPos3f(float zDepth);
		static Ray getMouseRay();
		static void handleActiveCheckRotationGizmo(GizmoState* gizmo, const Vec3& forward, const Vec3& up, float innerRadius, float outerRadius, float cameraZoom);
		static void handleActiveCheck(GizmoState* gizmo, const Vec3& offset, const Vec3& gizmoSize, float cameraZoom);
		static uint64 activeOrHoveredGizmo();
		static void renderGizmoArrow(const Vec3& gizmoPos, const Vec3& gizmoOffset, const Vec4* color, const Vec3& direction, const Vec3& normalVec);

		void init()
		{
			void* gMemory = g_memory_allocate(sizeof(GlobalContext));
			gGizmoManager = new(gMemory)GlobalContext();
			gGizmoManager->hoveredGizmo = NullGizmo;
			gGizmoManager->activeGizmo = NullGizmo;
			gGizmoManager->mouseWorldPos3f = Vec3{ 0.0f, 0.0f, 0.0f };
			gGizmoManager->mouseRay = Physics::createRay(Vec3{ 0, 0, 0 }, Vec3{ 1, 0, 0 });
			gGizmoManager->visualMode = GizmoType::Translation;

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

			float zDepth = Application::getEditorCamera()->focalDistance;
			if (activeOrHoveredGizmo() != NullGizmo)
			{
				zDepth = CMath::length(getGizmoById(activeOrHoveredGizmo())->position - Application::getEditorCamera()->position);
			}
			g->mouseWorldPos3f = getMouseWorldPos3f(zDepth);
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

		void changeVisualMode(GizmoType type)
		{
			gGizmoManager->visualMode = type;
		}

		const char* getVisualModeStr()
		{
			switch (gGizmoManager->visualMode)
			{
			case GizmoType::None:
				return "None";
			case GizmoType::Translation:
				return "Translation";
			case GizmoType::Rotation:
				return "Rotation";
			case GizmoType::Scaling:
				return "Scaling";
			}

			return "Undefined";
		}

		bool translateGizmo(const char* gizmoName, Vec3* position)
		{
			// Find or create the gizmo
			GizmoState* gizmo = getGizmoByName(gizmoName);
			if (!gizmo)
			{
				gizmo = createDefaultGizmoState(gizmoName, GizmoType::Translation);
				// Initialize to something sensible
				gizmo->positionMoveStart = *position;
			}
			gizmo->position = *position;

			Vec3 delta = Vec3{ 0.f, 0.f, 0.f };
			if (handleLinearGizmoKeyEvents(gizmo, *position, &delta, GLFW_KEY_G))
			{
				*position = gizmo->positionMoveStart + delta;
				return true;
			}

			// Only handle mouse events if the app is in translate mode
			if (gGizmoManager->visualMode == GizmoType::Translation)
			{
				if (handleLinearGizmoMouseEvents(gizmo, *position, &delta, GizmoSubComponent::XTranslate, GizmoSubComponent::XYPlaneTranslate))
				{
					*position = gizmo->positionMoveStart + delta;
					return true;
				}
			}

			return false;
		}

		bool rotateGizmo(const char* gizmoName, const Vec3& gizmoPosition, Vec3*)
		{
			// Find or create the gizmo
			GizmoState* gizmo = getGizmoByName(gizmoName);
			if (!gizmo)
			{
				gizmo = createDefaultGizmoState(gizmoName, GizmoType::Rotation);
				// Initialize to something sensible
				gizmo->positionMoveStart = gizmoPosition;
				//gizmo->scaleStart = *scale;
			}
			gizmo->position = gizmoPosition;

			Vec3 delta = Vec3{ 0.f, 0.f, 0.f };
			if (handleLinearGizmoKeyEvents(gizmo, gizmoPosition, &delta, GLFW_KEY_R))
			{
				// Invert the z-delta, it's weird for some reason
				// delta.z *= -1.0f;
				//*scale = gizmo->scaleStart + delta;
				return true;
			}

			// Only handle mouse events if the app is in rotation mode
			if (gGizmoManager->visualMode == GizmoType::Rotation)
			{
				if (handleLinearGizmoMouseEvents(gizmo, gizmoPosition, &delta, GizmoSubComponent::XRotate, GizmoSubComponent::ZRotate))
				{
					//*scale = gizmo->scaleStart + delta;
					//return true;
				}
			}

			return false;
		}

		bool scaleGizmo(const char* gizmoName, const Vec3& gizmoPosition, Vec3* scale)
		{
			// Find or create the gizmo
			GizmoState* gizmo = getGizmoByName(gizmoName);
			if (!gizmo)
			{
				gizmo = createDefaultGizmoState(gizmoName, GizmoType::Scaling);
				// Initialize to something sensible
				gizmo->positionMoveStart = gizmoPosition;
				gizmo->scaleStart = *scale;
			}
			gizmo->position = gizmoPosition;

			Vec3 delta = Vec3{ 0.f, 0.f, 0.f };
			if (handleLinearGizmoKeyEvents(gizmo, gizmoPosition, &delta, GLFW_KEY_S))
			{
				// Invert the z-delta, it's weird for some reason
				delta.z *= -1.0f;
				*scale = gizmo->scaleStart + delta;
				return true;
			}

			// Only handle mouse events if the app is in scale mode
			if (gGizmoManager->visualMode == GizmoType::Scaling)
			{
				if (handleLinearGizmoMouseEvents(gizmo, gizmoPosition, &delta, GizmoSubComponent::XScale, GizmoSubComponent::XYPlaneScale))
				{
					*scale = gizmo->scaleStart + delta;
					return true;
				}
			}

			return false;
		}

		// -------------- Internal Functions --------------
		static bool handleLinearGizmoMouseEvents(GizmoState* gizmo, const Vec3& gizmoPosition, Vec3* delta, GizmoSubComponent start, GizmoSubComponent end)
		{
			GlobalContext* g = gGizmoManager;
			const Camera* camera = Application::getEditorCamera();
			float zoom = camera->orthoZoomLevel;

			gizmo->shouldDraw = true;
			if (g->hoveredGizmo == NullGizmo && g->activeGizmo == NullGizmo)
			{
				// Check which sub-component is hovered
				// And only take the raycast hit for the closest component to the camera
				float minHitEntryDistance = FLT_MAX;
				for (uint8 i = (uint8)start; i <= (uint8)end; i++)
				{
					RaycastResult raycast = {};
					if (i >= (uint8)GizmoSubComponent::XRotate && i <= (uint8)GizmoSubComponent::ZRotate)
					{
						raycast = mouseHoveredRotationGizmoRaycast(
							getGizmoPos3D(gizmo->position, Vec3{0, 0, 0}, zoom),
							rotationGizmosForwardVec[i],
							rotationGizmosUpVec[i],
							rotationGizmoInnerRadius * zoom,
							rotationGizmoOuterRadius * zoom
						);
					}
					else
					{
						raycast = mouseHoveredRaycast(
							getGizmoPos3D(gizmo->position, translateGizmoOffsets[i], zoom),
							translateGizmoAABBs[i] * zoom
						);
					}

					if (raycast.hitEntry() && raycast.hitEntryDistance < minHitEntryDistance)
					{
						g->hoveredGizmo = gizmo->idHash;
						g->hotGizmoComponent = (GizmoSubComponent)i;
						minHitEntryDistance = raycast.hitEntryDistance;
					}
				}

				if (minHitEntryDistance == FLT_MAX)
				{
					g->hoveredGizmo = NullGizmo;
					g->hotGizmoComponent = GizmoSubComponent::None;
				}
			}

			if (g->hoveredGizmo == gizmo->idHash)
			{
				// Gizmo is "active" if the user is holding the mouse button down on the gizmo
				switch (g->hotGizmoComponent)
				{
				case GizmoSubComponent::XTranslate:
				case GizmoSubComponent::XScale:
					handleActiveCheck(gizmo, xTranslateOffset, xTranslateAABBSize, zoom);
					break;
				case GizmoSubComponent::YTranslate:
				case GizmoSubComponent::YScale:
					handleActiveCheck(gizmo, yTranslateOffset, yTranslateAABBSize, zoom);
					break;
				case GizmoSubComponent::ZTranslate:
				case GizmoSubComponent::ZScale:
					handleActiveCheck(gizmo, zTranslateOffset, zTranslateAABBSize, zoom);
					break;
				case GizmoSubComponent::XYPlaneTranslate:
				case GizmoSubComponent::XYPlaneScale:
					handleActiveCheck(gizmo, xyTranslateOffset, xyTranslateAABBSize, zoom);
					break;
				case GizmoSubComponent::XZPlaneTranslate:
				case GizmoSubComponent::XZPlaneScale:
					handleActiveCheck(gizmo, xzTranslateOffset, xzTranslateAABBSize, zoom);
					break;
				case GizmoSubComponent::YZPlaneTranslate:
				case GizmoSubComponent::YZPlaneScale:
					handleActiveCheck(gizmo, zyTranslateOffset, zyTranslateAABBSize, zoom);
					break;
				case GizmoSubComponent::XRotate:
					handleActiveCheckRotationGizmo(
						gizmo, 
						rotationGizmosForwardVec[(uint8)GizmoSubComponent::XRotate],
						rotationGizmosUpVec[(uint8)GizmoSubComponent::XRotate],
						rotationGizmoInnerRadius,
						rotationGizmoOuterRadius,
						zoom
						);
					break;
				case GizmoSubComponent::YRotate:
					handleActiveCheckRotationGizmo(
						gizmo,
						rotationGizmosForwardVec[(uint8)GizmoSubComponent::YRotate],
						rotationGizmosUpVec[(uint8)GizmoSubComponent::YRotate],
						rotationGizmoInnerRadius,
						rotationGizmoOuterRadius,
						zoom
					);
					break;
				case GizmoSubComponent::ZRotate:
					handleActiveCheckRotationGizmo(
						gizmo,
						rotationGizmosForwardVec[(uint8)GizmoSubComponent::ZRotate],
						rotationGizmosUpVec[(uint8)GizmoSubComponent::ZRotate],
						rotationGizmoInnerRadius,
						rotationGizmoOuterRadius,
						zoom
					);
					break;
				case GizmoSubComponent::None:
				case GizmoSubComponent::Length:
					break;
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
					resetGlobalContext();
					gizmo->moveMode = FollowMouseConstraint::None;
					*delta = Vec3{ 0.f, 0.f, 0.f };
					modified = true;
				}
				else
				{
					// Handle mouse dragging
					// Transform the position passed in to the current mouse position
					Vec3 newPos = Vec3{ 0.f, 0.f, 0.f };
					switch (g->hotGizmoComponent)
					{
					case GizmoSubComponent::YTranslate:
					case GizmoSubComponent::YScale:
						// NOTE: We subtract mouseDelta here to make sure it's offset properly from the mouse
						// and then when it gets added back in below it cancels the operation
						newPos = Vec3{ gizmoPosition.x - gizmo->mouseDelta.x, g->mouseWorldPos3f.y, gizmoPosition.z - gizmo->mouseDelta.z };
						break;
					case GizmoSubComponent::XTranslate:
					case GizmoSubComponent::XScale:
						// NOTE: Same note as above
						newPos = Vec3{ g->mouseWorldPos3f.x, gizmoPosition.y - gizmo->mouseDelta.y, gizmoPosition.z - gizmo->mouseDelta.z };
						break;
					case GizmoSubComponent::ZTranslate:
					case GizmoSubComponent::ZScale:
						// NOTE: Same note as above
						newPos = Vec3{ gizmoPosition.x - gizmo->mouseDelta.x, gizmoPosition.y - gizmo->mouseDelta.y, g->mouseWorldPos3f.z };
						break;
					case GizmoSubComponent::XYPlaneTranslate:
					case GizmoSubComponent::XYPlaneScale:
						newPos = Vec3{ g->mouseWorldPos3f.x, g->mouseWorldPos3f.y, gizmoPosition.z - gizmo->mouseDelta.z };
						break;
					case GizmoSubComponent::XZPlaneTranslate:
					case GizmoSubComponent::XZPlaneScale:
						newPos = Vec3{ g->mouseWorldPos3f.x, gizmoPosition.y - gizmo->mouseDelta.y, g->mouseWorldPos3f.z };
						break;
					case GizmoSubComponent::YZPlaneTranslate:
					case GizmoSubComponent::YZPlaneScale:
						newPos = Vec3{ gizmoPosition.x - gizmo->mouseDelta.x, g->mouseWorldPos3f.y, g->mouseWorldPos3f.z };
						break;
					case GizmoSubComponent::XRotate:
					case GizmoSubComponent::YRotate:
					case GizmoSubComponent::ZRotate:
					case GizmoSubComponent::None:
					case GizmoSubComponent::Length:
						break;
					}

					// Add back in mouse delta to make sure we maintain original distance
					// when we started dragging the object
					newPos += gizmo->mouseDelta;
					*delta = newPos - gizmo->positionMoveStart;
					modified = true;
				}
			}

			return modified;
		}

		static bool handleLinearGizmoKeyEvents(GizmoState* gizmo, const Vec3& gizmoPosition, Vec3* delta, int hotKeyStart)
		{
			GlobalContext* g = gGizmoManager;

			const Camera* camera = Application::getEditorCamera();
			if (Input::keyPressed(hotKeyStart))
			{
				// Use camera focal distance to move object
				Vec3 worldPosFocalDistance = getMouseWorldPos3f(camera->focalDistance);
				switch (gizmo->moveMode)
				{
				case FollowMouseConstraint::None:
					gizmo->positionMoveStart = gizmoPosition;
					gizmo->mouseDelta = gizmo->positionMoveStart - worldPosFocalDistance;
					gizmo->moveMode = FollowMouseConstraint::FreeMove;
					break;
				case FollowMouseConstraint::XOnly:
				case FollowMouseConstraint::YOnly:
				case FollowMouseConstraint::ZOnly:
				case FollowMouseConstraint::XYOnly:
				case FollowMouseConstraint::YZOnly:
				case FollowMouseConstraint::XZOnly:
				case FollowMouseConstraint::FreeMove:
					break;
				}
			}

			if (gizmo->moveMode != FollowMouseConstraint::None)
			{
				// Translate arrow hot-keys
				if (Input::keyPressed(GLFW_KEY_X))
				{
					switch (gizmo->moveMode)
					{
					case FollowMouseConstraint::YOnly:
					case FollowMouseConstraint::ZOnly:
					case FollowMouseConstraint::XYOnly:
					case FollowMouseConstraint::YZOnly:
					case FollowMouseConstraint::XZOnly:
					case FollowMouseConstraint::FreeMove:
						gizmo->moveMode = FollowMouseConstraint::XOnly;
						break;
					case FollowMouseConstraint::XOnly:
					case FollowMouseConstraint::None:
						break;
					}
					*delta = Vec3{ 0, 0, 0 };
				}
				else if (Input::keyPressed(GLFW_KEY_Y))
				{
					switch (gizmo->moveMode)
					{
					case FollowMouseConstraint::XOnly:
					case FollowMouseConstraint::ZOnly:
					case FollowMouseConstraint::XYOnly:
					case FollowMouseConstraint::YZOnly:
					case FollowMouseConstraint::XZOnly:
					case FollowMouseConstraint::FreeMove:
						gizmo->moveMode = FollowMouseConstraint::YOnly;
						break;
					case FollowMouseConstraint::YOnly:
					case FollowMouseConstraint::None:
						break;
					}
					*delta = Vec3{ 0, 0, 0 };
				}
				else if (Input::keyPressed(GLFW_KEY_Z))
				{
					switch (gizmo->moveMode)
					{
					case FollowMouseConstraint::XOnly:
					case FollowMouseConstraint::YOnly:
					case FollowMouseConstraint::XYOnly:
					case FollowMouseConstraint::YZOnly:
					case FollowMouseConstraint::XZOnly:
					case FollowMouseConstraint::FreeMove:
						gizmo->moveMode = FollowMouseConstraint::ZOnly;
						break;
					case FollowMouseConstraint::ZOnly:
					case FollowMouseConstraint::None:
						break;
					}
					*delta = Vec3{ 0, 0, 0 };
				}
				// Planar hot-keys
				else if (Input::keyPressed(GLFW_KEY_X, KeyMods::Shift))
				{
					switch (gizmo->moveMode)
					{
					case FollowMouseConstraint::XOnly:
					case FollowMouseConstraint::YOnly:
					case FollowMouseConstraint::ZOnly:
					case FollowMouseConstraint::XYOnly:
					case FollowMouseConstraint::XZOnly:
					case FollowMouseConstraint::FreeMove:
						gizmo->moveMode = FollowMouseConstraint::YZOnly;
						break;
					case FollowMouseConstraint::YZOnly:
					case FollowMouseConstraint::None:
						break;
					}
					*delta = Vec3{ 0, 0, 0 };
				}
				else if (Input::keyPressed(GLFW_KEY_Y, KeyMods::Shift))
				{
					switch (gizmo->moveMode)
					{
					case FollowMouseConstraint::XOnly:
					case FollowMouseConstraint::YOnly:
					case FollowMouseConstraint::ZOnly:
					case FollowMouseConstraint::XYOnly:
					case FollowMouseConstraint::YZOnly:
					case FollowMouseConstraint::FreeMove:
						gizmo->moveMode = FollowMouseConstraint::XZOnly;
						break;
					case FollowMouseConstraint::XZOnly:
					case FollowMouseConstraint::None:
						break;
					}
					*delta = Vec3{ 0, 0, 0 };
				}
				else if (Input::keyPressed(GLFW_KEY_Z, KeyMods::Shift))
				{
					switch (gizmo->moveMode)
					{
					case FollowMouseConstraint::XOnly:
					case FollowMouseConstraint::YOnly:
					case FollowMouseConstraint::ZOnly:
					case FollowMouseConstraint::YZOnly:
					case FollowMouseConstraint::XZOnly:
					case FollowMouseConstraint::FreeMove:
						gizmo->moveMode = FollowMouseConstraint::XYOnly;
						break;
					case FollowMouseConstraint::XYOnly:
					case FollowMouseConstraint::None:
						break;
					}
					*delta = Vec3{ 0, 0, 0 };
				}

				else if (Input::keyPressed(GLFW_KEY_ESCAPE))
				{
					// We can return immediately from a cancel operation
					resetGlobalContext();
					*delta = Vec3{ 0, 0, 0 };
					gizmo->moveMode = FollowMouseConstraint::None;
					return true;
				}
			}

			if (gizmo->moveMode != FollowMouseConstraint::None)
			{
				gizmo->shouldDraw = true;

				Vec3 unprojectedMousePos = getMouseWorldPos3f(camera->focalDistance);
				switch (gizmo->moveMode)
				{
				case FollowMouseConstraint::YOnly:
					delta->y = (unprojectedMousePos.y + gizmo->mouseDelta.y) - gizmo->positionMoveStart.y;
					break;
				case FollowMouseConstraint::XOnly:
					delta->x = (unprojectedMousePos.x + gizmo->mouseDelta.x) - gizmo->positionMoveStart.x;
					break;
				case FollowMouseConstraint::ZOnly:
					delta->z = (unprojectedMousePos.z + gizmo->mouseDelta.z) - gizmo->positionMoveStart.z;
					break;
				case FollowMouseConstraint::FreeMove:
					delta->x = (unprojectedMousePos.x + gizmo->mouseDelta.x) - gizmo->positionMoveStart.x;
					delta->y = (unprojectedMousePos.y + gizmo->mouseDelta.y) - gizmo->positionMoveStart.y;
					delta->z = (unprojectedMousePos.z + gizmo->mouseDelta.z) - gizmo->positionMoveStart.z;
					break;
				case FollowMouseConstraint::YZOnly:
					delta->y = (unprojectedMousePos.y + gizmo->mouseDelta.y) - gizmo->positionMoveStart.y;
					delta->z = (unprojectedMousePos.z + gizmo->mouseDelta.z) - gizmo->positionMoveStart.z;
					break;
				case FollowMouseConstraint::XYOnly:
					delta->x = (unprojectedMousePos.x + gizmo->mouseDelta.x) - gizmo->positionMoveStart.x;
					delta->y = (unprojectedMousePos.y + gizmo->mouseDelta.y) - gizmo->positionMoveStart.y;
					break;
				case FollowMouseConstraint::XZOnly:
					delta->x = (unprojectedMousePos.x + gizmo->mouseDelta.x) - gizmo->positionMoveStart.x;
					delta->z = (unprojectedMousePos.z + gizmo->mouseDelta.z) - gizmo->positionMoveStart.z;
					break;
				case FollowMouseConstraint::None:
					break;
				}

				g->activeGizmo = gizmo->idHash;
				if (Input::mouseClicked(MouseButton::Left))
				{
					gizmo->moveMode = FollowMouseConstraint::None;
					g->activeGizmo = NullGizmo;
				}

				// If we're in gizmo follow mouse mode, then every frame results in a change operation.
				// So, we return true here every time.
				return true;
			}

			return false;
		}

		static void resetGlobalContext()
		{
			gGizmoManager->hoveredGizmo = NullGizmo;
			gGizmoManager->activeGizmo = NullGizmo;
			gGizmoManager->mouseWorldPos3f = Vec3{ 0.0f, 0.0f, 0.0f };
			gGizmoManager->mouseRay = Physics::createRay(Vec3{ 0, 0, 0 }, Vec3{ 1, 0, 0 });
		}

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
			gizmoState->moveMode = FollowMouseConstraint::None;
			gizmoState->mouseDelta = Vec3{ 0.0f, 0.0f, 0.0f };

			GlobalContext* g = gGizmoManager;
			g->gizmos.push_back(gizmoState);
			g->gizmoById[hash] = (uint32)g->gizmos.size() - 1;

			return gizmoState;
		}

		static RaycastResult mouseHoveredRotationGizmoRaycast(const Vec3& center, const Vec3& forward, const Vec3& up, float innerRadius, float outerRadius)
		{
			const Ray& ray = gGizmoManager->mouseRay;
			Torus torus = Physics::createTorus(center, forward, up, innerRadius, outerRadius);
			RaycastResult res = Physics::rayIntersectsTorus(ray, torus);
			return res;
		}

		static bool isMouseHoveredRotationGizmo(const Vec3& center, const Vec3& forward, const Vec3& up, float innerRadius, float outerRadius)
		{
			return mouseHoveredRotationGizmoRaycast(center, forward, up, innerRadius, outerRadius).hit();
		}

		static RaycastResult mouseHoveredRaycast(const Vec3& centerPosition, const Vec3& size)
		{
			const Ray& ray = gGizmoManager->mouseRay;
			AABB aabb = Physics::createAABB(centerPosition, size);
			RaycastResult res = Physics::rayIntersectsAABB(ray, aabb);
			return res;
		}

		static bool isMouseHovered(const Vec3& centerPosition, const Vec3& size)
		{
			return mouseHoveredRaycast(centerPosition, size).hit();
		}

		static void handleActiveCheckRotationGizmo(GizmoState* gizmo, const Vec3& forward, const Vec3& up, float innerRadius, float outerRadius, float cameraZoom)
		{
			GlobalContext* g = gGizmoManager;
			Vec3 gizmoPos = getGizmoPos3D(gizmo->position, Vec3{0, 0, 0}, cameraZoom);
			if (Input::mouseDown(MouseButton::Left) && isMouseHoveredRotationGizmo(gizmoPos, forward, up, innerRadius * cameraZoom, outerRadius * cameraZoom))
			{
				gizmo->mouseDelta = gizmo->position - g->mouseWorldPos3f;
				g->activeGizmo = gizmo->idHash;
				g->hoveredGizmo = NullGizmo;
				gizmo->positionMoveStart = gizmo->position;
			}
			else if (!isMouseHoveredRotationGizmo(gizmoPos, forward, up, innerRadius * cameraZoom, outerRadius * cameraZoom))
			{
				g->hoveredGizmo = NullGizmo;
			}
		}

		static void handleActiveCheck(GizmoState* gizmo, const Vec3& offset, const Vec3& gizmoSize, float cameraZoom)
		{
			GlobalContext* g = gGizmoManager;
			Vec3 gizmoPos = getGizmoPos3D(gizmo->position, offset, cameraZoom);
			if (Input::mouseDown(MouseButton::Left) && isMouseHovered(gizmoPos, gizmoSize * cameraZoom))
			{
				gizmo->mouseDelta = gizmo->position - g->mouseWorldPos3f;
				g->activeGizmo = gizmo->idHash;
				g->hoveredGizmo = NullGizmo;
				gizmo->positionMoveStart = gizmo->position;
			}
			else if (!isMouseHovered(gizmoPos, gizmoSize * cameraZoom))
			{
				g->hoveredGizmo = NullGizmo;
			}
		}

		static uint64 activeOrHoveredGizmo()
		{
			GlobalContext* g = gGizmoManager;
			if (g->activeGizmo != NullGizmo)
			{
				return g->activeGizmo;
			}
			else if (g->hoveredGizmo != NullGizmo)
			{
				return g->hoveredGizmo;
			}

			return NullGizmo;
		}

		static Vec3 getMouseWorldPos3f(float zDepth)
		{
			Vec2 normalizedMousePos = EditorGui::mouseToNormalizedViewport();
			Camera* camera = Application::getEditorCamera();
			return camera->reverseProject(normalizedMousePos, zDepth);
		}

		static Ray getMouseRay()
		{
			Vec2 normalizedMousePos = EditorGui::mouseToNormalizedViewport();
			Camera* camera = Application::getEditorCamera();
			Vec3 origin = camera->reverseProject(normalizedMousePos, camera->nearFarRange.min);
			Vec3 end = camera->reverseProject(normalizedMousePos, camera->nearFarRange.max);
			return Physics::createRay(origin, end);
		}

		static void renderGizmoArrow(
			const Vec3& gizmoPos,
			const Vec3& gizmoOffset,
			const Vec4* color,
			const Vec3& direction,
			const Vec3& normalVec)
		{
			const Camera* camera = Application::getEditorCamera();
			Vec3 pos3D = getGizmoPos3D(gizmoPos, gizmoOffset, camera->orthoZoomLevel);

			g_logger_assert(color != nullptr, "How did this happen? Gizmo color was set to a nullptr for some reason.");

			float angleDp = CMath::abs(CMath::dot(camera->forward, direction));
			float alphaLevel = angleDp >= 0.9f ? (1.0f - angleDp) / 0.1f : 1.0f;
			alphaLevel = CMath::ease(alphaLevel, EaseType::Cubic, EaseDirection::InOut);
			Vec4 finalColor = { color->r, color->g, color->b, alphaLevel };

			Renderer::pushColor(finalColor);
			Vec3 halfSizeZoom = direction * GizmoManager::translateArrowHalfLength * camera->orthoZoomLevel;
			Renderer::drawCylinder(
				pos3D - halfSizeZoom,
				pos3D + halfSizeZoom,
				normalVec,
				GizmoManager::translateArrowTipRadius / 3.0f * camera->orthoZoomLevel
			);
			Vec3 baseCenter = pos3D + halfSizeZoom;
			Renderer::drawCone3D(
				baseCenter,
				direction,
				normalVec,
				GizmoManager::translateArrowTipRadius * camera->orthoZoomLevel,
				GizmoManager::translateArrowTipLength * camera->orthoZoomLevel
			);
			Renderer::popColor();
		}

		static void renderGizmoCubeArrow(
			const Vec3& gizmoPos,
			const Vec3& gizmoOffset,
			const Vec4* color,
			const Vec3& direction,
			const Vec3& normalVec)
		{
			const Camera* camera = Application::getEditorCamera();
			Vec3 pos3D = getGizmoPos3D(gizmoPos, gizmoOffset, camera->orthoZoomLevel);

			g_logger_assert(color != nullptr, "How did this happen? Gizmo color was set to a nullptr for some reason.");

			float angleDp = CMath::abs(CMath::dot(camera->forward, direction));
			float alphaLevel = angleDp >= 0.9f ? (1.0f - angleDp) / 0.1f : 1.0f;
			alphaLevel = CMath::ease(alphaLevel, EaseType::Cubic, EaseDirection::InOut);
			Vec4 finalColor = { color->r, color->g, color->b, alphaLevel };

			Renderer::pushColor(finalColor);
			Vec3 halfSizeZoom = direction * translateArrowHalfLength * camera->orthoZoomLevel;
			Renderer::drawCylinder(
				pos3D - halfSizeZoom,
				pos3D + halfSizeZoom,
				normalVec,
				GizmoManager::translateArrowTipRadius / 3.0f * camera->orthoZoomLevel
			);
			Vec3 baseCenter = pos3D + halfSizeZoom + (scaleCubeSideLength * direction * 0.5f);
			Renderer::drawCube3D(
				baseCenter,
				scaleCubeSize * camera->orthoZoomLevel,
				direction,
				normalVec
			);
			Renderer::popColor();
		}
	}

	// -------------- Gizmo Functions --------------
	static void renderXLine(const Vec3& leftGuideline, const Vec3& rightGuideline, float guidelineWidth)
	{
		Renderer::pushColor(Colors::AccentRed[4]);
		Renderer::drawCylinder(leftGuideline, rightGuideline, Vector3::Up, guidelineWidth);
		Renderer::popColor();
	}

	static void renderYLine(const Vec3& bottomGuideline, const Vec3& topGuideline, float guidelineWidth)
	{
		Renderer::pushColor(Colors::Primary[4]);
		Renderer::drawCylinder(bottomGuideline, topGuideline, Vector3::Right, guidelineWidth);
		Renderer::popColor();
	}

	static void renderZLine(const Vec3& backGuideline, const Vec3& frontGuideline, float guidelineWidth)
	{
		Renderer::pushColor(Colors::AccentGreen[4]);
		Renderer::drawCylinder(backGuideline, frontGuideline, Vector3::Up, guidelineWidth);
		Renderer::popColor();
	}

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
		if (moveMode != FollowMouseConstraint::None || idHash == g->activeGizmo)
		{
			const Vec2& cameraProjectionSize = projectionSize * zoom;
			Vec3 leftGuideline = this->positionMoveStart - Vec3{ cameraProjectionSize.x, 0.0f, 0.0f };
			Vec3 rightGuideline = this->positionMoveStart + Vec3{ cameraProjectionSize.x, 0.0f, 0.0f };
			Vec3 bottomGuideline = this->positionMoveStart - Vec3{ 0.0f, cameraProjectionSize.y, 0.0f };
			Vec3 topGuideline = this->positionMoveStart + Vec3{ 0.0f, cameraProjectionSize.y, 0.0f };
			Vec3 frontGuideline = this->positionMoveStart + Vec3{ 0.0f, 0.0f, camera->nearFarRange.max * zoom };
			Vec3 backGuideline = this->positionMoveStart - Vec3{ 0.0f, 0.0f, camera->nearFarRange.max * zoom };

			float guidelineWidth = 0.02f * zoom;

			// Render guidelines if we're in move mode
			switch (moveMode)
			{
			case FollowMouseConstraint::XOnly:
				renderXLine(leftGuideline, rightGuideline, guidelineWidth);
				break;
			case FollowMouseConstraint::YOnly:
				renderYLine(bottomGuideline, topGuideline, guidelineWidth);
				break;
			case FollowMouseConstraint::ZOnly:
				renderZLine(backGuideline, frontGuideline, guidelineWidth);
				break;

			case FollowMouseConstraint::YZOnly:
				renderYLine(bottomGuideline, topGuideline, guidelineWidth);
				renderZLine(backGuideline, frontGuideline, guidelineWidth);
				break;
			case FollowMouseConstraint::XZOnly:
				renderXLine(leftGuideline, rightGuideline, guidelineWidth);
				renderZLine(backGuideline, frontGuideline, guidelineWidth);
				break;
			case FollowMouseConstraint::XYOnly:
				renderXLine(leftGuideline, rightGuideline, guidelineWidth);
				renderYLine(bottomGuideline, topGuideline, guidelineWidth);
				break;

			case FollowMouseConstraint::FreeMove:
			case FollowMouseConstraint::None:
				break;
			}

			// Render guidelines if we're in translate mode
			if (moveMode == FollowMouseConstraint::None)
			{
				switch (g->hotGizmoComponent)
				{
				case GizmoSubComponent::XTranslate:
				case GizmoSubComponent::XScale:
					renderXLine(leftGuideline, rightGuideline, guidelineWidth);
					break;
				case GizmoSubComponent::YTranslate:
				case GizmoSubComponent::YScale:
					renderYLine(bottomGuideline, topGuideline, guidelineWidth);
					break;
				case GizmoSubComponent::ZTranslate:
				case GizmoSubComponent::ZScale:
					renderZLine(backGuideline, frontGuideline, guidelineWidth);
					break;
				case GizmoSubComponent::XYPlaneTranslate:
				case GizmoSubComponent::XYPlaneScale:
					renderXLine(leftGuideline, rightGuideline, guidelineWidth);
					renderYLine(bottomGuideline, topGuideline, guidelineWidth);
					break;
				case GizmoSubComponent::XZPlaneTranslate:
				case GizmoSubComponent::XZPlaneScale:
					renderXLine(leftGuideline, rightGuideline, guidelineWidth);
					renderZLine(backGuideline, frontGuideline, guidelineWidth);
					break;
				case GizmoSubComponent::YZPlaneTranslate:
				case GizmoSubComponent::YZPlaneScale:
					renderYLine(bottomGuideline, topGuideline, guidelineWidth);
					renderZLine(backGuideline, frontGuideline, guidelineWidth);
					break;
				case GizmoSubComponent::XRotate:
				case GizmoSubComponent::YRotate:
				case GizmoSubComponent::ZRotate:
				case GizmoSubComponent::None:
				case GizmoSubComponent::Length:
					break;
				}
			}

			return;
		}

		bool isScale = g->visualMode == GizmoType::Scaling;
		bool isRotation = g->visualMode == GizmoType::Rotation;

		// Render the Gizmo planar movement sub-components
		// Render XY Plane
		if (!isRotation)
		{
			int colorIndex = 4;
			if (g->hotGizmoComponent == GizmoSubComponent::XYPlaneScale ||
				g->hotGizmoComponent == GizmoSubComponent::XYPlaneTranslate)
			{
				if (idHash == g->activeGizmo)
				{
					colorIndex = 6;
				}
				else if (idHash == g->hoveredGizmo)
				{
					colorIndex = 5;
				}
			}

			Vec3 xyPlanePos = getGizmoPos3D(this->position, GizmoManager::xyTranslateOffset, camera->orthoZoomLevel);
			Renderer::pushColor(Colors::AccentRed[colorIndex]);
			Renderer::drawFilledQuad3D(
				xyPlanePos,
				GizmoManager::translationPlaneSize * zoom,
				Vector3::Forward,
				Vector3::Up
			);
			Renderer::popColor();
		}

		// Render XZ Plane
		if (!isRotation)
		{
			int colorIndex = 4;
			if (g->hotGizmoComponent == GizmoSubComponent::XZPlaneScale ||
				g->hotGizmoComponent == GizmoSubComponent::XZPlaneTranslate)
			{
				if (idHash == g->activeGizmo)
				{
					colorIndex = 6;
				}
				else if (idHash == g->hoveredGizmo)
				{
					colorIndex = 5;
				}
			}

			Vec3 xzPlanePos = getGizmoPos3D(this->position, GizmoManager::xzTranslateOffset, camera->orthoZoomLevel);
			Renderer::pushColor(Colors::Primary[colorIndex]);
			Renderer::drawFilledQuad3D(
				xzPlanePos,
				GizmoManager::translationPlaneSize * zoom,
				Vector3::Up,
				Vector3::Right
			);
			Renderer::popColor();

		}

		// Render ZY Plane
		if (!isRotation)
		{
			int colorIndex = 4;
			if (g->hotGizmoComponent == GizmoSubComponent::YZPlaneScale ||
				g->hotGizmoComponent == GizmoSubComponent::YZPlaneTranslate)
			{
				if (idHash == g->activeGizmo)
				{
					colorIndex = 6;
				}
				else if (idHash == g->hoveredGizmo)
				{
					colorIndex = 5;
				}
			}

			Vec3 zyPlanePos = getGizmoPos3D(this->position, GizmoManager::zyTranslateOffset, camera->orthoZoomLevel);
			Renderer::pushColor(Colors::AccentGreen[colorIndex]);
			Renderer::drawFilledQuad3D(
				zyPlanePos,
				GizmoManager::translationPlaneSize * zoom,
				Vector3::Right,
				Vector3::Up
			);
			Renderer::popColor();
		}

		// Render the X-Arrow translate gizmo subcomponent
		{
			const Vec4* color = &Colors::AccentRed[4];
			if (g->hotGizmoComponent == GizmoSubComponent::XScale ||
				g->hotGizmoComponent == GizmoSubComponent::XTranslate ||
				g->hotGizmoComponent == GizmoSubComponent::XRotate)
			{
				if (idHash == g->activeGizmo)
				{
					color = &Colors::AccentRed[6];
				}
				else if (idHash == g->hoveredGizmo)
				{
					color = &Colors::AccentRed[5];
				}
			}

			if (isScale)
			{
				GizmoManager::renderGizmoCubeArrow(
					this->position,
					GizmoManager::xScaleOffset,
					color,
					Vector3::Right,
					Vector3::Up
				);
			}
			else if (isRotation)
			{
				Renderer::pushColor(*color);
				Renderer::drawDonut(
					this->position,
					GizmoManager::rotationGizmoInnerRadius * zoom,
					GizmoManager::rotationGizmoOuterRadius * zoom,
					GizmoManager::rotationGizmosForwardVec[(uint8)GizmoSubComponent::XRotate],
					GizmoManager::rotationGizmosUpVec[(uint8)GizmoSubComponent::XRotate]
				);
				Renderer::popColor();
			}
			else
			{
				GizmoManager::renderGizmoArrow(
					this->position,
					GizmoManager::xTranslateOffset,
					color,
					Vector3::Right,
					Vector3::Up
				);
			}
		}

		// Render the Y-Arrow gizmo translate subcomponent
		{
			const Vec4* color = &Colors::Primary[4];
			if (g->hotGizmoComponent == GizmoSubComponent::YScale ||
				g->hotGizmoComponent == GizmoSubComponent::YTranslate ||
				g->hotGizmoComponent == GizmoSubComponent::YRotate)
			{
				if (idHash == g->activeGizmo)
				{
					color = &Colors::Primary[6];
				}
				else if (idHash == g->hoveredGizmo)
				{
					color = &Colors::Primary[5];
				}
			}

			if (isScale)
			{
				GizmoManager::renderGizmoCubeArrow(
					this->position,
					GizmoManager::yScaleOffset,
					color,
					Vector3::Up,
					Vector3::Right
				);
			}
			else if (isRotation)
			{
				Renderer::pushColor(*color);
				Renderer::drawDonut(
					this->position,
					GizmoManager::rotationGizmoInnerRadius * zoom,
					GizmoManager::rotationGizmoOuterRadius * zoom,
					GizmoManager::rotationGizmosForwardVec[(uint8)GizmoSubComponent::YRotate],
					GizmoManager::rotationGizmosUpVec[(uint8)GizmoSubComponent::YRotate]
				);
				Renderer::popColor();
			}
			else
			{
				GizmoManager::renderGizmoArrow(
					this->position,
					GizmoManager::yTranslateOffset,
					color,
					Vector3::Up,
					Vector3::Right
				);
			}
		}

		// Render the Z-Arrow gizmo translate subcomponent
		{
			const Vec4* color = &Colors::AccentGreen[4];;
			if (g->hotGizmoComponent == GizmoSubComponent::ZScale ||
				g->hotGizmoComponent == GizmoSubComponent::ZTranslate ||
				g->hotGizmoComponent == GizmoSubComponent::ZRotate)
			{
				if (idHash == g->activeGizmo)
				{
					color = &Colors::AccentGreen[6];
				}
				else if (idHash == g->hoveredGizmo)
				{
					color = &Colors::AccentGreen[5];
				}
			}

			if (isScale)
			{
				GizmoManager::renderGizmoCubeArrow(
					this->position,
					GizmoManager::zScaleOffset,
					color,
					Vector3::Back,
					Vector3::Up
				);
			}
			else if (isRotation)
			{
				Renderer::pushColor(*color);
				Renderer::drawDonut(
					this->position,
					GizmoManager::rotationGizmoInnerRadius* zoom,
					GizmoManager::rotationGizmoOuterRadius* zoom,
					GizmoManager::rotationGizmosForwardVec[(uint8)GizmoSubComponent::ZRotate],
					GizmoManager::rotationGizmosUpVec[(uint8)GizmoSubComponent::ZRotate]
				);
				Renderer::popColor();
			}
			else
			{
				GizmoManager::renderGizmoArrow(
					this->position,
					GizmoManager::zTranslateOffset,
					color,
					Vector3::Back,
					Vector3::Up
				);
			}
		}
	}

	// -------------- Interal Gizmo Functions --------------
}