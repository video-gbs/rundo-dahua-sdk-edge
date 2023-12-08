#include "client_manage.h"
#include <mutex>
#include "log.h"
#include "server_manage.h"

client_manage::client_manage()
{
	_client_type_str = "rabbitMQ";//Ĭ����MQ������������HTTP����
	commo_public_channel = 1;//������������ͨ��
	commo_consumer_channel = 1;//������������ͨ��
	business_public_channel = 3;//ҵ�񷢲�����ͨ��
	business_consumer_channel = 4;//ҵ���������ͨ��
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
	//�������ն��У�������Ӧ
	//��ѡ���������գ� Declare Queue
	iRet = objRabbitmq.QueueDeclare(strQueuename);
	LOG_CONSOLE("��ӹ���MQ���� Ret:%d %s", iRet, strQueuename.c_str());

	// ��ѡ���������գ� Queue Bind
	iRet = objRabbitmq.QueueBind(strQueuename, strExchange, strRoutekey);
	LOG_CONSOLE("�󶨹���MQ Ret:%d %s %s", iRet, strQueuename.c_str(), strExchange.c_str(), strRoutekey.c_str());

	//��������ע��ɹ�
	_public_ev.signal();

	LOG_CONSOLE("����%s����MQ��...", strRecvQueuename.c_str());
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
				LOG_CONSOLE("��������ע��");
				send_public_queue(gm.gateway_re_sign_in());
			}
			else if (0 == strcmp("GATEWAY_SIGN_IN", msgtype.c_str()))
			{
				LOG_CONSOLE("ע��ɹ�,��ȡ����������MQ");
				while_flag = false;//ע��ɹ����ڼ�����������
				boost::this_thread::sleep_for(boost::chrono::milliseconds(1000));
				connet_business(_ip, _port, _user, _pws);
			}
		}
	}

	iRet = objRabbitmq.CancelConsuming();

	LOG_CONSOLE("������а󶨣��˳�����%s����MQ:%d", strRecvQueuename.c_str(), iRet);

	start_business_queue();

	return true;
}

bool client_manage::start_business_queue()
{
	while_flag = true;

	int iRet = 0;
	//�������ն��У�������Ӧ
	// ��ѡ���������գ� Declare Queue
	iRet = objDynamicRabbitmq_recv.QueueDeclare(strDynamicQueuename);
	LOG_CONSOLE("objDynamicRabbitmq QueueDeclare %s Ret: %d", strDynamicQueuename.c_str(), iRet);

	// ��ѡ���������գ� Queue Bind
	iRet = objDynamicRabbitmq_recv.QueueBind(strDynamicQueuename, strDynamicExchange, strDynamicQueuename);
	LOG_CONSOLE("��ҵ����� %s %s %s Ret: %d", strDynamicQueuename.c_str(), strDynamicExchange.c_str(), strDynamicQueuename.c_str(), iRet);

	//ҵ����м����ɹ�
	_business_ev.signal();

	struct timeval timeout;
	timeout.tv_sec = 1.5 * 60;
	timeout.tv_usec = 0;
	LOG_CONSOLE("����%sҵ��MQ��...", strDynamicQueuename.c_str());
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
	LOG_CONSOLE("�˳�����%sҵ��MQ,error:%d", strDynamicQueuename.c_str(), iRet);

	restart_business_queue();
	return true;
}

bool client_manage::restart_business_queue()
{
	objDynamicRabbitmq_recv.Disconnect();

	LOG_CONSOLE("������ע�����ع�������");
	send_public_queue(gm.gateway_re_sign_in());

	LOG_CONSOLE("��ʼ�����������Ѷ���");
	int iRet = objDynamicRabbitmq_recv.Connect(_ip, _port, _user, _pws);
	if (iRet != 0)
	{
		LOG_ERROR("Connect objDynamicRabbitmq Ret: %d", iRet);
	}
	LOG_CONSOLE("��ʼ���������Ѷ��н�����");
	// ��ѡ���� Declare Exchange
	iRet = objDynamicRabbitmq_recv.ExchangeDeclare(strDynamicExchange, "topic", true);
	if (iRet != 0)
	{
		LOG_ERROR("���������Ѷ��н�����ʧ�� Ret: %d", iRet);
	}
	else
	{
		LOG_CONSOLE("���������Ѷ��н������ɹ� Ret: %d", iRet);
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
	LOG_CONSOLE("���objDynamicRabbitmq_recvͨ��״̬:%d", objDynamicRabbitmq_recv.CheckChannelStatu());
	LOG_CONSOLE("���objDynamicRabbitmq_sendͨ��״̬:%d", objDynamicRabbitmq_send.CheckChannelStatu());
	//LOG_CONSOLE("���objRabbitmqͨ��״̬:%d", objRabbitmq.CheckChannelStatu());
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
		//����
		LOG_INFO("��ʼ���ӹ���MQ��%s", strExchange.c_str());
		iRet = objRabbitmq.Connect(ip, port, user, pws);
		if (iRet != 0)
		{
			LOG_ERROR("���ӹ���MQʧ�� Ret: %d", iRet);
		}
		// ��ѡ���� Declare Exchange
		iRet = objRabbitmq.ExchangeDeclare(strExchange, "direct", 0);
		if (iRet != 0)
		{
			LOG_ERROR("���ù���MQ������ʧ�� Ret: %d", iRet);
		}
		else
		{
			LOG_CONSOLE("���ù���MQ�������ɹ� Ret: %d", iRet);
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
		LOG_CONSOLE("��ʼ���Ӷ�̬����MQ��%s", strDynamicExchange.c_str());
		iRet = objDynamicRabbitmq_recv.Connect(ip, port, user, pws);
		if (iRet != 0)
		{
			LOG_ERROR("����ҵ������MQʧ�� Ret: %d", iRet);
		}
		//����������
		iRet = objDynamicRabbitmq_recv.ExchangeDeclare(strDynamicExchange, "topic", true);
		if (iRet != 0)
		{
			LOG_ERROR("����ҵ������MQ������ʧ�� Ret: %d", iRet);
		}
		else
		{
			LOG_CONSOLE("����ҵ������MQ�������ɹ� Ret: %d", iRet);
		}

		LOG_CONSOLE("��ʼ���Ӷ�̬����MQ��%s", strDynamicExchange.c_str());
		iRet = objDynamicRabbitmq_send.Connect(ip, port, user, pws);
		if (iRet != 0)
		{
			LOG_ERROR("����ҵ�񷢲�MQʧ�� Ret: %d", iRet);
		}
		// ��ѡ���� Declare Exchange
		iRet = objDynamicRabbitmq_send.ExchangeDeclare(strDynamicExchange, "topic", true);
		if (iRet != 0)
		{
			LOG_ERROR("����ҵ�񷢲�MQ������ʧ�� Ret: %d", iRet);
		}
		else
		{
			LOG_CONSOLE("����ҵ�񷢲�MQ�������ɹ� Ret: %d", iRet);
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
		LOG_INFO("��ʼ������������ channel:%d", commo_public_channel);
		//iRet = objRabbitmq.Publish(msg, strExchange, strRoutekey, commo_public_channel);
		iRet = objRabbitmq.Publish(msg, strExchange, strRoutekey);
		if (iRet != 0)
		{
			LOG_ERROR("��������MQʧ��,Ret:%d %s", iRet, msg.c_str());
			objRabbitmq.Disconnect();

			//����
			LOG_INFO("��ʼ�������ӹ���MQ��%s", strExchange.c_str());
			iRet = objRabbitmq.Connect(_ip, _port, _user, _pws);
			if (iRet != 0)
			{
				LOG_ERROR("�������ӹ���MQʧ�� Ret: %d", iRet);
			}
			// ��ѡ���� Declare Exchange
			iRet = objRabbitmq.ExchangeDeclare(strExchange, "direct", 0);
			if (iRet != 0)
			{
				LOG_ERROR("�������ù���MQ������ʧ�� Ret: %d", iRet);
			}
			else
			{
				LOG_CONSOLE("�������ù���MQ�������ɹ� Ret: %d", iRet);
			}

			iRet = objRabbitmq.Publish(msg, strExchange, strRoutekey);
			if (iRet != 0)
			{
				LOG_ERROR("���·�������MQʧ��,Ret:%d %s", iRet, msg.c_str());
			}
			else
			{
				LOG_CONSOLE("���·�������MQ�ɹ�:%s:%s", strRoutekey.c_str(), msg.c_str());
			}
			return false;
		}
		else
		{
			LOG_INFO("��������MQ�ɹ�:%s:%s ", strRoutekey.c_str(), msg.c_str());
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
		LOG_INFO("��ʼ����ҵ��MQ:%s channel:%d", strDynamicExchange.c_str(), commo_public_channel);

		boost::posix_time::ptime currentSendTime = boost::posix_time::microsec_clock::local_time();
		if ((currentSendTime - lastSendTime).total_seconds() > 10)
		{//�ϴη���ʱ�䳬��10�����������MQ�ٷ���
			iRet = objDynamicRabbitmq_send.Disconnect();

			iRet = objDynamicRabbitmq_send.Connect(_ip, _port, _user, _pws);
			if (iRet != 0)
			{
				LOG_ERROR("����ҵ�񷢲�MQʧ��,Ret:%d", iRet);
			}
			else
			{
				iRet = objDynamicRabbitmq_send.ExchangeDeclare(strDynamicExchange, "topic", true);
				if (iRet != 0)
				{
					LOG_ERROR("����ҵ�񷢲�MQ������ʧ�� Ret:%d", iRet);
				}
				else
				{
					iRet = objDynamicRabbitmq_send.Publish(msg, strDynamicExchange, strDynamicRoutekey);
					if (iRet != 0)
					{
						LOG_ERROR("���·���ҵ��MQʧ��,Ret:%d %s", iRet, msg.c_str());
					}
					else
					{
						LOG_CONSOLE("����ҵ��MQ�ɹ�:%s:%s", strDynamicRoutekey.c_str(), msg.c_str());

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
				LOG_ERROR("���·���ҵ��MQʧ��,Ret:%d %s", iRet, msg.c_str());
			}
			else
			{
				LOG_CONSOLE("����ҵ��MQ�ɹ�:%s:%s", strDynamicRoutekey.c_str(), msg.c_str());

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