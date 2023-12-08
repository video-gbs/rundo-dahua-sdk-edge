#pragma once
#include <string>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iostream>
#include <vector>
#include "common.h"

#define heartbeat_timeout  1000//ms

class gateway_msg
{
public:
	gateway_msg();
	~gateway_msg();

	bool isMySerialNum(std::string json);
	int get_msg_type(std::string json);

	std::string gateway_sign_in();
	std::string gateway_heartbeat();
	std::string gateway_re_sign_in();
	bool pars_gateway_sign(std::string json, std::string& msgType, std::string& mqGetQueue, std::string& mqSetQueue, std::string& mqExchange);

	std::string device_sign_in();
	std::string device_add(int step, Platform platform);
	bool pars_device_add(std::string json, Platform& p);
	bool pars_device_sync(std::string json, Platform& platform); //返回对应的设备下面全部的通道数据
	std::string device_sync(std::vector<Device> device_list, Platform& platform);//响应
	bool pars_device_delete_soft(std::string json, Platform& platform); //返回对应的设备下面全部的通道数据
	bool pars_device_total_sync(std::string json, std::string& msgId); //返回对应的设备下面全部的通道数据
	std::string device_total_sync(std::vector<Platform> platform_list, std::string msgId);//全量设备同步响应

	std::string channel_sync(std::vector<Device> device_list, Platform& platform);

	std::string response(const std::string& msgId, int code, const std::string& msgType);
	bool pars_ptz_control(std::string json, PtzCMD& ptz);
	bool pars_ptz_preset(std::string json, Preset& p);
	std::string ptz_preset_list(Preset p, std::vector<Preset> presets, int code);
	bool pars_channel_play(std::string json, Play& p);
	bool pars_channel_record_info(std::string json, Record& p);
	std::string response_record_list(const std::string& msgId, int code, const std::string& msgType, std::vector<Record> list);
	bool pars_channel_playback(std::string json, Record& p);
	bool pars_channel_stop_play(std::string json, std::string& key);

	bool pars_streamId(std::string json, std::string& streamId);
	bool pars_record_speed(std::string json, float& speed, std::string& streamId);
	bool pars_requst(std::string json, std::string& msgId, std::string& msgType, std::string& deviceId, std::string& channelId);

private:
	std::string strSerialNum = "29fc51254c0c4809a9f4851f994111a2";
	std::string local_ip;
	std::string msg_id_h;
	std::string msg_id_b;
	long long msg_id;
};
