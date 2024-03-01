#include "rabbitMQ.h"
//#include <unistd.h>
#include <Windows.h>
#include "log.h"
#include "common.h"
#include "gateway_msg.h"

rabbitmqClient::rabbitmqClient()
	: m_strHostname("")
	, m_iPort(0)
	, m_strUser("")
	, m_strPasswd("")
	, m_iChannel(1) //默认用1号通道，通道无所谓
	, m_pSock(NULL)
	, m_pConn(NULL) {
}

rabbitmqClient::~rabbitmqClient() {
	if (NULL != m_pConn) 
	{
		Disconnect();
		m_pConn = NULL;
	}
}

int rabbitmqClient::Connect(const string& strHostname, int iPort, const string& strUser, const string& strPasswd)
{
	m_strHostname = strHostname;
	m_iPort = iPort;
	m_strUser = strUser;
	m_strPasswd = strPasswd;

	m_pConn = amqp_new_connection();
	if (NULL == m_pConn)
	{
		LOG_ERROR("amqp new connection failed");
		return -1;
	}

	//LOG_CONSOLE("amqp heartbeat:%d", amqp_get_heartbeat(m_pConn));

	m_pSock = amqp_tcp_socket_new(m_pConn);
	if (NULL == m_pSock)
	{
		LOG_ERROR("amqp tcp new socket failed");
		return -2;
	}

	int status = amqp_socket_open(m_pSock, m_strHostname.c_str(), m_iPort);
	if (status < 0)
	{
		LOG_ERROR("amqp socket open failed");
		return -3;
	}

	if (0 != ErrorMsg(amqp_login(m_pConn, "/", 0, AMQP_DEFAULT_FRAME_SIZE, 65, AMQP_SASL_METHOD_PLAIN, m_strUser.c_str(), m_strPasswd.c_str()), "Logging in"))
	{
		return -4;
	}

	LOG_CONSOLE("rabbitmq链接成功:[%p:%s:%d]", m_pConn, m_strHostname.c_str(), m_iPort);
	channel_id = NULL;

	return 0;
}

int rabbitmqClient::Disconnect()
{
	LOG_CONSOLE("开始销毁链接与通道，m_pConn:%p channel_id:%p channel:%d", m_pConn, channel_id, m_iChannel);

	// 先关闭通道
	if (channel_id != NULL)
	{
		ErrorMsg(amqp_channel_close(m_pConn, m_iChannel, AMQP_REPLY_SUCCESS), "Closing channel");
		channel_id = NULL;
	}

	if (0 != ErrorMsg(amqp_connection_close(m_pConn, AMQP_REPLY_SUCCESS), "Closing connection"))
		return -1;

	if (amqp_destroy_connection(m_pConn) < 0)
		return -2;

	channel_id = NULL;
	m_pConn = NULL;

	//LOG_CONSOLE("m_pConn:%p channel_id:%p", m_pConn, channel_id);
	return 0;
}

void rabbitmqClient::DisconnectEx()
{
	LOG_CONSOLE("开始异步销毁链接与通道，m_pConn:%p channel_id:%p channel:%d", m_pDisConn, channel_id, m_iDisChannel);

	// 先关闭通道
	if (channel_id != NULL)
	{
		ErrorMsg(amqp_channel_close(m_pDisConn, m_iDisChannel, AMQP_REPLY_SUCCESS), "Closing channel");
		channel_id = NULL;
	}

	if (0 != ErrorMsg(amqp_connection_close(m_pDisConn, AMQP_REPLY_SUCCESS), "Closing connection"))
		return ;

	if (amqp_destroy_connection(m_pDisConn) < 0)
		return ;

	channel_id = NULL;
	m_pDisConn = NULL;
	m_iDisChannel = 0;

	//LOG_CONSOLE("m_pConn:%p channel_id:%p", m_pConn, channel_id);
	return ;
}

int rabbitmqClient::ExchangeDeclare(const string& strExchange, const string& strType, bool durable)
{
	if (!channel_id)
	{
		m_iChannel++;
		channel_id = amqp_channel_open(m_pConn, m_iChannel);
		if (0 != ErrorMsg(amqp_get_rpc_reply(m_pConn), "open channel"))
		{
			amqp_channel_close(m_pConn, m_iChannel, AMQP_REPLY_SUCCESS);
			channel_id = NULL;
			LOG_ERROR("ExchangeDeclare.exchange:%s.amqp_channel_open fail", strExchange.c_str());
			return -2;
		}
	}

	//LOG_CONSOLE("m_pConn:%p channel_id:%p", m_pConn, channel_id);
	LOG_CONSOLE("ExchangeDeclare.exchange:%s.type:%s channel:%d", strExchange.c_str(), strType.c_str(), m_iChannel);

	amqp_bytes_t _exchange = amqp_cstring_bytes(strExchange.c_str());
	amqp_bytes_t _type = amqp_cstring_bytes(strType.c_str());
	int _passive = 0;
	int _durable = durable;      // 交换机是否持久化
	amqp_exchange_declare(m_pConn, m_iChannel, _exchange, _type, _passive, _durable, 0, 0, amqp_empty_table);
	if (0 != ErrorMsg(amqp_get_rpc_reply(m_pConn), "exchange_declare"))
	{
		amqp_channel_close(m_pConn, m_iChannel, AMQP_REPLY_SUCCESS);
		channel_id = NULL;
		return -1;
	}
	//启动发布者确认模式
	amqp_confirm_select(m_pConn, m_iChannel);
	LOG_CONSOLE("启动发布者确认模式,exchange:%s", strExchange.c_str());

	return 0;
}

int rabbitmqClient::QueueDeclare(const string& strQueueName)
{
	if (NULL == m_pConn)
	{
		fprintf(stderr, "QueueDeclare m_pConn is null\n");
		return -1;
	}

	if (!channel_id)
	{
		channel_id = amqp_channel_open(m_pConn, m_iChannel);
		if (0 != ErrorMsg(amqp_get_rpc_reply(m_pConn), "open channel"))
		{
			amqp_channel_close(m_pConn, m_iChannel, AMQP_REPLY_SUCCESS);
			channel_id = NULL;
			return -2;
		}
	}

	LOG_CONSOLE("队列声明:%s", strQueueName.c_str());

	amqp_bytes_t _queue = amqp_cstring_bytes(strQueueName.c_str());
	int32_t _passive = 1;
	int32_t _durable = 0;
	int32_t _exclusive = 0;
	int32_t _auto_delete = 1;
	amqp_queue_declare(m_pConn, m_iChannel, _queue, _passive, _durable, _exclusive, _auto_delete, amqp_empty_table);
	if (0 != ErrorMsg(amqp_get_rpc_reply(m_pConn), "queue_declare"))
	{
		amqp_channel_close(m_pConn, m_iChannel, AMQP_REPLY_SUCCESS);
		channel_id = NULL;
		return -1;
	}

	return 0;
}

int rabbitmqClient::QueueBind(const string& strQueueName, const string& strExchange, const string& strBindKey)
{
	if (NULL == m_pConn)
	{
		LOG_ERROR("QueueBind m_pConn is null");
		return -1;
	}
	int iRet = 0;

	if (!channel_id)
	{
		channel_id = amqp_channel_open(m_pConn, m_iChannel);
		iRet = ErrorMsg(amqp_get_rpc_reply(m_pConn), "open channel");
		if (0 != iRet)
		{
			amqp_channel_close(m_pConn, m_iChannel, AMQP_REPLY_SUCCESS);
			channel_id = NULL;
			return iRet;
		}
	}

	LOG_CONSOLE("队列绑定:[%s][%s][%s]", strQueueName.c_str(), strExchange.c_str(), strBindKey.c_str());

	amqp_bytes_t _queue = amqp_cstring_bytes(strQueueName.c_str());
	amqp_bytes_t _exchange = amqp_cstring_bytes(strExchange.c_str());
	amqp_bytes_t _routkey = amqp_cstring_bytes(strBindKey.c_str());
	amqp_queue_bind(m_pConn, m_iChannel, _queue, _exchange, _routkey, amqp_empty_table);
	if (0 != ErrorMsg(amqp_get_rpc_reply(m_pConn), "queue_bind"))
	{
		amqp_channel_close(m_pConn, m_iChannel, AMQP_REPLY_SUCCESS);
		channel_id = NULL;
		return -1;
	}

	return 0;
}

int rabbitmqClient::QueueUnbind(const string& strQueueName, const string& strExchange, const string& strBindKey)
{
	if (NULL == m_pConn)
	{
		LOG_ERROR("QueueUnbind m_pConn is null");
		return -1;
	}

	if (!channel_id)
	{
		channel_id = amqp_channel_open(m_pConn, m_iChannel);
		if (0 != ErrorMsg(amqp_get_rpc_reply(m_pConn), "open channel"))
		{
			amqp_channel_close(m_pConn, m_iChannel, AMQP_REPLY_SUCCESS);
			channel_id = NULL;
			return -2;
		}
	}
	amqp_bytes_t _queue = amqp_cstring_bytes(strQueueName.c_str());
	amqp_bytes_t _exchange = amqp_cstring_bytes(strExchange.c_str());
	amqp_bytes_t _routkey = amqp_cstring_bytes(strBindKey.c_str());
	amqp_queue_unbind(m_pConn, m_iChannel, _queue, _exchange, _routkey, amqp_empty_table);
	if (0 != ErrorMsg(amqp_get_rpc_reply(m_pConn), "queue_unbind")) {
		amqp_channel_close(m_pConn, m_iChannel, AMQP_REPLY_SUCCESS);
		channel_id = NULL;
		return -1;
	}

	return 0;
}

int rabbitmqClient::QueueDelete(const string& strQueueName, int iIfUnused) {
	if (NULL == m_pConn)
	{
		LOG_ERROR("QueueDelete m_pConn is null");
		return -1;
	}

	if (!channel_id)
	{
		channel_id = amqp_channel_open(m_pConn, m_iChannel);
		if (0 != ErrorMsg(amqp_get_rpc_reply(m_pConn), "open channel"))
		{
			amqp_channel_close(m_pConn, m_iChannel, AMQP_REPLY_SUCCESS);
			channel_id = NULL;
			return -2;
		}
	}

	amqp_queue_delete(m_pConn, m_iChannel, amqp_cstring_bytes(strQueueName.c_str()), iIfUnused, 0);
	if (0 != ErrorMsg(amqp_get_rpc_reply(m_pConn), "delete queue"))
	{
		amqp_channel_close(m_pConn, m_iChannel, AMQP_REPLY_SUCCESS);
		channel_id = NULL;
		return -3;
	}

	amqp_channel_close(m_pConn, m_iChannel, AMQP_REPLY_SUCCESS);
	channel_id = NULL;
	return 0;
}

int rabbitmqClient::Publish(const string& strMessage, const string& strExchange, const string& strRoutekey)
{
	if (NULL == m_pConn)
	{
		LOG_ERROR("publish m_pConn is null, publish failed");
		return -1;
	}

	if (channel_id == NULL)
	{
		channel_id = amqp_channel_open(m_pConn, m_iChannel);
		if (0 != ErrorMsg(amqp_get_rpc_reply(m_pConn), "open channel"))
		{
			amqp_channel_close(m_pConn, m_iChannel, AMQP_REPLY_SUCCESS);
			channel_id = NULL;
			return -4;
		}
	}

	int iRet = 0;
	amqp_bytes_t message_bytes;
	message_bytes.len = strMessage.length();
	message_bytes.bytes = (void*)(strMessage.c_str());

	amqp_bytes_t exchange = amqp_cstring_bytes(strExchange.c_str());
	amqp_bytes_t routekey = amqp_cstring_bytes(strRoutekey.c_str());

	//LOG_CONSOLE("发布：%s", strMessage.c_str());
	//if (0 != amqp_basic_publish(m_pConn, m_iChannel, exchange, routekey, 0, 0, &props, message_bytes)) {
	iRet = amqp_basic_publish(m_pConn, m_iChannel, exchange, routekey, 0, 0, NULL, message_bytes);
	if (iRet != AMQP_STATUS_OK)
	{
		LOG_ERROR("publish amqp_basic_publish failed:%d,exchange:%s routekey:%s", iRet, strExchange.c_str(), strRoutekey.c_str());
		if (0 != ErrorMsg(amqp_get_rpc_reply(m_pConn), "amqp_basic_publish"))
		{
			amqp_channel_close(m_pConn, m_iChannel, AMQP_REPLY_SUCCESS);
			channel_id = NULL;
			return iRet;
		}
	}
	// 准备接收帧
	amqp_frame_t frame = {0};
	timeval tv;
	tv.tv_sec = 2;//等待5秒
	tv.tv_usec = 0;//等待5微秒
	//amqp_simple_wait_frame(m_pConn, &frame);
	amqp_simple_wait_frame_noblock(m_pConn, &frame, &tv);
	if (frame.channel == m_iChannel)//首先检查从AMQP连接接收到的帧是否来自预期通道(在此例中是KChannel)
	{    //如果是，那么进一步检查帧的内容。它查看帧的载荷中的方法ID，看是否为AMQP_BASIC_ACK_METHOD
		if (frame.payload.method.id == AMQP_BASIC_ACK_METHOD)//AMQP_BASIC_ACK_METHOD是一个方法ID，表示这是确认帧，即服务器已经成功接收并处理了之前发送的消息
		{
			// 如果接收到ACK确认帧，那么它将载荷解码为一个amqp_basic_ack_t结构，并查看这个结构中的multiple字段
			amqp_basic_ack_t* ack = (amqp_basic_ack_t*)frame.payload.method.decoded; // 如果接收到ACK确认帧，打印确认消息
			if (ack->multiple)
				//如果multiple为真，那么表示服务器已经成功处理了所有到达指定delivery tag的消息。程序将打印一条确认消息，并显示成功处理的最后一条消息的delivery tag。
				LOG_INFO("服务器已所有消息,[delivery_tag:%s:%llu] ", strRoutekey.c_str(), ack->delivery_tag);
			else
			{
				/*如果multiple字段为假（false），那么表示服务器已经成功处理了具有指定delivery tag的单条消息。程序将打印一条确认消息，并显示成功处理的消息的delivery tag。*/
				if (strRoutekey.find("GATEWAY_SG") != std::string::npos)
					LOG_CONSOLE("服务器已收到消息,[delivery_tag:%s:%llu] %s ", strRoutekey.c_str(), ack->delivery_tag, strMessage.c_str());
				else
					LOG_INFO("服务器已收到消息,[delivery_tag:%s:%llu] %s ", strRoutekey.c_str(), ack->delivery_tag, strMessage.c_str());
			}

			return iRet;
		}
		else if (frame.payload.method.id == AMQP_BASIC_RETURN_METHOD) //检查帧的载荷中的方法ID，看是否为AMQP_BASIC_RETURN_METHOD。
		{ //这个方法ID表示这是一个返回帧，即消息未能被正确路由到队列，并被RabbitMQ服务器返回。
			//如果接收到的是返回帧，那么将读取并保存返回的消息，然后销毁这个消息对象。
			amqp_message_t returned_message;
			amqp_read_message(m_pConn, 1, &returned_message, 0);//读取并保存返回的消息
			LOG_ERROR("消息发送到队列失败，被RabbitMQ服务器退回,[delivery_tag:%s] %s", strRoutekey.c_str(), (char*)returned_message.body.bytes);
			amqp_destroy_message(&returned_message);//然后销毁这个消息对象
			//再次调用amqp_simple_wait_frame函数来接收下一个帧。这是因为在MQP_BASIC_RETURN_METHOD之后，
			//RabbitMQ服务器会发送另一个AMQP_BASIC_ACK_METHOD帧来确认消息已经被返回。
			//amqp_simple_wait_frame(m_pConn, &frame);
			amqp_simple_wait_frame_noblock(m_pConn, &frame, &tv);
			if (frame.payload.method.id == AMQP_BASIC_ACK_METHOD)
				//如果接收到的帧是AMQP_BASIC_ACK_METHOD帧，程序会打印出"Message returned"，来确认消息已经被返回。
				LOG_CONSOLE("Message returned");
			return -1;
		}
		LOG_ERROR("消息发送到队列失败,[delivery_tag:%s] ", strRoutekey.c_str());
		return -2;
	}

	m_iDisChannel = m_iChannel;
	m_pDisConn = m_pConn;

	return -3;
}

int rabbitmqClient::Consumer(const string& strQueueName, vector<string>& message_array, int GetNum, bool filterate, struct timeval* timeout)
{
	if (NULL == m_pConn)
	{
		LOG_ERROR("Consumer m_pConn is null, Consumer failed");
		return -1;
	}

	if (!channel_id)
	{
		channel_id = amqp_channel_open(m_pConn, m_iChannel);
		if (0 != ErrorMsg(amqp_get_rpc_reply(m_pConn), "open channel"))
		{
			amqp_channel_close(m_pConn, m_iChannel, AMQP_REPLY_SUCCESS);
			channel_id = NULL;
			return -2;
		}
	}

	consumer_tag = amqp_empty_bytes;

	amqp_basic_qos(m_pConn, m_iChannel, 0, GetNum, 0);

	int no_ack = filterate ? 0 : 1; // no_ack为true 那么后续不会发送Basic.Ack(调用amqp_basic_ack),no_ack为false，那么后续需要发送Basic.Ack(调用amqp_basic_ack)

	int no_local = 0; // 用于指示服务器不应将消息发送给发布者自己
	amqp_bytes_t queuename = amqp_cstring_bytes(strQueueName.c_str());
	amqp_basic_consume(m_pConn, m_iChannel, queuename, amqp_empty_bytes, no_local, no_ack, 0, amqp_empty_table);
	if (0 != ErrorMsg(amqp_get_rpc_reply(m_pConn), "Consuming"))
	{
		amqp_channel_close(m_pConn, m_iChannel, AMQP_REPLY_SUCCESS);
		channel_id = NULL;
		return -3;
	}

	int rtn = 0;
	gateway_msg gm;
	amqp_rpc_reply_t res;
	amqp_envelope_t envelope;
	while (GetNum > 0)
	{
		amqp_maybe_release_buffers(m_pConn);
		res = amqp_consume_message(m_pConn, &envelope, timeout, 0); 
		if (AMQP_RESPONSE_NORMAL != res.reply_type)
		{
			//if (AMQP_RESPONSE_LIBRARY_EXCEPTION == res.reply_type && AMQP_STATUS_UNEXPECTED_STATE == res.library_error)
			//{
			//	amqp_frame_t frame;
			//	if (AMQP_STATUS_OK != amqp_simple_wait_frame(m_pConn, &frame)) 
			//	{
			//		LOG_ERROR("Consumer library_error:%d queuename:%s", res.library_error, strQueueName.c_str());
			//		return res.library_error;
			//	}
			//}
			int iRet = ErrorMsg(amqp_get_rpc_reply(m_pConn), "consume_message");
			if (iRet == socket_error)
			{
				LOG_ERROR("网络报错，即将销毁并重连");
			}

			amqp_destroy_envelope(&envelope);
			amqp_channel_close(m_pConn, m_iChannel, AMQP_REPLY_SUCCESS);
			channel_id = NULL;

			if (iRet == 0 && timeout)
			{
				return timeout_error;
			}
			else
			{
				LOG_ERROR("Consumer library_error:%d queuename:%s", res.library_error, strQueueName.c_str());

				return res.library_error;
			}
		}
		string str((char*)envelope.message.body.bytes, (char*)envelope.message.body.bytes + envelope.message.body.len);
		if (str.empty())
		{
			amqp_destroy_envelope(&envelope);
		}
		if (filterate)
		{//需要过滤，一般处理公共队列
			if (gm.isMySerialNum(str))
			{
				amqp_boolean_t multiple = 0;  // false，只确认指定的单个消息
				message_array.push_back(str);
				//rtn = amqp_basic_ack(m_pConn, m_iChannel, envelope.delivery_tag, multiple);//通知服务器消息已被成功处理
				if (rtn == 0)
				{
					// 打印日志，确认消息被正确接收
					LOG_INFO("确认消息被正确接收:\n%s", str.c_str());
				}
			}
			else
			{
				rtn = amqp_basic_nack(m_pConn, m_iChannel, envelope.delivery_tag, 1, 1);//告知服务器消息未能成功处理
			}
		}
		else
		{//不需要过滤
			amqp_boolean_t multiple = 0;  // false，只确认指定的单个消息
			message_array.push_back(str);
			//rtn = amqp_basic_ack(m_pConn, m_iChannel, envelope.delivery_tag, multiple);//通知服务器消息已被成功处理
			if (rtn == 0)
			{
				// 打印日志，确认消息被正确接收
				LOG_INFO("确认消息被正确接收:\n%s", str.c_str());
			}
		}

		amqp_destroy_envelope(&envelope);
		if (rtn != 0)
		{
			amqp_channel_close(m_pConn, m_iChannel, AMQP_REPLY_SUCCESS);
			channel_id = NULL;
			return -4;
		}

		GetNum--;
		DWORD dwMilliseconds = 100;
		Sleep(dwMilliseconds);
	}

	return 0;
}

int rabbitmqClient::CancelConsuming()
{
	// 添加取消监听队列的条件
	if (1)
	{
		amqp_basic_cancel(m_pConn, m_iChannel, consumer_tag);
	}
	// 释放资源
	amqp_bytes_free(consumer_tag);
	return 0;
}

bool rabbitmqClient::CheckChannelStatu()
{
	//amqp_channel_close(m_pConn, m_iChannel, AMQP_REPLY_SUCCESS);
	amqp_channel_open_ok_t* channel_reply = amqp_channel_open(m_pConn, m_iChannel);
	if (channel_reply)
	{
		return true;
	}
	//int iRet = ErrorMsg(amqp_get_rpc_reply(m_pConn), "CheckChannelStatu");
	//if (iRet == socket_error)
	//{
	//	LOG_ERROR("网络报错，即将销毁并重连");
	//}
	return false;
}

int rabbitmqClient::ErrorMsg(amqp_rpc_reply_t x, char const* context)
{
	int val = -1;

	switch (x.reply_type)
	{
	case AMQP_RESPONSE_NORMAL:
		return 0;

	case AMQP_RESPONSE_NONE:
		LOG_ERROR("%s:missing RPC reply type!", context);
		break;

	case AMQP_RESPONSE_LIBRARY_EXCEPTION:
		LOG_ERROR("%s:%s %d", context, amqp_error_string2(x.library_error), x.library_error);//a socket error occurred 出现套接字错误
		val = socket_error;
		break;

	case AMQP_RESPONSE_SERVER_EXCEPTION:
		switch (x.reply.id)
		{
		case AMQP_CONNECTION_CLOSE_METHOD:
		{
			amqp_connection_close_t* m = (amqp_connection_close_t*)x.reply.decoded;
			LOG_ERROR("%s:server connection error %uh, message:%.*s",
				context, m->reply_code, (int)m->reply_text.len,
				(char*)m->reply_text.bytes);
			break;
		}
		case AMQP_CHANNEL_CLOSE_METHOD:
		{
			amqp_channel_close_t* m = (amqp_channel_close_t*)x.reply.decoded;
			LOG_ERROR("%s:server channel error %uh, message:%.*s",
				context, m->reply_code, (int)m->reply_text.len,
				(char*)m->reply_text.bytes);
			break;
		}
		default:
			LOG_ERROR("%s:unknown server error, method id 0x%08X",
				context, x.reply.id);
			break;
		}
		break;
	}

	return val;
}