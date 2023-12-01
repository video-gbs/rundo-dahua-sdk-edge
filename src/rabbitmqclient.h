#ifndef RABBITMQ_CLIENT_H_
#define RABBITMQ_CLIENT_H_

#include <string>
#include <vector>
#include <functional>
#include <boost/thread.hpp>

#include "amqp_tcp_socket.h"

using std::string;
using std::vector;

class CRabbitmqClient
{
public:
	CRabbitmqClient();
	~CRabbitmqClient();

	// �ص���������
	using Callback = std::function<void(const std::string&)>;

	/**
	*   @brief       ExchangeDeclare    ����exchange
	*	@param       [in]               strExchange
	*   @param       [in]               strType
	*	@param		 [in]				m_iChannel			Ĭ����1��ͨ����ͨ������ν
	*   @return ����0ֵ����ɹ�����exchange��С��0�������
	*/
	int ExchangeDeclare(const string& strExchange, const string& strType, int m_iChannel);

	/**
	*   @brief       QueueDeclare                     ������Ϣ����
	*	@param       [in]               strQueueName  ��Ϣ����ʵ��
	*	@param		 [in]				m_iChannel			Ĭ����1��ͨ����ͨ������ν
	*   @return ����0ֵ����ɹ�����queue��С��0�������
	*/
	int QueueDeclare(const string& strQueueName, int m_iChannel);

	/**
	*   @brief       QueueBind                        �����У��������Ͱ󶨹���������γ�һ��·�ɱ�
	*	@param       [in]               strQueueName  ��Ϣ����
	*	@param       [in]               strExchange   ����������
	*	@param       [in]               strBindKey    ·������  ��msg.#�� ��msg.weather.**��
	*	@param		 [in]				m_iChannel			Ĭ����1��ͨ����ͨ������ν
	*   @return ����0ֵ����ɹ��󶨣�С��0�������
	*/
	int QueueBind(const string& strQueueName, const string& strExchange, const string& strBindKey, int m_iChannel);

	/**
	*   @brief       QueueUnbind                      �����У��������Ͱ󶨹���󶨽��
	*	@param       [in]               strQueueName  ��Ϣ����
	*	@param       [in]               strExchange   ����������
	*	@param       [in]               strBindKey    ·������  ��msg.#�� ��msg.weather.**��
	*	@param		 [in]				m_iChannel			Ĭ����1��ͨ����ͨ������ν
	*   @return ����0ֵ����ɹ��󶨣�С��0�������
	*/
	int QueueUnbind(const string& strQueueName, const string& strExchange, const string& strBindKey, int m_iChannel);

	/**
	*   @brief       QueueDelete                      ɾ����Ϣ���С�
	*	@param       [in]               strQueueName  ��Ϣ��������
	*	@param       [in]               iIfUnused     ��Ϣ�����Ƿ����ã�1 �����Ƿ����ö�ɾ��
	* @param [in]  m_iChannel			Ĭ����1��ͨ����ͨ������ν
	*   @return ����0ֵ����ɹ�ɾ��queue��С��0�������
	*/
	int QueueDelete(const string& strQueueName, int iIfUnused, int m_iChannel);

	/**
	* @brief Publish  ������Ϣ
	* @param [in] strMessage        ��Ϣʵ��
	* @param [in] strExchange       ������
	* @param [in] strRoutekey       ·�ɹ���
	* @param [in]  m_iChannel			Ĭ����1��ͨ����ͨ������ν
	*   1.Direct Exchange �C ����·�ɼ�����Ҫ��һ�����а󶨵��������ϣ�Ҫ�����Ϣ��һ���ض���·�ɼ���ȫƥ�䡣
	*   2.Fanout Exchange �C ������·�ɼ��������а󶨵��������ϡ�һ�����͵�����������Ϣ���ᱻת������ý������󶨵����ж����ϡ�
	*   3.Topic Exchange �C ��·�ɼ���ĳģʽ����ƥ�䡣��ʱ������Ҫ��Ҫһ��ģʽ�ϡ����š�#��ƥ��һ�������ʣ����š�*��ƥ�䲻�಻��һ���ʡ�
	*      ��ˡ�audit.#���ܹ�ƥ�䵽��audit.irs.corporate�������ǡ�audit.*�� ֻ��ƥ�䵽��audit.irs��
	* @return ����0ֵ����ɹ�������Ϣʵ�壬С��0�����ʹ���
	*/
	int Publish(const string& strMessage, const string& strExchange, const string& strRoutekey, int m_iChannel);

	/**
	* @brief consumer  ������Ϣ
	* @param [in]  strQueueName         ��������
	* @param [in]  msg_serialNum        ��Ϣ���кţ�����ȡ����Ϣ��serialNum��֮��ͬ�򷵻�
	* @param [in]  m_iChannel			Ĭ����1��ͨ����ͨ������ν
	* @param [in] callback				�ص�����
	* @param [int] GetNum               ��Ҫȡ�õ���Ϣ����
	* @param [int] timeout              ȡ�õ���Ϣ���ӳ٣���ΪNULL����ʾ����ȡ�����ӳ٣�����״̬
	* @return ����0ֵ����ɹ���С��0������󣬴�����Ϣ��ErrorReturn����
	*/
	int Consumer(const string& strQueueName, const string& msg_serialNum, int m_iChannel, Callback callback, int GetNum = 1, struct timeval* timeout = NULL);

	void receiveFunction(int iChannel);

	int Connect(const string& strHostname, int iPort, const string& strUser, const string& strPasswd);
	int Disconnect();

	int SetSend(const string& strExchange, const string& strType, const string& strQueueName, int iChannel);
	int Send(const string& strMessage, const string& strExchange, const string& strRoutekey, int iChannel);

	int SetRecv(const string& strExchange, const string& strQueueName, const string& strBindKey, const string& msg_serialNum, Callback cb, int GetNum, int iChannel);
private:
	CRabbitmqClient(const CRabbitmqClient& rh);
	void operator=(const CRabbitmqClient& rh);

	int ErrorMsg(amqp_rpc_reply_t x, char const* context);

	bool                        m_consumer;         //������
	string                      m_strHostname;      // amqp����
	int                         m_iPort;            // amqp�˿�
	string					    m_strUser;
	string					    m_strPasswd;

	amqp_socket_t* m_pSock;
	amqp_connection_state_t     m_pConn;

	amqp_rpc_reply_t res;
	amqp_envelope_t envelope;

	boost::shared_ptr<boost::thread> thread_;
	Callback callback;

	int commo_public_channel;//������������ͨ��
	int commo_consumer_channel;//������������ͨ��
	int business_public_channel;//ҵ�񷢲�����ͨ��
	int business_consumer_channel;//ҵ���������ͨ��
	std::string serialNum;
};

#endif
