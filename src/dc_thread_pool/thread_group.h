#pragma once
#include <unordered_map>
#include <functional>
#include <thread>

namespace dc_thread_pool
{
	class thread_group
	{
	public:
		thread_group()
		{
		}
		~thread_group()
		{
			_threads.clear();
		}

		template<typename F>
		std::thread* create_thread(F&& threadfunc)
		{
			auto thread_new = std::make_shared<std::thread>(threadfunc);
			_thread_id = thread_new->get_id();
			_threads.insert(std::make_pair(_thread_id, thread_new));
			printf("创建线程ID%d\n", _thread_id);
			//返回智能指针中的原始指针(线程指针)
			return thread_new.get();
		}

		bool is_this_thread_in() {
			auto thread_id = std::this_thread::get_id();
			printf("_thread_id %d thread_id %d\n", _thread_id, thread_id);
			if (_thread_id == thread_id) {
				return true;
			}
			return _threads.find(thread_id) != _threads.end();
		}

		size_t size()
		{
			return _threads.size();
		}

		void join_all()
		{
			for (auto& it : _threads) {
				if (it.second->joinable()) {
					it.second->join(); //等待线程主动退出
				}
			}
			_threads.clear();
		}

	private:
		std::unordered_map<std::thread::id, std::shared_ptr<std::thread> > _threads;
		std::thread::id _thread_id;
	};
}