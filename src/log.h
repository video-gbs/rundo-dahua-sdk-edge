#ifndef _LOG_H_
#define _LOG_H_

#define LOG(level, format, ...)(write_log(level, "[%s:%d:%s]: "##format, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__))
#define LOG_TRACE(format, ...) (LOG(ns_server_base::e_trace_log, format, ##__VA_ARGS__))
#define LOG_DEBUG(format, ...) (LOG(ns_server_base::e_debug_log, format, ##__VA_ARGS__))
#define LOG_INFO(format, ...) (LOG(ns_server_base::e_info_log, format, ##__VA_ARGS__))
#define LOG_CONSOLE(format, ...) (LOG(ns_server_base::e_console_log, format, ##__VA_ARGS__))
#define LOG_WARNING(format, ...) (LOG(ns_server_base::e_warning_log, format, ##__VA_ARGS__))
#define LOG_ERROR(format, ...) (LOG(ns_server_base::e_error_log, format, ##__VA_ARGS__))
#define LOG_FATAL(format, ...) (LOG(ns_server_base::e_fatal_log, format, ##__VA_ARGS__))

namespace ns_server_base
{
	enum e_custom_severity_level
	{
		e_trace_log = 1,//��Դ��־,�������
		e_debug_log,	//������־
		e_info_log,		//������־
		e_console_log,	//����̨��־
		e_warning_log,	//������־
		e_error_log,	//������־
		e_fatal_log,	//������־
	};

	bool init_log(const char* filename, e_custom_severity_level level, bool console);
	bool write_log(e_custom_severity_level level, const char* fmt, ...);
	void flush_log();
	void deinit_log();
}

#endif