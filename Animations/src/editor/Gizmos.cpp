#include "editor/Gizmos.h"
#include "editor/EditorGui.h"
#include "animation/AnimationManager.h"
#include "renderer/Framebuffer.h"
#include "renderer/Renderer.h"
#include "renderer/OrthoCamera.h"
#include "renderer/PerspectiveCamera.h"
#include "core/Colors.h"
#include "core/Input.h"
#include "core/Application.h"

namespace MathAnim
{
	// -------------- Internal Structures --------------
	struct GizmoState
	{
		GizmoType gizmoType;
		GizmoVariant variant;
		uint64 idHash;
		Vec3 position;
		bool wasUpdated;

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
		Vec2 dragStartScreenCoords;
	};

	namespace GizmoManager
	{
		// -------------- Internal Variables --------------
		static GlobalContext* gGizmoManager;
		static constexpr Vec2 defaultFreeMoveSize = Vec2{ 0.45f, 0.45f };
		static constexpr Vec2 defaultVerticalMoveSize = Vec2{ 0.08f, 0.6f };
		static constexpr Vec2 defaultHorizontalMoveSize = Vec2{ 0.6f, 0.08f };

		static constexpr Vec2 defaultVerticalMoveOffset = Vec2{ -0.4f, 0.1f };
		static constexpr Vec2 defaultHorizontalMoveOffset = Vec2{ 0.1f, -0.4f };
		static constexpr float defaultArrowTipHeight = 0.25f;
		static constexpr float defaultArrowTipHalfWidth = 0.1f;

		static constexpr uint64 NullGizmo = UINT64_MAX;

		// -------------- Internal Functions --------------
		static GizmoState* getGizmoByName(const char* name);
		static GizmoState* getGizmoById(uint64 id);
		static uint64 hashName(const char* name);
		static GizmoState* createDefaultGizmoState(const char* name, GizmoType type);
		static bool isMouseHovered(const Vec2& centerPosition, const Vec2& size);
		static void handleActiveCheck(GizmoState* gizmo, const Vec2& offset, const Vec2& gizmoSize);

		void init()
		{
			void* gMemory = g_memory_allocate(sizeof(GlobalContext));
			gGizmoManager = new(gMemory)GlobalContext();
			gGizmoManager->hoveredGizmo = UINT64_MAX;
			gGizmoManager->activeGizmo = UINT64_MAX;
		}

		void update(AnimationManagerData* am)
		{
			GlobalContext* g = gGizmoManager;
			g->lastActiveGizmo = g->activeGizmo;

			EditorGui::onGizmo(am);
		}

		void render(AnimationManagerData* am)
		{
			// Render call stuff
			GlobalContext* g = gGizmoManager;
			for (auto iter : g->gizmos)
			{
				if (iter->wasUpdated)
				{
					// Draw the gizmo
					iter->render();
				}
			}

			// End Frame stuff
			for (auto iter : g->gizmos)
			{
				iter->wasUpdated = false;
			}

			const AnimObject* orthoCameraObj = AnimationManager::getActiveOrthoCamera(am);
			if (orthoCameraObj)
			{
				if (orthoCameraObj->as.camera.is2D)
				{
					const OrthoCamera& orthoCamera = orthoCameraObj->as.camera.camera2D;
					// Draw camera outlines
					Renderer::pushStrokeWidth(0.05f);
					Renderer::pushColor(Colors::Neutral[0]);
					Renderer::drawSquare(orthoCamera.position - orthoCamera.projectionSize / 2.0f, orthoCamera.projectionSize);
					Renderer::popColor();
					Renderer::popStrokeWidth();
				}
			}
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
		}

		bool anyGizmoActive()
		{
			// Return whether any gizmo was active this frame or last frame 
			// so that it reflects the state of queries accurately
			return gGizmoManager->activeGizmo != NullGizmo || gGizmoManager->lastActiveGizmo != NullGizmo;
		}

		bool translateGizmo(const char* gizmoName, Vec3* position, GizmoVariant variant)
		{
			// Find or create the gizmo
			GizmoState* gizmo = getGizmoByName(gizmoName);
			if (!gizmo)
			{
				gizmo = createDefaultGizmoState(gizmoName, GizmoType::Translation);
			}
			gizmo->wasUpdated = true;
			gizmo->position = *position;
			gizmo->variant = variant;

			GlobalContext* g = gGizmoManager;
			if (g->hoveredGizmo == NullGizmo && g->activeGizmo == NullGizmo)
			{
				// Check if free move variant is hovered
				if (isMouseHovered(CMath::vector2From3(gizmo->position), defaultFreeMoveSize))
				{
					g->hoveredGizmo = gizmo->idHash;
					g->hotGizmoVariant = GizmoVariant::Free;
				}
				else if (isMouseHovered(CMath::vector2From3(gizmo->position) + defaultVerticalMoveOffset, defaultVerticalMoveSize))
				{
					g->hoveredGizmo = gizmo->idHash;
					g->hotGizmoVariant = GizmoVariant::Vertical;
				}
				else if (isMouseHovered(CMath::vector2From3(gizmo->position) + defaultHorizontalMoveOffset, defaultHorizontalMoveSize))
				{
					g->hoveredGizmo = gizmo->idHash;
					g->hotGizmoVariant = GizmoVariant::Horizontal;
				}
			}

			if (g->hoveredGizmo == gizmo->idHash)
			{
				// Gizmo is "active" if the user is holding the mouse button down on the gizmo
				// Check if free move variant is changed to active
				if (g->hotGizmoVariant == GizmoVariant::Free)
				{
					handleActiveCheck(gizmo, Vec2{ 0, 0 }, defaultFreeMoveSize);
				}
				// Check if vertical move is changed to active
				else if (g->hotGizmoVariant == GizmoVariant::Vertical)
				{
					handleActiveCheck(gizmo, defaultVerticalMoveOffset, defaultVerticalMoveSize);
				}
				// Check if horizontal move is changed to active
				if (g->hotGizmoVariant == GizmoVariant::Horizontal)
				{
					handleActiveCheck(gizmo, defaultHorizontalMoveOffset, defaultHorizontalMoveSize);
				}
			}

			bool modified = false;
			if (g->activeGizmo == gizmo->idHash)
			{
				if (Input::mouseUp(MouseButton::Left))
				{
					g->activeGizmo = NullGizmo;
				}
				else
				{
					// Handle mouse dragging
					// Transform the position passed in to the current mouse position
					Vec2 newPos = EditorGui::mouseToNormalizedViewport();
					OrthoCamera* camera = Application::getEditorCamera();
					newPos = camera->reverseProject(newPos);
					if (g->hotGizmoVariant == GizmoVariant::Free)
					{
						// Don't modify the z-coord since this is a 2D gizmo
						newPos.y -= defaultFreeMoveSize.y;
						*position = Vec3{ newPos.x, newPos.y, position->z };
					}
					else if (g->hotGizmoVariant == GizmoVariant::Vertical)
					{
						newPos.y -= defaultVerticalMoveSize.y;
						*position = Vec3{ position->x, newPos.y, position->z };
					}
					else if (g->hotGizmoVariant == GizmoVariant::Horizontal)
					{
						newPos.y -= defaultHorizontalMoveSize.y;
						*position = Vec3{ newPos.x, position->y, position->z };
					}
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
			gizmoState->wasUpdated = false;

			GlobalContext* g = gGizmoManager;
			g->gizmos.push_back(gizmoState);
			g->gizmoById[hash] = (uint32)g->gizmos.size() - 1;

			return gizmoState;
		}

		static bool isMouseHovered(const Vec2& centerPosition, const Vec2& size)
		{
			Vec2 normalizedMousePos = EditorGui::mouseToNormalizedViewport();
			OrthoCamera* camera = Application::getEditorCamera();
			Vec2 worldCoords = camera->reverseProject(normalizedMousePos);
			Vec2 bottomLeft = centerPosition - (Vec2{ size.x, size.y } / 2.0f);

			return worldCoords.x >= bottomLeft.x && worldCoords.x <= bottomLeft.x + size.x &&
				worldCoords.y >= bottomLeft.y && worldCoords.y <= bottomLeft.y + size.y;
		}

		static void handleActiveCheck(GizmoState* gizmo, const Vec2& offset, const Vec2& gizmoSize)
		{
			GlobalContext* g = gGizmoManager;
			if (Input::mouseDown(MouseButton::Left) && isMouseHovered(CMath::vector2From3(gizmo->position) + offset, gizmoSize))
			{
				g->activeGizmo = gizmo->idHash;
				g->hoveredGizmo = NullGizmo;
			}
			else if (!isMouseHovered(CMath::vector2From3(gizmo->position) + offset, gizmoSize))
			{
				g->hoveredGizmo = NullGizmo;
			}
		}
	}

	// -------------- Interal Gizmo Functions --------------

	// -------------- Gizmo Functions --------------
	void GizmoState::render()
	{
		GlobalContext* g = GizmoManager::gGizmoManager;

		// Render the gizmo shapes
		if ((uint8)variant & (uint8)GizmoVariant::Free)
		{
			if ((idHash != g->hoveredGizmo && idHash != g->activeGizmo) || g->hotGizmoVariant != GizmoVariant::Free)
			{
				Renderer::pushColor(Colors::AccentRed[4]);
				Renderer::drawFilledQuad(CMath::vector2From3(this->position), GizmoManager::defaultFreeMoveSize);
				Renderer::popColor();
			}
			else if (idHash == g->hoveredGizmo)
			{
				Renderer::pushColor(Colors::AccentRed[5]);
				Renderer::drawFilledQuad(CMath::vector2From3(this->position), GizmoManager::defaultFreeMoveSize);
				Renderer::popColor();
			}
			else if (idHash == g->activeGizmo)
			{
				Renderer::pushColor(Colors::AccentRed[6]);
				Renderer::drawFilledQuad(CMath::vector2From3(this->position), GizmoManager::defaultFreeMoveSize);
				Renderer::popColor();
			}
		}

		if ((uint8)variant & (uint8)GizmoVariant::Horizontal)
		{
			Vec2 pos = CMath::vector2From3(this->position) + GizmoManager::defaultHorizontalMoveOffset;
			const Vec4* color = nullptr;
			if ((idHash != g->hoveredGizmo && idHash != g->activeGizmo) || g->hotGizmoVariant != GizmoVariant::Horizontal)
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
				Renderer::drawFilledQuad(pos, GizmoManager::defaultHorizontalMoveSize);
				float stemHalfSize = GizmoManager::defaultHorizontalMoveSize.x / 2.0f;
				Vec2 triP0 = pos + Vec2{ stemHalfSize, GizmoManager::defaultArrowTipHalfWidth };
				Vec2 triP1 = pos + Vec2{ stemHalfSize + GizmoManager::defaultArrowTipHeight, 0.0f };
				Vec2 triP2 = pos + Vec2{ stemHalfSize, -GizmoManager::defaultArrowTipHalfWidth };
				Renderer::drawFilledTri(triP0, triP1, triP2);
				Renderer::popColor();
			}
		}

		if ((uint8)variant & (uint8)GizmoVariant::Vertical)
		{
			Vec2 pos = CMath::vector2From3(this->position) + GizmoManager::defaultVerticalMoveOffset;
			const Vec4* color = nullptr;
			if ((idHash != g->hoveredGizmo && idHash != g->activeGizmo) || g->hotGizmoVariant != GizmoVariant::Vertical)
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
				Renderer::drawFilledQuad(pos, GizmoManager::defaultVerticalMoveSize);
				float stemHalfSize = GizmoManager::defaultVerticalMoveSize.y / 2.0f;
				Vec2 triP0 = pos + Vec2{ -GizmoManager::defaultArrowTipHalfWidth, stemHalfSize };
				Vec2 triP1 = pos + Vec2{ 0.0f, stemHalfSize + GizmoManager::defaultArrowTipHeight };
				Vec2 triP2 = pos + Vec2{ GizmoManager::defaultArrowTipHalfWidth, stemHalfSize };
				Renderer::drawFilledTri(triP0, triP1, triP2);
				Renderer::popColor();
			}
		}
	}

	// -------------- Interal Gizmo Functions --------------
}