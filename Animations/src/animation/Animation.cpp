#include "renderer/Renderer.h"
#include "animation/Animation.h"
#include "animation/Styles.h"

namespace MathAnim
{
	namespace AnimationManager
	{
		static std::vector<Animation> mAnimations = std::vector<Animation>();
		static std::vector<Style> mStyles = std::vector<Style>();
		static float mTime = 0.0f;

		void addAnimation(const Animation& animation, const Style& style)
		{
			mAnimations.push_back(animation);
			mStyles.push_back(style);
		}

		void update(float dt)
		{
			mTime += dt;
			for (int i = 0; i < mAnimations.size(); i++)
			{
				const Animation& anim = mAnimations[i];
				if (mTime < anim.startTime) continue;

				const Style& style = mStyles.at(i);

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
		}

		void reset()
		{
			mTime = 0.0f;
		}
	}
}