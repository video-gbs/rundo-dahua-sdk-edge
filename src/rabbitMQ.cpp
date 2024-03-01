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
	, m_iChannel(1) //Ĭ����1��ͨ����ͨ������ν
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

	LOG_CONSOLE("rabbitmq���ӳɹ�:[%p:%s:%d]", m_pConn, m_strHostname.c_str(), m_iPort);
	channel_id = NULL;

	return 0;
}

int rabbitmqClient::Disconnect()
{
	LOG_CONSOLE("��ʼ����������ͨ����m_pConn:%p channel_id:%p channel:%d", m_pConn, channel_id, m_iChannel);

	// �ȹر�ͨ��
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
	LOG_CONSOLE("��ʼ�첽����������ͨ����m_pConn:%p channel_id:%p channel:%d", m_pDisConn, channel_id, m_iDisChannel);

	// �ȹر�ͨ��
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
	int _durable = durable;      // �������Ƿ�־û�
	amqp_exchange_declare(m_pConn, m_iChannel, _exchange, _type, _passive, _durable, 0, 0, amqp_empty_table);
	if (0 != ErrorMsg(amqp_get_rpc_reply(m_pConn), "exchange_declare"))
	{
		amqp_channel_close(m_pConn, m_iChannel, AMQP_REPLY_SUCCESS);
		channel_id = NULL;
		return -1;
	}
	//����������ȷ��ģʽ
	amqp_confirm_select(m_pConn, m_iChannel);
	LOG_CONSOLE("����������ȷ��ģʽ,exchange:%s", strExchange.c_str());

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

	LOG_CONSOLE("��������:%s", strQueueName.c_str());

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

	LOG_CONSOLE("���а�:[%s][%s][%s]", strQueueName.c_str(), strExchange.c_str(), strBindKey.c_str());

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

	//LOG_CONSOLE("������%s", strMessage.c_str());
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
	// ׼������֡
	amqp_frame_t frame = {0};
	timeval tv;
	tv.tv_sec = 2;//�ȴ�5��
	tv.tv_usec = 0;//�ȴ�5΢��
	//amqp_simple_wait_frame(m_pConn, &frame);
	amqp_simple_wait_frame_noblock(m_pConn, &frame, &tv);
	if (frame.channel == m_iChannel)//���ȼ���AMQP���ӽ��յ���֡�Ƿ�����Ԥ��ͨ��(�ڴ�������KChannel)
	{    //����ǣ���ô��һ�����֡�����ݡ����鿴֡���غ��еķ���ID�����Ƿ�ΪAMQP_BASIC_ACK_METHOD
		if (frame.payload.method.id == AMQP_BASIC_ACK_METHOD)//AMQP_BASIC_ACK_METHOD��һ������ID����ʾ����ȷ��֡�����������Ѿ��ɹ����ղ�������֮ǰ���͵���Ϣ
		{
			// ������յ�ACKȷ��֡����ô�����غɽ���Ϊһ��amqp_basic_ack_t�ṹ�����鿴����ṹ�е�multiple�ֶ�
			amqp_basic_ack_t* ack = (amqp_basic_ack_t*)frame.payload.method.decoded; // ������յ�ACKȷ��֡����ӡȷ����Ϣ
			if (ack->multiple)
				//���multipleΪ�棬��ô��ʾ�������Ѿ��ɹ����������е���ָ��delivery tag����Ϣ�����򽫴�ӡһ��ȷ����Ϣ������ʾ�ɹ���������һ����Ϣ��delivery tag��
				LOG_INFO("��������������Ϣ,[delivery_tag:%s:%llu] ", strRoutekey.c_str(), ack->delivery_tag);
			else
			{
				/*���multiple�ֶ�Ϊ�٣�false������ô��ʾ�������Ѿ��ɹ������˾���ָ��delivery tag�ĵ�����Ϣ�����򽫴�ӡһ��ȷ����Ϣ������ʾ�ɹ��������Ϣ��delivery tag��*/
				if (strRoutekey.find("GATEWAY_SG") != std::string::npos)
					LOG_CONSOLE("���������յ���Ϣ,[delivery_tag:%s:%llu] %s ", strRoutekey.c_str(), ack->delivery_tag, strMessage.c_str());
				else
					LOG_INFO("���������յ���Ϣ,[delivery_tag:%s:%llu] %s ", strRoutekey.c_str(), ack->delivery_tag, strMessage.c_str());
			}

			return iRet;
		}
		else if (frame.payload.method.id == AMQP_BASIC_RETURN_METHOD) //���֡���غ��еķ���ID�����Ƿ�ΪAMQP_BASIC_RETURN_METHOD��
		{ //�������ID��ʾ����һ������֡������Ϣδ�ܱ���ȷ·�ɵ����У�����RabbitMQ���������ء�
			//������յ����Ƿ���֡����ô����ȡ�����淵�ص���Ϣ��Ȼ�����������Ϣ����
			amqp_message_t returned_message;
			amqp_read_message(m_pConn, 1, &returned_message, 0);//��ȡ�����淵�ص���Ϣ
			LOG_ERROR("��Ϣ���͵�����ʧ�ܣ���RabbitMQ�������˻�,[delivery_tag:%s] %s", strRoutekey.c_str(), (char*)returned_message.body.bytes);
			amqp_destroy_message(&returned_message);//Ȼ�����������Ϣ����
			//�ٴε���amqp_simple_wait_frame������������һ��֡��������Ϊ��MQP_BASIC_RETURN_METHOD֮��
			//RabbitMQ�������ᷢ����һ��AMQP_BASIC_ACK_METHOD֡��ȷ����Ϣ�Ѿ������ء�
			//amqp_simple_wait_frame(m_pConn, &frame);
			amqp_simple_wait_frame_noblock(m_pConn, &frame, &tv);
			if (frame.payload.method.id == AMQP_BASIC_ACK_METHOD)
				//������յ���֡��AMQP_BASIC_ACK_METHOD֡��������ӡ��"Message returned"����ȷ����Ϣ�Ѿ������ء�
				LOG_CONSOLE("Message returned");
			return -1;
		}
		LOG_ERROR("��Ϣ���͵�����ʧ��,[delivery_tag:%s] ", strRoutekey.c_str());
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

	int no_ack = filterate ? 0 : 1; // no_ackΪtrue ��ô�������ᷢ��Basic.Ack(����amqp_basic_ack),no_ackΪfalse����ô������Ҫ����Basic.Ack(����amqp_basic_ack)

	int no_local = 0; // ����ָʾ��������Ӧ����Ϣ���͸��������Լ�
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
				LOG_ERROR("���籨���������ٲ�����");
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
		{//��Ҫ���ˣ�һ�㴦��������
			if (gm.isMySerialNum(str))
			{
				amqp_boolean_t multiple = 0;  // false��ֻȷ��ָ���ĵ�����Ϣ
				message_array.push_back(str);
				//rtn = amqp_basic_ack(m_pConn, m_iChannel, envelope.delivery_tag, multiple);//֪ͨ��������Ϣ�ѱ��ɹ�����
				if (rtn == 0)
				{
					// ��ӡ��־��ȷ����Ϣ����ȷ����
					LOG_INFO("ȷ����Ϣ����ȷ����:\n%s", str.c_str());
				}
			}
			else
			{
				rtn = amqp_basic_nack(m_pConn, m_iChannel, envelope.delivery_tag, 1, 1);//��֪��������Ϣδ�ܳɹ�����
			}
		}
		else
		{//����Ҫ����
			amqp_boolean_t multiple = 0;  // false��ֻȷ��ָ���ĵ�����Ϣ
			message_array.push_back(str);
			//rtn = amqp_basic_ack(m_pConn, m_iChannel, envelope.delivery_tag, multiple);//֪ͨ��������Ϣ�ѱ��ɹ�����
			if (rtn == 0)
			{
				// ��ӡ��־��ȷ����Ϣ����ȷ����
				LOG_INFO("ȷ����Ϣ����ȷ����:\n%s", str.c_str());
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
	// ���ȡ���������е�����
	if (1)
	{
		amqp_basic_cancel(m_pConn, m_iChannel, consumer_tag);
	}
	// �ͷ���Դ
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
	//	LOG_ERROR("���籨���������ٲ�����");
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
		LOG_ERROR("%s:%s %d", context, amqp_error_string2(x.library_error), x.library_error);//a socket error occurred �����׽��ִ���
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