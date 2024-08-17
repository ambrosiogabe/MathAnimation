#include "editor/LayoutSerializer.h"
#include "core/Serialization.hpp"
#include "editor/imgui/ImGuiExtended.h"

#include <nlohmann/json.hpp>
#include <imgui_internal.h>

namespace MathAnim
{
	namespace LayoutSerializer
	{
		void imguiFirstFrame(std::vector<EditorWindowData> const& windows)
		{
			for (auto const& window : windows)
			{
				if (window.isActive)
				{
					ImGuiExtended::makeDockTabVisible(window.name.c_str(), true);
				}
			}
		}

		std::vector<EditorWindowData> imguiLastFrame()
		{
			auto res = std::vector<EditorWindowData>();
			ImGuiContext& g = *GImGui;
			for (auto const& window : g.Windows)
			{
				if (!window->DockNode)
				{
					continue;
				}

				if (!window->DockNode->TabBar)
				{
					continue;
				}

				bool isActive = window->DockNode->TabBar->SelectedTabId == window->TabId;
				res.emplace_back(EditorWindowData{ std::string(window->Name), isActive });
			}

			return res;
		}

		constexpr const char* JsonPropField = "EditorPanelLayout";
		void serialize(nlohmann::json& j, std::vector<EditorWindowData> const& windows)
		{
			nlohmann::json windowsJson = {};
			for (auto const& window : windows)
			{
				nlohmann::json data = {};
				SERIALIZE_NON_NULL_PROP(data, &window, name);
				SERIALIZE_NON_NULL_PROP(data, &window, isActive);
				windowsJson.push_back(data);
			}

			j[JsonPropField] = windowsJson;
		}

		std::vector<EditorWindowData> deserialize(const nlohmann::json& j)
		{
			std::vector<EditorWindowData> res = {};
			if (!j.contains(JsonPropField))
			{
				return res;
			}

			for (auto& editorJson : j[JsonPropField])
			{
				if (editorJson.is_null()) continue;

				EditorWindowData meta = {};
				DESERIALIZE_PROP(&meta, name, editorJson, "");
				DESERIALIZE_PROP(&meta, isActive, editorJson, false);

				if (meta.name == "")
				{
					g_logger_warning("Corrupted window encountered in editor layout while deserializing.");
					continue;
				}

				res.emplace_back(meta);
			}

			return res;
		}
	}
}
