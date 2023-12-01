#include <sstream>
#include <algorithm>
#include <fstream>
#include <cctype>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>
#include "log.h"
//#include "auto_lock.h"

#ifdef _WIN32
#pragma warning(disable: 4996)
#endif

namespace ns_server_base
{
	typedef boost::log::sinks::synchronous_sink<boost::log::sinks::text_ostream_backend> text_ostream_sink;
	typedef boost::log::sinks::synchronous_sink<boost::log::sinks::text_file_backend> text_file_backend;

	boost::log::sources::severity_logger_mt<e_custom_severity_level> _lg_mt;
	boost::shared_ptr<text_file_backend> _file_sink;
	boost::shared_ptr<text_ostream_sink> _console_sink;

	bool init_log(const char* filename, e_custom_severity_level level, bool console)
	{
		if (filename && (0 != strcmp("", filename)))
		{
			//add common attribute
			boost::log::add_common_attributes();

			//add file log
			std::string strfile(filename);
			std::transform(strfile.begin(), strfile.end(), strfile.begin(), ::tolower);
			if (!strfile.empty())
			{
				std::stringstream ss;

				std::string::size_type pos = strfile.find(".log");
				if (std::string::npos == pos)
				{
					ss << strfile << "_%N.log";
				}
				else
				{
					ss << strfile.substr(0, pos) << "_%N.log";
				}

				_file_sink = boost::log::add_file_log
				(
					boost::log::keywords::file_name = ss.str().c_str(),
					boost::log::keywords::rotation_size = 50 * 1024 * 1024,
					boost::log::keywords::time_based_rotation = boost::log::sinks::file::rotation_at_time_point(0, 0, 0),
					boost::log::keywords::auto_flush = true,
					boost::log::keywords::max_files = 31
				);
				if (_file_sink)
				{
					//set filter
					_file_sink->set_filter(boost::log::expressions::attr<e_custom_severity_level>("Severity") >= level);

					//set formatter
					_file_sink->set_formatter
					(
						/*[YYYY-MM-DD HH:MI:SS:<severity_level>(threadid)]message*/
						boost::log::expressions::format("[%1%:<%2%>(%3%)]%4%")
						% boost::log::expressions::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S.%f")
						% boost::log::expressions::attr<e_custom_severity_level>("Severity")
						% boost::log::expressions::attr<boost::log::attributes::current_thread_id::value_type>("ThreadID")
						% boost::log::expressions::smessage
					);

					//set file collector
					if (true)
					{
						_file_sink->locked_backend()->set_file_collector(boost::log::sinks::file::make_collector(
							boost::log::keywords::target = "log"
						));
					}
					else
					{
						//日志回滚
						_file_sink->locked_backend()->set_file_collector(boost::log::sinks::file::make_collector(
							boost::log::keywords::target = "log",						//目标文件夹
							boost::log::keywords::max_size = 100 * 1024 * 1024,			//所有日志加起来的最大大小 100M,
							boost::log::keywords::min_free_space = 1000 * 1024 * 1024	//最低磁盘空间限制 100M
						));
					}

					//scan for files
					_file_sink->locked_backend()->scan_for_files();
				}

				//add console
				if (console)
				{
					_console_sink = boost::log::add_console_log();
					if (_console_sink)
					{
						//set filter
						_console_sink->set_filter(boost::log::expressions::attr<e_custom_severity_level>("Severity") >= e_console_log);

						//set formatter
						_console_sink->set_formatter
						(
							/*[YYYY-MM-DD HH:MI:SS:<severity_level>(threadid)]message*/
							boost::log::expressions::format("[%1%:<%2%>(%3%)]%4%")
							% boost::log::expressions::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S")
							% boost::log::expressions::attr<e_custom_severity_level>("Severity")
							% boost::log::expressions::attr<boost::log::attributes::current_thread_id::value_type>("ThreadID")
							% boost::log::expressions::smessage
						);

						//auto flush
						_console_sink->locked_backend()->auto_flush();
					}
				}

				return true;
			}
		}

		return false;
	}

	//#ifdef USE_CORE_SYNC_MUTEX
	//	auto_lock::al_mutex _log_mtx;
	//#else
	//	auto_lock::al_spin _log_mtx;
	//#endif

	boost::mutex log_mtx;

	char _log_buffer[1 * 1024 * 1024] = { 0 };
	bool write_log(e_custom_severity_level level, const char* fmt, ...)
	{
		if (fmt && (0 != strcmp("", fmt)))
		{
			try
			{
				//#ifdef USE_CORE_SYNC_MUTEX
				//				auto_lock::al_lock<auto_lock::al_mutex> al(_log_mtx);
				//#else
				//				auto_lock::al_lock<auto_lock::al_spin> al(_log_mtx);
				//#endif
				boost::lock_guard<boost::mutex> lg(log_mtx);

				va_list vl;
				va_start(vl, fmt);
				vsprintf(_log_buffer, fmt, vl);
				va_end(vl);

				BOOST_LOG_SEV(_lg_mt, level) << _log_buffer;

				return true;
			}
			catch (boost::lock_error& e)
			{
				(void)e;
			}
			catch (...)
			{
			}
		}

		return false;
	}

	void flush_log()
	{
		if (_file_sink)
		{
			_file_sink->flush();
		}

		if (_console_sink)
		{
			_console_sink->flush();
		}
	}

	void deinit_log()
	{
		write_log(e_info_log, "[%s:%d]: start running deinit_log function", __FILE__, __LINE__);

		flush_log();

		if (_file_sink)
		{
			boost::log::core::get()->remove_sink(_file_sink);
			_file_sink.reset();
		}

		if (_console_sink)
		{
			boost::log::core::get()->remove_sink(_console_sink);
			_console_sink.reset();
		}
	}
}