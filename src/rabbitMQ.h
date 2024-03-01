#pragma once
#ifndef RABBITMQ_CLIENT_H_
#define RABBITMQ_CLIENT_H_

#include <string>
#include <vector>
#include "amqp_tcp_socket.h"

using std::string;
using std::vector;

class rabbitmqClient {
public:
	rabbitmqClient();
	~rabbitmqClient();

	int Connect(const string& strHostname, int iPort, const string& strUser, const string& strPasswd);
	int Disconnect();
	void DisconnectEx();

	/**
	*   @brief       ExchangeDeclare    ����exchange
	*	@param       [in]               strExchange
	*   @param       [in]               strType
	*   @return ����0ֵ����ɹ�����exchange��С��0�������
	*/
	int ExchangeDeclare(const string& strExchange, const string& strType, bool durable);

	/**
	*   @brief       QueueDeclare                     ������Ϣ����
	*	@param       [in]               strQueueName  ��Ϣ����ʵ��
	*   @param
	*   @return ����0ֵ����ɹ�����queue��С��0�������
	*/
	int QueueDeclare(const string& strQueueName);

	/**
	*   @brief       QueueBind                        �����У��������Ͱ󶨹���������γ�һ��·�ɱ�
	*	@param       [in]               strQueueName  ��Ϣ����
	*	@param       [in]               strExchange   ����������
	*	@param       [in]               strBindKey    ·������  ��msg.#�� ��msg.weather.**��
	*   @return ����0ֵ����ɹ��󶨣�С��0�������
	*/
	int QueueBind(const string& strQueueName, const string& strExchange, const string& strBindKey);

	/**
	*   @brief       QueueUnbind                      �����У��������Ͱ󶨹���󶨽��
	*	@param       [in]               strQueueName  ��Ϣ����
	*	@param       [in]               strExchange   ����������
	*	@param       [in]               strBindKey    ·������  ��msg.#�� ��msg.weather.**��
	*   @return ����0ֵ����ɹ��󶨣�С��0�������
	*/
	int QueueUnbind(const string& strQueueName, const string& strExchange, const string& strBindKey);

	/**
	*   @brief       QueueDelete                      ɾ����Ϣ���С�
	*	@param       [in]               strQueueName  ��Ϣ��������
	*	@param       [in]               iIfUnused     ��Ϣ�����Ƿ����ã�1 �����Ƿ����ö�ɾ��
	*   @return ����0ֵ����ɹ�ɾ��queue��С��0�������
	*/
	int QueueDelete(const string& strQueueName, int iIfUnused);

	/**
	* @brief Publish  ������Ϣ
	* @param [in] strMessage        ��Ϣʵ��
	* @param [in] strExchange       ������
	* @param [in] strRoutekey       ·�ɹ���
	*   1.Direct Exchange �C ����·�ɼ�����Ҫ��һ�����а󶨵��������ϣ�Ҫ�����Ϣ��һ���ض���·�ɼ���ȫƥ�䡣
	*   2.Fanout Exchange �C ������·�ɼ��������а󶨵��������ϡ�һ�����͵�����������Ϣ���ᱻת������ý������󶨵����ж����ϡ�
	*   3.Topic Exchange �C ��·�ɼ���ĳģʽ����ƥ�䡣��ʱ������Ҫ��Ҫһ��ģʽ�ϡ����š�#��ƥ��һ�������ʣ����š�*��ƥ�䲻�಻��һ���ʡ�
	*      ��ˡ�audit.#���ܹ�ƥ�䵽��audit.irs.corporate�������ǡ�audit.*�� ֻ��ƥ�䵽��audit.irs��
	* @return ����0ֵ����ɹ�������Ϣʵ�壬С��0�����ʹ���
	*/
	int Publish(const string& strMessage, const string& strExchange, const string& strRoutekey);

	/**
	* @brief consumer  ������Ϣ
	* @param [in]  strQueueName         ��������
	* @param [out] message_array        ��ȡ����Ϣʵ��
	* @param [int] GetNum               ��Ҫȡ�õ���Ϣ����
	* @param [int] filterate            �Ƿ������Ϣ��true:������Ҫ���ˣ�false: ���в���Ҫ����
	* @param [int] timeout              ȡ�õ���Ϣ���ӳ٣���ΪNULL����ʾ����ȡ�����ӳ٣�����״̬
	* @return ����0ֵ����ɹ���С��0������󣬴�����Ϣ��ErrorReturn����
	*/
	int Consumer(const string& strQueueName, vector<string>& message_array, int GetNum = 1, bool filterate = true, struct timeval* timeout = NULL);
	int CancelConsuming();

	bool CheckChannelStatu();

private:
	rabbitmqClient(const rabbitmqClient& rh);
	void operator=(const rabbitmqClient& rh);

	int ErrorMsg(amqp_rpc_reply_t x, char const* context);

	string                      m_strHostname;      // amqp����
	int                         m_iPort;            // amqp�˿�
	string					    m_strUser;
	string					    m_strPasswd;
	int                         m_iChannel;
	int                         m_iDisChannel;

	amqp_socket_t* m_pSock;
	amqp_connection_state_t     m_pDisConn;
	amqp_connection_state_t     m_pConn;
	amqp_channel_open_ok_t* channel_id;
	amqp_bytes_t consumer_tag;
};

#endif