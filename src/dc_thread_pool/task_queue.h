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

		//�����������ж�
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
		//��������ж�
		void push_exit(size_t n) {
			_sem.post(n);
		}
		//���жӻ�ȡһ��������ִ���߳�ִ��
		bool get_task(T& tsk) {
			_sem.wait();
			lock_guard<decltype(_mutex)> lock(_mutex);
			if (_queue.empty()) {
				return false;
			}
			//�ĳ���ֵ���ú�����������1���࣡
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