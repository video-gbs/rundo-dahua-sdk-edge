#include <Windows.h>
#include "rabbitmqclient.h"
//#include <unistd.h>
#include "common.h"

CRabbitmqClient::CRabbitmqClient()
	: m_strHostname("")
	, m_iPort(0)
	, m_strUser("")
	, m_strPasswd("")
	, m_pSock(NULL)
	, m_pConn(NULL)
	, m_consumer(false)
{
}

CRabbitmqClient::~CRabbitmqClient()
{
	if (NULL != m_pConn)
	{
		Disconnect();
		m_pConn = NULL;
	}
}

bool parese_json(std::string json, std::string serialNum)
{
	rapidjson::Document document;
	document.Parse(json);

	if (!document.IsObject())
	{
		return false;
	}

	if (document.HasMember("serialNum") && document["serialNum"].IsString())
	{
		if (serialNum == document["serialNum"].GetString())
			return true;
	}
	/*
	if (document.HasMember("code") && document["code"].IsInt()) {
		int code = document["code"].GetInt();
		std::cout << "Code: " << code << std::endl;
	}

	if (document.HasMember("data") && document["data"].IsObject()) {
		const rapidjson::Value& data = document["data"];
		if (data.HasMember("isFirstSignIn") && data["isFirstSignIn"].IsBool()) {
			bool isFirstSignIn = data["isFirstSignIn"].GetBool();
			std::cout << "isFirstSignIn: " << isFirstSignIn << std::endl;
		}
		if (data.HasMember("mqExchange") && data["mqExchange"].IsString()) {
			std::string mqExchange = data["mqExchange"].GetString();
			std::cout << "mqExchange: " << mqExchange << std::endl;
		}
		if (data.HasMember("mqGetQueue") && data["mqGetQueue"].IsString()) {
			std::string mqGetQueue = data["mqGetQueue"].GetString();
			std::cout << "mqGetQueue: " << mqGetQueue << std::endl;
		}
		if (data.HasMember("mqSetQueue") && data["mqSetQueue"].IsString()) {
			std::string mqSetQueue = data["mqSetQueue"].GetString();
			std::cout << "mqSetQueue: " << mqSetQueue << std::endl;
		}
		if (data.HasMember("signType") && data["signType"].IsString()) {
			std::string signType = data["signType"].GetString();
			std::cout << "signType: " << signType << std::endl;
		}
	}

	if (document.HasMember("error") && document["error"].IsBool()) {
		bool error = document["error"].GetBool();
		std::cout << "Error: " << error << std::endl;
	}

	if (document.HasMember("msg") && document["msg"].IsString()) {
		std::string msg = document["msg"].GetString();
		std::cout << "Message: " << msg << std::endl;
	}

	if (document.HasMember("msgId") && document["msgId"].IsString()) {
		std::string msgId = document["msgId"].GetString();
		std::cout << "Message ID: " << msgId << std::endl;
	}

	if (document.HasMember("msgType") && document["msgType"].IsString()) {
		std::string msgType = document["msgType"].GetString();
		std::cout << "Message Type: " << msgType << std::endl;
	}

	if (document.HasMember("time") && document["time"].IsString()) {
		std::string time = document["time"].GetString();
		std::cout << "Time: " << time << std::endl;
	}*/
	return false;
}

void CRabbitmqClient::receiveFunction(int iChannel)
{
	int rtn = 0;
	int hasget = 0;
	while (1)
	{
		//这几行代码接收一条消息，并将其放到envelope对象中，'amqp_mabe_release_buffers'函数释放之前使用过的内存
		printf("SetRecv:%d\n", iChannel);
		amqp_maybe_release_buffers(m_pConn);//释放内存
		res = amqp_consume_message(m_pConn, &envelope, NULL, 0);
		if (AMQP_RESPONSE_NORMAL != res.reply_type)
		{
			fprintf(stderr, "Consumer amqp_consume_message failed error:%d\n", res.reply_type);
			amqp_channel_close(m_pConn, iChannel, AMQP_REPLY_SUCCESS);
		}

		string str((char*)envelope.message.body.bytes, (char*)envelope.message.body.bytes + envelope.message.body.len);

		if (parese_json(str, serialNum))
		{
			callback(str);
			//message_array.push_back(str);
			rtn = amqp_basic_ack(m_pConn, iChannel, envelope.delivery_tag, 1);//确认消息，如果不是属于自己消息则不需要确认
		}
		else
		{
			rtn = amqp_basic_nack(m_pConn, iChannel, envelope.delivery_tag, 0, 1);//确认消息，如果不是属于自己消息则不需要确认
			printf("recv: \n%s\n", str.c_str());
			printf("serialNum: %s\n", serialNum.c_str());
		}

		//printf("serialNum: %s\n", decode_commo.serialNum.c_str());

		amqp_destroy_envelope(&envelope);
		if (rtn != 0)
		{
			amqp_channel_close(m_pConn, iChannel, AMQP_REPLY_SUCCESS);
			return;
		}

		// 等待100ms
		DWORD dwMilliseconds = 100;
		Sleep(dwMilliseconds);
	}
}

int CRabbitmqClient::Connect(const string& strHostname, int iPort, const string& strUser, const string& strPasswd)
{
	m_strHostname = strHostname;
	m_iPort = iPort;
	m_strUser = strUser;
	m_strPasswd = strPasswd;

	m_pConn = amqp_new_connection();
	if (NULL == m_pConn)
	{
		fprintf(stderr, "amqp new connection failed\n");
		return -1;
	}

	m_pSock = amqp_tcp_socket_new(m_pConn);
	if (NULL == m_pSock)
	{
		fprintf(stderr, "amqp tcp new socket failed\n");
		return -2;
	}

	int status = amqp_socket_open(m_pSock, m_strHostname.c_str(), m_iPort);
	if (status < 0)
	{
		fprintf(stderr, "amqp socket open failed\n");
		return -3;
	}

	// amqp_login(amqp_connection_state_t state,char const *vhost, int channel_max, int frame_max, int heartbeat, amqp_sasl_method_enum sasl_method, ..)
	if (0 != ErrorMsg(amqp_login(m_pConn, "/", 0, 131072, 0, AMQP_SASL_METHOD_PLAIN, m_strUser.c_str(), m_strPasswd.c_str()), "Logging in"))
	{
		return -4;
	}

	return 0;
}

int CRabbitmqClient::Disconnect()
{
	if (NULL != m_pConn)
	{
		fprintf(stderr, "amqp_connection_close\n");
		if (0 != ErrorMsg(amqp_connection_close(m_pConn, AMQP_REPLY_SUCCESS), "Closing connection"))
			return -1;

		fprintf(stderr, "amqp_destroy_connection\n");
		if (amqp_destroy_connection(m_pConn) < 0)
			return -2;

		m_pConn = NULL;
	}

	return 0;
}

int CRabbitmqClient::SetSend(const string& strExchange, const string& strType, const string& strQueueName, int iChannel)
{
	printf("SetSend:%d Exchange:%s QueueName:%s\n", iChannel, strExchange.c_str(), strQueueName.c_str());
	amqp_channel_open(m_pConn, iChannel);
	if (0 != ErrorMsg(amqp_get_rpc_reply(m_pConn), "open channel"))
	{
		amqp_channel_close(m_pConn, iChannel, AMQP_REPLY_SUCCESS);
		return -2;
	}

	amqp_bytes_t _exchange = amqp_cstring_bytes(strExchange.c_str());
	amqp_bytes_t _type = amqp_cstring_bytes(strType.c_str());
	int _passive = 1;
	int _durable = 0;      // 交换机是否持久化
	amqp_exchange_declare(m_pConn, iChannel, _exchange, _type, _passive, _durable, 0, 0, amqp_empty_table);
	if (0 != ErrorMsg(amqp_get_rpc_reply(m_pConn), "exchange_declare"))
	{
		amqp_channel_close(m_pConn, iChannel, AMQP_REPLY_SUCCESS);
		return -1;
	}

	return 0;
}

int CRabbitmqClient::Send(const string& strMessage, const string& strExchange, const string& strRoutekey, int iChannel)
{
	//amqp_channel_open(m_pConn, iChannel);
	//if (0 != ErrorMsg(amqp_get_rpc_reply(m_pConn), "open channel"))
	//{
	//	amqp_channel_close(m_pConn, iChannel, AMQP_REPLY_SUCCESS);
	//	return -2;
	//}

	amqp_bytes_t message_bytes;
	message_bytes.len = strMessage.length();
	message_bytes.bytes = (void*)(strMessage.c_str());

	amqp_bytes_t exchange = amqp_cstring_bytes(strExchange.c_str());
	amqp_bytes_t routekey = amqp_cstring_bytes(strRoutekey.c_str());

	printf("Send:%d \n%s\n", iChannel, strMessage.c_str());
	if (0 != amqp_basic_publish(m_pConn, iChannel, exchange, routekey, 0, 0, NULL, message_bytes))
	{
		fprintf(stderr, "publish amqp_basic_publish failed\n");
		if (0 != ErrorMsg(amqp_get_rpc_reply(m_pConn), "amqp_basic_publish"))
		{
			amqp_channel_close(m_pConn, iChannel, AMQP_REPLY_SUCCESS);
			return -3;
		}
	}

	return 0;
}

int CRabbitmqClient::SetRecv(const string& strExchange, const string& strQueueName, const string& strBindKey, const string& msg_serialNum, Callback cb, int GetNum, int iChannel)
{
	printf("SetRecv:%d Exchange:%s QueueName:%s BindKey:%s\n", iChannel, strExchange.c_str(), strQueueName.c_str(), strBindKey.c_str());
	amqp_channel_open(m_pConn, iChannel);
	if (0 != ErrorMsg(amqp_get_rpc_reply(m_pConn), "open channel"))
	{
		amqp_channel_close(m_pConn, iChannel, AMQP_REPLY_SUCCESS);
		return -2;
	}
	amqp_bytes_t _queue = amqp_cstring_bytes(strQueueName.c_str());
	int32_t _passive = 1;
	int32_t _durable = 0;
	int32_t _exclusive = 0;
	int32_t _auto_delete = 1;
	amqp_queue_declare(m_pConn, iChannel, _queue, _passive, _durable, _exclusive, _auto_delete, amqp_empty_table);
	if (0 != ErrorMsg(amqp_get_rpc_reply(m_pConn), "queue_declare"))
	{
		amqp_channel_close(m_pConn, iChannel, AMQP_REPLY_SUCCESS);
		return -1;
	}

	amqp_bytes_t _exchange = amqp_cstring_bytes(strExchange.c_str());
	amqp_bytes_t _routkey = amqp_cstring_bytes(strBindKey.c_str());
	amqp_queue_bind(m_pConn, iChannel, _queue, _exchange, _routkey, amqp_empty_table);
	if (0 != ErrorMsg(amqp_get_rpc_reply(m_pConn), "queue_bind"))
	{
		amqp_channel_close(m_pConn, iChannel, AMQP_REPLY_SUCCESS);
		return -1;
	}

	amqp_basic_qos(m_pConn, iChannel, 0, GetNum, 0);//设置每次获取消息的数量
	int ack = 0; // 1:自动确认消息 0:手动确认消息
	int no_local = 1;
	amqp_bytes_t queuename = amqp_cstring_bytes(strQueueName.c_str());
	amqp_basic_consume(m_pConn, iChannel, queuename, amqp_empty_bytes, no_local, ack, 0, amqp_empty_table);
	if (0 != ErrorMsg(amqp_get_rpc_reply(m_pConn), "Consuming")) //获取与最近一次 API 操作（一般是同步 AMQP 方法）相关的 amqp_rpc_reply_t 实例，以便了解操作的结果.这个函数在需要检查操作结果时非常有用，特别是在 API 调用没有明确返回 amqp_rpc_reply_t 的情况下
	{
		amqp_channel_close(m_pConn, iChannel, AMQP_REPLY_SUCCESS);
		return -3;
	}

	callback = cb;
	serialNum = msg_serialNum;
	thread_ = boost::make_shared<boost::thread>(boost::bind(&CRabbitmqClient::receiveFunction, this, iChannel));

	return 0;
}

int CRabbitmqClient::ExchangeDeclare(const string& strExchange, const string& strType, int m_iChannel)
{
	amqp_channel_open(m_pConn, m_iChannel);

	amqp_bytes_t _exchange = amqp_cstring_bytes(strExchange.c_str());
	amqp_bytes_t _type = amqp_cstring_bytes(strType.c_str());
	int _passive = 1;
	int _durable = 0;      // 交换机是否持久化
	amqp_exchange_declare(m_pConn, m_iChannel, _exchange, _type, _passive, _durable, 0, 0, amqp_empty_table);
	if (0 != ErrorMsg(amqp_get_rpc_reply(m_pConn), "exchange_declare"))
	{
		amqp_channel_close(m_pConn, m_iChannel, AMQP_REPLY_SUCCESS);
		return -1;
	}

	amqp_channel_close(m_pConn, m_iChannel, AMQP_REPLY_SUCCESS);
	return 0;
}

int CRabbitmqClient::QueueDeclare(const string& strQueueName, int m_iChannel)
{
	if (NULL == m_pConn)
	{
		fprintf(stderr, "QueueDeclare m_pConn is null\n");
		return -1;
	}

	amqp_channel_open(m_pConn, m_iChannel);
	amqp_bytes_t _queue = amqp_cstring_bytes(strQueueName.c_str());
	int32_t _passive = 1;
	int32_t _durable = 0;
	int32_t _exclusive = 0;
	int32_t _auto_delete = 1;
	amqp_queue_declare(m_pConn, m_iChannel, _queue, _passive, _durable, _exclusive, _auto_delete, amqp_empty_table);
	if (0 != ErrorMsg(amqp_get_rpc_reply(m_pConn), "queue_declare"))
	{
		amqp_channel_close(m_pConn, m_iChannel, AMQP_REPLY_SUCCESS);
		return -1;
	}

	amqp_channel_close(m_pConn, m_iChannel, AMQP_REPLY_SUCCESS);
	return 0;
}

int CRabbitmqClient::QueueBind(const string& strQueueName, const string& strExchange, const string& strBindKey, int m_iChannel)
{
	if (NULL == m_pConn)
	{
		fprintf(stderr, "QueueBind m_pConn is null\n");
		return -1;
	}

	amqp_channel_open(m_pConn, m_iChannel);
	amqp_bytes_t _queue = amqp_cstring_bytes(strQueueName.c_str());
	amqp_bytes_t _exchange = amqp_cstring_bytes(strExchange.c_str());
	amqp_bytes_t _routkey = amqp_cstring_bytes(strBindKey.c_str());
	amqp_queue_bind(m_pConn, m_iChannel, _queue, _exchange, _routkey, amqp_empty_table);
	if (0 != ErrorMsg(amqp_get_rpc_reply(m_pConn), "queue_bind"))
	{
		amqp_channel_close(m_pConn, m_iChannel, AMQP_REPLY_SUCCESS);
		return -1;
	}

	amqp_channel_close(m_pConn, m_iChannel, AMQP_REPLY_SUCCESS);
	return 0;
}

int CRabbitmqClient::QueueUnbind(const string& strQueueName, const string& strExchange, const string& strBindKey, int m_iChannel)
{
	if (NULL == m_pConn)
	{
		fprintf(stderr, "QueueUnbind m_pConn is null\n");
		return -1;
	}

	amqp_channel_open(m_pConn, m_iChannel);
	amqp_bytes_t _queue = amqp_cstring_bytes(strQueueName.c_str());
	amqp_bytes_t _exchange = amqp_cstring_bytes(strExchange.c_str());
	amqp_bytes_t _routkey = amqp_cstring_bytes(strBindKey.c_str());
	amqp_queue_unbind(m_pConn, m_iChannel, _queue, _exchange, _routkey, amqp_empty_table);
	if (0 != ErrorMsg(amqp_get_rpc_reply(m_pConn), "queue_unbind"))
	{
		amqp_channel_close(m_pConn, m_iChannel, AMQP_REPLY_SUCCESS);
		return -1;
	}

	amqp_channel_close(m_pConn, m_iChannel, AMQP_REPLY_SUCCESS);
	return 0;
}

int CRabbitmqClient::QueueDelete(const string& strQueueName, int iIfUnused, int m_iChannel)
{
	if (NULL == m_pConn)
	{
		fprintf(stderr, "QueueDelete m_pConn is null\n");
		return -1;
	}

	amqp_channel_open(m_pConn, m_iChannel);
	if (0 != ErrorMsg(amqp_get_rpc_reply(m_pConn), "open channel"))
	{
		amqp_channel_close(m_pConn, m_iChannel, AMQP_REPLY_SUCCESS);
		return -2;
	}

	amqp_queue_delete(m_pConn, m_iChannel, amqp_cstring_bytes(strQueueName.c_str()), iIfUnused, 0);
	if (0 != ErrorMsg(amqp_get_rpc_reply(m_pConn), "delete queue"))
	{
		amqp_channel_close(m_pConn, m_iChannel, AMQP_REPLY_SUCCESS);
		return -3;
	}

	amqp_channel_close(m_pConn, m_iChannel, AMQP_REPLY_SUCCESS);
	return 0;
}

int CRabbitmqClient::Publish(const string& strMessage, const string& strExchange, const string& strRoutekey, int m_iChannel)
{
	if (NULL == m_pConn)
	{
		fprintf(stderr, "publish m_pConn is null, publish failed\n");
		return -1;
	}

	amqp_channel_open(m_pConn, m_iChannel);
	if (0 != ErrorMsg(amqp_get_rpc_reply(m_pConn), "open channel"))
	{
		amqp_channel_close(m_pConn, m_iChannel, AMQP_REPLY_SUCCESS);
		return -2;
	}

	amqp_bytes_t message_bytes;
	message_bytes.len = strMessage.length();
	message_bytes.bytes = (void*)(strMessage.c_str());
	//fprintf(stderr, "publish message(%d): %.*s\n", (int)message_bytes.len, (int)message_bytes.len, (char *)message_bytes.bytes);

	/*
	amqp_basic_properties_t props;
	props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
	props.content_type = amqp_cstring_bytes(m_type.c_str());
	props.delivery_mode = m_durable;    // persistent delivery mode
	*/

	amqp_bytes_t exchange = amqp_cstring_bytes(strExchange.c_str());
	amqp_bytes_t routekey = amqp_cstring_bytes(strRoutekey.c_str());

	//if (0 != amqp_basic_publish(m_pConn, m_iChannel, exchange, routekey, 0, 0, &props, message_bytes)) {
	if (0 != amqp_basic_publish(m_pConn, m_iChannel, exchange, routekey, 0, 0, NULL, message_bytes))
	{
		fprintf(stderr, "publish amqp_basic_publish failed\n");
		if (0 != ErrorMsg(amqp_get_rpc_reply(m_pConn), "amqp_basic_publish")) {
			amqp_channel_close(m_pConn, m_iChannel, AMQP_REPLY_SUCCESS);
			return -3;
		}
	}

	amqp_channel_close(m_pConn, m_iChannel, AMQP_REPLY_SUCCESS);
	return 0;
}

int CRabbitmqClient::Consumer(const string& strQueueName, const string& msg_serialNum, int m_iChannel, Callback callback, int GetNum, struct timeval* timeout)
{
	if (NULL == m_pConn)
	{
		fprintf(stderr, "Consumer m_pConn is null, Consumer failed\n");
		return -1;
	}
	if (!m_consumer)
	{
		amqp_channel_open(m_pConn, m_iChannel);
		if (0 != ErrorMsg(amqp_get_rpc_reply(m_pConn), "open channel"))
		{
			amqp_channel_close(m_pConn, m_iChannel, AMQP_REPLY_SUCCESS);
			return -2;
		}

		amqp_basic_qos(m_pConn, m_iChannel, 0, GetNum, 0);
		int ack = 0; // 1:自动确认消息 0:手动确认消息
		amqp_bytes_t queuename = amqp_cstring_bytes(strQueueName.c_str());
		amqp_basic_consume(m_pConn, m_iChannel, queuename, amqp_empty_bytes, 0, ack, 0, amqp_empty_table);
		if (0 != ErrorMsg(amqp_get_rpc_reply(m_pConn), "Consuming")) //获取与最近一次 API 操作（一般是同步 AMQP 方法）相关的 amqp_rpc_reply_t 实例，以便了解操作的结果.这个函数在需要检查操作结果时非常有用，特别是在 API 调用没有明确返回 amqp_rpc_reply_t 的情况下
		{
			amqp_channel_close(m_pConn, m_iChannel, AMQP_REPLY_SUCCESS);
			return -3;
		}
	}
	//cb = callback;
	int rtn = 0;
	int hasget = 0;
	while (GetNum > 0)
	{
		m_consumer = true;
		amqp_maybe_release_buffers(m_pConn);//释放内存
		res = amqp_consume_message(m_pConn, &envelope, timeout, 0);
		if (AMQP_RESPONSE_NORMAL != res.reply_type)
		{
			fprintf(stderr, "Consumer amqp_channel_close failed\n");
			amqp_channel_close(m_pConn, m_iChannel, AMQP_REPLY_SUCCESS);
			m_consumer = false;

			if (0 == hasget)
				return -res.reply_type;
			else
				return 0;
		}

		string str((char*)envelope.message.body.bytes, (char*)envelope.message.body.bytes + envelope.message.body.len);

		printf("recv: %s\n", str.c_str());

		if (parese_json(str, msg_serialNum))
		{
			callback(str);
			//message_array.push_back(str);
			rtn = amqp_basic_ack(m_pConn, m_iChannel, envelope.delivery_tag, 1);//确认消息，如果不是属于自己消息则不需要确认
		}
		else
		{
			rtn = amqp_basic_nack(m_pConn, m_iChannel, envelope.delivery_tag, 0, 1);//确认消息，如果不是属于自己消息则不需要确认
		}
		printf("serialNum: %s\n", msg_serialNum.c_str());
		//printf("serialNum: %s\n", decode_commo.serialNum.c_str());

		amqp_destroy_envelope(&envelope);
		if (rtn != 0)
		{
			amqp_channel_close(m_pConn, m_iChannel, AMQP_REPLY_SUCCESS);
			return -4;
		}

		GetNum--;
		hasget++;
		// 等待100ms
		DWORD dwMilliseconds = 100;
		Sleep(dwMilliseconds);
	}

	return 0;
}

int CRabbitmqClient::ErrorMsg(amqp_rpc_reply_t x, char const* context)
{
	switch (x.reply_type)
	{
	case AMQP_RESPONSE_NORMAL:
		return 0;

	case AMQP_RESPONSE_NONE:
		fprintf(stderr, "%s: missing RPC reply type!\n", context);
		break;

	case AMQP_RESPONSE_LIBRARY_EXCEPTION:
		fprintf(stderr, "%s: %s\n", context, amqp_error_string2(x.library_error));
		break;

	case AMQP_RESPONSE_SERVER_EXCEPTION:
		switch (x.reply.id)
		{
		case AMQP_CONNECTION_CLOSE_METHOD:
		{
			amqp_connection_close_t* m = (amqp_connection_close_t*)x.reply.decoded;
			fprintf(stderr, "%s: server connection error %uh, message: %.*s\n",
				context, m->reply_code, (int)m->reply_text.len,
				(char*)m->reply_text.bytes);
			break;
		}
		case AMQP_CHANNEL_CLOSE_METHOD:
		{
			amqp_channel_close_t* m = (amqp_channel_close_t*)x.reply.decoded;
			fprintf(stderr, "%s: server channel error %uh, message: %.*s\n",
				context, m->reply_code, (int)m->reply_text.len,
				(char*)m->reply_text.bytes);
			break;
		}
		default:
			fprintf(stderr, "%s: unknown server error, method id 0x%08X\n",
				context, x.reply.id);
			break;
		}
		break;
	}

	return -1;
}