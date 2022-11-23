#ifndef MATH_ANIM_SVG_FILE_OBJECT_H
#define MATH_ANIM_SVG_FILE_OBJECT_H
#include "core.h"

struct NVGcontext;

namespace MathAnim
{
	struct AnimationManagerData;
	struct AnimObject;
	struct SvgGroup;

	struct SvgFileObject
	{
		char* filepath;
		uint32 filepathLength;
		SvgGroup* svgGroup;

		void init(AnimationManagerData* am, AnimObjId parentId);
		void reInit(AnimationManagerData* am, AnimObject* obj);
		bool setFilepath(const std::string& newFilepath);
		bool setFilepath(const char* newFilepath);
		void render(AnimationManagerData* am, NVGcontext* vg, AnimObjId obj) const;
		void renderCreateAnimation(NVGcontext* vg, float t, const AnimObject* parent) const;
		void serialize(RawMemory& memory) const;
		void free();

		static SvgFileObject deserialize(RawMemory& memory, uint32 version);
		static SvgFileObject createDefault();
	};
}

#endif 