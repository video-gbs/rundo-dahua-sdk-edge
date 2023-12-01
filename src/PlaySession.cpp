#include "dhnetsdk.h"
#include "PlaySession.h"
#include "log.h"

#define DHSDK_GetLastError (CLIENT_GetLastError() & (0x7fffffff))

#define NET_DATA_CALL_BACK_VALUE 1000

extern void LIBNET_CALLMETHOD	onclose(NETHANDLE srvhandle, NETHANDLE clihandle);
extern void LIBNET_CALLMETHOD	onaccept(NETHANDLE srvhandle, NETHANDLE clihandle, void* address);
extern void LIBNET_CALLMETHOD   onread(NETHANDLE srvhandle, NETHANDLE clihandle, uint8_t* data, uint32_t datasize, void* address);
extern void LIBNET_CALLMETHOD	onconnect(NETHANDLE clihandle, uint8_t result);

static time_t StringToDatetime(const std::string& str)
{
	tm tm_;                                    // 定义tm结构体。
	sscanf(str.c_str(), "%04d%02d%02d%02d%02d%02d", &tm_.tm_year, &tm_.tm_mon, &tm_.tm_mday, &tm_.tm_hour, &tm_.tm_min, &tm_.tm_sec);// 将string存储的日期时间，转换为int临时变量。
	tm_.tm_year -= 1900;                 // 年，由于tm结构体存储的是从1900年开始的时间，所以tm_year减去1900。
	tm_.tm_mon -= 1;                    // 月，由于tm结构体的月份存储范围为0-11，所以tm_mon减去1。
	tm_.tm_isdst = 0;                          // 非夏令时。
	time_t t_ = mktime(&tm_);                  // 将tm结构体转换成time_t格式。
	return t_;
}

PlaySession::PlaySession(LLONG handle, int channel, void* pUser)
	:m_lLoginID(handle), m_lRealPlayHandle(0), m_lRecordPlayHandle(0), m_nChannel(channel), m_bGetIFrame(false), m_llDataFrameCount(0), m_lastRecvDataTime(0),
	m_originVideoTimestamp(0), m_videoTimestamp(0), m_originAudioTimestamp(0), m_audioTimestamp(0), m_cbUserData(pUser), nMediaClient(0), nVideoStreamID(-1), _is_frist(true), m_nSpeed(1.0), nCurrentVideoTimestamp(0)
{
	LOG_CONSOLE("创建播放会话:%d", m_nChannel);
}
PlaySession::~PlaySession()
{
	//tm* _t = localtime(&m_lastRecvDataTime);
	//char s[64] = { 0 };
	//sprintf(s, "%04d-%02d-%02d %02d:%02d:%02d", _t->tm_year + 1900, _t->tm_mon + 1, _t->tm_mday,
	//	_t->tm_hour, _t->tm_min, _t->tm_sec);

	disconnetMS();//关闭流媒体链接
	LOG_CONSOLE("回收播放会话:%d", m_nChannel);
}

static void* ps_alloc(void* param, size_t bytes)
{
	PlaySession* pThis = (PlaySession*)param;

	return pThis->s_buffer;
}

static void ps_free(void* param, void* /*packet*/)
{
	return;
}

static int ps_write(void* param, int stream, void* packet, size_t bytes)
{
	PlaySession* pThis = (PlaySession*)param;

	if (pThis->_is_run)
		pThis->ps_to_rtp((unsigned char*)packet, bytes);

	return true;
}

//rtp打包回调视频
void GB28181RtpServer_rtp_packet_callback_func_send(_rtp_packet_cb* cb)
{
	PlaySession* pThis = (PlaySession*)cb->userdata;
	if (pThis == NULL || !pThis->_is_run)
		return;

	//LOG_CONSOLE("rtp:0x%X 0x%X 0x%X 0x%X 0x%X 0x%X", cb->data[0], cb->data[1], cb->data[2], cb->data[3], cb->data[4], cb->data[5]);
	pThis->sendMS(cb->data, cb->datasize);
}

int CALLBACK RecordDataCallbackEx2(LLONG lRealHandle, DWORD dwDataType, BYTE* pBuffer, DWORD dwBufSize, LDWORD dwUser)
{
	PlaySession* session = reinterpret_cast<PlaySession*>(dwUser);
	if (session)
	{
		if (dwDataType == NET_DATA_CALL_BACK_VALUE + EM_REAL_DATA_TYPE_H264)
		{
			// 处理 ps 流的业务逻辑
			//LOG_CONSOLE("ES: 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X", pBuffer[0], pBuffer[1], pBuffer[2], pBuffer[3], pBuffer[4], pBuffer[5]);
			if (pBuffer[4] == 0x67 && session->_is_frist)
			{
				session->es_to_ps(pBuffer, dwBufSize);
				session->_is_frist = false;
				return 0;
			}
			else
			{
				session->es_to_ps(pBuffer, dwBufSize);
			}
		}
		else
		{
			//LOG_CONSOLE("dwDataType:%d", dwDataType);
			// 处理其他类型的流
		}
		if (dwDataType == NET_DATA_CALL_BACK_VALUE + EM_REAL_DATA_TYPE_GBPS)
		{
			session->ps_to_rtp(pBuffer, dwBufSize);
			return 0;
		}
	}
}

void CALLBACK RealDataCallbackEx(LLONG lRealHandle, DWORD dwDataType, BYTE* pBuffer, DWORD dwBufSize, LONG param, LDWORD dwUser)
{
	PlaySession* session = reinterpret_cast<PlaySession*>(dwUser);
	if (session)
	{
		if (dwDataType == NET_DATA_CALL_BACK_VALUE + EM_REAL_DATA_TYPE_H264)
		{
			// 处理 ps 流的业务逻辑
			//LOG_CONSOLE("ES: 0x%X 0x%X 0x%X 0x%X 0x%X 0x%X", pBuffer[0], pBuffer[1], pBuffer[2], pBuffer[3], pBuffer[4], pBuffer[5]);
			if (pBuffer[4] == 0x67 && session->_is_frist)
			{
				session->es_to_ps(pBuffer, dwBufSize);
				session->_is_frist = false;
				return;
			}
			else
			{
				session->es_to_ps(pBuffer, dwBufSize);
			}
		}
		else
		{
			//LOG_CONSOLE("dwDataType:%d", dwDataType);
			// 处理其他类型的流
		}
		if (dwDataType == NET_DATA_CALL_BACK_VALUE + EM_REAL_DATA_TYPE_GBPS)
		{
			session->ps_to_rtp(pBuffer, dwBufSize);
		}
	}
}

int CALLBACK DataCallbackEx(LLONG lRealHandle, NET_DATA_CALL_BACK_INFO* pDataCallBack, LDWORD dwUser)
{
	// Your implementation here
	return 0;  // Return appropriate value
}

void PlaySession::afterDHReadDataCallBack(BYTE* streambuf, DWORD bufSize)
{
}

bool PlaySession::setMSAddr(std::string ip, int port, int link_type)
{
	ms_dispatch.ip = ip;
	ms_dispatch.port = port;
	ms_dispatch.link_type = link_type;

	int nRet = connetMS(ms_dispatch);

	creat_packet(1, 0);

	return nRet == 0;
}

int PlaySession::es_to_ps(uint8_t* data, uint32_t datasize)
{
	if (_ps_muxer)
	{
		ps_muxer_input((ps_muxer_t*)_ps_muxer, nVideoStreamID, true, videoPTS, videoPTS, data, datasize);
		videoPTS += (90000 / 25);
	}
	return 0;
}

int PlaySession::ps_to_rtp(uint8_t* data, uint32_t datasize)
{
	if (hRtpPS > 0)
	{
		//LOG_CONSOLE("PS:0x%X 0x%X 0x%X 0x%X 0x%X 0x%X", data[0], data[1], data[2], data[3], data[4], data[5]);
		inputPS.data = data;
		inputPS.datasize = datasize;
		rtp_packet_input(&inputPS);
	}
	return 0;
}

bool PlaySession::creat_packet(int videoEncType, int audioEncType)
{
	//创建ps
	handler.alloc = ps_alloc;
	handler.write = ps_write;
	handler.free = ps_free;

	_ps_muxer = ps_muxer_create(&handler, this);

	//创建rtp
	int nRet = rtp_packet_start(GB28181RtpServer_rtp_packet_callback_func_send, (void*)this, &hRtpPS);
	if (nRet != e_rtppkt_err_noerror)
	{
		LOG_ERROR("创建视频rtp打包失败,session:%llu,  nRet = %d", nRet);
		return false;
	}
	else
	{
		LOG_CONSOLE("创建视频rtp打包成功:%d", hRtpPS);
	}
	optionPS.handle = hRtpPS;
	optionPS.mediatype = e_rtppkt_mt_video;
	optionPS.payload = 96;
	optionPS.streamtype = e_rtppkt_st_gb28181;
	optionPS.ssrc = rand();
	optionPS.ttincre = (90000 / 25);//默认25帧
	rtp_packet_setsessionopt(&optionPS);

	inputPS.handle = hRtpPS;
	inputPS.ssrc = optionPS.ssrc;

	if (nVideoStreamID == -1 && _ps_muxer != NULL)
	{//增加视频
		if (videoEncType == 1)
		{
			nVideoStreamID = ps_muxer_add_stream((ps_muxer_t*)_ps_muxer, PSI_STREAM_H264, NULL, 0);
			LOG_CONSOLE("创建视频ps打包类型:H264");
		}
		else if (videoEncType == 10)
		{
			nVideoStreamID = ps_muxer_add_stream((ps_muxer_t*)_ps_muxer, PSI_STREAM_H265, NULL, 0);
			LOG_CONSOLE("创建视频ps打包类型:H265");
		}
	}

	return true;
}

int PlaySession::startRealPlaySession(DH_PLAY_STREAM_TYPE streamType)
{
	int ret = 0;
	LOG_INFO("start RealPlay channel:%d", m_nChannel);
	if (!CLIENT_MakeKeyFrame(m_lLoginID, m_nChannel))
	{
		LOG_ERROR("强制I帧 CLIENT_MakeKeyFrame  Fail! Errcode:%d", DHSDK_GetLastError);
		return ret;
	}

	DH_RealPlayType  rType = (DH_PLAY_STREAM_TYPE::MAIN_STREAM == streamType) ? DH_RType_Realplay : DH_RType_Realplay_1; //主辅码流

	NET_IN_REALPLAY_BY_DATA_TYPE stIn = { sizeof(stIn) };
	NET_OUT_REALPLAY_BY_DATA_TYPE stOut = { sizeof(stOut) };
	stIn.cbRealData = RealDataCallbackEx;
	stIn.emDataType = EM_REAL_DATA_TYPE_GBPS;
	stIn.nChannelID = m_nChannel;
	stIn.hWnd = NULL;
	stIn.dwUser = reinterpret_cast<LDWORD>(this);

	m_lRealPlayHandle = CLIENT_RealPlayByDataType(m_lLoginID, &stIn, &stOut, 5000); // Adjust wait time as needed
	if (0 == m_lRealPlayHandle)//失败
	{
		ret = DHSDK_GetLastError;
		LOG_ERROR("实时播放，CLIENT_RealPlayByDataType Fail! Errcode:%d", ret);
	}

	_is_run = true;

	return ret;
}

int PlaySession::readDataCallBack(LLONG lPlayHandle, DWORD dwDataType, BYTE* pBuffer, DWORD dwBufSize, LDWORD dwUser)
{
	PlaySession* session = reinterpret_cast<PlaySession*>(dwUser);
	if (session)
	{
		//PrintHEX(pBuffer,dwBufSize);
		//do something
		session->afterDHReadDataCallBack(pBuffer, dwBufSize);
	}

	//参数返回
	//	0：表示本次回调失败，下次回调会返回相同的数据，
	//	1：表示本次回调成功，下次回调会返回后续的数据
	return 1;
}
void PlaySession::posCallBack(LLONG lPlayHandle, DWORD dwTotalSize, DWORD dwDownLoadSize, LDWORD dwUser)
{
	PlaySession* session = reinterpret_cast<PlaySession*>(dwUser);
	if (session)
	{
		// LOGDEBUG("playBack session:"<<lPlayHandle<<" LoadSize/TotalSize"<<dwDownLoadSize<<"/"<<dwTotalSize<<".");
		 //do something
		 //session->
	}
	return;
}

int PlaySession::startRecordPlaySession(Record record, DH_PLAY_STREAM_TYPE streamType)
{
	int ret = 0;
	LOG_INFO("start PlayBack channel:%d", m_nChannel);
	//queryRecordFiles(); //在回放之前需要先调用本接口查询录像记录，当根据输入的时间段查询到的录像记录信息大于定义的缓冲区大小，则只返回缓冲所能存放的录像记录，可以根据需要继续查询。
// 设置回放时的码流类型
	int nStreamType = (DH_PLAY_STREAM_TYPE::MAIN_STREAM == streamType) ? 1 : 2; // 0-主辅码流,1-主码流,2-辅码流
	if (!CLIENT_SetDeviceMode(m_lLoginID, DH_RECORD_STREAM_TYPE, &nStreamType))
	{
		ret = DHSDK_GetLastError;
		LOG_ERROR("设置回放时的码流类型，CLIENT_SetDeviceMode DH_RECORD_STREAM_TYPE Fail! Errcode:%d", ret);
		return ret;
	}
	// 设置回放时的录像文件类型
	NET_RECORD_TYPE emFileType = NET_RECORD_TYPE_ALL; // 所有录像
	if (!CLIENT_SetDeviceMode(m_lLoginID, DH_RECORD_TYPE, &emFileType))
	{
		ret = DHSDK_GetLastError;
		LOG_ERROR("设置回放时的录像文件类型，CLIENT_SetDeviceMode DH_RECORD_TYPE Fail! Errcode:%d", ret);
		return ret;
	}

	// 录像时间段
	NET_TIME	stStartTime;
	NET_TIME	stStopTime;
	sscanf(record.startTime.c_str(), "%04d-%02d-%02d %02d:%02d:%02d",
		&stStartTime.dwYear, &stStartTime.dwMonth, &stStartTime.dwDay,
		&stStartTime.dwHour, &stStartTime.dwMinute, &stStartTime.dwSecond);

	sscanf(record.endTime.c_str(), "%04d-%02d-%02d %02d:%02d:%02d",
		&stStopTime.dwYear, &stStopTime.dwMonth, &stStopTime.dwDay,
		&stStopTime.dwHour, &stStopTime.dwMinute, &stStopTime.dwSecond);

	NET_IN_PLAYBACK_BY_DATA_TYPE stIn = { sizeof(stIn) };
	NET_OUT_PLAYBACK_BY_DATA_TYPE stOut = { sizeof(stOut) };

	stIn.fDownLoadDataCallBack = RecordDataCallbackEx2;
	stIn.emDataType = EM_REAL_DATA_TYPE_GBPS;
	stIn.nChannelID = m_nChannel;
	stIn.hWnd = NULL;
	//stIn.dwUser = reinterpret_cast<LDWORD>(this);
	stIn.dwDataUser = reinterpret_cast<LDWORD>(this);
	stIn.stStartTime = stStartTime;
	stIn.stStopTime = stStopTime;

	m_lRecordPlayHandle = CLIENT_PlayBackByDataType(m_lLoginID, &stIn, &stOut, 5000);
	if (0 == m_lRecordPlayHandle)//失败
	{
		ret = DHSDK_GetLastError;
		LOG_ERROR("历史回放，CLIENT_PlayBackByTimeEx Fail! Errcode:%d", ret);
		return ret;
	}
	_is_run = true;

	//m_bPause = TRUE;
	//pausePlayBack();
	return ret;
}

int PlaySession::stopSession()
{
	_is_run = false;

	int ret = 0;
	if (0 == m_lRecordPlayHandle && 0 == m_lDowloadPlayHandle && 0 == m_lRealPlayHandle)
		return ret;

	if (0 != m_lRecordPlayHandle)
	{
		LOG_INFO("stop RecordSession:%d", m_nChannel);
		if (!CLIENT_StopPlayBack(m_lRecordPlayHandle))
		{
			ret = DHSDK_GetLastError;
			LOG_ERROR("CLIENT_StopPlayBack  Fail! Errcode:%d", ret);
		}
		m_lRecordPlayHandle = 0;
	}

	if (0 != m_lRealPlayHandle)
	{
		LOG_INFO("stop RealPlay:%d", m_nChannel);
		if (!CLIENT_StopRealPlayEx(m_lRealPlayHandle))
		{
			ret = DHSDK_GetLastError;
			LOG_ERROR("CLIENT_StopRealPlayEx  Fail! Errcode:%d", ret);
		}
		m_lRealPlayHandle = 0;
	}

	if (0 != m_lDowloadPlayHandle)
	{
		LOG_INFO("stop Download:%d", m_nChannel);
		if (!CLIENT_StopDownload(m_lDowloadPlayHandle))
		{
			ret = DHSDK_GetLastError;
			LOG_ERROR("CLIENT_StopDownload  Fail! Errcode:%d", ret);
		}
		m_lDowloadPlayHandle = 0;
	}

	disconnetMS();//关闭流媒体链接

	return ret;
}

int PlaySession::pausePlayBack(BOOL bPause)
{//TRUE 暂停 FALSE 播放
	int ret = 0;
	m_bPause = bPause;
	if (!CLIENT_PausePlayBack(m_lRecordPlayHandle, m_bPause))
	{
		m_bPause = !m_bPause;
		ret = DHSDK_GetLastError;
		LOG_ERROR("CLIENT_PausePlayBack  Fail! Errcode:%d", ret);
	}

	return ret;
}

int PlaySession::seekPlayBack(const std::string& time)
{//拖动播放
	int ret = 0;
	unsigned int  offsettime = 0;//相对开始处偏移时间，单位为秒，当其值为0xffffffff时，该参数无效。

	offsettime = StringToDatetime(time) - StringToDatetime(m_sStartTime);
	return seekPlayBack(offsettime);
}

int PlaySession::seekPlayBack(const unsigned int& time)
{
	int ret = 0;
	if (!CLIENT_SeekPlayBack(m_lRecordPlayHandle, time, 0xffffffff))
	{
		ret = DHSDK_GetLastError;
		LOG_ERROR("CLIENT_SeekPlayBack  Fail! Errcode:%d", ret);
	}

	return ret;
}

int PlaySession::playBackControlDirection()
{//控制播放方向--正放或者倒放
	int ret = 0;
	m_bBackward = !m_bBackward;
	if (!CLIENT_PlayBackControlDirection(m_lRecordPlayHandle, m_bBackward))
	{
		m_bBackward = !m_bBackward;
		ret = DHSDK_GetLastError;
		LOG_ERROR("CLIENT_PlayBackControlDirection  Fail! Errcode:%d", ret);
	}

	return ret;
}

int PlaySession::setSpeed(int type/*,float speed*/)
{
	int ret = 0;
	BOOL DHRET = FALSE;
	float speed = 1;
	if (_FastPlay == type)//speed >  m_nSpeed)
	{
		DHRET = CLIENT_FastPlayBack(m_lRecordPlayHandle);//快放，将当前帧率提高一倍，但是不能无限制的快放，目前最大200帧，大于时返回FALSE
		speed = 2 * m_nSpeed;
	}
	else if (_SlowPlay == type)//speed < m_nSpeed)
	{
		DHRET = CLIENT_SlowPlayBack(m_lRecordPlayHandle);//慢放，将当前帧率降低一倍，最慢为每秒一帧，小于1则返回FALSE，在打开图像的函数参数hWnd为0时，设备支持回放速度控制情况下，此函数才会起作用
		speed = 1 / 2 * m_nSpeed;
	}
	else//_NormalPlay
	{//恢复正常播放速度
		DHRET = CLIENT_NormalPlayBack(m_lRecordPlayHandle);
		speed = 1;
	}

	if (!DHRET)
	{
		ret = DHSDK_GetLastError;
		LOG_ERROR("setSpeed  Fail! Errcode:%d", ret);
	}
	else
		m_nSpeed = speed;
	return ret;
}
int PlaySession::setSpeed(float speed)
{
	int ret = 0;
	BOOL DHRET = TRUE;
	if (1.0f == speed)
		//_NormalPlay
	{//恢复正常播放速度
		DHRET = CLIENT_NormalPlayBack(m_lRecordPlayHandle);
	}
	else if (speed > m_nSpeed)
	{
		int n = sqrt(speed) / sqrt(m_nSpeed);
		for (int i = 0; i < n; ++i)
		{
			DHRET = CLIENT_FastPlayBack(m_lRecordPlayHandle);//快放，将当前帧率提高一倍，但是不能无限制的快放，目前最大200帧，大于时返回FALSE
			if (DHRET)
				break;
			speed = 2 * m_nSpeed;
		}
	}
	else
	{
		int n = sqrt(m_nSpeed) / sqrt(speed);
		for (int i = 0; i < n; ++i)
		{
			DHRET = CLIENT_SlowPlayBack(m_lRecordPlayHandle);//慢放，将当前帧率降低一倍，最慢为每秒一帧，小于1则返回FALSE，在打开图像的函数参数hWnd为0时，设备支持回放速度控制情况下，此函数才会起作用
			if (DHRET)
				break;
			speed = 1 / 2 * m_nSpeed;
		}
	}

	if (!DHRET)
	{
		ret = DHSDK_GetLastError;
		LOG_ERROR("setSpeed  Fail! Errcode:%d", ret);
	}
	else
		m_nSpeed = speed;
	return ret;
}

int PlaySession::recordDataCallBack(LLONG lPlayHandle, DWORD dwDataType, BYTE* pBuffer, DWORD dwBufSize, LDWORD dwUser)
{
	PlaySession* session = reinterpret_cast<PlaySession*>(dwUser);
	if (session)
	{
		while (session->m_bPause)//回放
			std::this_thread::yield();
		//while (session->m_bDownloadPause)//需要下载接口再优化
		//    std::this_thread::yield()

		session->afterDHReadDataCallBack(pBuffer, dwBufSize);
	}
	return 0;
}
//void PlaySession::posCallBack(LLONG lPlayHandle, DWORD dwTotalSize, DWORD dwDownLoadSize, LDWORD dwUser)
//{
//    PlaySession* session = reinterpret_cast<PlaySession*>(dwUser);
//    if(session)
//    {
//        LOG_INFO("Download session:%ll  LoadSize:%d TotalSize:%d.", lPlayHandle,dwDownLoadSize,dwTotalSize);
//        //do something
//        //session->
//    }
//    return;
//}

int PlaySession::startDownloadPlaySession(DH_PLAY_STREAM_TYPE streamType)
{
	int ret = 0;
	LOG_INFO("start Download:", m_nChannel);
	// 设置下载时的码流类型
	int nStreamType = (DH_PLAY_STREAM_TYPE::MAIN_STREAM == streamType) ? 1 : 2; // 0-主辅码流,1-主码流,2-辅码流
	if (!CLIENT_SetDeviceMode(m_lLoginID, DH_RECORD_STREAM_TYPE, &nStreamType))
	{
		ret = DHSDK_GetLastError;
		LOG_ERROR("设置下載时的码流类型，CLIENT_SetDeviceMode DH_RECORD_STREAM_TYPE Fail! Errcode:%d", ret);
		return ret;
	}
	// 设置下载时的录像文件类型
	NET_RECORD_TYPE emFileType = NET_RECORD_TYPE_ALL; // 所有录像
	if (!CLIENT_SetDeviceMode(m_lLoginID, DH_RECORD_TYPE, &emFileType))
	{
		ret = DHSDK_GetLastError;
		LOG_ERROR("设置下载时的录像文件类型，CLIENT_SetDeviceMode DH_RECORD_TYPE Fail! Errcode:%d", ret);
		return ret;
	}

	m_lDowloadPlayHandle = CLIENT_DownloadByTimeEx(m_lLoginID, m_nChannel, 0, &m_stStartTime, &m_stStopTime, NULL/*sSavedFileName 要保存的录像文件名，全路径*/,
		NULL, NULL,//&DownloadSession::posCallBack,  reinterpret_cast<LDWORD>(this), //cbDownLoadPos  进度回调用户参数，说明参见:CLIENT_PlayBackByRecordFile
		&PlaySession::readDataCallBack, reinterpret_cast<LDWORD>(this)); //数据回调函数,说明参见:CLIENT_PlayBackByRecordFileEx

	if (0 == m_lDowloadPlayHandle)//失败
	{
		ret = DHSDK_GetLastError;
		LOG_ERROR("录像下载，CLIENT_PlayBackByTimeEx Fail! Errcode:%d", ret);
		return ret;
	}

	return ret;
}

int PlaySession::pauseDownload(bool bPause)
{//TRUE 暂停 FALSE 播放
	int ret = 0;
	m_bDownloadPause = bPause;
	// if(!CLIENT_PausePlayBack(m_lPlayHandle,bPause))
	// {
	//    // m_bPause = !m_bPause;
	//     ret = DHSDK_GetLastError;
	//     LOG_ERROR("CLIENT_PausePlayBack  Fail! Errcode:%d",ret );
	// }

	return ret;
}

int PlaySession::getPos(int pos, int total)
{//CLIENT_GetDownloadPos
	int ret = 0;
	if (0 == m_lDowloadPlayHandle)
		return ret;
	LOG_INFO("getPos:%d", m_nChannel);
	if (!CLIENT_GetDownloadPos(m_lDowloadPlayHandle, &total, &pos))
	{
		ret = DHSDK_GetLastError;
		LOG_ERROR("CLIENT_GetDownloadPos  Fail! Errcode:%d", ret);
	}
	return 0;
}

int PlaySession::connetMS(DispatchInfo dispatch)
{
	LOG_CONSOLE("mediaserverIP:%s:%d 开始连接流媒体", dispatch.ip.c_str(), dispatch.port);
	int val = XHNetSDK_Connect((int8_t*)dispatch.ip.c_str(), dispatch.port, (int8_t*)(NULL), 0, (uint64_t*)&nMediaClient, onread, onclose, onconnect, 0, 5 * 1000, 1);
	if (val == 0)
	{
		LOG_CONSOLE("mediaserverIP:%s:%d 链接成功 Client:%llu", dispatch.ip.c_str(), dispatch.port, nMediaClient);
	}
	else
	{
		LOG_ERROR("mediaserverIP:%s:%d 链接失败 错误码:%d", dispatch.ip.c_str(), dispatch.port, val);
	}
	return val;
}

void PlaySession::disconnetMS()
{
	if (nMediaClient == 0)
	{
		LOG_CONSOLE("nMediaClient==0，关闭推流默认成功");
		return;
	}
	int val = XHNetSDK_Disconnect(nMediaClient);
	if (val == 0)
	{
		LOG_CONSOLE("关闭推流成功, 流媒体句柄:%d", nMediaClient);
		nMediaClient = 0;
		return;
	}
	LOG_ERROR("关闭推流失败，流媒体句柄:%d，错误码:%d", nMediaClient, val);
}

int PlaySession::sendMS(uint8_t* pRtpVideo, uint32_t nDataLength)
{
	int nSendRet = 0;

	if (_is_run == false)
		return nSendRet;

	if (nMediaClient != 0 && (nMaxRtpSendVideoMediaBufferLength - nSendRtpVideoMediaBufferLength < nDataLength + 4) && nSendRtpVideoMediaBufferLength > 0)
	{//剩余空间不够存储 ,防止出错
		nSendRet = XHNetSDK_Write(nMediaClient, szSendRtpVideoMediaBuffer, nSendRtpVideoMediaBufferLength, 1);
		if (nSendRet != 0)
		{
			LOG_ERROR("发送国标RTP码流出错 ，Length:%d, nSendRet:%d %s", nSendRtpVideoMediaBufferLength, nSendRet, (nSendRet == 16777218) ? "句柄失效" : "");
			_is_run = false;
			nMediaClient = 0;
			return nSendRet;
		}

		nSendRtpVideoMediaBufferLength = 0;
	}

	nCurrentVideoTimestamp++;
	//memcpy((char*)&nCurrentVideoTimestamp, pRtpVideo + 4, sizeof(uint32_t));
	if (nMediaClient != 0 && nStartVideoTimestamp != GB28181VideoStartTimestampFlag && nStartVideoTimestamp != nCurrentVideoTimestamp && nSendRtpVideoMediaBufferLength > 0)
	{//产生一帧新的视频
		nSendRet = XHNetSDK_Write(nMediaClient, szSendRtpVideoMediaBuffer, nSendRtpVideoMediaBufferLength, 1);
		if (nSendRet != 0)
		{
			LOG_ERROR("clihandle:%llu 发送国标RTP码流出错，Length:%d, nSendRet:%d %s", nMediaClient, nSendRtpVideoMediaBufferLength, nSendRet, (nSendRet == 16777218) ? "句柄失效" : "");
			_is_run = false;
			nMediaClient = 0;
			return nSendRet;
		}

		nSendRtpVideoMediaBufferLength = 0;
	}

	if (1)
	{//国标 TCP发送 4个字节方式
		szSendRtpVideoMediaBuffer[nSendRtpVideoMediaBufferLength + 0] = '$';
		szSendRtpVideoMediaBuffer[nSendRtpVideoMediaBufferLength + 1] = 0;
		nVideoRtpLen = htons(nDataLength);
		memcpy(szSendRtpVideoMediaBuffer + (nSendRtpVideoMediaBufferLength + 2), (unsigned char*)&nVideoRtpLen, sizeof(nVideoRtpLen));
		memcpy(szSendRtpVideoMediaBuffer + (nSendRtpVideoMediaBufferLength + 4), pRtpVideo, nDataLength);

		nStartVideoTimestamp = nCurrentVideoTimestamp;
		nSendRtpVideoMediaBufferLength += nDataLength + 4;
	}
	else
	{//国标 TCP发送 2 个字节方式
		nVideoRtpLen = htons(nDataLength);
		memcpy(szSendRtpVideoMediaBuffer + nSendRtpVideoMediaBufferLength, (unsigned char*)&nVideoRtpLen, sizeof(nVideoRtpLen));
		memcpy(szSendRtpVideoMediaBuffer + (nSendRtpVideoMediaBufferLength + 2), pRtpVideo, nDataLength);

		nStartVideoTimestamp = nCurrentVideoTimestamp;
		nSendRtpVideoMediaBufferLength += nDataLength + 2;
	}

	return nSendRet;
}

PlaySessionPtr SessionMange::GetPlaySession(std::string key)
{
	boost::lock_guard<boost::mutex> lock(session_map_lock);

	auto it = session_map.find(key);
	if (it != session_map.end())
	{
		return it->second;
	}
}

bool SessionMange::InsertPlaySession(std::string key, PlaySessionPtr p)
{
	boost::lock_guard<boost::mutex> lock(session_map_lock);
	if (p)
	{
		session_map.insert(std::make_pair(key, p));
		return true;
	}
	return false;
}

void SessionMange::DelPlaySession(std::string key)
{
	boost::lock_guard<boost::mutex> lock(session_map_lock);
	auto it = session_map.find(key);
	if (it != session_map.end())
	{
		session_map.erase(it);
	}
	return;
}