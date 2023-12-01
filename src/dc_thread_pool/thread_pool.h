#pragma once
#include "thread_group.h"
#include "task_queue.h"

namespace dc_thread_pool
{
	class thread_pool
	{
	public:
		//num:线程池线程个数
		thread_pool(int num = 2) :_thread_num(num)
		{
			//创建线程池
			strat();
		}
		~thread_pool()
		{
			//清空任务队列
			//清空线程池
			stop();
		}

		using TaskIn = std::function<void()>;
		bool input_task(TaskIn task, bool may_sync = true)
		{
			if (may_sync && _thread_group.is_this_thread_in())
			{
				task();
				return true;
			}
			//插入任务队列中
			_queue.push_task(task);
			return true;
		}
		//最高优先级插入任务
		bool input_task_first(TaskIn task, bool may_sync = true)
		{
			if (may_sync)
			{
				task();
				return true;
			}
			//插入任务队列中
			_queue.push_task_first(task);
			return true;
		}

		size_t task_size()
		{
			return _queue.size();
		}

	private:
		void strat()
		{
			if (_thread_num <= 0)
				return;
			//创建线程
			size_t total = _thread_num - _thread_group.size();
			for (size_t i = 0; i < total; i++)
			{
				_thread_group.create_thread(std::bind(&thread_pool::run, this));
			}
		}

		void run()
		{
			TaskIn task;
			while (true)
			{
				//获取任务并执行，没有则break
				if (!_queue.get_task(task))
				{
					//空任务，退出线程
					break;
				}

				try
				{
					task();
					task = nullptr;
				}
				catch (std::exception& ex) {
					printf("ThreadPool执行任务捕获到异常:%s\n", ex.what());
				}
			}
		}

		void stop()
		{
			_thread_group.join_all();
			_queue.push_exit(_thread_num);
		}
	private:
		size_t _thread_num;
		//线程池
		thread_group _thread_group;
		//任务队列
		task_queue<TaskIn> _queue;
	};
}
