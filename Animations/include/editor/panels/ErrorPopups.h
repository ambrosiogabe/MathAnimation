#ifndef MATH_ANIM_ERROR_POPUPS_H
#define MATH_ANIM_ERROR_POPUPS_H
#include "core.h"

namespace MathAnim
{
	struct AnimationManagerData;

	namespace ErrorPopups
	{
		void update(AnimationManagerData* am);

		void popupSvgImportError();

		void popupMissingFileError(const std::string& filename, const std::string& additionalMessage = "");
	}
}

#endif