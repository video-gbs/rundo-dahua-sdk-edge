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
	if (NULL != m_pConn) {
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

	if (0 != ErrorMsg(amqp_login(m_pConn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, m_strUser.c_str(), m_strPasswd.c_str()), "Logging in"))
	{
		return -4;
	}

	channel_id = NULL;

	return 0;
}

int rabbitmqClient::Disconnect()
{
	LOG_CONSOLE("开始销毁链接与通道，m_pConn:%p channel_id:%p channel:%d", m_pConn, channel_id, m_iChannel);

	// 先关闭通道
	//if (channel_id != NULL)
	//{
	//	ErrorMsg(amqp_channel_close(m_pConn, m_iChannel, AMQP_REPLY_SUCCESS), "Closing channel");
	//	channel_id = NULL;
	//}

	if (0 != ErrorMsg(amqp_connection_close(m_pConn, AMQP_REPLY_SUCCESS), "Closing connection"))
		return -1;

	if (amqp_destroy_connection(m_pConn) < 0)
		return -2;

	channel_id = NULL;
	m_pConn = NULL;

	//LOG_CONSOLE("m_pConn:%p channel_id:%p", m_pConn, channel_id);
	return 0;
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
			return -2;
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
	if (iRet != 0)
	{
		LOG_ERROR("publish amqp_basic_publish failed:%d,exchange:%s routekey:%s", iRet, strExchange.c_str(), strRoutekey.c_str());
		if (0 != ErrorMsg(amqp_get_rpc_reply(m_pConn), "amqp_basic_publish"))
		{
			amqp_channel_close(m_pConn, m_iChannel, AMQP_REPLY_SUCCESS);
			channel_id = NULL;
			return iRet;
		}
	}

	return iRet;
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
				LOG_ERROR("Consumer amqp_channel_close failed:-%d queuename:%s", res.reply_type, strQueueName.c_str());

				return -res.reply_type;
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