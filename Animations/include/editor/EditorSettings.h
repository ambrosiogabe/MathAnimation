#ifndef MATH_ANIM_EDITOR_SETTINGS
#define MATH_ANIM_EDITOR_SETTINGS
#include "core.h"

namespace MathAnim
{
	struct AnimationManagerData;

	enum class ViewMode : int
	{
		WireMesh,
		Normal,
		Length
	};

	constexpr auto _viewModeEnumNames = fixedSizeArray<const char*, (size_t)ViewMode::Length>(
		"Wire Mesh View",
		"Normal View"
	);

	enum class PreviewSvgFidelity : uint8
	{
		Low,
		Medium,
		High,
		Ultra,
		Length
	};

	constexpr auto _previewFidelityEnumNames = fixedSizeArray<const char*, (size_t)PreviewSvgFidelity::Length>(
		"Low",
		"Medium",
		"High",
		"Ultra"
	);

	constexpr auto _previewFidelityValues = fixedSizeArray<float, (size_t)PreviewSvgFidelity::Length>(
		100.0f,
		150.0f,
		200.0f,
		300.0f
	);

	struct EditorSettingsData
	{
		float mouseSensitivity;
		float scrollSensitvity;
		ViewMode viewMode;
		PreviewSvgFidelity previewFidelity;
		float svgTargetScale;
		Vec4 activeObjectHighlightColor;
		float activeObjectOutlineWidth;
	};

	namespace EditorSettings
	{
		void init();
		void imgui(AnimationManagerData* am);
		void free();

		void setFidelity(PreviewSvgFidelity fidelity);
		const EditorSettingsData& getSettings();
	}
}

#endif 