#ifndef MATH_ANIM_WINDOW_H
#define MATH_ANIM_WINDOW_H
#include "core.h"

namespace MathAnim
{
	enum CursorMode
	{
		Hidden,
		Locked,
		Normal
	};

	enum WindowFlags : uint8
	{
		None,
		OpenMaximized = 0x1,
	};
    
	struct Window
	{
		int width;
		int height;
		const char* title;
		void* windowPtr;
        
		Window(int width, int height, const char* title, WindowFlags flags = WindowFlags::None);
        
		void makeContextCurrent();
        
		void pollInput();
        
		void swapBuffers();
        
		void update(float dt);
        
		void setCursorMode(CursorMode cursorMode);
        
		bool shouldClose();
        
		void setVSync(bool on);
        
		void setTitle(const std::string& newTitle);
        
		float getContentScale() const;

		void close();
        
        static glm::ivec2 getMonitorWorkingSize();
        
        static void cleanup();
	};
}

#endif
