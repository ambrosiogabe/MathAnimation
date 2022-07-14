#ifndef MATH_ANIM_PROMISE_H
#define MATH_ANIM_PROMISE_H
#include "core.h"

// TODO: Needs some work
//namespace MathAnim
//{
//	template<typename T>
//	class Promise
//	{
//	public:
//		Promise(T& reference)
//			: dataRef(reference) {}
//
//		void resolve()
//		{
//			bool expected = isReadyAtomic;
//			while (!isReadyAtomic.compare_exchange_strong(expected, true))
//			{
//				expected = isReadyAtomic;
//			}
//		}
//
//		bool isReady()
//		{
//			return isReadyAtomic.load();
//		}
//
//	private:
//		T& dataRef;
//		std::atomic<bool> isReadyAtomic;
//	};
//}

#endif