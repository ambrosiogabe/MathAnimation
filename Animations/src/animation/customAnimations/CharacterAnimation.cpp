#include "animation/customAnimations/CharacterAnimation.h"
#include "animation/Animation.h"
#include "animation/AnimationBuilders.h"
#include "animation/Styles.h"
#include "renderer/Renderer.h"
#include "utils/CMath.h"

namespace MathAnim
{
	namespace CharacterAnimation
	{
		static glm::vec2 circlePosition = glm::vec2(0.0f, 0.0f);
		static float circleRadius = 0.5f;
		static Style characterColor;
		static Style eyeWhite;
		static Style eyeBlack;

		glm::vec2 circle(float t)
		{
			return {
				circleRadius * glm::cos(glm::radians(t)),
				circleRadius * glm::sin(glm::radians(t))
			};
		}

		void drawCharacterPose1()
		{

		}

		void init()
		{
			characterColor = Styles::defaultStyle;
			eyeWhite = Styles::defaultStyle;
			eyeBlack = Styles::defaultStyle;

			characterColor.color = "#5387b8"_hex;
			eyeWhite.color = "#FFFFFF"_hex;
			eyeBlack.color = "#000000"_hex;

			eyeWhite.strokeWidth = 0.04f;

			drawCharacterPose1();
		}
	}
}