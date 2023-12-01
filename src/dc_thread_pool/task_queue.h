#pragma once
#include <atomic>
#include <mutex>
#include <functional>
#include "semaphore.h"
#include "List.h"

namespace dc_thread_pool
{
	template<typename T>
	class task_queue
	{
	public:
		task_queue() {}
		~task_queue() {}

		//打入任务至列队
		template<typename C>
		void push_task(C&& task_func) {
			{
				lock_guard<decltype(_mutex)> lock(_mutex);
				_queue.emplace_back(std::forward<C>(task_func));
			}
			_sem.post();
		}
		template<typename C>
		void push_task_first(C&& task_func) {
			{
				lock_guard<decltype(_mutex)> lock(_mutex);
				_queue.emplace_front(std::forward<C>(task_func));
			}
			_sem.post();
		}
		//清空任务列队
		void push_exit(size_t n) {
			_sem.post(n);
		}
		//从列队获取一个任务，由执行线程执行
		bool get_task(T& tsk) {
			_sem.wait();
			lock_guard<decltype(_mutex)> lock(_mutex);
			if (_queue.empty()) {
				return false;
			}
			//改成右值引用后性能提升了1倍多！
			tsk = std::move(_queue.front());
			_queue.pop_front();
			return true;
		}
		size_t size() const {
			lock_guard<decltype(_mutex)> lock(_mutex);
			return _queue.size();
		}
	private:
		dc_tool::List<T> _queue;
		mutable std::mutex _mutex;
		dc_tool::semaphore _sem;
	};
}