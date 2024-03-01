#include "server_manage.h"
#include "mysql_manage.h"
#include "log.h"

extern dc_thread_pool::thread_pool sdk_thread;

server_manage::server_manage()
{
}

server_manage::~server_manage()
{
}

bool server_manage::start_server(std::string strIP_, int iPort_, std::string strUser_, std::string strPasswd_)
{
	strIP = strIP_;
	iPort = iPort_;
	strUser = strUser_;
	strPasswd = strPasswd_;
	_sdk.Init();

	//读取设备信息，本地文件方式
	init_platform();

	_client.init_client();

	_client.set_client_type(1);
	_client.connet_public(strIP, iPort, strUser, strPasswd);

	_client.send_public_queue(gm.gateway_sign_in());
	_client.send_public_queue(gm.gateway_heartbeat());

	_MQ_msg_thread = boost::make_shared<boost::thread>(boost::bind(&server_manage::MQ_msg_func, this));

	boost::this_thread::sleep_for(boost::chrono::milliseconds(1000));

	_handle_msg_thread = boost::make_shared<boost::thread>(boost::bind(&server_manage::handle_msg_func, this));

	//_device_upload_thread = boost::make_shared<boost::thread>(boost::bind(&server_manage::device_upload_func, this));

	//等待public连接与监听成功
	LOG_CONSOLE("等待public连接与监听");
	_client.wait_public();
	LOG_CONSOLE("等待public连接与监听成功");

	LOG_CONSOLE("等待business连接与监听");
	_client.wait_business();
	LOG_CONSOLE("等待business连接与监听成功");
	if (!platform_list.platforms.empty())
	{
		LOG_CONSOLE("更新设备状态,设备数量:%d", platform_list.platforms.size());
		for (const auto& platform : platform_list.platforms)
		{
			if (!platform.soft_del)
			{
				// Assuming deviceId is unique
				_platform_map[platform.deviceId] = platform;

				std::string build_json = gm.device_add(2, platform);//设置nvr设备状态
				_client.send_business_queueEx(build_json);
			}
		}
	}
	return true;
}

void server_manage::stop_server()
{
	if (_handle_msg_thread && _handle_msg_thread->joinable())
	{
		_handle_msg_thread->join();
	}
}

bool server_manage::insert_task(std::string msg)
{
	{
		boost::lock_guard<boost::mutex> lock(_msg_mux);
		_msg_list.push_back(msg);
	}
	_msg_ev.signal();
	return true;
}

bool server_manage::erase_task(std::string& msg)
{
	if (_msg_list.empty())
		_msg_ev.wait();

	{
		boost::lock_guard<boost::mutex> lock(_msg_mux);
		if (!_msg_list.empty())
		{
			msg = std::move(_msg_list[0]);  // Move the string to avoid unnecessary copy
			_msg_list.erase(_msg_list.begin());
			return true;
		}
	}
	return false;
}

bool server_manage::add_platform_for_local_file(const Platform& save)
{
	// 在 platform_list 中添加新平台
	platform_list.platforms.push_back(save);

	// 将更新后的 platform_list 写入本地文件
	writeJsonFile(filename, xpack::json::encode(platform_list));

	LOG_INFO("添加新平台成功");
	return true;
}

bool server_manage::del_platform_for_local_file(const Platform& del)
{
	// 在 platform_list 中查找并删除平台
	for (auto& platform : platform_list.platforms)
	{
		if (platform.deviceId == del.deviceId)
		{
			if (del.soft_del)
			{
				//软删除
				platform.soft_del = del.soft_del;
				LOG_INFO("软删除平台[%s]成功", del.deviceId.c_str());
			}
			else {
				//硬删除
				platform_list.platforms.erase(
					std::remove_if(platform_list.platforms.begin(), platform_list.platforms.end(),
						[&del](const Platform& p) { return p.deviceId == del.deviceId; }),
					platform_list.platforms.end());
				LOG_INFO("硬删除平台[%s]成功", del.deviceId.c_str());
			}

			LOG_CONSOLE("更新平台信息:[%s]", xpack::json::encode(platform_list).c_str());
			//将更新后的 platform_list 写入本地文件
			writeJsonFile(filename, xpack::json::encode(platform_list));

			return true;
		}
	}

	LOG_ERROR("未找到匹配的平台");
	return false;
}

bool server_manage::update_platform_for_local_file(const Platform& update)
{
	// 在 platform_list 中查找并更新平台
	for (auto& platform : platform_list.platforms)
	{
		if (platform.deviceId == update.deviceId)
		{
			if (update.soft_del)
			{
				//软删除
				platform.soft_del = update.soft_del;
				LOG_INFO("软删除平台[%s]成功", update.deviceId.c_str());
			}

			LOG_CONSOLE("更新平台信息:[%s]", xpack::json::encode(platform_list).c_str());
			// 将更新后的 platform_list 写入本地文件
			writeJsonFile(filename, xpack::json::encode(platform_list));

			return true;
		}
	}

	LOG_ERROR("未找到匹配的平台");
	return false;
}

bool server_manage::readJsonFile(const std::string& filename, std::string& jsonData)
{
	jsonData = "";  // 初始化 jsonData

	std::ifstream ifs(filename);
	if (!ifs.is_open())
	{
		// 文件不存在，创建并返回 false
		LOG_ERROR("无法打开文件：%s，即将创建文件", filename.c_str());

		ifs.close();  // 关闭文件句柄

		// 创建新文件
		std::ofstream ofs(filename, std::ios::trunc);
		if (!ofs.is_open())
		{
			LOG_ERROR("无法创建文件：%s", filename.c_str());
			return false;
		}

		return false;
	}

	// 文件存在，读取数据
	std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
	ifs.close();
	jsonData = content;
	return true;
}

bool server_manage::writeJsonFile(const std::string& filename, const std::string& jsonData)
{
	std::ofstream ofs(filename, std::ios::trunc); // 使用 std::ios::trunc 模式确保覆盖写入
	if (!ofs.is_open())
	{
		// 处理文件打开失败的情况
		LOG_ERROR("无法打开文件：%s 即将新建", filename.c_str());
		return false;
	}

	ofs << jsonData;
	ofs.close();
	return true;
}

void server_manage::MQ_msg_func()
{
	LOG_CONSOLE("创建接收消息线程");

	_client.start_public_queue();
}

void server_manage::handle_msg_func()
{
	LOG_CONSOLE("创建消息处理线程");

	while (true)
	{
		std::string msg_;

		if (erase_task(msg_))
		{
			// 获取请求
			sdk_thread.input_task([this, msg_]() {
				LoginResults Results;
				Platform platform;
				Platform temp_platform;
				Device dev;
				std::string device_id;
				std::string channel_id;
				std::string build_json;
				std::vector<Platform> platform_list_;
				std::vector<Device> device_list_;

				// 解析请求
				switch (gm.get_msg_type(msg_))
				{
				case msg_GATEWAY_SIGN_IN:
					gateway_sign_in(msg_);
					break;
				case msg_DEVICE_ADD:
					device_add(msg_);
					break;
				case msg_CHANNEL_SYNC:
					channel_sync(msg_);
					break;
				case msg_DEVICE_TOTAL_SYNC:
					device_total_sync(msg_);
					break;
				case msg_DEVICE_DELETE:
					device_delete(msg_);
					break;
				case msg_DEVICE_DELETE_SOFT:
					device_delete_soft(msg_);
					break;
				case msg_DEVICE_DELETE_RECOVER:
					device_delete_recover(msg_);
					break;
				case msg_CHANNEL_DELETE_SOFT:
					channel_delete_soft(msg_);
					break;
				case msg_CHANNEL_DELETE_RECOVER:
					channel_delete_recover(msg_);
					break;
				case msg_CHANNEL_PTZ_CONTROL:
					channel_ptz_control(msg_);
					break;
				case msg_CHANNEL_PTZ_PRESET:
					channel_ptz_preset(msg_);
					break;
				case msg_CHANNEL_3D_OPERATION:
					break;
				case msg_CHANNEL_PLAY:
					channel_play(msg_);
					break;
				case msg_CHANNEL_RECORD_INFO:
					channel_record_info(msg_);
					break;
				case msg_CHANNEL_PLAYBACK:
					channel_playback(msg_);
					break;
				case msg_CHANNEL_STOP_PLAY:
					channel_stop_play(msg_);
					break;
				case msg_DEVICE_RECORD_PAUSE:
					device_record_pause(msg_);
					break;
				case msg_DEVICE_RECORD_RESUME:
					device_record_resume(msg_);
					break;
				case msg_DEVICE_RECORD_SPEED:
					device_record_speed(msg_);
					break;
				default:
					break;
				}
				});
		}
	}
	LOG_CONSOLE("退出任务分发线程");
}

void server_manage::device_upload_func()
{
	LOG_CONSOLE("创建设备状态上报线程");
	while (true)
	{
		boost::this_thread::sleep_for(boost::chrono::milliseconds(1000 * 60 * 3));

		_client.check_channel_statu();

		if (!platform_list.platforms.empty())
		{
			LOG_CONSOLE("更新设备状态,设备数量:%d", platform_list.platforms.size());
			for (const auto& platform : platform_list.platforms)
			{
				if (!platform.soft_del)
				{
					// Assuming deviceId is unique
					_platform_map[platform.deviceId] = platform;

					std::string build_json = gm.device_add(2, platform);//设置nvr设备状态
					_client.send_business_queueEx(build_json);
				}
			}
		}
	}
}

bool server_manage::init_platform()
{
	std::string list_;
	readJsonFile(filename, list_);
	if (list_.empty())
	{
		LOG_ERROR("list_str is empty");
		return false;
	}
	xpack::json::decode(list_, platform_list);
	if (platform_list.platforms.empty())
	{
		LOG_ERROR("platforms is empty");
		return false;
	}
	for (const auto& platform : platform_list.platforms)
	{
		// Assuming deviceId is unique
		_platform_map[platform.deviceId] = platform;
	}

	LOG_CONSOLE("_platform_map size:%d", _platform_map.size());
	return true;
}

bool server_manage::get_platform(std::string k, Platform& p)
{
	boost::lock_guard<boost::mutex> lock(_platform_map_mux);
	auto it = _platform_map.find(k);
	if (it != _platform_map.end())
	{
		p = it->second;
		return true;
	}
	return false;
}

bool server_manage::add_platform(Platform& p)
{
	boost::lock_guard<boost::mutex> lock(_platform_map_mux);
	_platform_map.insert(std::make_pair(p.deviceId, p));
	add_platform_for_local_file(p);
	return true;
}

bool server_manage::del_platform(std::string k)
{
	boost::lock_guard<boost::mutex> lock(_platform_map_mux);
	auto it = _platform_map.find(k);
	if (it != _platform_map.end())
	{
		it->second.soft_del = true;
		update_platform_for_local_file(it->second);//该用哪个
		//del_platform_for_local_file(it->second);//该用哪个
		return true;
	}
	return false;
}

void server_manage::gateway_sign_in(std::string json)
{
}

void server_manage::channel_sync(std::string msg_)
{
	LoginResults Results;
	Platform platform;
	Platform temp_platform;
	std::string build_json;
	LOG_CONSOLE("开始处理通道同步业务");

	//解析出json中的channel_id
	gm.pars_device_sync(msg_, temp_platform);

	if (get_platform(temp_platform.deviceId, platform))
	{
		// 获取SDK登录句柄
		platform.login_results = _sdk.connet_equipment(platform.ip, platform.port, platform.username, platform.password, Results);
	}
	//mysql_manage_singleton::get_mutable_instance().Query_sql(dev, (char*)"SELECT * FROM rundo_device_channel WHERE channel_id = %s;", channel_id.c_str());

	//获取镜头列表
	std::vector<Device> temp_list;
	int ret = _sdk.get_camera_list(Results, temp_list);
	//for (int i = 0; i < temp_list.size(); i++)
	//{
	//	char sql[512] = { 0 };
	//	//sprintf_s(sql,"INSERT INTO rundo_device_channel (device_id, device_name, device_type, manufacturer, ip_address, port) VALUES(%d, '%s', 1, 'Manufacturer1', '192.168.1.1', 8080); ",);
	//	//mysql_manage_singleton::get_mutable_instance().Execute_sql(sql);
	//}

	//生成响应json
	build_json = gm.channel_sync(temp_list, temp_platform);

	//发布到业务队列
	_client.send_business_queueEx(build_json);
	LOG_CONSOLE("结束处理通道同步业务:msgID:%s", temp_platform.msgId.c_str());
}

void server_manage::device_add(std::string msg_)
{
	LoginResults Results;
	Platform platform;
	Platform temp_platform;
	std::string build_json;

	LOG_CONSOLE("开始处理设备添加业务");

	//解析请求
	gm.pars_device_add(msg_, platform);

	if (!platform.ip.empty())
	{
		//登陆SDK，管理并返回登录信息
		platform.login_results = _sdk.connet_equipment(platform.ip, platform.port, platform.username, platform.password, Results);
		if (platform.login_results != 0)
		{
			platform.login_err_msg = "login fail";
			//生成json 登陆结果
			LOG_CONSOLE("开始发送设备添加结果");
			build_json = gm.device_add(1, platform);
			//响应
			_client.send_business_queueEx(build_json);
		}
		else
		{
			//生成json 登陆结果
			LOG_CONSOLE("开始发送设备添加结果");
			build_json = gm.device_add(1, platform);
			//响应
			_client.send_business_queueEx(build_json);
			Sleep(1000);
			LOG_CONSOLE("开始发送设备状态");
			build_json = gm.device_add(2, platform);//设置nvr设备状态
			_client.send_business_queueEx(build_json);

			//添加设备map管理
			add_platform(platform);
		}
	}
	else
	{
		LOG_ERROR("Equipment IP is NULL");
	}
	LOG_CONSOLE("结束处理设备添加业务:msgID:%s", platform.msgId.c_str());
}

void server_manage::device_total_sync(std::string json)
{
	LOG_CONSOLE("开始处理设备全量同步业务");
	std::string temp_msgId;
	std::string build;
	gm.pars_device_total_sync(json, temp_msgId);
	build = gm.device_total_sync(platform_list.platforms, temp_msgId);
	_client.send_business_queueEx(build);

	//LoginResults Results;
	//Platform platform;
	//Platform temp_platform;
	//for (const auto& platform_ : platform_list.platforms)
	//{
	//	if (!platform_.soft_del)
	//	{
	//		if (get_platform(platform_.deviceId, platform))
	//		{
	//			// 获取SDK登录句柄
	//			platform.login_results = _sdk.connet_equipment(platform.ip, platform.port, platform.username, platform.password, Results);
	//		}
	//	}
	//}
	////获取镜头列表
	//std::vector<Device> temp_list;
	//int ret = _sdk.get_camera_list(Results, temp_list);
	////生成响应json
	//build = gm.channel_sync(temp_list, temp_platform);
	//_client.send_business_queueEx(build);

	LOG_CONSOLE("结束处理设备全量同步业务:msgId:%s", temp_msgId.c_str());
}

void server_manage::device_delete(std::string json)
{
	LOG_CONSOLE("开始处理设备删除业务");
	std::string build;
	Platform platform;

	gm.pars_device_delete_soft(json, platform);

	build = gm.response(platform.msgId, 0, "DEVICE_DELETE");
	_client.send_business_queueEx(build);
	LOG_CONSOLE("结束处理设备删除业务,msgId:", platform.msgId.c_str());
}

void server_manage::device_delete_soft(std::string msg_)
{
	Platform platform;
	std::string build;

	LOG_CONSOLE("开始处理设备软删除业务");
	gm.pars_device_delete_soft(msg_, platform);
	del_platform(platform.deviceId);

	build = gm.response(platform.msgId, 0, "DEVICE_DELETE_SOFT");
	_client.send_business_queueEx(build);
	LOG_CONSOLE("结束处理[%s]设备软删除业务,msgId:", platform.deviceId.c_str(), platform.msgId.c_str());
}

void server_manage::device_delete_recover(std::string json)
{
	LOG_CONSOLE("开始处理设备删除恢复业务");
	std::string build;
	Platform platform;

	gm.pars_device_delete_soft(json, platform);

	build = gm.response(platform.msgId, 0, "DEVICE_DELETE_RECOVER");
	_client.send_business_queueEx(build);
	LOG_CONSOLE("结束处理设备删除恢复业务,msgId:", platform.msgId.c_str());
}

void server_manage::channel_delete_soft(std::string json)
{
	LOG_CONSOLE("开始处理通道软删除业务");
	std::string build;
	Platform platform;

	gm.pars_device_delete_soft(json, platform);

	build = gm.response(platform.msgId, 0, "CHANNEL_DELETE_SOFT");
	_client.send_business_queueEx(build);
	LOG_CONSOLE("结束处理通道软删除业务,msgId:", platform.msgId.c_str());
}

void server_manage::channel_delete_recover(std::string json)
{
	LOG_CONSOLE("开始处理通道删除恢复业务");
	std::string build;
	Platform platform;

	gm.pars_device_delete_soft(json, platform);

	build = gm.response(platform.msgId, 0, "CHANNEL_DELETE_RECOVER");
	_client.send_business_queueEx(build);
	LOG_CONSOLE("结束处理通道删除恢复业务,msgId:", platform.msgId.c_str());
}

void server_manage::channel_ptz_control(std::string json)
{
	LoginResults Results;
	Platform platform;
	std::string build;
	PtzCMD ptz;
	LOG_CONSOLE("开始处理云台控制业务");
	//解析出json中的channel_id
	gm.pars_ptz_control(json, ptz);
	// 获取SDK登录句柄
	if (get_platform(ptz.device_id, platform))
	{
		// 获取SDK登录句柄
		platform.login_results = _sdk.connet_equipment(platform.ip, platform.port, platform.username, platform.password, Results);
	}
	// 处理相对应业务

	int val = _sdk.ptz_control(Results, ptz);
	build = gm.response(ptz.msgId, val, ptz.msgType);
	_client.send_business_queueEx(build);

	if (val != 0)
	{
		LOG_ERROR("处理[%d]云台控制业务失败[%d]:msgID:%s", ptz.channel, val, ptz.msgId.c_str());
		return;
	}
	LOG_CONSOLE("处理[%d]云台控制业务成功:msgID:%s", ptz.channel, ptz.msgId.c_str());
}

void server_manage::channel_ptz_preset(std::string json)
{
	LOG_CONSOLE("开始处理云台预置位业务");
	LoginResults Results;
	Platform platform;
	std::string build;

	Preset temp_;
	gm.pars_ptz_preset(json, temp_);
	// 获取SDK登录句柄
	if (get_platform(temp_.device_id, platform))
	{
		// 获取SDK登录句柄
		platform.login_results = _sdk.connet_equipment(platform.ip, platform.port, platform.username, platform.password, Results);
	}

	std::vector<Preset> temp_list;
	int val = _sdk.query_ptz_preset(Results, temp_.channel, temp_list);
	build = gm.ptz_preset_list(temp_, temp_list, val);
	_client.send_business_queueEx(build);

	if (val != 0)
	{
		LOG_ERROR("处理[%d]云台预置位业务失败[%d]:msgID:%s", temp_.channel, val, temp_.msgId.c_str());
		return;
	}
	LOG_CONSOLE("处理[%d]云台预置位业务成功:msgID:%s", temp_.channel, temp_.msgId.c_str());
}

void server_manage::channel_3d_operation(std::string json)
{//暂不做
	//LOG_CONSOLE("开始处理云台3D控制业务");
	LOG_CONSOLE("云台3D控制业务未开通");
	//LOG_CONSOLE("结束处理云台3D控制业务");
}

void server_manage::channel_play(std::string json)
{
	LOG_CONSOLE("开始处理实况播放业务");
	LoginResults Results;
	Platform platform;
	std::string build;
	std::string key;
	std::string addr;

	Record temp_record;
	Play temp_;
	gm.pars_channel_play(json, temp_);
	// 获取SDK登录句柄
	if (get_platform(temp_.device_id, platform))
	{
		// 获取SDK登录句柄
		platform.login_results = _sdk.connet_equipment(platform.ip, platform.port, platform.username, platform.password, Results);
	}

	int val = _sdk.start_real_stream(Results, temp_, MAIN_STREAM);
	//build = gm.response(temp_.msgId, val, temp_.msgType);
	//_client.send_business_queueEx(build);

	if (val != 0)
	{
		LOG_ERROR("处理[%d:%s]实况播放业务失败[%d]:msgID:%s，剩余会话:%d", temp_.channel, temp_.streamId.c_str(), val, temp_.msgId.c_str(), SessionMange_singleton::get_mutable_instance().GetPlaySessionCount());
		return;
	}
	LOG_CONSOLE("处理[%d:%s]实况播放业务成功:msgID:%s，剩余会话:%d", temp_.channel, temp_.streamId.c_str(), temp_.msgId.c_str(), SessionMange_singleton::get_mutable_instance().GetPlaySessionCount());
}

void server_manage::channel_record_info(std::string json)
{
	LOG_CONSOLE("开始处理录像列表业务");
	LoginResults Results;
	Platform platform;
	std::string build;

	std::vector<Record> list_;
	Record temp_;
	gm.pars_channel_record_info(json, temp_);
	// 获取SDK登录句柄
	if (get_platform(temp_.device_id, platform))
	{
		// 获取SDK登录句柄
		platform.login_results = _sdk.connet_equipment(platform.ip, platform.port, platform.username, platform.password, Results);
	}

	std::vector<Preset> temp_list;
	int val = _sdk.query_record_file(Results, temp_.channel, temp_.startTime, temp_.endTime, list_);
	build = gm.response_record_list(temp_.msgId, val, temp_.msgType, list_);
	_client.send_business_queueEx(build);

	if (val != 0)
	{
		LOG_ERROR("处理[%d]录像列表业务失败[%d]:msgID:%s", temp_.channel, val, temp_.msgId.c_str());
		return;
	}
	LOG_CONSOLE("处理[%d]录像列表业务成功:msgID:%s", temp_.channel, temp_.msgId.c_str());
}

void server_manage::channel_playback(std::string json)
{
	LOG_CONSOLE("开始处理录像回放业务");
	LoginResults Results;
	Platform platform;
	std::string build;

	Record temp_;
	gm.pars_channel_playback(json, temp_);
	// 获取SDK登录句柄
	if (get_platform(temp_.device_id, platform))
	{
		// 获取SDK登录句柄
		platform.login_results = _sdk.connet_equipment(platform.ip, platform.port, platform.username, platform.password, Results);
	}

	int val = _sdk.start_play_back(Results, temp_);
	if (val != 0)
	{
		LOG_ERROR("处理[%s:%s-%s]录像回放业务失败[%d]:msgID:%s，剩余会话:%d", temp_.streamId.c_str(), temp_.startTime.c_str(), temp_.endTime.c_str(), val, temp_.msgId.c_str(), SessionMange_singleton::get_mutable_instance().GetPlaySessionCount());
		return;
	}
	LOG_CONSOLE("处理[%s:%s-%s]录像回放业务成功:msgID:%s，剩余会话:%d", temp_.streamId.c_str(), temp_.startTime.c_str(), temp_.endTime.c_str(), temp_.msgId.c_str(), SessionMange_singleton::get_mutable_instance().GetPlaySessionCount());
	//需要处理响应
}

void server_manage::channel_stop_play(std::string json)
{
	LOG_CONSOLE("开始处理停止播放业务");
	LoginResults Results;
	Platform platform;
	std::string build;
	std::string key;

	gm.pars_channel_stop_play(json, key);
	int val = _sdk.stop_play(key);
	if (val != 0)
	{
		LOG_ERROR("处理[%s]停止播放业务失败，剩余会话:%d", key.c_str(), SessionMange_singleton::get_mutable_instance().GetPlaySessionCount());
		return;
	}
	LOG_CONSOLE("处理[%s]停止播放业务成功，剩余会话:%d", key.c_str(), SessionMange_singleton::get_mutable_instance().GetPlaySessionCount());
}

void server_manage::device_record_pause(std::string json)
{
	LOG_CONSOLE("开始处理回放暂停业务");
	LoginResults Results;
	Platform platform;
	std::string build;
	std::string streamID;

	gm.pars_streamId(json, streamID);

	int val = _sdk.device_record_pause(streamID);
	if (val != 0)
	{
		LOG_ERROR("处理[%s]回放暂停业务失败", streamID.c_str());
		return;
	}
	LOG_CONSOLE("处理[%s]回放暂停业务成功", streamID.c_str());
}

void server_manage::device_record_resume(std::string json)
{
	LOG_CONSOLE("开始处理回放暂停恢复业务");
	LoginResults Results;
	Platform platform;
	std::string build;
	std::string streamID;

	gm.pars_streamId(json, streamID);

	int val = _sdk.device_record_resume(streamID);
	if (val != 0)
	{
		LOG_ERROR("处理[%s]回放暂停恢复业务失败", streamID.c_str());
		return;
	}
	LOG_CONSOLE("处理[%s]回放暂停恢复业务成功", streamID.c_str());
}

void server_manage::device_record_speed(std::string json)
{
	LOG_CONSOLE("开始处理设置回放倍速业务");
	LoginResults Results;
	Platform platform;
	std::string build;
	std::string streamID;
	float speed;

	gm.pars_record_speed(json, speed, streamID);

	int val = _sdk.device_record_speed(streamID, speed);
	if (val != 0)
	{
		LOG_ERROR("处理[%s]设置回放倍速业务失败", streamID.c_str());
		return;
	}
	LOG_CONSOLE("处理[%s]设置回放倍速业务成功,speed:%f", streamID.c_str(), speed);
}