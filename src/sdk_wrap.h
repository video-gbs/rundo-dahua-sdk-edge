#pragma once
#include <string>
#include <boost/unordered_map.hpp>
#include <boost/thread.hpp>
#include"dhnetsdk.h"
#include "common.h"
#include "PlaySession.h"

static int64_t _uid;
//#define DHSDK_GetLastError (CLIENT_GetLastError() & (0x7fffffff))

class EquipmentMgr
{
public:
	EquipmentMgr()
	{
		_uid = 0;
	}

	~EquipmentMgr()
	{
	}

	bool Login(std::string m_sIp, int m_nPort, std::string m_sUserName, std::string m_sPwd, LoginResults& handle);
	void Logout(int64_t uid);

private:
	boost::mutex m_loginLock;
	boost::unordered_map<std::string, LoginResults> Equipment_map;
};

//sdk接口封装
class sdk_wrap
{
public:
	sdk_wrap();
	~sdk_wrap();

	void Init(fDisConnect cb = nullptr, LDWORD dwUser = 0);

	int connet_equipment(std::string m_sIp, int m_nPort, std::string m_sUserName, std::string m_sPwd, LoginResults& handle);

	int get_camera_list(LoginResults login, std::vector<Device>& list);

	int ptz_control(LoginResults login, PtzCMD ptz);
	int query_ptz_preset(LoginResults login, int nChannelID, std::vector<Preset>& lists);
	int ptz_3D(LoginResults login, PtzCMD ptz);//目前没有

	int start_real_stream(LoginResults login, Play real_play, DH_PLAY_STREAM_TYPE streamType);
	//int stop_real_stream(LoginResults login, std::string key);

	int start_play_back(LoginResults login, Record record);
	int stop_play(std::string key);
	//跳转回放
	int seek_play_back(LoginResults login);
	//设置回放速度
	int set_speed(LoginResults login);
	//获取录像列表
	int query_record_file(LoginResults login, int channel, const std::string& start, const std::string& end, std::vector<Record>& files);

	int device_record_pause(std::string streamID);
	int device_record_resume(std::string streamID);
	int device_record_speed(std::string streamID, float speed);
private:
	EquipmentMgr eqm_mgr;
	int m_nChanNum;

	int temp_ptz_cmd;
};
