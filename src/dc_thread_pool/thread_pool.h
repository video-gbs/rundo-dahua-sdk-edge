#pragma once
#include "thread_group.h"
#include "task_queue.h"

namespace dc_thread_pool
{
	class thread_pool
	{
	public:
		//num:�̳߳��̸߳���
		thread_pool(int num = 2) :_thread_num(num)
		{
			//�����̳߳�
			strat();
		}
		~thread_pool()
		{
			//����������
			//����̳߳�
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
			//�������������
			_queue.push_task(task);
			return true;
		}
		//������ȼ���������
		bool input_task_first(TaskIn task, bool may_sync = true)
		{
			if (may_sync)
			{
				task();
				return true;
			}
			//�������������
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
			//�����߳�
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
				//��ȡ����ִ�У�û����break
				if (!_queue.get_task(task))
				{
					//�������˳��߳�
					break;
				}

				try
				{
					task();
					task = nullptr;
				}
				catch (std::exception& ex) {
					printf("ThreadPoolִ�����񲶻��쳣:%s\n", ex.what());
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
		//�̳߳�
		thread_group _thread_group;
		//�������
		task_queue<TaskIn> _queue;
	};
}
