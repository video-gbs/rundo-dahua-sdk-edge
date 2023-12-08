#pragma once
#include <string>
#include <boost/serialization/singleton.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread.hpp>
#include <vector>
#include "event_condition_variable.h"
#include "client_manage.h"
#include "sdk_wrap.h"
#include "dc_thread_pool/thread_pool.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/ostreamwrapper.h"

class server_manage :public boost::noncopyable
{
public:
	server_manage();
	~server_manage();

	bool start_server(std::string ip, int port, std::string user, std::string pws);
	void stop_server();

	bool insert_task(std::string msg);
	bool erase_task(std::string& msg);

private:
	//void createEmptyPlatformsFile(const std::string& filename);
	bool readJsonFile(const std::string& filename, std::string& jsonData);
	bool writeJsonFile(const std::string& filename, const std::string& jsonData);

	bool add_platform_for_local_file(const Platform& save);
	bool del_platform_for_local_file(const Platform& del);
	bool update_platform_for_local_file(const Platform& update);

	void MQ_msg_func();
	void handle_msg_func();
	void device_upload_func();

	bool init_platform();
	bool get_platform(std::string k, Platform& p);
	bool add_platform(Platform& p);
	bool del_platform(std::string k);

	void gateway_sign_in(std::string json);
	void channel_sync(std::string json);
	void device_add(std::string json);
	void device_total_sync(std::string json);
	void device_delete(std::string json);
	void device_delete_soft(std::string json);
	void device_delete_recover(std::string json);
	void channel_delete_soft(std::string json);
	void channel_delete_recover(std::string json);
	void channel_ptz_control(std::string json);
	//获取预置位列表
	void channel_ptz_preset(std::string json);
	void channel_3d_operation(std::string json);
	void channel_play(std::string json);
	//获取路线列表
	void channel_record_info(std::string json);
	void channel_playback(std::string json);
	//回放实况通用消息
	void channel_stop_play(std::string json);
	void device_record_pause(std::string json);
	void device_record_resume(std::string json);
	void device_record_speed(std::string json);

	event_condition_variable _msg_ev;
	boost::mutex _msg_mux;
	std::vector<std::string> _msg_list;

	boost::shared_ptr<boost::thread>	_device_upload_thread;//
	boost::shared_ptr<boost::thread>	_handle_msg_thread;//
	boost::shared_ptr<boost::thread>	_MQ_msg_thread;//
	client_manage _client;
	sdk_wrap _sdk;
	gateway_msg gm;

	boost::mutex _platform_map_mux;
	boost::unordered_map<std::string, Platform> _platform_map;
	Platforms platform_list;
	std::string filename = "platforms.json";

	boost::mutex _device_map_mux;
	boost::unordered_map<std::string, Device> _device_map;

	//MQ
	std::string strIP;
	int iPort;
	std::string strUser;
	std::string strPasswd;

	std::string temp_channel_list;
	std::string temp_msgId;
	int temp_size;
};
typedef boost::serialization::singleton<server_manage> server_manage_singleton;
