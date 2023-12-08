#include "client_manage.h"
#include <mutex>
#include "log.h"
#include "server_manage.h"

client_manage::client_manage()
{
	_client_type_str = "rabbitMQ";//默认是MQ，后续会增多HTTP类型
	commo_public_channel = 1;//公共发布队列通道
	commo_consumer_channel = 1;//公共监听队列通道
	business_public_channel = 3;//业务发布队列通道
	business_consumer_channel = 4;//业务监听队列通道
	exchangedeclare_flags = true;
	exchangedeclare_flagr = true;
}

client_manage::~client_manage()
{
	objRabbitmq.Disconnect();
}

void client_manage::heartbeat_func()
{
	LOG_INFO("start heartbeat_func");
	while (true)
	{
		Sleep(heartbeat_timeout / 3);
		bool b = send_public_queue(gm.gateway_heartbeat());
		if (b)
		{
			LOG_INFO("send heartbeat_func sessce");
		}
		else
		{
			LOG_ERROR("send heartbeat_func fail start reconnet_public");

			reconnet_public(_ip, _port, _user, _pws);
			send_public_queue(gm.gateway_heartbeat());
		}
		//Sleep(heartbeat_timeout / 3);
	}
}

bool client_manage::start_public_queue()
{
	while_flag = true;

	int iRet = 0;
	//监听接收队列，处理响应
	//可选操作（接收） Declare Queue
	iRet = objRabbitmq.QueueDeclare(strQueuename);
	LOG_CONSOLE("添加公共MQ声明 Ret:%d %s", iRet, strQueuename.c_str());

	// 可选操作（接收） Queue Bind
	iRet = objRabbitmq.QueueBind(strQueuename, strExchange, strRoutekey);
	LOG_CONSOLE("绑定公共MQ Ret:%d %s %s", iRet, strQueuename.c_str(), strExchange.c_str(), strRoutekey.c_str());

	//公共队列注册成功
	_public_ev.signal();

	LOG_CONSOLE("监听%s公共MQ中...", strRecvQueuename.c_str());
	while (while_flag)
	{
		// Recv Msg
		std::vector<std::string> vecRecvMsg;
		iRet = objRabbitmq.Consumer(strRecvQueuename, vecRecvMsg, 1, true);
		//LOG_CONSOLE("Public Consumer Ret: %d", iRet);

		for (size_t i = 0; i < vecRecvMsg.size(); ++i)
		{
			//LOG_CONSOLE("Public Consumer: %s", vecRecvMsg[i].c_str());
			std::string msgtype;
			gm.pars_gateway_sign(vecRecvMsg[i], msgtype, strDynamicQueuename, strDynamicRoutekey, strDynamicExchange);
			//if (msgtype == "GATEWAY_RE_SIGN_IN")
			if (0 == strcmp("GATEWAY_RE_SIGN_IN", msgtype.c_str()))
			{
				LOG_CONSOLE("即将重新注册");
				send_public_queue(gm.gateway_re_sign_in());
			}
			else if (0 == strcmp("GATEWAY_SIGN_IN", msgtype.c_str()))
			{
				LOG_CONSOLE("注册成功,将取消监听公共MQ");
				while_flag = false;//注册成功后不在监听公共队列
				boost::this_thread::sleep_for(boost::chrono::milliseconds(1000));
				connet_business(_ip, _port, _user, _pws);
			}
		}
	}

	iRet = objRabbitmq.CancelConsuming();

	LOG_CONSOLE("解出队列绑定，退出监听%s公共MQ:%d", strRecvQueuename.c_str(), iRet);

	start_business_queue();

	return true;
}

bool client_manage::start_business_queue()
{
	while_flag = true;

	int iRet = 0;
	//监听接收队列，处理响应
	// 可选操作（接收） Declare Queue
	iRet = objDynamicRabbitmq_recv.QueueDeclare(strDynamicQueuename);
	LOG_CONSOLE("objDynamicRabbitmq QueueDeclare %s Ret: %d", strDynamicQueuename.c_str(), iRet);

	// 可选操作（接收） Queue Bind
	iRet = objDynamicRabbitmq_recv.QueueBind(strDynamicQueuename, strDynamicExchange, strDynamicQueuename);
	LOG_CONSOLE("绑定业务队列 %s %s %s Ret: %d", strDynamicQueuename.c_str(), strDynamicExchange.c_str(), strDynamicQueuename.c_str(), iRet);

	//业务队列监听成功
	_business_ev.signal();

	struct timeval timeout;
	timeout.tv_sec = 1.5 * 60;
	timeout.tv_usec = 0;
	LOG_CONSOLE("监听%s业务MQ中...", strDynamicQueuename.c_str());
	while (while_flag)
	{
		// Recv Msg
		std::vector<std::string> vecRecvMsg;
		iRet = objDynamicRabbitmq_recv.Consumer(strDynamicQueuename, vecRecvMsg, 1, false, &timeout);
		//LOG_CONSOLE("objDynamicRabbitmq Consumer Ret: %d", iRet);
		if (iRet == 0)
		{
			for (size_t i = 0; i < vecRecvMsg.size(); ++i)
			{
				//LOG_CONSOLE("Business Consumer: %s", vecRecvMsg[i].c_str());
				server_manage_singleton::get_mutable_instance().insert_task(vecRecvMsg[i]);
			}
		}
		else if (timeout_error == iRet)
		{
			continue;
		}
		else
		{
			while_flag = false;
			break;
		}
	}
	LOG_CONSOLE("退出监听%s业务MQ,error:%d", strDynamicQueuename.c_str(), iRet);

	restart_business_queue();
	return true;
}

bool client_manage::restart_business_queue()
{
	objDynamicRabbitmq_recv.Disconnect();

	LOG_CONSOLE("先重新注册网关公共队列");
	send_public_queue(gm.gateway_re_sign_in());

	LOG_CONSOLE("开始重新链接消费队列");
	int iRet = objDynamicRabbitmq_recv.Connect(_ip, _port, _user, _pws);
	if (iRet != 0)
	{
		LOG_ERROR("Connect objDynamicRabbitmq Ret: %d", iRet);
	}
	LOG_CONSOLE("开始重新声明费队列交换器");
	// 可选操作 Declare Exchange
	iRet = objDynamicRabbitmq_recv.ExchangeDeclare(strDynamicExchange, "topic", true);
	if (iRet != 0)
	{
		LOG_ERROR("重新声明费队列交换器失败 Ret: %d", iRet);
	}
	else
	{
		LOG_CONSOLE("重新声明费队列交换器成功 Ret: %d", iRet);
	}

	start_business_queue();
	return false;
}

void client_manage::stop()
{
	while_flag = false;

	if (thread_new && thread_new->joinable())
	{
		thread_new->join();
	}
}

bool client_manage::check_channel_statu()
{
	LOG_CONSOLE("检查objDynamicRabbitmq_recv通道状态:%d", objDynamicRabbitmq_recv.CheckChannelStatu());
	LOG_CONSOLE("检查objDynamicRabbitmq_send通道状态:%d", objDynamicRabbitmq_send.CheckChannelStatu());
	//LOG_CONSOLE("检查objRabbitmq通道状态:%d", objRabbitmq.CheckChannelStatu());
	return false;
}

bool client_manage::set_client_type(int type)
{
	switch (type)
	{
	case 1:
		_client_type_str = "rabbitMQ";
		break;
	case 2:
		_client_type_str = "http";
		break;
	default:
		break;
	}
	_client_type = type;

	return true;
}

bool client_manage::reconnet_public(std::string ip, int port, std::string user, std::string pws)
{
	disconnet_public();
	return connet_public(ip, port, user, pws);
}

bool client_manage::connet_public(std::string ip, int port, std::string user, std::string pws)
{
	int iRet = 0;
	switch (_client_type)
	{
	case 1:
		//发送
		LOG_INFO("开始链接公共MQ：%s", strExchange.c_str());
		iRet = objRabbitmq.Connect(ip, port, user, pws);
		if (iRet != 0)
		{
			LOG_ERROR("链接公共MQ失败 Ret: %d", iRet);
		}
		// 可选操作 Declare Exchange
		iRet = objRabbitmq.ExchangeDeclare(strExchange, "direct", 0);
		if (iRet != 0)
		{
			LOG_ERROR("设置公共MQ交换器失败 Ret: %d", iRet);
		}
		else
		{
			LOG_CONSOLE("设置公共MQ交换器成功 Ret: %d", iRet);
		}

		break;
	case 2:
		break;
	default:
		break;
	}

	_ip = ip;
	_port = port;
	_user = user;
	_pws = pws;

	if (!thread_new)
	{
		thread_new = boost::make_shared<boost::thread>(boost::bind(&client_manage::heartbeat_func, this));
	}

	return true;
}

bool client_manage::connet_business(std::string ip, int port, std::string user, std::string pws)
{
	int iRet = 0;
	switch (_client_type)
	{
	case 1:
		LOG_CONSOLE("开始链接动态消费MQ：%s", strDynamicExchange.c_str());
		iRet = objDynamicRabbitmq_recv.Connect(ip, port, user, pws);
		if (iRet != 0)
		{
			LOG_ERROR("链接业务消费MQ失败 Ret: %d", iRet);
		}
		//声明交换器
		iRet = objDynamicRabbitmq_recv.ExchangeDeclare(strDynamicExchange, "topic", true);
		if (iRet != 0)
		{
			LOG_ERROR("设置业务消费MQ交换器失败 Ret: %d", iRet);
		}
		else
		{
			LOG_CONSOLE("设置业务消费MQ交换器成功 Ret: %d", iRet);
		}

		LOG_CONSOLE("开始链接动态发布MQ：%s", strDynamicExchange.c_str());
		iRet = objDynamicRabbitmq_send.Connect(ip, port, user, pws);
		if (iRet != 0)
		{
			LOG_ERROR("链接业务发布MQ失败 Ret: %d", iRet);
		}
		// 可选操作 Declare Exchange
		iRet = objDynamicRabbitmq_send.ExchangeDeclare(strDynamicExchange, "topic", true);
		if (iRet != 0)
		{
			LOG_ERROR("设置业务发布MQ交换器失败 Ret: %d", iRet);
		}
		else
		{
			LOG_CONSOLE("设置业务发布MQ交换器成功 Ret: %d", iRet);
		}

		break;
	case 2:
		break;
	default:
		break;
	}
	return true;
}

bool client_manage::send_public_queue(std::string msg)
{
	int iRet = 0;
	switch (_client_type)
	{
	case 1:
		LOG_INFO("开始发布公共队列 channel:%d", commo_public_channel);
		//iRet = objRabbitmq.Publish(msg, strExchange, strRoutekey, commo_public_channel);
		iRet = objRabbitmq.Publish(msg, strExchange, strRoutekey);
		if (iRet != 0)
		{
			LOG_ERROR("发布公共MQ失败,Ret:%d %s", iRet, msg.c_str());
			objRabbitmq.Disconnect();

			//发送
			LOG_INFO("开始重新链接公共MQ：%s", strExchange.c_str());
			iRet = objRabbitmq.Connect(_ip, _port, _user, _pws);
			if (iRet != 0)
			{
				LOG_ERROR("重新链接公共MQ失败 Ret: %d", iRet);
			}
			// 可选操作 Declare Exchange
			iRet = objRabbitmq.ExchangeDeclare(strExchange, "direct", 0);
			if (iRet != 0)
			{
				LOG_ERROR("重新设置公共MQ交换器失败 Ret: %d", iRet);
			}
			else
			{
				LOG_CONSOLE("重新设置公共MQ交换器成功 Ret: %d", iRet);
			}

			iRet = objRabbitmq.Publish(msg, strExchange, strRoutekey);
			if (iRet != 0)
			{
				LOG_ERROR("重新发布公共MQ失败,Ret:%d %s", iRet, msg.c_str());
			}
			else
			{
				LOG_CONSOLE("重新发布公共MQ成功:%s:%s", strRoutekey.c_str(), msg.c_str());
			}
			return false;
		}
		else
		{
			LOG_INFO("发布公共MQ成功:%s:%s ", strRoutekey.c_str(), msg.c_str());
		}
		break;
	case 2:
		break;
	default:
		break;
	}
	return true;
}

bool client_manage::send_business_queue(std::string msg)
{
	int iRet = 0;
	switch (_client_type)
	{
	case 1:
	{
		LOG_INFO("开始发布业务MQ:%s channel:%d", strDynamicExchange.c_str(), commo_public_channel);

		boost::posix_time::ptime currentSendTime = boost::posix_time::microsec_clock::local_time();
		if ((currentSendTime - lastSendTime).total_seconds() > 10)
		{//上次发送时间超过10秒就重新连接MQ再发送
			iRet = objDynamicRabbitmq_send.Disconnect();

			iRet = objDynamicRabbitmq_send.Connect(_ip, _port, _user, _pws);
			if (iRet != 0)
			{
				LOG_ERROR("链接业务发布MQ失败,Ret:%d", iRet);
			}
			else
			{
				iRet = objDynamicRabbitmq_send.ExchangeDeclare(strDynamicExchange, "topic", true);
				if (iRet != 0)
				{
					LOG_ERROR("设置业务发布MQ交换器失败 Ret:%d", iRet);
				}
				else
				{
					iRet = objDynamicRabbitmq_send.Publish(msg, strDynamicExchange, strDynamicRoutekey);
					if (iRet != 0)
					{
						LOG_ERROR("重新发布业务MQ失败,Ret:%d %s", iRet, msg.c_str());
					}
					else
					{
						LOG_CONSOLE("发布业务MQ成功:%s:%s", strDynamicRoutekey.c_str(), msg.c_str());

						lastSendTime = boost::posix_time::microsec_clock::local_time();
					}
				}
			}
		}
		else
		{
			iRet = objDynamicRabbitmq_send.Publish(msg, strDynamicExchange, strDynamicRoutekey);
			if (iRet != 0)
			{
				LOG_ERROR("重新发布业务MQ失败,Ret:%d %s", iRet, msg.c_str());
			}
			else
			{
				LOG_CONSOLE("发布业务MQ成功:%s:%s", strDynamicRoutekey.c_str(), msg.c_str());

				lastSendTime = boost::posix_time::microsec_clock::local_time();
			}
		}
	}
	break;
	case 2:
		break;
	default:
		break;
	}
	return true;
}

void client_manage::disconnet_public()
{
	switch (_client_type)
	{
	case 1:
		objRabbitmq.Disconnect();
		break;
	case 2:
		break;
	default:
		break;
	}
}

void client_manage::disconnet_business()
{
	switch (_client_type)
	{
	case 1:
		objDynamicRabbitmq_recv.Disconnect();
		objDynamicRabbitmq_send.Disconnect();
		break;
	case 2:
		break;
	default:
		break;
	}
}