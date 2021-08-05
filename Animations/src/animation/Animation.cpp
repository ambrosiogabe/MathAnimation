#include "renderer/Renderer.h"
#include "animation/Animation.h"
#include "animation/Styles.h"

namespace MathAnim
{
	namespace AnimationManager
	{
		static std::vector<ParametricAnimation> mParametricAnimations = std::vector<ParametricAnimation>();
		static std::vector<Style> mParametricStyles = std::vector<Style>();

		static std::vector<TextAnimation> mTextAnimations = std::vector<TextAnimation>();
		static std::vector<Style> mTextStyles = std::vector<Style>();
		static float mTime = 0.0f;

		void addParametricAnimation(const ParametricAnimation& animation, const Style& style)
		{
			mParametricAnimations.push_back(animation);
			mParametricStyles.push_back(style);
		}

		void addTextAnimation(const TextAnimation& animation, const Style& style)
		{
			mTextAnimations.push_back(animation);
			mTextStyles.push_back(style);
		}

		void update(float dt)
		{
			mTime += dt;
			for (int i = 0; i < mParametricAnimations.size(); i++)
			{
				const ParametricAnimation& anim = mParametricAnimations[i];
				if (mTime < anim.startTime) continue;

				const Style& style = mParametricStyles.at(i);

				float lerp = glm::clamp((mTime - anim.startTime) / anim.duration, 0.0f, 1.0f);
				float percentToDo = ((anim.endT - anim.startT) * lerp) / (anim.endT - anim.startT);

				float t = anim.startT;
				const float granularity = ((anim.endT - anim.startT) * percentToDo) / anim.granularity;
				for (int i = 0; i < anim.granularity; i++)
				{
					glm::vec2 coord = anim.parametricEquation(t);
					float nextT = t + granularity;
					glm::vec2 nextCoord = anim.parametricEquation(nextT);
					Renderer::drawLine(glm::vec2(coord.x, coord.y), glm::vec2(nextCoord.x, nextCoord.y), style);
					t = nextT;
				}
			}

			for (int i = 0; i < mTextAnimations.size(); i++)
			{
				const TextAnimation& anim = mTextAnimations[i];
				if (mTime < anim.startTime) continue;

				const Style& style = mTextStyles.at(i);

				int numCharsToShow = glm::clamp((int)((mTime - anim.startTime) / anim.typingTime), 0, (int)anim.text.length());
				Renderer::drawString(anim.text.substr(0, numCharsToShow), *anim.font, anim.position, anim.scale, style.color);
			}
		}

		void reset()
		{
			mTime = 0.0f;
		}
	}
}