#ifndef MATH_ANIM_EXPORT_PANEL_H
#define MATH_ANIM_EXPORT_PANEL_H
#include "core.h"

namespace MathAnim
{
	struct AnimationManagerData;

	namespace ExportPanel
	{
		void init(uint32 outputWidth, uint32 outputHeight);

		void update(AnimationManagerData* am);

		bool isExportingVideo();

		float getExportSecondsPerFrame();

		void free();
	}
}

#endif 