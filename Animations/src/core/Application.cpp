#include "core.h"
#include "core/Window.h"
#include "core/Input.h"
#include "renderer/Renderer.h"
#include "renderer/OrthoCamera.h"
#include "renderer/Shader.h"
#include "renderer/Framebuffer.h"
#include "renderer/Texture.h"
#include "renderer/VideoWriter.h"
#include "renderer/Fonts.h"
#include "animation/Animation.h"
#include "animation/GridLines.h"
#include "animation/Settings.h"
#include "animation/Sandbox.h"
#include "animation/Styles.h"

#include "nanovg.h"
#define NANOVG_GL3_IMPLEMENTATION
#include "nanovg_gl.h"

#include <math.h>

namespace MathAnim
{
	namespace Application
	{
		static bool playAnim = false;
		static bool outputVideoFile = false;

		static int numFramesWritten = 0;
		static int framerate = 60;
		static int outputWidth = 3840;
		static int outputHeight = 2160;
		static NVGcontext* vg = NULL;
		static const char* str = "@";
		Font* baskVillFont;

		static const char* winTitle = "Math Animations";

		void run()
		{
			// Initiaize GLFW/Glad
			bool isRunning = true;

			Window window = Window(1920, 1080, winTitle);
			window.setVSync(true);

			OrthoCamera camera;
			camera.position = Vec2{ 0, 0 };
			camera.projectionSize = Vec2{ 6.0f * (1920.0f / 1080.0f), 6.0f };

			Fonts::init();
			Renderer::init(camera);
			Sandbox::init();

			baskVillFont = Fonts::loadFont("C:/Windows/Fonts/BASKVILL.TTF");
			if (baskVillFont == nullptr)
			{
				g_logger_error("Failed to load BASKVILL.ttf");
				return;
			}

			vg = nvgCreateGL3(NVG_STENCIL_STROKES | NVG_DEBUG);
			if (vg == NULL)
			{
				g_logger_error("Failed to initialize nanovg.");
				return;
			}

			int res = nvgCreateFont(vg, "baskvill", "C:/Windows/Fonts/BASKVILL.TTF");
			if (res == -1)
			{
				g_logger_error("Failed to create font.\n");
				return;
			}

			Texture mainTexture = TextureBuilder()
				.setFormat(ByteFormat::RGBA8_UI)
				.setWidth(outputWidth)
				.setHeight(outputHeight)
				.build();

			Framebuffer mainFramebuffer = FramebufferBuilder(outputWidth, outputHeight)
				.addColorAttachment(mainTexture)
				.includeDepthStencil()
				.generate();

			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			// Run game loop
			// Start with a 60 fps frame rate
			float previousTime = glfwGetTime() - 0.16f;
			constexpr float fixedDeltaTime = 1.0f / 60.0f;
			while (isRunning && !window.shouldClose())
			{
				float deltaTime = glfwGetTime() - previousTime;
				previousTime = glfwGetTime();

				window.setTitle(winTitle + std::string(" -- ") + std::to_string(deltaTime));

				// Bind main framebuffer
				mainFramebuffer.bind();
				glViewport(0, 0, mainFramebuffer.width, mainFramebuffer.height);
				Renderer::clearColor(Colors::greenBrown);

				nvgBeginFrame(vg, (float)mainFramebuffer.width, (float)mainFramebuffer.height, 1.0f);

				// Update components
				if (Settings::displayGrid)
				{
					GridLines::update(camera);
				}

				if (playAnim)
				{
					float customDeltaTime = deltaTime;
					if (outputVideoFile)
					{
						customDeltaTime = 1.0f / 60.0f;
						if (numFramesWritten % 60 == 0)
						{
							g_logger_info("Number of seconds rendered: %d", numFramesWritten / 60);
						}
						numFramesWritten++;
					}
					AnimationManager::update(customDeltaTime);
					Sandbox::update(customDeltaTime);
				}

				static uint32 charIndex = 33;
				const GlyphOutline& glyphOutline = baskVillFont->getGlyphInfo(charIndex);

				glm::mat4 proj = camera.calculateProjectionMatrix();
				static float time = 0.0f;
				constexpr float animTime = 1.0f;
				constexpr float startTime = 2.0f;
				constexpr float fadeInTime = 1.0f;
				constexpr float fadeInStartTime = startTime + (4.5f * animTime / 5.0f);
				float lengthToDraw = ((time - startTime) / animTime) * (float)glyphOutline.totalCurveLengthApprox;
				float amountToFadeIn = ((time - fadeInStartTime) / fadeInTime) * (float)glyphOutline.totalCurveLengthApprox;
				float fontScale = 600.0f;


				static float keyDebounceTime = 0.2f;
				static float keyDebounceTimeLeft = 0.0f;
				keyDebounceTimeLeft -= fixedDeltaTime;
				if (Input::isKeyPressed(GLFW_KEY_UP) && keyDebounceTimeLeft < 0.0f)
				{
					charIndex++;
					keyDebounceTimeLeft = keyDebounceTime;
					time = 2.0f;
					lengthToDraw = 0.0f;
					amountToFadeIn = 0.0f;
				}
				else if (Input::isKeyPressed(GLFW_KEY_DOWN) && keyDebounceTimeLeft < 0.0f)
				{
					charIndex--;
					keyDebounceTimeLeft = keyDebounceTime;
					time = 2.0f;
					lengthToDraw = 0.0f;
					amountToFadeIn = 0.0f;
				}


				glm::vec4 glyphPos = glm::vec4(500.0f, 500.0f, 0.0f, 0.0f);
				if (lengthToDraw > 0)
				{
					float lengthDrawn = 0.0f;
					for (int c = 0; c < glyphOutline.numContours; c++)
					{
						nvgBeginPath(vg);
						nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 255));
						nvgStrokeWidth(vg, 5.0f);

						if (glyphOutline.contours[c].numVertices > 0)
						{
							glm::vec4 pos = glm::vec4(
								glyphOutline.contours[c].vertices[0].position.x + glyphOutline.bearingX,
								glyphOutline.contours[c].vertices[0].position.y - glyphOutline.descentY,
								0.0f,
								1.0f
							);
							nvgMoveTo(
								vg,
								pos.x * fontScale + glyphPos.x,
								(1.0f - pos.y) * fontScale + glyphPos.y
							);
						}

						bool stoppedEarly = false;
						for (int vi = 0; vi < glyphOutline.contours[c].numVertices; vi += 2)
						{
							float lengthLeft = lengthToDraw - (float)lengthDrawn;
							if (lengthLeft < 0.0f)
							{
								break;
							}

							bool isLine = !glyphOutline.contours[c].vertices[vi + 1].controlPoint;
							if (isLine)
							{
								glm::vec4 p0 = glm::vec4(
									glyphOutline.contours[c].vertices[vi].position.x + glyphOutline.bearingX,
									glyphOutline.contours[c].vertices[vi].position.y - glyphOutline.descentY,
									0.0f, 
									1.0f
								);
								glm::vec4 p1 = glm::vec4(
									glyphOutline.contours[c].vertices[vi + 1].position.x + glyphOutline.bearingX,
									glyphOutline.contours[c].vertices[vi + 1].position.y - glyphOutline.descentY,
									0.0f,
									1.0f
								);
								float curveLength = glm::length(p1 - p0);
								lengthDrawn += curveLength;

								if (lengthLeft < curveLength)
								{
									float percentOfCurveToDraw = lengthLeft / curveLength;
									p1 = (p1 - p0) * percentOfCurveToDraw + p0;
								}

								glm::vec4 projectedPos = p1;
								nvgLineTo(
									vg,
									projectedPos.x * fontScale + glyphPos.x,
									(1.0f - projectedPos.y) * fontScale + glyphPos.y
								);
							}
							else
							{
								const Vec2& p0 = glyphOutline.contours[c].vertices[vi + 0].position;
								const Vec2& p1 = glyphOutline.contours[c].vertices[vi + 1].position;
								const Vec2& p1a = glyphOutline.contours[c].vertices[vi + 1].position;
								const Vec2& p2 = glyphOutline.contours[c].vertices[vi + 2].position;

								// Degree elevated quadratic bezier curve
								glm::vec4 pr0 = glm::vec4(
									p0.x + glyphOutline.bearingX, 
									p0.y - glyphOutline.descentY,
									0.0f, 
									1.0f
								);
								glm::vec4 pr1 = (1.0f / 3.0f) * glm::vec4(
									p0.x + glyphOutline.bearingX, 
									p0.y - glyphOutline.descentY,
									0.0f, 
									1.0f
								) + (2.0f / 3.0f) * glm::vec4(
									p1.x + glyphOutline.bearingX, 
									p1.y - glyphOutline.descentY,
									0.0f, 
									1.0f
								);
								glm::vec4 pr2 = (2.0f / 3.0f) * glm::vec4(
									p1.x + glyphOutline.bearingX, 
									p1.y - glyphOutline.descentY,
									0.0f, 
									1.0f
								) + (1.0f / 3.0f) * glm::vec4(
									p2.x + glyphOutline.bearingX, 
									p2.y - glyphOutline.descentY,
									0.0f, 
									1.0f
								);
								glm::vec4 pr3 = glm::vec4(
									p2.x + glyphOutline.bearingX, 
									p2.y - glyphOutline.descentY,
									0.0f, 
									1.0f
								);

								float chordLength = glm::length(pr3 - pr0);
								float controlNetLength = glm::length(pr1 - pr0) + glm::length(pr2 - pr1) + glm::length(pr3 - pr2);
								float approxLength = (chordLength + controlNetLength) / 2.0f;
								lengthDrawn += approxLength;

								if (lengthLeft < approxLength)
								{
									// Interpolate the curve
									float percentOfCurveToDraw = lengthLeft / approxLength;

									pr1 = (pr1 - pr0) * percentOfCurveToDraw + pr0;
									pr2 = (pr2 - pr1) * percentOfCurveToDraw + pr1;
									pr3 = (pr3 - pr2) * percentOfCurveToDraw + pr2;
								}

								// Project the verts
								pr1 = pr1;
								pr2 = pr2;
								pr3 = pr3;

								nvgBezierTo(
									vg,
									pr1.x * fontScale + glyphPos.x,
									(1.0f - pr1.y) * fontScale + glyphPos.y,
									pr2.x * fontScale + glyphPos.x,
									(1.0f - pr2.y) * fontScale + glyphPos.y,
									pr3.x * fontScale + glyphPos.x,
									(1.0f - pr3.y) * fontScale + glyphPos.y
								);

								vi++;
							}

							if (lengthDrawn > lengthToDraw)
							{
								stoppedEarly = true;
								break;
							}
						}

						nvgStroke(vg);

						if (lengthDrawn > lengthToDraw)
						{
							break;
						}
					}
				}

				if (amountToFadeIn > 0)
				{
					std::string str = std::string("") + (char)charIndex;
					float percentToFadeIn = glm::min(amountToFadeIn, 1.0f);
					nvgFillColor(vg, nvgRGBA(255, 255, 255, (unsigned char)(255.0f * percentToFadeIn)));
					nvgFontFace(vg, "baskvill");
					nvgFontSize(vg, fontScale);
					nvgText(vg, glyphPos.x, fontScale + glyphPos.y, str.c_str(), NULL);
				}

				time += fixedDeltaTime;
				//nvgFill(vg);
				//nvgStroke(vg);

				//nvgBeginPath(vg);
				//nvgRect(vg, 100, 100, 120, 30);
				//nvgCircle(vg, 120, 120, 5);
				//nvgPathWinding(vg, NVG_HOLE);	// Mark circle as a hole.
				//nvgFillColor(vg, nvgRGBA(255, 192, 0, 255));
				//nvgFill(vg);

				nvgEndFrame(vg);

				// Render to main framebuffer
				Renderer::render();


				// Render to screen
				mainFramebuffer.unbind();
				glViewport(0, 0, window.width, window.height);
				Renderer::renderFramebuffer(mainFramebuffer);

				if (outputVideoFile)
				{
					Pixel* pixels = mainFramebuffer.readAllPixelsRgb8(0);
					VideoWriter::pushFrame(pixels, outputHeight * outputWidth);
					mainFramebuffer.freePixels(pixels);
				}

				window.swapBuffers();

				if (Input::isKeyPressed(GLFW_KEY_ESCAPE))
				{
					isRunning = false;
				}
				else if (Input::isKeyPressed(GLFW_KEY_F5) && !playAnim)
				{
					playAnim = true;
				}
				else if (Input::isKeyPressed(GLFW_KEY_F6) && !outputVideoFile)
				{
					outputVideoFile = true;
					AnimationManager::reset();
					VideoWriter::startEncodingFile("output.mp4", outputWidth, outputHeight, framerate);
				}
				else if (Input::isKeyPressed(GLFW_KEY_F7) && outputVideoFile)
				{
					outputVideoFile = false;
					VideoWriter::finishEncodingFile();
				}

				window.pollInput();
			}

			Fonts::unloadFont(baskVillFont);
			nvgDeleteGL3(vg);
			Window::cleanup();
		}
	}
}