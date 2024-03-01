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

	//��ȡ�豸��Ϣ�������ļ���ʽ
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

	//�ȴ�public����������ɹ�
	LOG_CONSOLE("�ȴ�public���������");
	_client.wait_public();
	LOG_CONSOLE("�ȴ�public����������ɹ�");

	LOG_CONSOLE("�ȴ�business���������");
	_client.wait_business();
	LOG_CONSOLE("�ȴ�business����������ɹ�");
	if (!platform_list.platforms.empty())
	{
		LOG_CONSOLE("�����豸״̬,�豸����:%d", platform_list.platforms.size());
		for (const auto& platform : platform_list.platforms)
		{
			if (!platform.soft_del)
			{
				// Assuming deviceId is unique
				_platform_map[platform.deviceId] = platform;

				std::string build_json = gm.device_add(2, platform);//����nvr�豸״̬
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
	// �� platform_list �������ƽ̨
	platform_list.platforms.push_back(save);

	// �����º�� platform_list д�뱾���ļ�
	writeJsonFile(filename, xpack::json::encode(platform_list));

	LOG_INFO("�����ƽ̨�ɹ�");
	return true;
}

bool server_manage::del_platform_for_local_file(const Platform& del)
{
	// �� platform_list �в��Ҳ�ɾ��ƽ̨
	for (auto& platform : platform_list.platforms)
	{
		if (platform.deviceId == del.deviceId)
		{
			if (del.soft_del)
			{
				//��ɾ��
				platform.soft_del = del.soft_del;
				LOG_INFO("��ɾ��ƽ̨[%s]�ɹ�", del.deviceId.c_str());
			}
			else {
				//Ӳɾ��
				platform_list.platforms.erase(
					std::remove_if(platform_list.platforms.begin(), platform_list.platforms.end(),
						[&del](const Platform& p) { return p.deviceId == del.deviceId; }),
					platform_list.platforms.end());
				LOG_INFO("Ӳɾ��ƽ̨[%s]�ɹ�", del.deviceId.c_str());
			}

			LOG_CONSOLE("����ƽ̨��Ϣ:[%s]", xpack::json::encode(platform_list).c_str());
			//�����º�� platform_list д�뱾���ļ�
			writeJsonFile(filename, xpack::json::encode(platform_list));

			return true;
		}
	}

	LOG_ERROR("δ�ҵ�ƥ���ƽ̨");
	return false;
}

bool server_manage::update_platform_for_local_file(const Platform& update)
{
	// �� platform_list �в��Ҳ�����ƽ̨
	for (auto& platform : platform_list.platforms)
	{
		if (platform.deviceId == update.deviceId)
		{
			if (update.soft_del)
			{
				//��ɾ��
				platform.soft_del = update.soft_del;
				LOG_INFO("��ɾ��ƽ̨[%s]�ɹ�", update.deviceId.c_str());
			}

			LOG_CONSOLE("����ƽ̨��Ϣ:[%s]", xpack::json::encode(platform_list).c_str());
			// �����º�� platform_list д�뱾���ļ�
			writeJsonFile(filename, xpack::json::encode(platform_list));

			return true;
		}
	}

	LOG_ERROR("δ�ҵ�ƥ���ƽ̨");
	return false;
}

bool server_manage::readJsonFile(const std::string& filename, std::string& jsonData)
{
	jsonData = "";  // ��ʼ�� jsonData

	std::ifstream ifs(filename);
	if (!ifs.is_open())
	{
		// �ļ������ڣ����������� false
		LOG_ERROR("�޷����ļ���%s�����������ļ�", filename.c_str());

		ifs.close();  // �ر��ļ����

		// �������ļ�
		std::ofstream ofs(filename, std::ios::trunc);
		if (!ofs.is_open())
		{
			LOG_ERROR("�޷������ļ���%s", filename.c_str());
			return false;
		}

		return false;
	}

	// �ļ����ڣ���ȡ����
	std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
	ifs.close();
	jsonData = content;
	return true;
}

bool server_manage::writeJsonFile(const std::string& filename, const std::string& jsonData)
{
	std::ofstream ofs(filename, std::ios::trunc); // ʹ�� std::ios::trunc ģʽȷ������д��
	if (!ofs.is_open())
	{
		// �����ļ���ʧ�ܵ����
		LOG_ERROR("�޷����ļ���%s �����½�", filename.c_str());
		return false;
	}

	ofs << jsonData;
	ofs.close();
	return true;
}

void server_manage::MQ_msg_func()
{
	LOG_CONSOLE("����������Ϣ�߳�");

	_client.start_public_queue();
}

void server_manage::handle_msg_func()
{
	LOG_CONSOLE("������Ϣ�����߳�");

	while (true)
	{
		std::string msg_;

		if (erase_task(msg_))
		{
			// ��ȡ����
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

				// ��������
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
	LOG_CONSOLE("�˳�����ַ��߳�");
}

void server_manage::device_upload_func()
{
	LOG_CONSOLE("�����豸״̬�ϱ��߳�");
	while (true)
	{
		boost::this_thread::sleep_for(boost::chrono::milliseconds(1000 * 60 * 3));

		_client.check_channel_statu();

		if (!platform_list.platforms.empty())
		{
			LOG_CONSOLE("�����豸״̬,�豸����:%d", platform_list.platforms.size());
			for (const auto& platform : platform_list.platforms)
			{
				if (!platform.soft_del)
				{
					// Assuming deviceId is unique
					_platform_map[platform.deviceId] = platform;

					std::string build_json = gm.device_add(2, platform);//����nvr�豸״̬
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
		update_platform_for_local_file(it->second);//�����ĸ�
		//del_platform_for_local_file(it->second);//�����ĸ�
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
	LOG_CONSOLE("��ʼ����ͨ��ͬ��ҵ��");

	//������json�е�channel_id
	gm.pars_device_sync(msg_, temp_platform);

	if (get_platform(temp_platform.deviceId, platform))
	{
		// ��ȡSDK��¼���
		platform.login_results = _sdk.connet_equipment(platform.ip, platform.port, platform.username, platform.password, Results);
	}
	//mysql_manage_singleton::get_mutable_instance().Query_sql(dev, (char*)"SELECT * FROM rundo_device_channel WHERE channel_id = %s;", channel_id.c_str());

	//��ȡ��ͷ�б�
	std::vector<Device> temp_list;
	int ret = _sdk.get_camera_list(Results, temp_list);
	//for (int i = 0; i < temp_list.size(); i++)
	//{
	//	char sql[512] = { 0 };
	//	//sprintf_s(sql,"INSERT INTO rundo_device_channel (device_id, device_name, device_type, manufacturer, ip_address, port) VALUES(%d, '%s', 1, 'Manufacturer1', '192.168.1.1', 8080); ",);
	//	//mysql_manage_singleton::get_mutable_instance().Execute_sql(sql);
	//}

	//������Ӧjson
	build_json = gm.channel_sync(temp_list, temp_platform);

	//������ҵ�����
	_client.send_business_queueEx(build_json);
	LOG_CONSOLE("��������ͨ��ͬ��ҵ��:msgID:%s", temp_platform.msgId.c_str());
}

void server_manage::device_add(std::string msg_)
{
	LoginResults Results;
	Platform platform;
	Platform temp_platform;
	std::string build_json;

	LOG_CONSOLE("��ʼ�����豸���ҵ��");

	//��������
	gm.pars_device_add(msg_, platform);

	if (!platform.ip.empty())
	{
		//��½SDK���������ص�¼��Ϣ
		platform.login_results = _sdk.connet_equipment(platform.ip, platform.port, platform.username, platform.password, Results);
		if (platform.login_results != 0)
		{
			platform.login_err_msg = "login fail";
			//����json ��½���
			LOG_CONSOLE("��ʼ�����豸��ӽ��");
			build_json = gm.device_add(1, platform);
			//��Ӧ
			_client.send_business_queueEx(build_json);
		}
		else
		{
			//����json ��½���
			LOG_CONSOLE("��ʼ�����豸��ӽ��");
			build_json = gm.device_add(1, platform);
			//��Ӧ
			_client.send_business_queueEx(build_json);
			Sleep(1000);
			LOG_CONSOLE("��ʼ�����豸״̬");
			build_json = gm.device_add(2, platform);//����nvr�豸״̬
			_client.send_business_queueEx(build_json);

			//����豸map����
			add_platform(platform);
		}
	}
	else
	{
		LOG_ERROR("Equipment IP is NULL");
	}
	LOG_CONSOLE("���������豸���ҵ��:msgID:%s", platform.msgId.c_str());
}

void server_manage::device_total_sync(std::string json)
{
	LOG_CONSOLE("��ʼ�����豸ȫ��ͬ��ҵ��");
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
	//			// ��ȡSDK��¼���
	//			platform.login_results = _sdk.connet_equipment(platform.ip, platform.port, platform.username, platform.password, Results);
	//		}
	//	}
	//}
	////��ȡ��ͷ�б�
	//std::vector<Device> temp_list;
	//int ret = _sdk.get_camera_list(Results, temp_list);
	////������Ӧjson
	//build = gm.channel_sync(temp_list, temp_platform);
	//_client.send_business_queueEx(build);

	LOG_CONSOLE("���������豸ȫ��ͬ��ҵ��:msgId:%s", temp_msgId.c_str());
}

void server_manage::device_delete(std::string json)
{
	LOG_CONSOLE("��ʼ�����豸ɾ��ҵ��");
	std::string build;
	Platform platform;

	gm.pars_device_delete_soft(json, platform);

	build = gm.response(platform.msgId, 0, "DEVICE_DELETE");
	_client.send_business_queueEx(build);
	LOG_CONSOLE("���������豸ɾ��ҵ��,msgId:", platform.msgId.c_str());
}

void server_manage::device_delete_soft(std::string msg_)
{
	Platform platform;
	std::string build;

	LOG_CONSOLE("��ʼ�����豸��ɾ��ҵ��");
	gm.pars_device_delete_soft(msg_, platform);
	del_platform(platform.deviceId);

	build = gm.response(platform.msgId, 0, "DEVICE_DELETE_SOFT");
	_client.send_business_queueEx(build);
	LOG_CONSOLE("��������[%s]�豸��ɾ��ҵ��,msgId:", platform.deviceId.c_str(), platform.msgId.c_str());
}

void server_manage::device_delete_recover(std::string json)
{
	LOG_CONSOLE("��ʼ�����豸ɾ���ָ�ҵ��");
	std::string build;
	Platform platform;

	gm.pars_device_delete_soft(json, platform);

	build = gm.response(platform.msgId, 0, "DEVICE_DELETE_RECOVER");
	_client.send_business_queueEx(build);
	LOG_CONSOLE("���������豸ɾ���ָ�ҵ��,msgId:", platform.msgId.c_str());
}

void server_manage::channel_delete_soft(std::string json)
{
	LOG_CONSOLE("��ʼ����ͨ����ɾ��ҵ��");
	std::string build;
	Platform platform;

	gm.pars_device_delete_soft(json, platform);

	build = gm.response(platform.msgId, 0, "CHANNEL_DELETE_SOFT");
	_client.send_business_queueEx(build);
	LOG_CONSOLE("��������ͨ����ɾ��ҵ��,msgId:", platform.msgId.c_str());
}

void server_manage::channel_delete_recover(std::string json)
{
	LOG_CONSOLE("��ʼ����ͨ��ɾ���ָ�ҵ��");
	std::string build;
	Platform platform;

	gm.pars_device_delete_soft(json, platform);

	build = gm.response(platform.msgId, 0, "CHANNEL_DELETE_RECOVER");
	_client.send_business_queueEx(build);
	LOG_CONSOLE("��������ͨ��ɾ���ָ�ҵ��,msgId:", platform.msgId.c_str());
}

void server_manage::channel_ptz_control(std::string json)
{
	LoginResults Results;
	Platform platform;
	std::string build;
	PtzCMD ptz;
	LOG_CONSOLE("��ʼ������̨����ҵ��");
	//������json�е�channel_id
	gm.pars_ptz_control(json, ptz);
	// ��ȡSDK��¼���
	if (get_platform(ptz.device_id, platform))
	{
		// ��ȡSDK��¼���
		platform.login_results = _sdk.connet_equipment(platform.ip, platform.port, platform.username, platform.password, Results);
	}
	// �������Ӧҵ��

	int val = _sdk.ptz_control(Results, ptz);
	build = gm.response(ptz.msgId, val, ptz.msgType);
	_client.send_business_queueEx(build);

	if (val != 0)
	{
		LOG_ERROR("����[%d]��̨����ҵ��ʧ��[%d]:msgID:%s", ptz.channel, val, ptz.msgId.c_str());
		return;
	}
	LOG_CONSOLE("����[%d]��̨����ҵ��ɹ�:msgID:%s", ptz.channel, ptz.msgId.c_str());
}

void server_manage::channel_ptz_preset(std::string json)
{
	LOG_CONSOLE("��ʼ������̨Ԥ��λҵ��");
	LoginResults Results;
	Platform platform;
	std::string build;

	Preset temp_;
	gm.pars_ptz_preset(json, temp_);
	// ��ȡSDK��¼���
	if (get_platform(temp_.device_id, platform))
	{
		// ��ȡSDK��¼���
		platform.login_results = _sdk.connet_equipment(platform.ip, platform.port, platform.username, platform.password, Results);
	}

	std::vector<Preset> temp_list;
	int val = _sdk.query_ptz_preset(Results, temp_.channel, temp_list);
	build = gm.ptz_preset_list(temp_, temp_list, val);
	_client.send_business_queueEx(build);

	if (val != 0)
	{
		LOG_ERROR("����[%d]��̨Ԥ��λҵ��ʧ��[%d]:msgID:%s", temp_.channel, val, temp_.msgId.c_str());
		return;
	}
	LOG_CONSOLE("����[%d]��̨Ԥ��λҵ��ɹ�:msgID:%s", temp_.channel, temp_.msgId.c_str());
}

void server_manage::channel_3d_operation(std::string json)
{//�ݲ���
	//LOG_CONSOLE("��ʼ������̨3D����ҵ��");
	LOG_CONSOLE("��̨3D����ҵ��δ��ͨ");
	//LOG_CONSOLE("����������̨3D����ҵ��");
}

void server_manage::channel_play(std::string json)
{
	LOG_CONSOLE("��ʼ����ʵ������ҵ��");
	LoginResults Results;
	Platform platform;
	std::string build;
	std::string key;
	std::string addr;

	Record temp_record;
	Play temp_;
	gm.pars_channel_play(json, temp_);
	// ��ȡSDK��¼���
	if (get_platform(temp_.device_id, platform))
	{
		// ��ȡSDK��¼���
		platform.login_results = _sdk.connet_equipment(platform.ip, platform.port, platform.username, platform.password, Results);
	}

	int val = _sdk.start_real_stream(Results, temp_, MAIN_STREAM);
	//build = gm.response(temp_.msgId, val, temp_.msgType);
	//_client.send_business_queueEx(build);

	if (val != 0)
	{
		LOG_ERROR("����[%d:%s]ʵ������ҵ��ʧ��[%d]:msgID:%s��ʣ��Ự:%d", temp_.channel, temp_.streamId.c_str(), val, temp_.msgId.c_str(), SessionMange_singleton::get_mutable_instance().GetPlaySessionCount());
		return;
	}
	LOG_CONSOLE("����[%d:%s]ʵ������ҵ��ɹ�:msgID:%s��ʣ��Ự:%d", temp_.channel, temp_.streamId.c_str(), temp_.msgId.c_str(), SessionMange_singleton::get_mutable_instance().GetPlaySessionCount());
}

void server_manage::channel_record_info(std::string json)
{
	LOG_CONSOLE("��ʼ����¼���б�ҵ��");
	LoginResults Results;
	Platform platform;
	std::string build;

	std::vector<Record> list_;
	Record temp_;
	gm.pars_channel_record_info(json, temp_);
	// ��ȡSDK��¼���
	if (get_platform(temp_.device_id, platform))
	{
		// ��ȡSDK��¼���
		platform.login_results = _sdk.connet_equipment(platform.ip, platform.port, platform.username, platform.password, Results);
	}

	std::vector<Preset> temp_list;
	int val = _sdk.query_record_file(Results, temp_.channel, temp_.startTime, temp_.endTime, list_);
	build = gm.response_record_list(temp_.msgId, val, temp_.msgType, list_);
	_client.send_business_queueEx(build);

	if (val != 0)
	{
		LOG_ERROR("����[%d]¼���б�ҵ��ʧ��[%d]:msgID:%s", temp_.channel, val, temp_.msgId.c_str());
		return;
	}
	LOG_CONSOLE("����[%d]¼���б�ҵ��ɹ�:msgID:%s", temp_.channel, temp_.msgId.c_str());
}

void server_manage::channel_playback(std::string json)
{
	LOG_CONSOLE("��ʼ����¼��ط�ҵ��");
	LoginResults Results;
	Platform platform;
	std::string build;

	Record temp_;
	gm.pars_channel_playback(json, temp_);
	// ��ȡSDK��¼���
	if (get_platform(temp_.device_id, platform))
	{
		// ��ȡSDK��¼���
		platform.login_results = _sdk.connet_equipment(platform.ip, platform.port, platform.username, platform.password, Results);
	}

	int val = _sdk.start_play_back(Results, temp_);
	if (val != 0)
	{
		LOG_ERROR("����[%s:%s-%s]¼��ط�ҵ��ʧ��[%d]:msgID:%s��ʣ��Ự:%d", temp_.streamId.c_str(), temp_.startTime.c_str(), temp_.endTime.c_str(), val, temp_.msgId.c_str(), SessionMange_singleton::get_mutable_instance().GetPlaySessionCount());
		return;
	}
	LOG_CONSOLE("����[%s:%s-%s]¼��ط�ҵ��ɹ�:msgID:%s��ʣ��Ự:%d", temp_.streamId.c_str(), temp_.startTime.c_str(), temp_.endTime.c_str(), temp_.msgId.c_str(), SessionMange_singleton::get_mutable_instance().GetPlaySessionCount());
	//��Ҫ������Ӧ
}

void server_manage::channel_stop_play(std::string json)
{
	LOG_CONSOLE("��ʼ����ֹͣ����ҵ��");
	LoginResults Results;
	Platform platform;
	std::string build;
	std::string key;

	gm.pars_channel_stop_play(json, key);
	int val = _sdk.stop_play(key);
	if (val != 0)
	{
		LOG_ERROR("����[%s]ֹͣ����ҵ��ʧ�ܣ�ʣ��Ự:%d", key.c_str(), SessionMange_singleton::get_mutable_instance().GetPlaySessionCount());
		return;
	}
	LOG_CONSOLE("����[%s]ֹͣ����ҵ��ɹ���ʣ��Ự:%d", key.c_str(), SessionMange_singleton::get_mutable_instance().GetPlaySessionCount());
}

void server_manage::device_record_pause(std::string json)
{
	LOG_CONSOLE("��ʼ����ط���ͣҵ��");
	LoginResults Results;
	Platform platform;
	std::string build;
	std::string streamID;

	gm.pars_streamId(json, streamID);

	int val = _sdk.device_record_pause(streamID);
	if (val != 0)
	{
		LOG_ERROR("����[%s]�ط���ͣҵ��ʧ��", streamID.c_str());
		return;
	}
	LOG_CONSOLE("����[%s]�ط���ͣҵ��ɹ�", streamID.c_str());
}

void server_manage::device_record_resume(std::string json)
{
	LOG_CONSOLE("��ʼ����ط���ͣ�ָ�ҵ��");
	LoginResults Results;
	Platform platform;
	std::string build;
	std::string streamID;

	gm.pars_streamId(json, streamID);

	int val = _sdk.device_record_resume(streamID);
	if (val != 0)
	{
		LOG_ERROR("����[%s]�ط���ͣ�ָ�ҵ��ʧ��", streamID.c_str());
		return;
	}
	LOG_CONSOLE("����[%s]�ط���ͣ�ָ�ҵ��ɹ�", streamID.c_str());
}

void server_manage::device_record_speed(std::string json)
{
	LOG_CONSOLE("��ʼ�������ûطű���ҵ��");
	LoginResults Results;
	Platform platform;
	std::string build;
	std::string streamID;
	float speed;

	gm.pars_record_speed(json, speed, streamID);

	int val = _sdk.device_record_speed(streamID, speed);
	if (val != 0)
	{
		LOG_ERROR("����[%s]���ûطű���ҵ��ʧ��", streamID.c_str());
		return;
	}
	LOG_CONSOLE("����[%s]���ûطű���ҵ��ɹ�,speed:%f", streamID.c_str(), speed);
}