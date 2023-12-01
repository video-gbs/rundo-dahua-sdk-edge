#pragma once
#include <atomic>
#include <mutex>
#include <condition_variable>

class event_condition_variable {
public:
	explicit event_condition_variable(size_t initial = 0)
	{
		_count = 0;
	}

	~event_condition_variable() {}

	void signal(size_t n = 1)
	{
		std::unique_lock<std::recursive_mutex> lock(_mutex);
		_count += n;
		if (n == 1)
		{
			_condition.notify_one();
		}
		else
		{
			_condition.notify_all();
		}
	}

	void wait()
	{
		std::unique_lock<std::recursive_mutex> lock(_mutex);
		while (_count == 0)
		{
			_condition.wait(lock);
		}
		--_count;
	}

	void wait_for(size_t timeout)
	{
		std::unique_lock<std::recursive_mutex> lock(_mutex);
		while (_count == 0)
		{
			_condition.wait_for(lock, std::chrono::seconds(timeout));
			break;
		}
		_count = 0;
	}

private:

	size_t _count;
	std::recursive_mutex _mutex;
	std::condition_variable_any _condition;
};
