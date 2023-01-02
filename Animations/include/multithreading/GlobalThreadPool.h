#ifndef MATH_ANIM_GLOBAL_THREAD_POOL_H
#define MATH_ANIM_GLOBAL_THREAD_POOL_H
#include "core.h"

namespace MathAnim
{
	enum class Priority : uint8
	{
		High = 0,
		Medium,
		Low,
		None
	};

	typedef void (*TaskFunction)(void* data, size_t dataSize);
	typedef void (*ThreadCallback)(void* data, size_t dataSize);
	struct ThreadTask
	{
		TaskFunction fn;
		ThreadCallback callback;
		uint64 counter;
		void* data;
		size_t dataSize;
		Priority priority;
		const char* taskName;
	};

	struct CompareThreadTask
	{
		// Returning true means lesser priority
		bool operator()(const ThreadTask& a, const ThreadTask& b) const;
	};

	class GlobalThreadPool
	{
	public:
		GlobalThreadPool(uint32 numThreads);
#ifndef NDEBUG
		GlobalThreadPool(bool forceSynchronous);
#endif

		void free();

		void processLoop(uint32 threadIndex);
		void queueTask(
			TaskFunction function,
			const char* taskName = "Default",
			void* data = nullptr,
			size_t dataSize = 0,
			Priority priority = Priority::None,
			ThreadCallback callback = nullptr
		);
		void beginWork(bool notifyAll = true);

	private:
		std::priority_queue<ThreadTask, std::vector<ThreadTask>, CompareThreadTask>* tasks;
		std::thread* workerThreads;
		std::condition_variable* cv;
		std::mutex* generalMtx;
		std::mutex* queueMtx;
		bool doWork;
		uint32 numThreads;
#ifndef NDEBUG
		bool forceSynchronous;
#endif
	};
}

#endif 