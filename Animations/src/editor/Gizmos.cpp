#include "editor/Gizmos.h"
#include "editor/EditorGui.h"
#include "animation/AnimationManager.h"
#include "renderer/Framebuffer.h"

namespace MathAnim
{
	// -------------- Internal Structures --------------
	enum class GizmoStatus : uint8
	{
		None,
		Hovered,
		Active
	};

	enum class GizmoType : uint8
	{
		None,
		FreeMove,
		VerticalMove,
		HorizontalMove
	};

	struct GizmoState
	{
		GizmoStatus status;
		GizmoType gizmoType;
		uint64 idHash;
		bool wasUpdated;

		void render();
	};

	struct GlobalContext
	{
		// Maps from idHash to index in gizmos vector
		std::unordered_map<uint64, uint32> gizmoById;
		std::vector<GizmoState*> gizmos;
	};

	namespace GizmoManager
	{
		// -------------- Internal Variables --------------
		static GlobalContext* gGizmoManager;

		// -------------- Internal Functions --------------
		static GizmoState* getGizmoByName(const char* name);
		static GizmoState* getGizmoById(uint64 id);
		static uint64 hashName(const char* name);
		static GizmoState* createDefaultGizmoState(const char* name, GizmoType type);

		void init()
		{
			void* gMemory = g_memory_allocate(sizeof(GlobalContext));
			gGizmoManager = new(gMemory)GlobalContext();
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
		}

		void free()
		{
			if (gGizmoManager)
			{
				gGizmoManager->~GlobalContext();
				g_memory_free(gGizmoManager);
				gGizmoManager = nullptr;
			}
		}

		void freeMoveGizmo(const char* gizmoName, Vec3& position, FreeMoveVariant variant)
		{
			// Find or create the gizmo
			GizmoState* gizmo = getGizmoByName(gizmoName);
			if (!gizmo)
			{
				gizmo = createDefaultGizmoState(gizmoName, GizmoType::FreeMove);
			}
			gizmo->wasUpdated = true;

			if (gizmo->status == GizmoStatus::Hovered)
			{
				
			}
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
			gizmoState->status = GizmoStatus::None;
			gizmoState->wasUpdated = false;

			GlobalContext* g = gGizmoManager;
			g->gizmos.push_back(gizmoState);
			g->gizmoById[hash] = g->gizmos.size() - 1;

			return gizmoState;
		}
	}

	// -------------- Interal Gizmo Functions --------------
	static void renderFreeMove();
	static void renderVerticalMove();
	static void renderHorizontalMove();

	// -------------- Gizmo Functions --------------
	void GizmoState::render()
	{
		switch (gizmoType)
		{
		case GizmoType::FreeMove:
			renderFreeMove();
			break;
		case GizmoType::HorizontalMove:
			renderHorizontalMove();
			break;
		case GizmoType::VerticalMove:
			renderVerticalMove();
			break;
		}
	}

	// -------------- Interal Gizmo Functions --------------
	static void renderFreeMove()
	{

	}

	static void renderVerticalMove()
	{

	}

	static void renderHorizontalMove()
	{

	}
}