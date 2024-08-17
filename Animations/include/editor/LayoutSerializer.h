#ifndef LAYOUT_SERIALIZER_H
#define LAYOUT_SERIALIZER_H
#include "core.h"

#include <nlohmann/json_fwd.hpp>

namespace MathAnim
{
	struct EditorWindowData
	{
		std::string name;
		bool isActive;
	};

	namespace LayoutSerializer
	{
		void imguiFirstFrame(std::vector<EditorWindowData> const& windows);
		std::vector<EditorWindowData> imguiLastFrame();

		// NOTE: Important to serialize/deserialize this at the first/last frames with an active ImGui
		//       context to ensure it gets appropriate data.
		void serialize(nlohmann::json& j, std::vector<EditorWindowData> const& windows);

		// NOTE: Important to serialize/deserialize this at the first/last frames with an active ImGui
		//       context to ensure it gets appropriate data.
		std::vector<EditorWindowData> deserialize(const nlohmann::json& j);
	}
}

#endif