#include "editor/ExportPanel.h"
#include "editor/EditorSettings.h"
#include "core.h"
#include "core/Application.h"
#include "video/Encoder.h"
#include "animation/AnimationManager.h"
#include "renderer/Renderer.h"
#include "renderer/Framebuffer.h"
#include "renderer/Texture.h"
#include "renderer/PixelBufferDownloader.h"

#include <nfd.h>

namespace MathAnim
{
	namespace ExportPanel
	{
		static constexpr int framerate = 60;
		static constexpr Mbps bitrate = 60;

		static VideoEncoder* encoder;
		static bool outputVideoFile;
		static Framebuffer yuvFramebuffer;
		static PixelBufferDownload pboDownloader;
		static std::string outputVideoFilename;
		static uint32 outputWidth;
		static uint32 outputHeight;
		static PreviewSvgFidelity fidelityBeforeExport = PreviewSvgFidelity::Low;

		// -------------------- Internal Functions --------------------
		static void imgui(AnimationManagerData* am);
		static void processEncoderData(AnimationManagerData* am);
		static void exportVideoTo(AnimationManagerData* am, const std::string& filename);
		static void endExport();

		void init(uint32 inOutputWidth, uint32 inOutputHeight)
		{
			outputWidth = inOutputWidth;
			outputHeight = inOutputHeight;
			outputVideoFile = false;
			encoder = nullptr;

			pboDownloader = PixelBufferDownload();
			pboDownloader.create(outputWidth, outputHeight);

			Texture textureSpec = TextureBuilder()
				.setWidth(outputWidth)
				.setHeight(outputHeight)
				.setFormat(ByteFormat::R8_UI)
				.setMagFilter(FilterMode::Linear)
				.setMinFilter(FilterMode::Linear)
				.build();
			yuvFramebuffer = FramebufferBuilder(outputWidth, outputHeight)
				.addColorAttachment(textureSpec)
				.addColorAttachment(textureSpec)
				.addColorAttachment(textureSpec)
				.generate();
		}

		void update(AnimationManagerData* am)
		{
			if (outputVideoFile)
			{
				processEncoderData(am);
			}

			imgui(am);
		}

		bool isExportingVideo()
		{
			return encoder && encoder->getPercentComplete() < 1.0f;
		}

		float getExportSecondsPerFrame()
		{
			return 1.0f / (float)framerate;
		}

		void free()
		{
			pboDownloader.free();

			// Free it just in case, if the encoder isn't active this does nothing
			VideoEncoder::finalizeEncodingFile(encoder);
			VideoEncoder::freeEncoder(encoder);
		}

		// -------------------- Internal Functions --------------------
		static void imgui(AnimationManagerData* am)
		{
			ImGui::Begin("Export Video");

			constexpr int filenameBufferSize = _MAX_PATH;
			static char filenameBuffer[filenameBufferSize];
			ImGui::BeginDisabled();
			ImGui::InputText(": Filename", filenameBuffer, filenameBufferSize);
			ImGui::EndDisabled();

			ImGui::BeginDisabled(isExportingVideo());
			if (ImGui::Button("Export"))
			{
				nfdchar_t* outPath = NULL;
				nfdresult_t result = NFD_SaveDialog("mov", NULL, &outPath);

				if (result == NFD_OKAY)
				{
					size_t pathLength = std::strlen(outPath);
					if (pathLength < filenameBufferSize)
					{
						g_memory_copyMem(filenameBuffer, outPath, pathLength);
						filenameBuffer[pathLength] = '\0';
					}

					std::free(outPath);
				}
				else if (result == NFD_CANCEL)
				{
					filenameBuffer[0] = '\0';
				}
				else
				{
					filenameBuffer[0] = '\0';
					g_logger_error("Error opening file to save to for video export:\n\t%s", NFD_GetError());
				}

				std::string filename = std::string(filenameBuffer);
				if (filename != "")
				{
					std::filesystem::path filepath = filename;
					if (!filepath.has_extension())
					{
						filepath.replace_extension(".mov");
					}
					g_logger_info("Exporting video to %s", filepath.string().c_str());
					exportVideoTo(am, filepath.string());
				}
			}
			ImGui::EndDisabled();

			ImGui::SameLine();
			ImGui::BeginDisabled(!isExportingVideo());
			if (ImGui::Button("Stop Exporting"))
			{
				endExport();
			}
			ImGui::EndDisabled();

			ImGui::End();
		}

		static void processEncoderData(AnimationManagerData* am)
		{
			const Framebuffer& mainFramebuffer = Application::getMainFramebuffer();
			if (!AnimationManager::isPastLastFrame(am))
			{
				// Render to yuvFramebuffer
				Renderer::renderTextureToFramebuffer(mainFramebuffer.getColorAttachment(0), yuvFramebuffer, ShaderType::RgbToYuvShader);

				// Transfer pixels from this framebuffer to our PBOs for async downloads
				pboDownloader.queueDownloadFrom(yuvFramebuffer.getColorAttachment(0), yuvFramebuffer.getColorAttachment(1), yuvFramebuffer.getColorAttachment(2));
			}

			if (pboDownloader.pixelsAreReady)
			{
				const Pixels& yuvPixels = pboDownloader.getPixels();
				// TODO: Add a hardware accelerated version that usee CUDA and NVENC
				encoder->pushYuvFrame(yuvPixels.yColorBuffer, yuvPixels.dataSize);
			}

			if (AnimationManager::isPastLastFrame(am) && pboDownloader.numItemsInQueue <= 0)
			{
				endExport();
				pboDownloader.reset();
			}
		}

		static void exportVideoTo(AnimationManagerData* am, const std::string& filename)
		{
			if (encoder)
			{
				g_logger_warning("Tried to export video to '%s' while another export for file '%s' was in progress.", filename.c_str(), outputVideoFilename.c_str());
				return;
			}

			outputVideoFilename = filename;
			encoder = VideoEncoder::startEncodingFile(
				outputVideoFilename.c_str(),
				outputWidth,
				outputHeight,
				framerate,
				bitrate,
				VideoEncoderFlags::LogProgress
			);

			if (encoder)
			{
				Application::resetToFrame(-1);
				AnimationManager::resetToFrame(am, 0);
				Application::setEditorPlayState(AnimState::PlayForwardFixedFrameTime);
				outputVideoFile = true;
				fidelityBeforeExport = EditorSettings::getSettings().previewFidelity;
				EditorSettings::setFidelity(PreviewSvgFidelity::Ultra);
				AnimationManager::retargetSvgScales(am);
			}
		}

		void endExport()
		{
			VideoEncoder::finalizeEncodingFile(encoder);
			outputVideoFile = false;
			EditorSettings::setFidelity(fidelityBeforeExport);
			Application::setEditorPlayState(AnimState::Pause);
		}
	}
}