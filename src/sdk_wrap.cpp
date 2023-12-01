#include "sdk_wrap.h"
#include "log.h"

#define DHSDK_GetLastError (CLIENT_GetLastError() & (0x7fffffff))
bool EquipmentMgr::Login(std::string m_sIp, int m_nPort, std::string m_sUserName, std::string m_sPwd, LoginResults& handle)
{
	int m_nChanNum = 0;
	std::string key = m_sIp;
	key.append(":");
	key.append(std::to_string(m_nPort));
	key.append(":");
	key.append(m_sUserName);

	boost::lock_guard<boost::mutex> lg(m_loginLock);
	auto it = Equipment_map.find(key);
	if (it != Equipment_map.end())
	{
		handle = it->second;
	}
	else
	{
		NET_IN_LOGIN_WITH_HIGHLEVEL_SECURITY stInparam;
		memset(&stInparam, 0, sizeof(stInparam));
		stInparam.dwSize = sizeof(stInparam);
		strncpy(stInparam.szIP, m_sIp.c_str(), m_sIp.size());
		strncpy(stInparam.szUserName, m_sUserName.c_str(), m_sUserName.size());
		strncpy(stInparam.szPassword, m_sPwd.c_str(), m_sPwd.size());
		stInparam.nPort = m_nPort;
		stInparam.emSpecCap = EM_LOGIN_SPEC_CAP_TCP;    //设备支持的能力

		NET_OUT_LOGIN_WITH_HIGHLEVEL_SECURITY m_stLoginObject;
		memset(&m_stLoginObject, 0, sizeof(m_stLoginObject));
		m_stLoginObject.dwSize = sizeof(m_stLoginObject);//结构体大小

		handle.handle = CLIENT_LoginWithHighLevelSecurity(&stInparam, &m_stLoginObject);
		if (0 == handle.handle)
		{
			static boost::unordered_map<int, std::string> LoginErrMsg{ {1,"密码不正确"},{2,"用户名不存在"},{3,"登录超时"},{4,"帐号已登录"},
			{5,"帐号已被锁定"},{6,"帐号被列为黑名单"},{7,"资源不足，系统忙"},{8,"子连接失败"},{9,"主连接失败"},
			{10,"超过最大用户连接数"},{11,"只支持3代协议"},{12,"设备未插入U盾或U盾信息错误"},{13,"客户端IP地址没有登录权限"},
			{18,"设备未初始化，无法登陆"} };

			LOG_ERROR("CLIENT_LoginWithHighLevelSecurity Error:%d | %s | %d", m_stLoginObject.nError, LoginErrMsg.at(m_stLoginObject.nError), DHSDK_GetLastError);
			return false;
		}
		handle.uid = ++_uid;
		handle.chanNum = m_stLoginObject.stuDeviceInfo.nChanNum;
		handle.equipment_key = key;
		Equipment_map.insert(std::make_pair(key, handle));
		LOG_INFO("登陆%s成功,句柄:%ll,数量:%d,uid:%ll", key.c_str(), handle.handle, handle.chanNum, handle.uid);
	}
	return true;
}

void EquipmentMgr::Logout(int64_t uid)
{
}

//比较两个时间的早晚:
static int CompareTime(const char* strTime1, const char* strTime2)
{
	if (strTime1 == NULL || strTime2 == NULL)
	{
		return -1;
	}

	int nYear, nMonth, nDay, nHour, nMinute, nSecond;
	sscanf(strTime1, "%04d%02d%02d%02d%02d%02d", &nYear, &nMonth, &nDay, &nHour, &nMinute, &nSecond);//
	struct tm struTm = { 0 };
	struTm.tm_year = nYear - 1900;
	struTm.tm_mon = nMonth - 1;
	struTm.tm_mday = nDay;
	struTm.tm_hour = nHour;
	struTm.tm_min = nMinute;
	struTm.tm_sec = nSecond;
	time_t tmpBegTime = mktime(&struTm);

	sscanf(strTime2, "%04d%02d%02d%02d%02d%02d", &nYear, &nMonth, &nDay, &nHour, &nMinute, &nSecond);//
	struTm = { 0 };
	struTm.tm_year = nYear - 1900;
	struTm.tm_mon = nMonth - 1;
	struTm.tm_mday = nDay;
	struTm.tm_hour = nHour;
	struTm.tm_min = nMinute;
	struTm.tm_sec = nSecond;
	time_t tmpEndTime = mktime(&struTm);

	if (tmpEndTime == tmpBegTime)
	{
		return -1;
	}
	else if (tmpEndTime > tmpBegTime)
	{
		return 0;
	}

	return 1;
}

sdk_wrap::sdk_wrap()
{
}

sdk_wrap::~sdk_wrap()
{
}

void sdk_wrap::Init(fDisConnect cb, LDWORD dwUser)
{
	LOG_INFO("开始初始化DHSDK");
	fHaveReConnect cb_ = nullptr;
	bool bRet = CLIENT_Init(NULL, NULL);
	if (bRet)
	{
		CLIENT_SetAutoReconnect(NULL, NULL);
		LOG_ERROR("初始化DHSDK成功");
	}
}

int sdk_wrap::connet_equipment(std::string m_sIp, int m_nPort, std::string m_sUserName, std::string m_sPwd, LoginResults& handle)
{
	LOG_INFO("开始登陆%s:%d:%s", m_sIp.c_str(), m_nPort, m_sUserName.c_str());
	if (!eqm_mgr.Login(m_sIp, m_nPort, m_sUserName, m_sPwd, handle))
	{
		LOG_ERROR("登陆失败");
		return 1;
	}
	return 0;
}

int sdk_wrap::get_camera_list(LoginResults login, std::vector<Device>& list)
{
	int ret = 0;
	m_nChanNum = login.chanNum;
	LOG_INFO("ChanNum:%d", m_nChanNum);
	if (0 == m_nChanNum)
		return ret;
	list.resize(m_nChanNum);

	DHDEV_CHANNEL_CFG* cfg = new DHDEV_CHANNEL_CFG[m_nChanNum];
	//DHDEV_CHANNEL_CFG cfg[m_nChanNum];
	DWORD bytesReturned = 0;
	for (int i = 0; i < m_nChanNum; i++)
	{
		memset(&cfg[i], 0x00, sizeof(DHDEV_CHANNEL_CFG));
		cfg[i].dwSize = sizeof(DHDEV_CHANNEL_CFG);
	}
	BOOL dhRet = CLIENT_GetDevConfig(login.handle, DH_DEV_CHANNELCFG, -1, (void*)cfg, m_nChanNum * sizeof(DHDEV_CHANNEL_CFG), &bytesReturned);
	if (!dhRet)
	{
		// ret = DHSDK_GetLastError;
		LOG_ERROR("CLIENT_GetDevConfig DH_DEV_CHANNELCFG Fail! Errcode:%d", DHSDK_GetLastError);
		//return ret;
	}
	bytesReturned /= sizeof(DHDEV_CHANNEL_CFG);
	LOG_INFO("bytesReturned num:%d", bytesReturned);
	//获取每个通道信息
	DH_IN_MATRIX_GET_CAMERAS stuInParm = { sizeof(DH_IN_MATRIX_GET_CAMERAS) };
	DH_OUT_MATRIX_GET_CAMERAS stuOutParam = { sizeof(DH_OUT_MATRIX_GET_CAMERAS) };
	//DH_MATRIX_CAMERA_INFO stuAllmatrixcamerinfo[m_nChanNum];
	DH_MATRIX_CAMERA_INFO* stuAllmatrixcamerinfo = new DH_MATRIX_CAMERA_INFO[m_nChanNum];
	memset(stuAllmatrixcamerinfo, 0, sizeof(DH_MATRIX_CAMERA_INFO) * m_nChanNum);
	stuOutParam.nMaxCameraCount = m_nChanNum; //这里获取到的最大数
	stuOutParam.pstuCameras = stuAllmatrixcamerinfo;
	for (int i = 0; i < stuOutParam.nMaxCameraCount; ++i)
	{
		stuOutParam.pstuCameras[i].dwSize = sizeof(DH_MATRIX_CAMERA_INFO);
		stuOutParam.pstuCameras[i].stuRemoteDevice.dwSize = sizeof(DH_REMOTE_DEVICE);
	}
	// 获取所有有效显示源
	BOOL dhIpRet = CLIENT_MatrixGetCameras(login.handle, &stuInParm, &stuOutParam);
	if (!dhIpRet)//有些设备不支持
	{
		LOG_INFO("CLIENT_MatrixGetCameras error Fail! Errcode:%d", ret);
	}
	// else
	// {
	// }

	//状态
	NET_IN_GET_CAMERA_STATEINFO state_info;
	state_info.dwSize = sizeof(NET_IN_GET_CAMERA_STATEINFO);
	state_info.bGetAllFlag = TRUE;	// 获取全部
	//NET_CAMERA_STATE_INFO camera_state[8];
	NET_CAMERA_STATE_INFO* camera_state = new NET_CAMERA_STATE_INFO[m_nChanNum];
	memset(camera_state, 0x00, sizeof(camera_state));
	NET_OUT_GET_CAMERA_STATEINFO out_state_info;
	out_state_info.dwSize = sizeof(NET_OUT_GET_CAMERA_STATEINFO);
	out_state_info.pCameraStateInfo = camera_state;
	out_state_info.nMaxNum = m_nChanNum;
	if (!CLIENT_QueryDevInfo(login.handle, NET_QUERY_GET_CAMERA_STATE, (void*)&state_info, (void*)&out_state_info))
	{//DVR不支持,用CLIENT_QueryDevInfo遍历去调
		LOG_INFO("CLIENT_QueryDevInfo  NET_QUERY_GET_CAMERA_STATE error Fail! Errcode:%d Try CLIENT_QueryRemotDevState", ret);
		for (int i = 0; i < m_nChanNum; ++i)
		{
			DWORD online = 0;
			int retlen = 0;
			dhRet = CLIENT_QueryRemotDevState(login.handle, DH_DEVSTATE_ONLINE, i, (char*)&online, sizeof(online), &retlen);
			if (!dhRet)
			{
				LOG_ERROR("CLIENT_QueryRemotDevState DH_DEVSTATE_ONLINE error:%d Try CLIENT_QueryDevState", DHSDK_GetLastError);
				dhRet = CLIENT_QueryDevState(login.handle, DH_DEVSTATE_ONLINE, (char*)&online, sizeof(online), &retlen);
				if (!dhRet)
				{
					LOG_ERROR("%d:CLIENT_QueryDevState DH_DEVSTATE_ONLINE error:%d ", i, DHSDK_GetLastError);
					// continue;
				}
			}

			camera_state[i].nChannel = i;
			camera_state[i].emConnectionState = (online) ? EM_CAMERA_STATE_TYPE_CONNECTED : EM_CAMERA_STATE_TYPE_UNCONNECT;
		}
	}

	for (int i = 0; i < m_nChanNum; ++i)
	{
		list[i].id = i;
		list[i].name = cfg[i].szChannelName;//pChannelName+i*32;
		if (dhIpRet && (strcmp(stuOutParam.pstuCameras[i].stuRemoteDevice.szIp, "192.168.0.0") != 0))//
		{
			DH_MATRIX_CAMERA_INFO& stuinfo = stuOutParam.pstuCameras[i];
			list[i].ip = stuinfo.stuRemoteDevice.szIp;
			list[i].port = stuinfo.stuRemoteDevice.nPort;
			list[i].username = stuinfo.stuRemoteDevice.szUserEx;
			list[i].password = stuinfo.stuRemoteDevice.szPwdEx;
		}
		list[i].online = camera_state[i].emConnectionState == EM_CAMERA_STATE_TYPE_CONNECTED;
		//list[camera_state[i].nChannel].status = camera_state[i].emConnectionState;
	}

	delete[] stuAllmatrixcamerinfo;
	delete[] cfg;
	return ret;
}

int sdk_wrap::ptz_control(LoginResults login, PtzCMD ptz)
{
	const static boost::unordered_map<int, DWORD> XH_TO_DH_PTZBASECMD
	{
	{8,DH_PTZ_UP_CONTROL},{4,DH_PTZ_DOWN_CONTROL},    //上下
	{2,DH_PTZ_LEFT_CONTROL},{1,DH_PTZ_RIGHT_CONTROL},//左右
	{10,DH_EXTPTZ_LEFTTOP},{9,DH_EXTPTZ_RIGHTTOP},//左上右上
	{6,DH_EXTPTZ_LEFTDOWN},{5,DH_EXTPTZ_RIGHTDOWN},//左下右下
	{72,DH_PTZ_APERTURE_ADD_CONTROL},{68,DH_PTZ_APERTURE_DEC_CONTROL},//放大缩写光圈
	{32,DH_PTZ_ZOOM_ADD_CONTROL},{16,DH_PTZ_ZOOM_DEC_CONTROL},//倍率变大变小
	{65,DH_PTZ_FOCUS_ADD_CONTROL},{66,DH_PTZ_FOCUS_DEC_CONTROL},//聚焦近远
	{130,DH_PTZ_POINT_MOVE_CONTROL},{129,DH_PTZ_POINT_SET_CONTROL},{131,DH_PTZ_POINT_DEL_CONTROL}//跳转设置删除预置位
	};

	//IRISE_AND_FOCUS_STOP(64, "IRISE_AND_FOCUS_STOP")光圈和聚焦 操作的停止

	int ret = 0;
	if (ptz.contrl_param > 8)
		ptz.contrl_param = 8;

	DWORD cmd = 0;
	auto it = XH_TO_DH_PTZBASECMD.find(ptz.contrl_cmd);
	if (it != XH_TO_DH_PTZBASECMD.end())
		cmd = it->second;
	else
		if (it == XH_TO_DH_PTZBASECMD.end())
		{
			if (0 == ptz.contrl_cmd || 64 == ptz.contrl_cmd)
			{
				ptz.dwStop = TRUE;
				ptz.contrl_cmd = temp_ptz_cmd;
			}
		}

	LOG_CONSOLE("CLIENT_DHPTZControlEx2.[lLoginID=%llu, nChannelID=%d, dwPTZCommand=%d, param1=%d, param2=%d, param3=%d, dwStop=%d, param4=NULL.]", login.handle, ptz.channel, cmd, 0, ptz.contrl_param, 0, ptz.dwStop);
	if (!CLIENT_DHPTZControlEx2(login.handle, ptz.channel, cmd, 0, ptz.contrl_param, 0, ptz.dwStop, NULL))
	{
		ret = DHSDK_GetLastError;
		LOG_ERROR("CLIENT_DHPTZControlEx2  Fail! Errcode: %d", ret);
		return ret;
	}
	temp_ptz_cmd = ptz.contrl_cmd;
	return 0;
}

#define MAX_PRESETNUM 128
int sdk_wrap::query_ptz_preset(LoginResults login, int channel, std::vector<Preset>& lists)
{
	int ret = 0;

	NET_PTZ_PRESET preset[MAX_PRESETNUM];
	for (int i = 0; i < MAX_PRESETNUM; i++)
		memset(&preset[i], 0x00, sizeof(NET_PTZ_PRESET));

	NET_PTZ_PRESET_LIST preset_list;
	preset_list.dwSize = sizeof(NET_PTZ_PRESET_LIST);
	preset_list.dwMaxPresetNum = MAX_PRESETNUM;
	preset_list.dwRetPresetNum = 0;
	preset_list.pstuPtzPorsetList = preset;

	int ret_length = 0;
	// CLIENT_QueryDevState
	if (!CLIENT_QueryRemotDevState(login.handle, DH_DEVSTATE_PTZ_PRESET_LIST, channel,
		reinterpret_cast<char*>(&preset_list), preset_list.dwSize, &ret_length, 3000))
	{
		ret = DHSDK_GetLastError;
		LOG_ERROR("CLIENT_QueryRemotDevState DH_DEVSTATE_PTZ_PRESET_LIST Fail! Errcode:%d", ret);
		return ret;
	}
	LOG_INFO("get Preset Num:%d", preset_list.dwRetPresetNum);
	lists.reserve(preset_list.dwRetPresetNum);

	for (int i = 0; i < preset_list.dwRetPresetNum; ++i)
	{
		Preset p;
		p.name = preset[i].szName;
		p.index = preset[i].nIndex;
		lists.push_back(p);
	}
	return ret;
}

int sdk_wrap::ptz_3D(LoginResults login, PtzCMD ptz)
{
	return 0;
}

int sdk_wrap::start_real_stream(LoginResults login, Play real_play, DH_PLAY_STREAM_TYPE streamType)
{
	//使用streamID作为key
	int ret = 0;
	PlaySessionPtr session = SessionMange_singleton::get_mutable_instance().GetPlaySession(real_play.streamId);
	if (!session)
	{
		session = boost::make_shared<PlaySession>(login.handle, real_play.channel, this);
		if (!real_play.sdpIp.empty())//分发地址
		{
			//real_play.sdpIp = "172.20.0.120";
			//real_play.port = 22000;
			if (session->setMSAddr(real_play.sdpIp, real_play.port, real_play.streamMode))
				LOG_INFO("设置流媒体地址%s:%d %s", real_play.sdpIp.c_str(), real_play.port, (real_play.streamMode == 2 ? "TCP" : "UDP"));
		}
		session->m_streamtype = 1;
		ret = session->startRealPlaySession(streamType);
		if (ret)
			return ret;

		if (!SessionMange_singleton::get_mutable_instance().InsertPlaySession(real_play.streamId, session))
		{
			LOG_ERROR("insert RealSession Fail!!");
			return 1;
		}
	}
	else
	{
		LOG_INFO("this cam [%d] has playing. key:%s", real_play.channel, real_play.streamId.c_str());
		/* code */
	}

	return ret;
}

int sdk_wrap::start_play_back(LoginResults login, Record record)
{
	//使用streamID作为key
	int ret = 0;
	PlaySessionPtr session = SessionMange_singleton::get_mutable_instance().GetPlaySession(record.streamId);
	if (!session)
	{
		session = boost::make_shared<PlaySession>(login.handle, record.channel, this);
		if (!record.sdpIp.empty())//分发地址
		{
			//record.sdpIp = "172.20.0.120";
			//record.port = 22000;
			if (session->setMSAddr(record.sdpIp, record.port, record.streamMode))
				LOG_INFO("设置流媒体地址%s:%d %s", record.sdpIp.c_str(), record.port, (record.streamMode == 2 ? "TCP" : "UDP"));
		}
		session->m_streamtype = 2;
		ret = session->startRecordPlaySession(record);
		if (ret)
			return ret;

		if (!SessionMange_singleton::get_mutable_instance().InsertPlaySession(record.streamId, session))
		{
			LOG_ERROR("insert RealSession Fail!!");
			return 1;
		}
	}
	else
	{
		LOG_INFO("this cam [%d] has playing. key:%s", record.channel, record.streamId.c_str());
		/* code */
	}

	return ret;
}

int sdk_wrap::stop_play(std::string key)
{
	//使用streamID作为key
	int ret = -1;
	PlaySessionPtr session = SessionMange_singleton::get_mutable_instance().GetPlaySession(key);
	if (!session)
	{
		//查找不到session
		LOG_ERROR("not find session:%s", key.c_str());
		return ret;
	}
	else
	{
		//结束会话
		ret = session->stopSession();
		SessionMange_singleton::get_mutable_instance().DelPlaySession(key);
	}

	return ret;
}

int sdk_wrap::seek_play_back(LoginResults login)
{
	return 0;
}

int sdk_wrap::set_speed(LoginResults login)
{
	return 0;
}

int sdk_wrap::query_record_file(LoginResults login, int channel, const std::string& start, const std::string& end, std::vector<Record>& files)
{
	int ret = 0;
	//channel += 1;
	LOG_INFO("QueryRecordFile:%d start:%s-end:%s", channel, start.c_str(), end.c_str());

	int nRecordFileType = NET_RECORD_TYPE_ALL;
	NET_TIME	stStartTime;
	NET_TIME	stStopTime;
	sscanf(start.c_str(), "%04d-%02d-%02d %02d:%02d:%02d",
		&stStartTime.dwYear, &stStartTime.dwMonth, &stStartTime.dwDay,
		&stStartTime.dwHour, &stStartTime.dwMinute, &stStartTime.dwSecond);

	sscanf(end.c_str(), "%04d-%02d-%02d %02d:%02d:%02d",
		&stStopTime.dwYear, &stStopTime.dwMonth, &stStopTime.dwDay,
		&stStopTime.dwHour, &stStopTime.dwMinute, &stStopTime.dwSecond);

	std::string temp_start;
	{
		// 将字符串解析为时间结构
		std::tm timeStruct = {};
		std::istringstream ss(start);
		ss >> std::get_time(&timeStruct, "%Y-%m-%d %H:%M:%S");

		if (ss.fail()) {
			LOG_ERROR("解析日期时间失败");
			return 1;
		}

		// 将时间结构重新格式化为字符串
		std::ostringstream formattedSS;
		formattedSS << std::put_time(&timeStruct, "%Y%m%d%H%M%S");
		temp_start = formattedSS.str();
	}

	std::string temp_end;
	{
		// 将字符串解析为时间结构
		std::tm timeStruct = {};
		std::istringstream ss(end);
		ss >> std::get_time(&timeStruct, "%Y-%m-%d %H:%M:%S");

		if (ss.fail()) {
			LOG_ERROR("解析日期时间失败");
			return 1;
		}

		// 将时间结构重新格式化为字符串
		std::ostringstream formattedSS;
		formattedSS << std::put_time(&timeStruct, "%Y%m%d%H%M%S");
		temp_end = formattedSS.str();
	}

	int filecount = 0;
	NET_RECORDFILE_INFO recordFile[/*200*//*750*/1024] = { 0 }; //为能查询到一个月内的录像，需要750个录像文件缓冲区(200个能查8天，即25个/天，750/25=30天)
	if (!CLIENT_QueryRecordFile(login.handle, channel, nRecordFileType, &stStartTime, &stStopTime, NULL /*pchCardid */, &recordFile[0], sizeof(recordFile), &filecount, 5 * 1000))//查询录像文件
	{
		ret = DHSDK_GetLastError;

		if (NET_NETWORK_ERROR == ret)//超时，可能录像文件太多了（例如一个月），直接当成功返回
		{
			Record f;
			f.startTime = start;
			f.endTime = end;
			files.emplace_back(f);
			return 0;
		}

		LOG_ERROR("CLIENT_QueryRecordFile Fail! Errcode:%d", ret);
		return ret;
	}

	LOG_INFO("total file count : %d", filecount);
	for (int i = 0; i < filecount; ++i)
	{
		NET_RECORDFILE_INFO& file = recordFile[i];
		char startTime[32] = { 0 };
		snprintf(startTime, sizeof(startTime), "%04d%02d%02d%02d%02d%02d",
			file.starttime.dwYear, file.starttime.dwMonth, file.starttime.dwDay,
			file.starttime.dwHour, file.starttime.dwMinute, file.starttime.dwSecond);

		char endTime[32] = { 0 };
		snprintf(endTime, sizeof(endTime), "%04d%02d%02d%02d%02d%02d",
			file.endtime.dwYear, file.endtime.dwMonth, file.endtime.dwDay,
			file.endtime.dwHour, file.endtime.dwMinute, file.endtime.dwSecond);

		LOG_INFO("filename:%s, time:%s bHint:%d", file.filename, startTime, endTime, (unsigned int)file.bHint);

		// 如果查询到的某一段录像的开始时间都已经超过了传入查询条件的结束时间，这一段录像直接扔掉
		int nRet = CompareTime(startTime, temp_end.c_str());
		if (1 == nRet)
		{
			LOG_INFO("starttime[%s]is early than endtime[%s], drop the file", startTime, end);
			continue;
		}

		Record f;
		f.startTime = 0 == CompareTime(startTime, temp_start.c_str()) ? temp_start : startTime;// startTime<start?
		f.endTime = 0 == CompareTime(endTime, temp_end.c_str()) ? endTime : temp_end;//endTime<end
		f.fileSize = file.size;
		f.fileName = file.filename;
		files.emplace_back(f);
	}

	LOG_INFO("finish deal ,total files count : %d", files.size());
	return ret;
}

int sdk_wrap::device_record_pause(std::string key)
{
	//使用streamID作为key
	int ret = -1;
	PlaySessionPtr session = SessionMange_singleton::get_mutable_instance().GetPlaySession(key);
	if (!session)
	{
		//查找不到session
		LOG_ERROR("not find session:%s", key.c_str());
		return ret;
	}
	else
	{
		//结束会话
		ret = session->pausePlayBack(true);
	}

	return ret;
}

int sdk_wrap::device_record_resume(std::string key)
{
	//使用streamID作为key
	int ret = -1;
	PlaySessionPtr session = SessionMange_singleton::get_mutable_instance().GetPlaySession(key);
	if (!session)
	{
		//查找不到session
		LOG_ERROR("not find session:%s", key.c_str());
		return ret;
	}
	else
	{
		//结束会话
		ret = session->pausePlayBack(false);
	}

	return ret;
}

int sdk_wrap::device_record_speed(std::string key, float speed)
{
	//使用streamID作为key
	int ret = -1;
	PlaySessionPtr session = SessionMange_singleton::get_mutable_instance().GetPlaySession(key);
	if (!session)
	{
		//查找不到session
		LOG_ERROR("not find session:%s", key.c_str());
		return ret;
	}
	else
	{
		//结束会话
		ret = session->setSpeed(speed);
	}

	return ret;
}