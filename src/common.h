#pragma once
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <chrono>
#include <string>
#include <sstream>
#include <random>
#include "xpack-master/json.h"

enum GATEWAY_MSG_COM
{//公共队列消息枚举:
	//注册
	GATEWAY_SIGN_IN = 1,
	//心跳
	GATEWAY_HEARTBEAT,
	//重新注册
	GATEWAY_RE_SIGN_IN,
};

enum GATEWAY_MSG_BUSINESS
{//业务队列的消息枚举：/********设备通道服务相关*************/
	//设备注册
	REGISTER,
	//设备信息
	DEVICEINFO,
	DEVICE_ADD,
	//设备删除//设备软删除
	DEVICE_DELETE,
	DEVICE_DELETE_SOFT,
	//通道软硬删除
	CHANNEL_DELETE_HARD,
	CHANNEL_DELETE_SOFT,
	//删除恢复
	CHANNEL_DELETE_RECOVER,
	DEVICE_DELETE_RECOVER,
	//设备通道
	CATALOG,
	//云台控制
	PTZ_CONTROL,
	//录像列表
	RECORD_INFO,
	//全量设备信息
	DEVICE_TOTAL_SYNC,
	//设备录像倍速
	DEVICE_RECORD_SPEED,
	//回放拖动播放
	DEVICE_RECORD_SEEK,
	//回放暂停
	DEVICE_RECORD_PAUSE,
	//回放恢复
	DEVICE_RECORD_RESUME,
	//预置位--操作查询
	CHANNEL_PTZ_PRESET,
	//云台控制操作
	CHANNEL_PTZ_OPERATION,
	//拉框放大/缩小
	CHANNEL_3D_OPERATION,
	/******调度服务调用网关业务队列场景*************/
	//点播
	PLAY,
	//回放
	PLAY_BACK,
	//停播调度服务请求网关
	STOP_PLAY,
};

enum error_code
{
	socket_error = 10,//a socket error occurred
	timeout_error = 20,
};

enum msg_type
{
	msg_GATEWAY_SIGN_IN = 1,//注册结果
	msg_DEVICE_ADD,
	msg_CHANNEL_SYNC,
	msg_DEVICE_TOTAL_SYNC,
	msg_DEVICE_DELETE,
	msg_DEVICE_DELETE_SOFT,
	msg_DEVICE_DELETE_RECOVER,
	msg_CHANNEL_DELETE_SOFT,
	msg_CHANNEL_DELETE_RECOVER,
	msg_CHANNEL_PTZ_CONTROL,
	msg_CHANNEL_PTZ_PRESET,
	msg_CHANNEL_3D_OPERATION,
	msg_CHANNEL_PLAY,
	msg_CHANNEL_RECORD_INFO,
	msg_CHANNEL_PLAYBACK,
	msg_CHANNEL_STOP_PLAY,
	msg_DEVICE_RECORD_PAUSE,
	msg_DEVICE_RECORD_RESUME,
	msg_DEVICE_RECORD_SPEED,
};

enum DH_PLAY_STREAM_TYPE
{
	MAIN_STREAM = 0,
	SUB_STREAM,
};//主辅码流

enum SetSpeedTyep
{
	_SlowPlay = 0,
	_NormalPlay,
	_FastPlay,
};

struct struct_GATEWAY_SIGN_IN
{
	int id;
	std::string ip;
	int port;
	std::string protocol;
	std::string gatewayType;
	long long outTime;//1693970447227

	XPACK(O(id, ip, port, protocol, gatewayType, outTime));// 添加宏定义XTOSTRUCT在结构体定义结尾
	struct_GATEWAY_SIGN_IN()
	{
		id = 0;
		port = 0;
		outTime = 0;
	}
	~struct_GATEWAY_SIGN_IN() {}
};

struct COMMO_GATEWAY
{
	std::string serialNum;//uuid
	std::string msgType;
	std::string msgId;
	std::string time;//pattern="yyyy-MM-ddHH:mm:ss",timezone="GMT+8"
	int code;
	std::string msg;
	std::string data;

	XPACK(O(serialNum, msgType, msgId, time, code, msg, data));// 添加宏定义XTOSTRUCT在结构体定义结尾
	COMMO_GATEWAY()
	{
		code = 0;
	}
	~COMMO_GATEWAY() {}
};

struct DispatchInfo
{
	std::string ip;
	int port;
	int link_type;//0:udp,1:tcp
};

struct LoginResults//登陆结果
{
	int64_t handle;
	int32_t chanNum;
	int64_t uid;
	std::string equipment_key;
};

struct PtzCMD
{
	std::string msgType;
	std::string msgId;
	std::string device_id;
	int channel;
	unsigned long contrl_cmd;
	unsigned char contrl_param;
	bool dwStop = false;
};

struct Preset
{
	std::string msgType;
	std::string msgId;
	std::string device_id;
	int channel;

	unsigned int index;
	std::string name;
};

struct Play
{
	std::string msgType;
	std::string msgId;
	std::string device_id;
	int channel;

	int streamMode;			//链接方式:1：udp，2：tcp
	std::string dispatchUrl;//调度服务地址
	std::string streamId;	//作为key，与流媒体的streamId一样
	std::string ip;			//无意义
	int port;				//流媒体端口
	std::string ssrc;
	std::string sdpIp;		//流媒体IP
	std::string ssrc_streamId;
	std::string mediaServerId;
};

struct Record
{
	std::string msgType;
	std::string msgId;
	std::string device_id;
	int channel;

	std::string fileName;			//文件名称
	int    fileSize;			//文件大小
	std::string filePos;			//文件位置
	std::string startTime;		//开始时间
	std::string endTime;			//结束时间

	int streamMode;
	std::string dispatchUrl;
	std::string streamId;
	std::string ip;
	int port;
	std::string ssrc;
	std::string sdpIp;
	std::string ssrc_streamId;
	std::string mediaServerId;
};

struct Platform
{
	int deviceType;//?
	std::string deviceId;//nvr的唯一编码
	std::string ip;
	int port;
	std::string username;
	std::string password;
	std::string msgId;
	int login_results;
	std::string login_err_msg;

	bool soft_del;//true:软删除，不向上级注册

	XPACK(O(deviceType, deviceId, ip, port, username, password, msgId, login_results, login_err_msg, soft_del));// 添加宏定义XTOSTRUCT在结构体定义结尾
	Platform() {
		soft_del = false;
		deviceType = 0;
		port = 0;
		login_results = 0;
		login_err_msg = "SUCCESS";

		// 获取当前时间戳
		auto now = std::chrono::system_clock::now();
		auto duration = now.time_since_epoch();
		auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();

		// 生成随机数
		std::random_device rd;
		std::mt19937_64 gen(rd());
		std::uniform_int_distribution<> dis(1, 1000);
		int randomPart = dis(gen);

		// 组合时间戳和随机数
		std::ostringstream oss;
		oss << millis << randomPart;

		deviceId = oss.str();
	}
	~Platform() {}
};

struct Platforms
{
	std::vector<Platform> platforms;
	//boost::unordered_map<std::string, Platform> platforms;

	XPACK(O(platforms));// 添加宏定义XTOSTRUCT在结构体定义结尾

	Platforms() {
	}
	~Platforms() {}
};

struct Device
{
	int id;//数据id
	std::string serialNumber;//序列号
	std::string name;	//设备名称
	int deviceType;		//设备类型
	std::string charset;//编码
	std::string deviceId;//设备ID
	std::string ip;
	int port;
	std::string username;
	std::string password;
	std::string manufacturer;
	std::string model;
	std::string firmware;
	int online;
	int deleted;
	std::string createdAt;
	std::string updatedAt;

	Device() {
		id = 0;
		online = 0;
		deleted = 0;
		port = 0;
	}
};