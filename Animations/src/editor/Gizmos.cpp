#include "editor/Gizmos.h"
#include "editor/EditorGui.h"
#include "animation/AnimationManager.h"
#include "renderer/Framebuffer.h"
#include "renderer/Renderer.h"
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
		static constexpr Vec2 defaultVerticalMoveSize = Vec2{ 0.12f, 0.6f };
		static constexpr Vec2 defaultHorizontalMoveSize = Vec2{ 0.6f, 0.12f };

		static constexpr Vec2 defaultVerticalMoveOffset = Vec2{ -0.4f, 0.1f };
		static constexpr Vec2 defaultHorizontalMoveOffset = Vec2{ 0.1f, -0.4f };

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
			EditorGui::onGizmo(am);
		}

		void render(const Framebuffer& framebuffer)
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
			g->lastActiveGizmo = g->activeGizmo;
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
					handleActiveCheck(gizmo, Vec2{0, 0}, defaultFreeMoveSize);
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
					OrthoCamera* camera = Application::getCamera();
					newPos = camera->reverseProject(newPos);
					if (g->hotGizmoVariant == GizmoVariant::Free)
					{
						// Don't modify the z-coord since this is a 2D gizmo
						*position = Vec3{ newPos.x, newPos.y, position->z };
					}
					else if (g->hotGizmoVariant == GizmoVariant::Vertical)
					{
						*position = Vec3{ position->x, newPos.y, position->z };
					}
					else if (g->hotGizmoVariant == GizmoVariant::Horizontal)
					{
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
			g->gizmoById[hash] = g->gizmos.size() - 1;

			return gizmoState;
		}

		static bool isMouseHovered(const Vec2& centerPosition, const Vec2& size)
		{
			Vec2 normalizedMousePos = EditorGui::mouseToNormalizedViewport();
			OrthoCamera* camera = Application::getCamera();
			Vec2 worldCoords = camera->reverseProject(normalizedMousePos);
			Vec2 bottomLeft = centerPosition - (size / 2.0f);

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
			if ((idHash != g->hoveredGizmo && idHash != g->activeGizmo) || g->hotGizmoVariant != GizmoVariant::Horizontal)
			{
				Renderer::pushColor(Colors::Primary[4]);
				Renderer::drawFilledQuad(pos, GizmoManager::defaultHorizontalMoveSize);
				Renderer::popColor();
			}
			else if (idHash == g->hoveredGizmo)
			{
				Renderer::pushColor(Colors::Primary[5]);
				Renderer::drawFilledQuad(pos, GizmoManager::defaultHorizontalMoveSize);
				Renderer::popColor();
			}
			else if (idHash == g->activeGizmo)
			{
				Renderer::pushColor(Colors::Primary[6]);
				Renderer::drawFilledQuad(pos, GizmoManager::defaultHorizontalMoveSize);
				Renderer::popColor();
			}
		}

		if ((uint8)variant & (uint8)GizmoVariant::Vertical)
		{
			Vec2 pos = CMath::vector2From3(this->position) + GizmoManager::defaultVerticalMoveOffset;
			if ((idHash != g->hoveredGizmo && idHash != g->activeGizmo) || g->hotGizmoVariant != GizmoVariant::Vertical)
			{
				Renderer::pushColor(Colors::AccentGreen[4]);
				Renderer::drawFilledQuad(pos, GizmoManager::defaultVerticalMoveSize);
				Renderer::popColor();
			}
			else if (idHash == g->hoveredGizmo)
			{
				Renderer::pushColor(Colors::AccentGreen[5]);
				Renderer::drawFilledQuad(pos, GizmoManager::defaultVerticalMoveSize);
				Renderer::popColor();
			}
			else if (idHash == g->activeGizmo)
			{
				Renderer::pushColor(Colors::AccentGreen[6]);
				Renderer::drawFilledQuad(pos, GizmoManager::defaultVerticalMoveSize);
				Renderer::popColor();
			}
		}
	}

	// -------------- Interal Gizmo Functions --------------
}