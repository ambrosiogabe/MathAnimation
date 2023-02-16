#ifndef MATH_ANIM_INSPECTOR_PANEL_H
#define MATH_ANIM_INSPECTOR_PANEL_H
#include "core.h"

namespace MathAnim
{
	struct AnimationManagerData;

	struct AnimObjectPayload
	{
		AnimObjId animObjectId;
		int32 sceneHierarchyIndex;
	};

	namespace InspectorPanel
	{
		void update(AnimationManagerData* am);
		void free();

		const char* getAnimObjectPayloadId();

		void setActiveAnimation(AnimId animationId);
		void setActiveAnimObject(AnimObjId animObjectId);
		AnimObjId getActiveAnimObject();
		AnimId getActiveAnimation();
	}
}

#endif 
