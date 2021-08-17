#include "core.h"
#include "animation/GridLines.h"
#include "animation/Styles.h"
#include "animation/Settings.h"
#include "renderer/OrthoCamera.h"
#include "renderer/Renderer.h"

namespace MathAnim
{
	namespace GridLines
	{
		void update(const OrthoCamera& camera)
		{
			// Since the viewport is centered, pretend the camera is slightly down-left to get the grid-lines centered properly
			const glm::vec2& projectionSize = camera.projectionSize;
			const glm::vec2& cameraPos = camera.position - (projectionSize / 2.0f);
			const float cameraZoom = 1.0f;

			float firstX = ((int)glm::floor(cameraPos.x / Settings::gridGranularity.x)) * Settings::gridGranularity.x;
			float firstY = ((int)glm::floor(cameraPos.y / Settings::gridGranularity.y)) * Settings::gridGranularity.y;

			int numVtLines = (int)(projectionSize.x * cameraZoom / Settings::gridGranularity.x) + 2;
			int numHzLines = (int)(projectionSize.y * cameraZoom / Settings::gridGranularity.y) + 2;

			float width = (int)(projectionSize.x * cameraZoom) + (5 * Settings::gridGranularity.x);
			float height = (int)(projectionSize.y * cameraZoom) + (5 * Settings::gridGranularity.y);

			int maxLines = glm::max(numVtLines, numHzLines);
			for (int i = 0; i < maxLines; i++)
			{
				float x = firstX + (Settings::gridGranularity.x * i);
				float y = firstY + (Settings::gridGranularity.y * i);

				if (i < numVtLines)
				{
					Renderer::drawLine(glm::vec2(x, firstY), glm::vec2(x, firstY + height), Styles::gridStyle);
				}

				if (i < numHzLines)
				{
					Renderer::drawLine(glm::vec2(firstX, y), glm::vec2(firstX + width, y), Styles::gridStyle);
				}
			}

			// Draw vertical and horizontal axes
			if (Settings::colorGridAxes)
			{
				Renderer::drawLine(glm::vec2(0.0f, -100.0f), glm::vec2(0.0f, 100.0f), Styles::verticalAxisStyle);
				Renderer::drawLine(glm::vec2(-100.0f, 0.0f), glm::vec2(100.0f, 0.0f), Styles::horizontalAxisStyle);
			}
		}
	}
}