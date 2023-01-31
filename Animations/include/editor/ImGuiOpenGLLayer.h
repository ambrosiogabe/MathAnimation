#ifndef MATH_ANIM_IMGUI_OPENGL_LAYER_H
#define MATH_ANIM_IMGUI_OPENGL_LAYER_H

struct ImDrawData;

// Backend API
bool MathAnim_ImplOpenGL3_Init(int glVersionMajor, int glVersionMinor, const char* glsl_version = nullptr);
void MathAnim_ImplOpenGL3_Shutdown();
void MathAnim_ImplOpenGL3_NewFrame();
void MathAnim_ImplOpenGL3_RenderDrawData(ImDrawData* draw_data);

// (Optional) Called by Init/NewFrame/Shutdown
bool MathAnim_ImplOpenGL3_CreateFontsTexture();
void MathAnim_ImplOpenGL3_DestroyFontsTexture();
bool MathAnim_ImplOpenGL3_CreateDeviceObjects();
void MathAnim_ImplOpenGL3_DestroyDeviceObjects();

#endif
