#ifndef MATH_ANIM_IMGUI_LAYER_H
#define MATH_ANIM_IMGUI_LAYER_H
#include "core.h"

struct ImFont;

namespace MathAnim
{
	struct Window;
	union Vec2;

	enum class ImGuiLayerFlags : uint8
	{
		None = 0,
		EnableDocking =    1 << 0,
		EnableViewports =  1 << 1,
		SaveIniSettings =  1 << 2
	};

	enum class SaveEditorLayoutError : uint8
	{
		None = 0,
		ReservedLayoutName,
		FailedToSaveImGuiIni,
		FailedToConvertIniToJson
	};

	namespace ImGuiLayer
	{
		void init(
			const Window& window, 
			const char* jsonLayoutFile = nullptr,
			ImGuiLayerFlags flags = (ImGuiLayerFlags)(
				(uint8)ImGuiLayerFlags::EnableDocking | 
				(uint8)ImGuiLayerFlags::EnableViewports |
				(uint8)ImGuiLayerFlags::SaveIniSettings
				)
		);

		void beginFrame();
		void endFrame();

		void keyEvent();
		void mouseEvent();

		void free();
		SaveEditorLayoutError saveEditorLayout(const char* name);
		void loadEditorLayout(const std::filesystem::path& layoutPath, const Vec2& targetResolution);

		ImFont* getDefaultFont();
		ImFont* getMediumFont();
		ImFont* getMonoFont();

		ImFont* getLargeSolidIconFont();
		ImFont* getMediumSolidIconFont();
		ImFont* getLargeRegularIconFont();
		ImFont* getMediumRegularIconFont();
	}
}

#endif 