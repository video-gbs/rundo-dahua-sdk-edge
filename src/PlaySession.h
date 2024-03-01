#ifndef  __PLAYSESSION_H__
#define  __PLAYSESSION_H__
//#include "DHAccessdef.h"
#include <boost/serialization/singleton.hpp>
#include <boost/unordered_map.hpp>
#include <boost/make_shared.hpp>
#include <boost/thread.hpp>
#include <functional>
#include<string>
#include <sstream>
#include <boost/shared_ptr.hpp>
#include "common.h"
#include "XHNetSDK.h"

#include "mpeg-ps.h"
#include "rtp_packet.h"
//#include "util/tc_thread_pool.h"

#define  MaxGB28181RtpSendVideoMediaBufferLength  1024*64
#define  GB28181VideoStartTimestampFlag           0xEFEFEFEF   //视频开始时间戳
#define INPUTSRC_BUFFER_SIZE	(1920*1080*10)
namespace XH_TYPE {
	enum E_SDK_MEDIA_TYPE
	{
		E_SDK_MEDIA_UNKNOWN = 0,
		E_SDK_MEDIA_VIDEO,
		E_SDK_MEDIA_AUDIO,
		E_SDK_MEDIA_PRIVATE
	};

	enum E_SDK_STREAM_TYPE
	{
		E_SDK_STREAM_UNKNOWN = 0,
		E_SDK_STREAM_ES,
		E_SDK_STREAM_PS,
		E_SDK_STREAM_TS
	};

	enum E_SDK_CODEC_TYPE
	{
		E_SDK_CODEC_UNKNOWN = 0,
		E_SDK_CODEC_ADTS = 0x0f, // aac
		E_SDK_CODEC_H264 = 0x1b, // h264
		E_SDK_CODEC_H265 = 0x24, // h265
		E_SDK_CODEC_SVACV = 0x80, // svac video
		E_SDK_CODEC_PCM = 0x81, // pcm
		E_SDK_CODEC_G711A = 0x90, // g711 a
		E_SDK_CODEC_G711U = 0x91, // g711 u
		E_SDK_CODEC_G7221 = 0x92, // g722.1
		E_SDK_CODEC_G7231 = 0x93, // g723.1
		E_SDK_CODEC_G729 = 0x99, // g729
		E_SDK_CODEC_SVACA = 0x9b, // svac audio
		E_SDK_CODEC_HIKP = 0xf0, // HK private
		E_SDK_CODEC_HWP = 0xf1, // HW private
		E_SDK_CODEC_DHP = 0xf2 // DH private
	};

	enum E_SDK_FRAME_TYPE
	{
		E_SDK_FRAME_UNKNOWN = 0,
		E_SDK_FRAME_I,
		E_SDK_FRAME_P,
		E_SDK_FRAME_B
	};
};

struct DHStreamParam
{
	int32_t mediaType;      // SDKModuleWrapper::E_SDK_MEDIA_TYPE
	int32_t streamType;     // SDKModuleWrapper::E_SDK_STREAM_TYPE   //此回调都是裸码流（ES）
	int32_t encType;        // SDKModuleWrapper::E_SDK_CODEC_TYPE
	int32_t frameType;      // SDKModuleWrapper::E_SDK_FRAME_TYPE  I P B
	int64_t timeStamp;
	uint8_t* data;
	uint32_t size;

	DHStreamParam()
		:mediaType(0), streamType(0), encType(0), frameType(0), timeStamp(0), data(nullptr), size(0)
	{
	}
	~DHStreamParam()
	{
		if (data)
			delete[] data;
	}
};

class PlaySession
{
public:
	//PlaySession();
	PlaySession(LLONG handle, int channel, std::string streamid, void* pUser = nullptr);
	~PlaySession();

	NETHANDLE get_clihandle() {
		return nMediaClient;
	}

public:
	void afterDHReadDataCallBack(BYTE* streambuf, DWORD bufSize);
	bool setMSAddr(std::string ip, int port, int link_type);
	//实时
	int startRealPlaySession(DH_PLAY_STREAM_TYPE streamType = DH_PLAY_STREAM_TYPE::MAIN_STREAM);
	//回放
	int startRecordPlaySession(Record record, DH_PLAY_STREAM_TYPE streamType = DH_PLAY_STREAM_TYPE::MAIN_STREAM);
	int stopSession();
	int pausePlayBack(BOOL bPause);//网络回放暂停与恢复播放
	int seekPlayBack(const std::string& time);//拖动播放
	int seekPlayBack(const unsigned int& time);//拖动播放
	int playBackControlDirection();//控制播放方向--正放或者倒放
	//type:0:慢放 1：正常 2:快放 speed: 1/8 1/4 1/2 1.0 2 4 8
	int setSpeed(int type/*,float speed*/);
	int setSpeed(float speed);

	//下载
	int startDownloadPlaySession(DH_PLAY_STREAM_TYPE streamType = DH_PLAY_STREAM_TYPE::MAIN_STREAM);
	int stopDownloadPlaySession();
	int pauseDownload(bool bPause);//网络回放暂停与恢复播放
	int getPos(int pos, int total);

	int es_to_ps(uint8_t* data, uint32_t datasize);
	int ps_to_rtp(uint8_t* data, uint32_t datasize);
	bool creat_packet(int videoEncType, int audioEncType);

	int sendMS(uint8_t* data, uint32_t datasize);
	void update_source_stream();
private:
	//连接流媒体
	int connetMS(DispatchInfo dispatch);
	void disconnetMS();
private:
	/*
   lPlayHandle     回放ID
   dwDataType  标识回调出来的数据类型，只有dwFlag设置标识的数据才会回调出来：
   pBuffer     回调数据，根据数据类型的不同每次回调不同的长度的数据，除类型0，其他数据类型都是按帧，每次回调一帧数据
   dwBufSize   回调数据的长度，根据不同的类型，长度也不同(单位字节)
   dwUser      用户数据，就是上面输入的用户数据
*/
	static int readDataCallBack(LLONG lPlayHandle, DWORD dwDataType, BYTE* pBuffer, DWORD dwBufSize, LDWORD dwUser);
	static int recordDataCallBack(LLONG lPlayHandle, DWORD dwDataType, BYTE* pBuffer, DWORD dwBufSize, LDWORD dwUser);

	/*进度回调
		lPlayHandle  CLIENT_PlayBackByRecordFile的返回值
		dwTotalSize  指本次播放总大小，单位为KB
		dwDownLoadSize  指已经播放的大小，单位为KB，当其值为-1时表示本次回放结束，-2表示写文件失败
		dwUser  用户数据，就是上面输入的用户数据
	*/
	static void posCallBack(LLONG lPlayHandle, DWORD dwTotalSize, DWORD dwDownLoadSize, LDWORD dwUser);
public:
	uint32_t    m_streamtype; //1 实时 2 历史

	int                     nMaxRtpSendVideoMediaBufferLength;//实际上累计视频，音频总量 ，如果国标发送视频时，可以为64K，如果只发送音频就设置640即可
	unsigned  char          szSendRtpVideoMediaBuffer[MaxGB28181RtpSendVideoMediaBufferLength];
	int                     nSendRtpVideoMediaBufferLength; //已经积累的长度  视频
	uint32_t                nStartVideoTimestamp; //上一帧视频初始时间戳 ，
	uint32_t                nCurrentVideoTimestamp;// 当前帧时间戳
	unsigned short          nVideoRtpLen;

	std::string stream_id;
	int		m_nChannel;				// camera channel
	uint32_t source_stream_count_;
	int64_t source_stream_timestamp_;
	bool _is_frist;
	bool _is_run;
	char* s_buffer;
protected:
	uint64_t	  videoPTS, audioPTS;
	int           nVideoStreamID, nAudioStreamID;
	struct ps_muxer_func_t handler;
	ps_muxer_t* _ps_muxer;

	_rtp_packet_sessionopt  optionPS;
	_rtp_packet_input       inputPS;
	uint32_t                hRtpPS;

	DispatchInfo ms_dispatch;
	NETHANDLE nMediaClient;//流媒体连接句柄

	LLONG       m_lLoginID;//设备登陆句柄
	std::atomic<LLONG>       m_lDowloadPlayHandle;//播放句柄
	std::atomic<LLONG>       m_lRecordPlayHandle;//播放句柄
	std::atomic<LLONG>       m_lRealPlayHandle;//播放句柄
	std::string addr;			// ip与端口
	bool        m_bGetIFrame;//是否已经获取到I帧
	unsigned long long m_llDataFrameCount; //数据回调的次数;
	time_t      m_lastRecvDataTime; //最后一次回调时间

	uint64_t m_originVideoTimestamp;
	uint64_t m_videoTimestamp;
	//  uint64_t m_changeVideoTimestamp;//sdk回调时间戳超过65535会重新累计，需要自己记录
	uint64_t m_originAudioTimestamp;
	uint64_t m_audioTimestamp;

	uint32_t    m_cbPlayHandle;
	void* m_cbUserData;
	//DHStreamParser m_Parser;//解析库

	//回放
	std::string m_sStartTime;			// 20190508101010
	std::string m_sEndTime;			// 20190508121010
	NET_TIME	m_stStartTime;
	NET_TIME	m_stStopTime;
	float 		m_nSpeed;				// speed
	BOOL        m_bPause;//网络回放暂停与恢复播放参数 1:暂停，0:恢复
	BOOL        m_bBackward;//是否倒放，在 bBackward = TRUE 时倒放，在 bBackward = FALSE 时正放

	//下载
	volatile std::atomic<bool> m_bDownloadPause;//暂停
};
typedef boost::shared_ptr<PlaySession> PlaySessionPtr;

//会话管理单例
class SessionMange
{
public:
	SessionMange() {}
	~SessionMange() {}

	PlaySessionPtr GetPlaySession(std::string key);
	bool InsertPlaySession(std::string key, PlaySessionPtr p);
	void DelPlaySession(std::string key);
	void DisconnetPlaySession(NETHANDLE clihandle);
	int GetPlaySessionCount()
	{
		boost::lock_guard<boost::mutex> lock(session_map_lock);
		return session_map.size();
	}
private:
	boost::unordered_map<std::string, PlaySessionPtr>session_map;
	boost::mutex session_map_lock;
};
typedef boost::serialization::singleton<SessionMange> SessionMange_singleton;

#endif // !__DHHANDLER_H__
