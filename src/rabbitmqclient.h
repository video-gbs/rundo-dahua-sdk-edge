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

	// 回调函数类型
	using Callback = std::function<void(const std::string&)>;

	/**
	*   @brief       ExchangeDeclare    声明exchange
	*	@param       [in]               strExchange
	*   @param       [in]               strType
	*	@param		 [in]				m_iChannel			默认用1号通道，通道无所谓
	*   @return 等于0值代表成功创建exchange，小于0代表错误
	*/
	int ExchangeDeclare(const string& strExchange, const string& strType, int m_iChannel);

	/**
	*   @brief       QueueDeclare                     声明消息队列
	*	@param       [in]               strQueueName  消息队列实例
	*	@param		 [in]				m_iChannel			默认用1号通道，通道无所谓
	*   @return 等于0值代表成功创建queue，小于0代表错误
	*/
	int QueueDeclare(const string& strQueueName, int m_iChannel);

	/**
	*   @brief       QueueBind                        将队列，交换机和绑定规则绑定起来形成一个路由表
	*	@param       [in]               strQueueName  消息队列
	*	@param       [in]               strExchange   交换机名称
	*	@param       [in]               strBindKey    路由名称  “msg.#” “msg.weather.**”
	*	@param		 [in]				m_iChannel			默认用1号通道，通道无所谓
	*   @return 等于0值代表成功绑定，小于0代表错误
	*/
	int QueueBind(const string& strQueueName, const string& strExchange, const string& strBindKey, int m_iChannel);

	/**
	*   @brief       QueueUnbind                      将队列，交换机和绑定规则绑定解除
	*	@param       [in]               strQueueName  消息队列
	*	@param       [in]               strExchange   交换机名称
	*	@param       [in]               strBindKey    路由名称  “msg.#” “msg.weather.**”
	*	@param		 [in]				m_iChannel			默认用1号通道，通道无所谓
	*   @return 等于0值代表成功绑定，小于0代表错误
	*/
	int QueueUnbind(const string& strQueueName, const string& strExchange, const string& strBindKey, int m_iChannel);

	/**
	*   @brief       QueueDelete                      删除消息队列。
	*	@param       [in]               strQueueName  消息队列名称
	*	@param       [in]               iIfUnused     消息队列是否在用，1 则论是否在用都删除
	* @param [in]  m_iChannel			默认用1号通道，通道无所谓
	*   @return 等于0值代表成功删除queue，小于0代表错误
	*/
	int QueueDelete(const string& strQueueName, int iIfUnused, int m_iChannel);

	/**
	* @brief Publish  发布消息
	* @param [in] strMessage        消息实体
	* @param [in] strExchange       交换器
	* @param [in] strRoutekey       路由规则
	* @param [in]  m_iChannel			默认用1号通道，通道无所谓
	*   1.Direct Exchange – 处理路由键。需要将一个队列绑定到交换机上，要求该消息与一个特定的路由键完全匹配。
	*   2.Fanout Exchange – 不处理路由键。将队列绑定到交换机上。一个发送到交换机的消息都会被转发到与该交换机绑定的所有队列上。
	*   3.Topic Exchange – 将路由键和某模式进行匹配。此时队列需要绑定要一个模式上。符号“#”匹配一个或多个词，符号“*”匹配不多不少一个词。
	*      因此“audit.#”能够匹配到“audit.irs.corporate”，但是“audit.*” 只会匹配到“audit.irs”
	* @return 等于0值代表成功发送消息实体，小于0代表发送错误
	*/
	int Publish(const string& strMessage, const string& strExchange, const string& strRoutekey, int m_iChannel);

	/**
	* @brief consumer  消费消息
	* @param [in]  strQueueName         队列名称
	* @param [in]  msg_serialNum        消息序列号，若获取的消息中serialNum与之相同则返回
	* @param [in]  m_iChannel			默认用1号通道，通道无所谓
	* @param [in] callback				回调函数
	* @param [int] GetNum               需要取得的消息个数
	* @param [int] timeout              取得的消息是延迟，若为NULL，表示持续取，无延迟，阻塞状态
	* @return 等于0值代表成功，小于0代表错误，错误信息从ErrorReturn返回
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

	bool                        m_consumer;         //消费中
	string                      m_strHostname;      // amqp主机
	int                         m_iPort;            // amqp端口
	string					    m_strUser;
	string					    m_strPasswd;

	amqp_socket_t* m_pSock;
	amqp_connection_state_t     m_pConn;

	amqp_rpc_reply_t res;
	amqp_envelope_t envelope;

	boost::shared_ptr<boost::thread> thread_;
	Callback callback;

	int commo_public_channel;//公共发布队列通道
	int commo_consumer_channel;//公共监听队列通道
	int business_public_channel;//业务发布队列通道
	int business_consumer_channel;//业务监听队列通道
	std::string serialNum;
};

#endif
