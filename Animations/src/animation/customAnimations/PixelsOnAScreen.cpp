#include "animation/customAnimations/PixelsOnAScreen.h"
#include "animation/customAnimations/GridAnimation.h"
#include "animation/Animation.h"
#include "animation/AnimationBuilders.h"
#include "animation/Styles.h"
#include "renderer/Renderer.h"
#include "utils/CMath.h"

namespace MathAnim
{
	namespace PixelsOnAScreen
	{
		void init()
		{

			Vec4 sky = "#5fcde4"_hex;
			Vec4 cld = "#ffffff"_hex;
			Vec4 rf1 = "#d9a066"_hex;
			Vec4 rf2 = "#d9a066"_hex;
			Vec4 wll = "#595652"_hex;
			Vec4 drt = "#663931"_hex;
			Vec4 grs = "#6abe30"_hex;
			Vec4 dr2 = "#d9a066"_hex;

			Vec4 c0 = "#eec39a"_hex;
			Vec4 c1 = "#ac3232"_hex;
			Vec4 c2 = "#639bff"_hex;

			Vec4 leftSide[16][16] = {
				{ sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky },
				{ sky, sky, cld, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky },
				{ sky, cld, cld, cld, sky, sky, sky, cld, cld, cld, sky, sky, sky, sky, sky, sky },
				{ sky, sky, sky, sky, sky, sky, sky, sky, cld, sky, sky, sky, sky, sky, sky, sky },
				{ sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky },
				{ sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, grs },
				{ sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, grs },
				{ sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, grs },
				{ sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky },
				{ sky, sky, sky, c0, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky },
				{ sky, sky, sky, c1, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky },
				{ sky, sky, sky, c2, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky },
				{ grs, grs, grs, grs, grs, grs, grs, grs, grs, grs, grs, grs, grs, grs, grs, grs },
				{ drt, drt, drt, dr2, drt, drt, drt, drt, drt, drt, drt, drt, drt, drt, drt, drt },
				{ drt, drt, drt, drt, drt, drt, drt, drt, dr2, drt, drt, drt, drt, drt, drt, drt },
				{ drt, dr2, drt, drt, drt, drt, drt, drt, drt, drt, drt, drt, drt, drt, drt, drt }
			};

			Vec4 rightSide[16][16] = {
				{ sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky },
				{ sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky },
				{ sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, cld, cld, sky, sky },
				{ sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, cld, cld, sky },
				{ sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, rf1, rf1 },
				{ grs, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, rf1, rf1, rf1 },
				{ grs, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, rf1, rf1, rf1, rf1 },
				{ grs, grs, sky, sky, sky, sky, sky, sky, sky, sky, sky, rf1, rf1, rf2, wll, wll },
				{ drt, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, wll, wll, wll, wll },
				{ drt, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, wll, wll, wll, wll },
				{ drt, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, sky, wll, wll, wll, drt },
				{ drt, sky, sky, sky, sky, sky, sky, sky, sky, grs, grs, grs, grs, grs, wll, drt },
				{ grs, grs, grs, grs, grs, grs, grs, grs, grs, grs, grs, grs, grs, grs, grs, grs },
				{ drt, drt, drt, drt, drt, drt, drt, drt, drt, drt, drt, drt, drt, drt, drt, drt },
				{ drt, drt, drt, drt, drt, drt, drt, drt, drt, drt, drt, dr2, drt, drt, drt, drt },
				{ drt, dr2, drt, drt, drt, drt, drt, drt, drt, drt, drt, drt, drt, drt, drt, drt }
			};

			GridAnimation::init({ -4.0f, -2.0f }, { 8.0f, 4.0f }, { 0.25f, 0.25f });

			AnimationManager::addAnimation(
				BitmapAnimationBuilder()
				.setCanvasPosition({ -4.0f, -2.0f })
				.setCanvasSize({ 4.0f, 4.0f })
				.setBitmap(leftSide)
				.setDuration(4.0f)
				.setRevealTime(0.16f)
				.build(),
				Styles::defaultStyle
			);

			AnimationManager::addAnimation(
				BitmapAnimationBuilder()
				.setCanvasPosition({ 0.0f, -2.0f })
				.setCanvasSize({ 4.0f, 4.0f })
				.setBitmap(rightSide)
				.setDuration(4.0f)
				.setRevealTime(0.16f)
				.setDelay(-4.0f)
				.build(),
				Styles::defaultStyle
			);
		}
	}
}
