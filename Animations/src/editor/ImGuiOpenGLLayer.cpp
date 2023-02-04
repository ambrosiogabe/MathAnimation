#include "editor/ImGuiOpenGLLayer.h"
#include "core.h"
#include "core/Profiling.h"
#include "renderer/GLApi.h"

#include <imgui.h>

// OpenGL Data
struct MathAnim_ImplOpenGL3_Data
{
	GLuint          GlVersion;               // Extracted at runtime using GL_MAJOR_VERSION, GL_MINOR_VERSION queries (e.g. 320 for GL 3.2)
	char            GlslVersionString[32];   // Specified by user or detected based on compile time GL settings.
	GLuint          FontTexture;
	GLuint          ShaderHandle;
	GLint           AttribLocationTex;       // Uniforms location
	GLint           AttribLocationProjMtx;
	GLuint          AttribLocationVtxPos;    // Vertex attributes location
	GLuint          AttribLocationVtxUV;
	GLuint          AttribLocationVtxColor;
	unsigned int    VaoHandle, VboHandle, ElementsHandle;
	GLsizeiptr      VertexBufferSize;
	GLsizeiptr      IndexBufferSize;
	bool            HasClipOrigin;

	MathAnim_ImplOpenGL3_Data() { memset((void*)this, 0, sizeof(*this)); }
};

// Backend data stored in io.BackendRendererUserData to allow support for multiple Dear ImGui contexts
// It is STRONGLY preferred that you use docking branch with multi-viewports (== single Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
static MathAnim_ImplOpenGL3_Data* MathAnim_ImplOpenGL3_GetBackendData()
{
	return ImGui::GetCurrentContext() ? (MathAnim_ImplOpenGL3_Data*)ImGui::GetIO().BackendRendererUserData : nullptr;
}

// Forward Declarations
static void MathAnim_ImplOpenGL3_InitPlatformInterface();
static void MathAnim_ImplOpenGL3_ShutdownPlatformInterface();

// Functions
bool MathAnim_ImplOpenGL3_Init(int glVersionMajor, int glVersionMinor, const char* glsl_version)
{
	ImGuiIO& io = ImGui::GetIO();
	IM_ASSERT(io.BackendRendererUserData == nullptr && "Already initialized a renderer backend!");

	// Setup backend capabilities flags
	MathAnim_ImplOpenGL3_Data* bd = IM_NEW(MathAnim_ImplOpenGL3_Data)();
	io.BackendRendererUserData = (void*)bd;
	io.BackendRendererName = "math_anim_impl_opengl3";

	// Query for GL version (e.g. 320 for GL 3.2)
	bd->GlVersion = (GLuint)(glVersionMajor * 100 + glVersionMinor * 10);

	if (bd->GlVersion >= 320)
		io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;  // We can honor the ImDrawCmd::VtxOffset field, allowing for large meshes.
	io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;  // We can create multi-viewports on the Renderer side (optional)

	// Store GLSL version string so we can refer to it later in case we recreate shaders.
	// Note: GLSL version is NOT the same as GL version. Leave this to nullptr if unsure.
	if (glsl_version == nullptr)
	{
		glsl_version = "#version 140";
	}
	IM_ASSERT((int)strlen(glsl_version) + 2 < IM_ARRAYSIZE(bd->GlslVersionString));
	strcpy(bd->GlslVersionString, glsl_version);
	strcat(bd->GlslVersionString, "\n");

	// Detect extensions we support
	bd->HasClipOrigin = (bd->GlVersion >= 450);
	GLint num_extensions = 0;
	MathAnim::GL::getIntegerv(GL_NUM_EXTENSIONS, &num_extensions);
	for (GLint i = 0; i < num_extensions; i++)
	{
		const char* extension = (const char*)MathAnim::GL::getStringi(GL_EXTENSIONS, i);
		if (extension != nullptr && strcmp(extension, "GL_ARB_clip_control") == 0)
			bd->HasClipOrigin = true;
	}

	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		MathAnim_ImplOpenGL3_InitPlatformInterface();

	return true;
}

void MathAnim_ImplOpenGL3_Shutdown()
{
	MathAnim_ImplOpenGL3_Data* bd = MathAnim_ImplOpenGL3_GetBackendData();
	IM_ASSERT(bd != nullptr && "No renderer backend to shutdown, or already shutdown?");
	ImGuiIO& io = ImGui::GetIO();

	MathAnim_ImplOpenGL3_ShutdownPlatformInterface();
	MathAnim_ImplOpenGL3_DestroyDeviceObjects();
	io.BackendRendererName = nullptr;
	io.BackendRendererUserData = nullptr;
	IM_DELETE(bd);
}

void MathAnim_ImplOpenGL3_NewFrame()
{
	MathAnim_ImplOpenGL3_Data* bd = MathAnim_ImplOpenGL3_GetBackendData();
	IM_ASSERT(bd != nullptr && "Did you call ImGui_ImplOpenGL3_Init()?");

	if (!bd->ShaderHandle)
		MathAnim_ImplOpenGL3_CreateDeviceObjects();
}

static void MathAnim_ImplOpenGL3_SetupRenderState(ImDrawData* draw_data, int fb_width, int fb_height)
{
	MP_PROFILE_EVENT("MathAnim_ImplOpenGL3_RenderDrawData_SetupRenderState");
	MathAnim_ImplOpenGL3_Data* bd = MathAnim_ImplOpenGL3_GetBackendData();

	// Setup render state: alpha-blending enabled, no face culling, no depth testing, scissor enabled, polygon fill
	MathAnim::GL::enable(GL_BLEND);
	MathAnim::GL::blendEquation(GL_FUNC_ADD);
	MathAnim::GL::blendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	MathAnim::GL::disable(GL_CULL_FACE);
	MathAnim::GL::disable(GL_DEPTH_TEST);
	MathAnim::GL::disable(GL_STENCIL_TEST);
	MathAnim::GL::enable(GL_SCISSOR_TEST);
	if (bd->GlVersion >= 310)
	{
		MathAnim::GL::disable(GL_PRIMITIVE_RESTART);
	}
	MathAnim::GL::polygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// Setup viewport, orthographic projection matrix
	// Our visible imgui space lies from draw_data->DisplayPos (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
	MathAnim::GL::viewport(0, 0, (GLsizei)fb_width, (GLsizei)fb_height);
	float L = draw_data->DisplayPos.x;
	float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
	float T = draw_data->DisplayPos.y;
	float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y;
	const float ortho_projection[4][4] =
	{
		{ 2.0f / (R - L),   0.0f,         0.0f,   0.0f },
		{ 0.0f,         2.0f / (T - B),   0.0f,   0.0f },
		{ 0.0f,         0.0f,        -1.0f,   0.0f },
		{ (R + L) / (L - R),  (T + B) / (B - T),  0.0f,   1.0f },
	};
	MathAnim::GL::useProgram(bd->ShaderHandle);
	MathAnim::GL::uniform1i(bd->AttribLocationTex, 0);
	MathAnim::GL::uniformMatrix4fv(bd->AttribLocationProjMtx, 1, GL_FALSE, &ortho_projection[0][0]);

	// Bind vertex/index buffers and setup attributes for ImDrawVert
	MathAnim::GL::bindVertexArray(bd->VaoHandle);
	MathAnim::GL::bindBuffer(GL_ARRAY_BUFFER, bd->VboHandle);
	MathAnim::GL::bindBuffer(GL_ELEMENT_ARRAY_BUFFER, bd->ElementsHandle);
}

// OpenGL3 Render function.
// Note that this implementation is little overcomplicated because we are saving/setting up/restoring every OpenGL state explicitly.
// This is in order to be able to run within an OpenGL engine that doesn't do so.
void MathAnim_ImplOpenGL3_RenderDrawData(ImDrawData* draw_data)
{
	MP_PROFILE_EVENT("MathAnim_ImplOpenGL3_RenderDrawData");

	// Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates != framebuffer coordinates)
	int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
	int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
	if (fb_width <= 0 || fb_height <= 0)
		return;

	MathAnim_ImplOpenGL3_Data* bd = MathAnim_ImplOpenGL3_GetBackendData();

	// Setup desired GL state
	// Recreate the VAO every time (this is to easily allow multiple GL contexts to be rendered to. VAO are not shared among GL contexts)
	// The renderer would actually work without any VAO bound, but then our VertexAttrib calls would overwrite the default one currently bound.
	MathAnim_ImplOpenGL3_SetupRenderState(draw_data, fb_width, fb_height);

	// Will project scissor/clipping rectangles into framebuffer space
	ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
	ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display which are often (2,2)

	// Render command lists
	{
		MP_PROFILE_EVENT("MathAnim_ImplOpenGL3_RenderDrawData_RenderCommandLists");
		for (int n = 0; n < draw_data->CmdListsCount; n++)
		{
			const ImDrawList* cmd_list = draw_data->CmdLists[n];

			// Upload vertex/index buffers
			const GLsizeiptr vtx_buffer_size = (GLsizeiptr)cmd_list->VtxBuffer.Size * (int)sizeof(ImDrawVert);
			const GLsizeiptr idx_buffer_size = (GLsizeiptr)cmd_list->IdxBuffer.Size * (int)sizeof(ImDrawIdx);
			MathAnim::GL::bufferData(GL_ARRAY_BUFFER, vtx_buffer_size, (const GLvoid*)cmd_list->VtxBuffer.Data, GL_STREAM_DRAW);
			MathAnim::GL::bufferData(GL_ELEMENT_ARRAY_BUFFER, idx_buffer_size, (const GLvoid*)cmd_list->IdxBuffer.Data, GL_STREAM_DRAW);

			for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
			{
				const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
				if (pcmd->UserCallback != nullptr)
				{
					// User callback, registered via ImDrawList::AddCallback()
					// (ImDrawCallback_ResetRenderState is a special callback value used by the user to request the renderer to reset render state.)
					if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
						MathAnim_ImplOpenGL3_SetupRenderState(draw_data, fb_width, fb_height);
					else
						pcmd->UserCallback(cmd_list, pcmd);
				}
				else
				{
					// Project scissor/clipping rectangles into framebuffer space
					ImVec2 clip_min((pcmd->ClipRect.x - clip_off.x) * clip_scale.x, (pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
					ImVec2 clip_max((pcmd->ClipRect.z - clip_off.x) * clip_scale.x, (pcmd->ClipRect.w - clip_off.y) * clip_scale.y);
					if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
						continue;

					// Apply scissor/clipping rectangle (Y is inverted in OpenGL)
					MathAnim::GL::scissor((int)clip_min.x, (int)((float)fb_height - clip_max.y), (int)(clip_max.x - clip_min.x), (int)(clip_max.y - clip_min.y));

					// Bind texture, Draw
					MathAnim::GL::activeTexture(GL_TEXTURE0);
					MathAnim::GL::bindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->GetTexID());
					if (bd->GlVersion >= 320)
						MathAnim::GL::drawElementsBaseVertex(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, (void*)(intptr_t)(pcmd->IdxOffset * sizeof(ImDrawIdx)), (GLint)pcmd->VtxOffset);
					else
						MathAnim::GL::drawElements(GL_TRIANGLES, (GLsizei)pcmd->ElemCount, sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT, (void*)(intptr_t)(pcmd->IdxOffset * sizeof(ImDrawIdx)));
				}
			}
		}
	}

	MathAnim::GL::enable(GL_BLEND);
	MathAnim::GL::blendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	MathAnim::GL::blendFuncSeparate(GL_ONE, GL_ZERO, GL_ONE, GL_ZERO);
	MathAnim::GL::disable(GL_SCISSOR_TEST);
}

bool MathAnim_ImplOpenGL3_CreateFontsTexture()
{
	ImGuiIO& io = ImGui::GetIO();
	MathAnim_ImplOpenGL3_Data* bd = MathAnim_ImplOpenGL3_GetBackendData();

	// Build texture atlas
	unsigned char* pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);   // Load as RGBA 32-bit (75% of the memory is wasted, but default font is so small) because it is more likely to be compatible with user's existing shaders. If your ImTextureId represent a higher-level concept than just a GL texture id, consider calling GetTexDataAsAlpha8() instead to save on GPU memory.

	// Upload texture to graphics system
	// (Bilinear sampling is required by default. Set 'io.Fonts->Flags |= ImFontAtlasFlags_NoBakedLines' or 'style.AntiAliasedLinesUseTex = false' to allow point/nearest sampling)
	MathAnim::GL::genTextures(1, &bd->FontTexture);
	MathAnim::GL::bindTexture(GL_TEXTURE_2D, bd->FontTexture);
	MathAnim::GL::texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	MathAnim::GL::texParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	MathAnim::GL::pixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	MathAnim::GL::texImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

	// Store our identifier
	io.Fonts->SetTexID((ImTextureID)(intptr_t)bd->FontTexture);

	return true;
}

void MathAnim_ImplOpenGL3_DestroyFontsTexture()
{
	ImGuiIO& io = ImGui::GetIO();
	MathAnim_ImplOpenGL3_Data* bd = MathAnim_ImplOpenGL3_GetBackendData();
	if (bd->FontTexture)
	{
		MathAnim::GL::deleteTextures(1, &bd->FontTexture);
		io.Fonts->SetTexID(0);
		bd->FontTexture = 0;
	}
}

// If you get an error please report on github. You may try different GL context version or GLSL version. See GL<>GLSL version table at the top of this file.
static bool CheckShader(GLuint handle, const char* desc)
{
	MathAnim_ImplOpenGL3_Data* bd = MathAnim_ImplOpenGL3_GetBackendData();
	GLint status = 0, log_length = 0;
	MathAnim::GL::getShaderiv(handle, GL_COMPILE_STATUS, &status);
	MathAnim::GL::getShaderiv(handle, GL_INFO_LOG_LENGTH, &log_length);
	if ((GLboolean)status == GL_FALSE)
		fprintf(stderr, "ERROR: ImGui_ImplOpenGL3_CreateDeviceObjects: failed to compile %s! With GLSL: %s\n", desc, bd->GlslVersionString);
	if (log_length > 1)
	{
		ImVector<char> buf;
		buf.resize((int)(log_length + 1));
		MathAnim::GL::getShaderInfoLog(handle, log_length, nullptr, (GLchar*)buf.begin());
		fprintf(stderr, "%s\n", buf.begin());
	}
	return (GLboolean)status == GL_TRUE;
}

// If you get an error please report on GitHub. You may try different GL context version or GLSL version.
static bool CheckProgram(GLuint handle, const char* desc)
{
	MathAnim_ImplOpenGL3_Data* bd = MathAnim_ImplOpenGL3_GetBackendData();
	GLint status = 0, log_length = 0;
	MathAnim::GL::getProgramiv(handle, GL_LINK_STATUS, &status);
	MathAnim::GL::getProgramiv(handle, GL_INFO_LOG_LENGTH, &log_length);
	if ((GLboolean)status == GL_FALSE)
		fprintf(stderr, "ERROR: ImGui_ImplOpenGL3_CreateDeviceObjects: failed to link %s! With GLSL %s\n", desc, bd->GlslVersionString);
	if (log_length > 1)
	{
		ImVector<char> buf;
		buf.resize((int)(log_length + 1));
		MathAnim::GL::getProgramInfoLog(handle, log_length, nullptr, (GLchar*)buf.begin());
		fprintf(stderr, "%s\n", buf.begin());
	}
	return (GLboolean)status == GL_TRUE;
}

bool MathAnim_ImplOpenGL3_CreateDeviceObjects()
{
	MathAnim_ImplOpenGL3_Data* bd = MathAnim_ImplOpenGL3_GetBackendData();

	// Parse GLSL version string
	int glsl_version = 140;
	sscanf(bd->GlslVersionString, "#version %d", &glsl_version);

	const GLchar* vertex_shader_glsl_300_es =
		"precision highp float;\n"
		"layout (location = 0) in vec2 Position;\n"
		"layout (location = 1) in vec2 UV;\n"
		"layout (location = 2) in vec4 Color;\n"
		"uniform mat4 ProjMtx;\n"
		"out vec2 Frag_UV;\n"
		"out vec4 Frag_Color;\n"
		"void main()\n"
		"{\n"
		"    Frag_UV = UV;\n"
		"    Frag_Color = Color;\n"
		"    gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
		"}\n";

	const GLchar* vertex_shader_glsl_410_core =
		"layout (location = 0) in vec2 Position;\n"
		"layout (location = 1) in vec2 UV;\n"
		"layout (location = 2) in vec4 Color;\n"
		"uniform mat4 ProjMtx;\n"
		"out vec2 Frag_UV;\n"
		"out vec4 Frag_Color;\n"
		"void main()\n"
		"{\n"
		"    Frag_UV = UV;\n"
		"    Frag_Color = Color;\n"
		"    gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
		"}\n";

	const GLchar* fragment_shader_glsl_300_es =
		"precision mediump float;\n"
		"uniform sampler2D Texture;\n"
		"in vec2 Frag_UV;\n"
		"in vec4 Frag_Color;\n"
		"layout (location = 0) out vec4 Out_Color;\n"
		"void main()\n"
		"{\n"
		"    Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
		"}\n";

	const GLchar* fragment_shader_glsl_410_core =
		"in vec2 Frag_UV;\n"
		"in vec4 Frag_Color;\n"
		"uniform sampler2D Texture;\n"
		"layout (location = 0) out vec4 Out_Color;\n"
		"void main()\n"
		"{\n"
		"    Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
		"}\n";

	// Select shaders matching our GLSL versions
	const GLchar* vertex_shader = nullptr;
	const GLchar* fragment_shader = nullptr;
	if (glsl_version >= 410)
	{
		vertex_shader = vertex_shader_glsl_410_core;
		fragment_shader = fragment_shader_glsl_410_core;
	}
	else if (glsl_version >= 300)
	{
		vertex_shader = vertex_shader_glsl_300_es;
		fragment_shader = fragment_shader_glsl_300_es;
	}
	else
	{
		g_logger_assert(false, "Unsupported glsl version '%d'.", glsl_version);
	}

	// Create shaders
	const GLchar* vertex_shader_with_version[2] = { bd->GlslVersionString, vertex_shader };
	GLuint vert_handle = MathAnim::GL::createShader(GL_VERTEX_SHADER);
	MathAnim::GL::shaderSource(vert_handle, 2, vertex_shader_with_version, nullptr);
	MathAnim::GL::compileShader(vert_handle);
	CheckShader(vert_handle, "vertex shader");

	const GLchar* fragment_shader_with_version[2] = { bd->GlslVersionString, fragment_shader };
	GLuint frag_handle = MathAnim::GL::createShader(GL_FRAGMENT_SHADER);
	MathAnim::GL::shaderSource(frag_handle, 2, fragment_shader_with_version, nullptr);
	MathAnim::GL::compileShader(frag_handle);
	CheckShader(frag_handle, "fragment shader");

	// Link
	bd->ShaderHandle = MathAnim::GL::createProgram();
	MathAnim::GL::attachShader(bd->ShaderHandle, vert_handle);
	MathAnim::GL::attachShader(bd->ShaderHandle, frag_handle);
	MathAnim::GL::linkProgram(bd->ShaderHandle);
	CheckProgram(bd->ShaderHandle, "shader program");

	MathAnim::GL::detachShader(bd->ShaderHandle, vert_handle);
	MathAnim::GL::detachShader(bd->ShaderHandle, frag_handle);
	MathAnim::GL::deleteShader(vert_handle);
	MathAnim::GL::deleteShader(frag_handle);

	bd->AttribLocationTex = MathAnim::GL::getUniformLocation(bd->ShaderHandle, "Texture");
	bd->AttribLocationProjMtx = MathAnim::GL::getUniformLocation(bd->ShaderHandle, "ProjMtx");
	bd->AttribLocationVtxPos = (GLuint)MathAnim::GL::getAttribLocation(bd->ShaderHandle, "Position");
	bd->AttribLocationVtxUV = (GLuint)MathAnim::GL::getAttribLocation(bd->ShaderHandle, "UV");
	bd->AttribLocationVtxColor = (GLuint)MathAnim::GL::getAttribLocation(bd->ShaderHandle, "Color");

	// Create buffers
	MathAnim::GL::createVertexArray(&bd->VaoHandle);
	MathAnim::GL::genBuffers(1, &bd->VboHandle);
	MathAnim::GL::genBuffers(1, &bd->ElementsHandle);

	// Bind vertex/index buffers and setup attributes for ImDrawVert
	MathAnim::GL::bindVertexArray(bd->VaoHandle);
	MathAnim::GL::bindBuffer(GL_ARRAY_BUFFER, bd->VboHandle);
	MathAnim::GL::bindBuffer(GL_ELEMENT_ARRAY_BUFFER, bd->ElementsHandle);
	MathAnim::GL::enableVertexAttribArray(bd->AttribLocationVtxPos);
	MathAnim::GL::enableVertexAttribArray(bd->AttribLocationVtxUV);
	MathAnim::GL::enableVertexAttribArray(bd->AttribLocationVtxColor);
	MathAnim::GL::vertexAttribPointer(bd->AttribLocationVtxPos, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, pos));
	MathAnim::GL::vertexAttribPointer(bd->AttribLocationVtxUV, 2, GL_FLOAT, GL_FALSE, sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, uv));
	MathAnim::GL::vertexAttribPointer(bd->AttribLocationVtxColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, col));

	MathAnim_ImplOpenGL3_CreateFontsTexture();

	return true;
}

void MathAnim_ImplOpenGL3_DestroyDeviceObjects()
{
	MathAnim_ImplOpenGL3_Data* bd = MathAnim_ImplOpenGL3_GetBackendData();
	if (bd->VboHandle) { MathAnim::GL::deleteBuffers(1, &bd->VboHandle); bd->VboHandle = 0; }
	if (bd->ElementsHandle) { MathAnim::GL::deleteBuffers(1, &bd->ElementsHandle); bd->ElementsHandle = 0; }
	if (bd->ShaderHandle) { MathAnim::GL::deleteProgram(bd->ShaderHandle); bd->ShaderHandle = 0; }
	if (bd->VaoHandle) { MathAnim::GL::deleteVertexArrays(1, &bd->VaoHandle); bd->VaoHandle = 0; }
	MathAnim_ImplOpenGL3_DestroyFontsTexture();
}

//--------------------------------------------------------------------------------------------------------
// MULTI-VIEWPORT / PLATFORM INTERFACE SUPPORT
// This is an _advanced_ and _optional_ feature, allowing the backend to create and handle multiple viewports simultaneously.
// If you are new to dear imgui or creating a new binding for dear imgui, it is recommended that you completely ignore this section first..
//--------------------------------------------------------------------------------------------------------

static void MathAnim_ImplOpenGL3_RenderWindow(ImGuiViewport* viewport, void*)
{
	if (!(viewport->Flags & ImGuiViewportFlags_NoRendererClear))
	{
		ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
		MathAnim::GL::clearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
		MathAnim::GL::clear(GL_COLOR_BUFFER_BIT);
	}
	MathAnim_ImplOpenGL3_RenderDrawData(viewport->DrawData);
}

static void MathAnim_ImplOpenGL3_InitPlatformInterface()
{
	ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
	platform_io.Renderer_RenderWindow = MathAnim_ImplOpenGL3_RenderWindow;
}

static void MathAnim_ImplOpenGL3_ShutdownPlatformInterface()
{
	ImGui::DestroyPlatformWindows();
}