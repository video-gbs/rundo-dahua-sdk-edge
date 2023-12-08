// DHGatewayServer.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <Windows.h>

#include <stdio.h>
#include <stdlib.h>
//#include <unistd.h>
#include <iostream>
#include <thread>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>
#include "log.h"

//#include "rabbitmqclient.h"
#include "common.h"
#include "server_manage.h"
#include "rabbitMQ.h"
#include "gateway_msg.h"

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "amqp_tcp_socket.h"
#include "config_manage.h"

//#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Crypt32.lib")
#pragma comment(lib, "libeay32.lib")
#pragma comment(lib, "ssleay32.lib")
#pragma comment(lib, "librabbitmq.4.lib")

#pragma comment(lib,"dhnetsdk.lib")
#pragma comment(lib,"MySQLDLL.lib")
#pragma comment(lib,"XHNetSDK.lib")
#pragma comment(lib,"rtppacket.lib")
#pragma comment(lib,"libmpeg.lib")

std::string strIP = "124.71.16.209";
int iPort = 5672;
std::string strUser = "wvp";
std::string strPasswd = "wvp12345678";

std::string strExchange = "PUBLIC";
std::string strRoutekey = "rundo.public.sg";
std::string strQueuename = "rundo.public.sg";
std::string strRecvRoutekey = "rundo.public.gs";
std::string strRecvQueuename = "rundo.public.gs";
std::string serialNum = "29fc51254c0c4809a9f4851f994111a2";

dc_thread_pool::thread_pool sdk_thread(5);

void LIBNET_CALLMETHOD	onaccept(NETHANDLE srvhandle,
	NETHANDLE clihandle,
	void* address)
{
	LOG_CONSOLE("clihandle:%d onaccept succese", clihandle);
}

void LIBNET_CALLMETHOD	onconnect(NETHANDLE clihandle,
	uint8_t result)
{
	if (result == 0)
	{//失败
		LOG_ERROR("clihandle:%d onconnect false", clihandle);
	}
	else if (result == 1)
	{//成功
		LOG_CONSOLE("clihandle:%d onconnect succese", clihandle);
	}
}
void LIBNET_CALLMETHOD onread(NETHANDLE srvhandle,
	NETHANDLE clihandle,
	uint8_t* data,
	uint32_t datasize,
	void* address)
{
	if (srvhandle == 0)
	{
		return;
	}
	std::string s((char*)data, datasize);
	LOG_INFO("clihandle:%llu temp_str1：%s", clihandle, s.c_str());

	/*size_t post = s.find("POST");
	size_t get = s.find("GET");
	size_t startPos = s.find("{");
	size_t endPos = s.rfind("}");
	std::string respon;
	if (startPos == std::string::npos && endPos == std::string::npos && post != std::string::npos)
	{
		LOG_INFO("clihandle:%llu temp_str2：%s", clihandle, s.c_str());
		temp_msg += s;
		return;
	}
	if (startPos != std::string::npos && endPos != std::string::npos && get == std::string::npos)
	{
		LOG_INFO("clihandle:%llu temp_str3：%s", clihandle, s.c_str());
		temp_msg += s;
	}
	if (get != std::string::npos)
	{
		temp_msg += s;
	}

	HTTP_REQUEST_INFO msg;
	msg._msg = temp_msg;
	msg._req_handle = clihandle;
	LOG_CONSOLE("srvhandle:%llu clihandle:%llu datasize:%d %s", srvhandle, clihandle, datasize, msg._msg.c_str());
	http_api_singleton::get_mutable_instance().push_requst(msg);

	http_api_singleton::get_mutable_instance().push_ev();

	temp_msg = "";*/
}

void LIBNET_CALLMETHOD	onclose(NETHANDLE srvhandle,
	NETHANDLE clihandle)
{
	LOG_CONSOLE("clihandle:%d onclose.", clihandle);
	//session_mgr_singleton::get_mutable_instance().disconnection_session(clihandle);
}

std::string get_application_directory_path()
{
	std::string path;

	boost::filesystem::path fullpath = boost::filesystem::initial_path();
	path = fullpath.string();

	return path;
}

int main(int argc, char* argv[])
{
	XHNetSDK_Init(2, 8);

	gateway_msg gm;

	//initialize log
	std::string strLogPath = get_application_directory_path() + "\\log\\";
	std::string strAppName(argv[0]);
	std::string::size_type pos = strAppName.find(".exe");
	if (std::string::npos != pos)
	{
		strAppName = strAppName.substr(0, pos);
	}
	pos = strAppName.find_last_of("\\");
	if (std::string::npos != pos)
	{
		strAppName = strAppName.substr(pos + 1, strAppName.size() - pos - 1);
	}
	strLogPath += strAppName;
	ns_server_base::init_log(strLogPath.c_str(), static_cast<ns_server_base::e_custom_severity_level>(ns_server_base::e_custom_severity_level::e_info_log), true);
	LOG_CONSOLE("启动大华网关");

	Config config_;
	config_manager_singleton::get_mutable_instance().add_loacl_media_config(argv[0]);
	config_manager_singleton::get_mutable_instance().get_media_config(config_);

	server_manage_singleton::get_mutable_instance().start_server(config_.MQIp, config_.MQPort, config_.MQUser, config_.MQPsw);

	//client_manage _client;
	//_client.set_client_type(1);
	//_client.connet(strIP, iPort, strUser, strPasswd);
	////_client.send_public_queue(gm.gateway_sign_in());
	////_client.send_public_queue(gm.gateway_heartbeat());
	////_client.start_public_queue();

	//_client.start_business_queue();
	while (true)
	{
		Sleep(1000);
	}
	server_manage_singleton::get_mutable_instance().stop_server();
	return 0;
}