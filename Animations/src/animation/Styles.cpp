#include "animation/Styles.h"

namespace MathAnim
{
	namespace Colors
	{
		extern glm::vec4 greenBrown = "#272822"_hex;
		extern glm::vec4 offWhite = "#F8F8F2"_hex;
		extern glm::vec4 darkGray = "#75715E"_hex;
		extern glm::vec4 red = "#F92672"_hex;
		extern glm::vec4 orange = "#FD971F"_hex;
		extern glm::vec4 lightOrange = "#E69F66"_hex;
		extern glm::vec4 yellow = "#E6DB74"_hex;
		extern glm::vec4 green = "#A6E22E"_hex;
		extern glm::vec4 blue = "#66D9EF"_hex;
		extern glm::vec4 purple = "#AE81FF"_hex;
	}

	namespace Styles
	{
		extern Style defaultStyle = {
			Colors::offWhite,
			0.03f,
			CapType::Flat
		};

		extern Style gridStyle = {
			Colors::offWhite,
			0.005f,
			CapType::Flat
		};

		extern Style verticalAxisStyle = {
			Colors::green,
			0.02f,
			CapType::Flat
		};

		extern Style horizontalAxisStyle = {
			Colors::red,
			0.02f,
			CapType::Flat
		};
	}
}