#ifndef MATH_ANIMATIONS_PROFILING_H
#define MATH_ANIMATIONS_PROFILING_H

#ifdef _PROFILER 
	#include <optick.h>

	#define MP_PROFILE_FRAME(name) OPTICK_FRAME(name)
	#define MP_PROFILE_EVENT(name) OPTICK_EVENT(name)
	#define MP_PROFILE_THREAD(name) OPTICK_THREAD(name)
	#define MP_PROFILE_DYNAMIC_EVENT(name) OPTICK_EVENT_DYNAMIC(name)
#else
	#define MP_PROFILE_FRAME(name)
	#define MP_PROFILE_EVENT(name)
	#define MP_PROFILE_THREAD(name)
	#define MP_PROFILE_DYNAMIC_EVENT(name)
#endif

#endif