#ifndef MATH_ANIM_SVG_FILE_OBJECT_H
#define MATH_ANIM_SVG_FILE_OBJECT_H
#include "core.h"

#include <nlohmann/json_fwd.hpp>

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
		void free();

		void serialize(nlohmann::json& j) const;
		static SvgFileObject deserialize(const nlohmann::json& j, uint32 version);

		static SvgFileObject createDefault();

		[[deprecated("This is for upgrading legacy projects developed in beta")]]
		static SvgFileObject legacy_deserialize(RawMemory& memory, uint32 version);
	};
}

#endif 