#pragma once
#include <boost/thread.hpp>
#include <memory>
#include <string>
//#include "rabbitmqclient.h"
#include "rabbitMQ.h"
#include "gateway_msg.h"
#include "event_condition_variable.h"

class client_manage
{
public:
	client_manage();
	~client_manage();

	bool init_client();

	bool start_public_queue();
	bool start_business_queue();
	bool restart_business_queue();
	void stop();

	bool check_channel_statu();

	std::string get_client_type() { return _client_type_str; }
	bool set_client_type(int type);//1:rabbitmq

	bool reconnet_public(std::string ip, int port, std::string user, std::string pws);

	bool connet_public(std::string ip, int port, std::string user, std::string pws);
	bool connet_business(std::string ip, int port, std::string user, std::string pws);
	void disconnet_public();
	void disconnet_business();

	bool send_public_queue(std::string msg);
	bool send_business_queue(std::string msg);
	bool send_business_queueEx(std::string msg);

	void wait_public()
	{
		_public_ev.wait();
	}
	void wait_business()
	{
		_business_ev.wait();
	}

private:
	void heartbeat_func();
	void disconnet_func();
	void sent_business_func();
	event_condition_variable _disconnet_ev;
	gateway_msg gm;
	bool reconnet;//重连中

	event_condition_variable _business_msg_list_ev;
	boost::mutex _business_msg_list_mtx;
	std::vector<std::string> _business_msg_list;
private://http

private://Rabbitmq
	rabbitmqClient objRabbitmq;
	std::string strExchange = "PUBLIC";
	std::string strRoutekey = "rundo.public.sg";
	std::string strQueuename = "rundo.public.sg";
	std::string strRecvRoutekey = "rundo.public.gs";
	std::string strRecvQueuename = "rundo.public.gs";
	std::string serialNum ;

	int mqListenTimeout;//秒
	int heartbeatTimeout;//秒

	rabbitmqClient objDynamicRabbitmq_recv;
	rabbitmqClient objDynamicRabbitmq_send;
	std::string strDynamicQueuename;//动态业务队列
	std::string strDynamicRoutekey ;//动态路由
	std::string strDynamicExchange ;

	int commo_public_channel;//公共发布队列通道
	int commo_consumer_channel;//公共监听队列通道
	int business_public_channel;//业务发布队列通道
	int business_consumer_channel;//业务监听队列通道
private:
	boost::posix_time::ptime lastSendTime;

	event_condition_variable _public_ev;
	event_condition_variable _business_ev;

	bool _republic;
	bool while_flag;
	int _client_type;
	std::string _client_type_str;

	boost::shared_ptr<boost::thread> _send_msg_thread;//
	boost::shared_ptr<boost::thread> _handle_disconnet_thread;//
	boost::shared_ptr<boost::thread> thread_new;
	bool exchangedeclare_flags;
	bool exchangedeclare_flagr;

	std::string _ip;
	int _port;
	std::string _user;
	std::string _pws;
};
