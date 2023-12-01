#include "gateway_msg.h"

#include <boost/asio.hpp>
#include <boost/unordered_map.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/locale.hpp>
#include <boost/algorithm/string.hpp>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

gateway_msg::gateway_msg()
{
	try {
		std::string hostName = boost::asio::ip::host_name();
		//std::cout << "Host name: " << hostName << std::endl;

		boost::asio::io_context io_context;
		boost::asio::ip::tcp::resolver resolver(io_context);

		// 设置查询为 IPv4
		boost::asio::ip::tcp::resolver::query query(hostName, "");
		boost::asio::ip::tcp::resolver::iterator iter = resolver.resolve(query);

		while (iter != boost::asio::ip::tcp::resolver::iterator())
		{
			boost::asio::ip::tcp::endpoint ep = *iter++;
			if (ep.size() == 16)
			{
				local_ip = ep.address().to_string();
				//std::cout << "IP Address: " << ep.address().to_string() << std::endl;
			}
		}
	}
	catch (std::exception& e) {
		std::cerr << "Exception: " << e.what() << std::endl;
	}

	msg_id = 1;
	msg_id_h = "h_";
	msg_id_b = "b_";
}

gateway_msg::~gateway_msg()
{
}

std::string getCurrentTime() 
{
	std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	std::tm timeInfo = *std::localtime(&now);
	std::stringstream ss;
	ss << std::put_time(&timeInfo, "%Y-%m-%d %H:%M:%S");
	return ss.str();
}

std::string gateway_msg::gateway_sign_in()
{
	// 获取当前时间点
	auto now = std::chrono::system_clock::now();
	// 将时间点转换为绝对秒数
	auto epoch = now.time_since_epoch();
	auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(epoch);

	// 获取当前时间点
	std::time_t time_ = std::chrono::system_clock::to_time_t(now);
	// 格式化输出到字符串
	std::tm timeInfo = *std::localtime(&time_);
	std::stringstream ss;
	ss << std::put_time(&timeInfo, "%Y-%m-%d %H:%M:%S");
	// 将时间存储到 std::string
	std::string formatted_time = ss.str();

	rapidjson::Document doc;
	doc.SetObject();

	rapidjson::Value serialNum;
	serialNum.SetString(strSerialNum.c_str(), doc.GetAllocator());
	doc.AddMember("serialNum", serialNum, doc.GetAllocator());

	rapidjson::Value msgType;
	msgType.SetString("GATEWAY_SIGN_IN", doc.GetAllocator());
	doc.AddMember("msgType", msgType, doc.GetAllocator());

	msg_id++;
	rapidjson::Value msgId;
	msgId.SetString((msg_id_h += std::to_string(msg_id)).c_str(), doc.GetAllocator());
	doc.AddMember("msgId", msgId, doc.GetAllocator());
	msg_id_h = "h_";

	rapidjson::Value time;
	time.SetString(formatted_time.c_str(), doc.GetAllocator());
	doc.AddMember("time", time, doc.GetAllocator());

	doc.AddMember("code", 0, doc.GetAllocator());

	rapidjson::Value msg;
	msg.SetString("SUCCESS", doc.GetAllocator());
	doc.AddMember("msg", msg, doc.GetAllocator());

	// 创建内部 JSON 对象
	if (local_ip.empty())
	{
		try {
			std::string hostName = boost::asio::ip::host_name();
			//std::cout << "Host name: " << hostName << std::endl;

			boost::asio::io_context io_context;
			boost::asio::ip::tcp::resolver resolver(io_context);

			// 设置查询为 IPv4
			boost::asio::ip::tcp::resolver::query query(hostName, "");
			boost::asio::ip::tcp::resolver::iterator iter = resolver.resolve(query);

			while (iter != boost::asio::ip::tcp::resolver::iterator())
			{
				boost::asio::ip::tcp::endpoint ep = *iter++;
				if (ep.size() == 16)
				{
					local_ip = ep.address().to_string();
					//std::cout << "IP Address: " << ep.address().to_string() << std::endl;
				}
			}
		}
		catch (std::exception& e) {
			std::cerr << "Exception: " << e.what() << std::endl;
		}
	}
	rapidjson::Value ip_;
	ip_.SetString(local_ip.c_str(), doc.GetAllocator());

	rapidjson::Value data;
	data.SetObject();
	data.AddMember("id", 0, doc.GetAllocator());
	data.AddMember("ip", ip_, doc.GetAllocator());
	data.AddMember("port", 18082, doc.GetAllocator());
	data.AddMember("protocol", "DH-DEVICE_NET_SDK_V6", doc.GetAllocator());
	data.AddMember("gatewayType", "OTHER", doc.GetAllocator());
	std::string outtime_(std::to_string(milliseconds.count() + 3 * 60 * 1000));
	rapidjson::Value outtime;
	outtime.SetString(outtime_.c_str(), doc.GetAllocator());
	data.AddMember("outTime", outtime, doc.GetAllocator());

	doc.AddMember("data", data, doc.GetAllocator());

	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	doc.Accept(writer);

	return buffer.GetString();
}

std::string gateway_msg::gateway_heartbeat()
{
	// 获取当前时间点
	auto now = std::chrono::system_clock::now();
	// 将时间点转换为绝对秒数
	auto epoch = now.time_since_epoch();
	auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(epoch);

	// 获取当前时间点
	std::time_t time_ = std::chrono::system_clock::to_time_t(now);
	// 格式化输出到字符串
	std::tm timeInfo = *std::localtime(&time_);
	std::stringstream ss;
	ss << std::put_time(&timeInfo, "%Y-%m-%d %H:%M:%S");
	// 将时间存储到 std::string
	std::string formatted_time = ss.str();

	rapidjson::Document doc;
	doc.SetObject();

	rapidjson::Value serialNum;
	serialNum.SetString(strSerialNum.c_str(), doc.GetAllocator());
	doc.AddMember("serialNum", serialNum, doc.GetAllocator());

	rapidjson::Value msgType;
	msgType.SetString("GATEWAY_HEARTBEAT", doc.GetAllocator());
	doc.AddMember("msgType", msgType, doc.GetAllocator());

	msg_id++;
	rapidjson::Value msgId;
	msgId.SetString((msg_id_h += std::to_string(msg_id)).c_str(), doc.GetAllocator());
	doc.AddMember("msgId", msgId, doc.GetAllocator());
	msg_id_h = "h_";

	rapidjson::Value time;
	time.SetString(formatted_time.c_str(), doc.GetAllocator());
	doc.AddMember("time", time, doc.GetAllocator());

	doc.AddMember("code", 0, doc.GetAllocator());

	rapidjson::Value msg;
	msg.SetString("false", doc.GetAllocator());
	doc.AddMember("error", msg, doc.GetAllocator());

	rapidjson::Value data;
	data.SetString(std::to_string(milliseconds.count() + heartbeat_timeout).c_str(), doc.GetAllocator());
	doc.AddMember("data", data, doc.GetAllocator());

	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	doc.Accept(writer);

	return buffer.GetString();
}

std::string gateway_msg::gateway_re_sign_in()
{
	// 获取当前时间点
	auto now = std::chrono::system_clock::now();
	// 将时间点转换为绝对秒数
	auto epoch = now.time_since_epoch();
	auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(epoch);

	// 获取当前时间点
	std::time_t time_ = std::chrono::system_clock::to_time_t(now);
	// 格式化输出到字符串
	std::tm timeInfo = *std::localtime(&time_);
	std::stringstream ss;
	ss << std::put_time(&timeInfo, "%Y-%m-%d %H:%M:%S");
	// 将时间存储到 std::string
	std::string formatted_time = ss.str();

	rapidjson::Document doc;
	doc.SetObject();

	rapidjson::Value serialNum;
	serialNum.SetString(strSerialNum.c_str(), doc.GetAllocator());
	doc.AddMember("serialNum", serialNum, doc.GetAllocator());

	rapidjson::Value msgType;
	msgType.SetString("GATEWAY_RE_SIGN_IN", doc.GetAllocator());
	doc.AddMember("msgType", msgType, doc.GetAllocator());

	msg_id++;
	rapidjson::Value msgId;
	msgId.SetString((msg_id_h += std::to_string(msg_id)).c_str(), doc.GetAllocator());
	doc.AddMember("msgId", msgId, doc.GetAllocator());
	msg_id_h = "h_";

	rapidjson::Value time;
	time.SetString(formatted_time.c_str(), doc.GetAllocator());
	doc.AddMember("time", time, doc.GetAllocator());

	doc.AddMember("code", 0, doc.GetAllocator());

	rapidjson::Value msg;
	msg.SetString("SUCCESS", doc.GetAllocator());
	doc.AddMember("msg", msg, doc.GetAllocator());

	// 创建内部 JSON 对象
	if (local_ip.empty())
	{
		try {
			std::string hostName = boost::asio::ip::host_name();
			//std::cout << "Host name: " << hostName << std::endl;

			boost::asio::io_context io_context;
			boost::asio::ip::tcp::resolver resolver(io_context);

			// 设置查询为 IPv4
			boost::asio::ip::tcp::resolver::query query(hostName, "");
			boost::asio::ip::tcp::resolver::iterator iter = resolver.resolve(query);

			while (iter != boost::asio::ip::tcp::resolver::iterator())
			{
				boost::asio::ip::tcp::endpoint ep = *iter++;
				if (ep.size() == 16)
				{
					local_ip = ep.address().to_string();
					//std::cout << "IP Address: " << ep.address().to_string() << std::endl;
				}
			}
		}
		catch (std::exception& e) {
			std::cerr << "Exception: " << e.what() << std::endl;
		}
	}
	rapidjson::Value ip_;
	ip_.SetString(local_ip.c_str(), doc.GetAllocator());

	rapidjson::Value data;
	data.SetObject();
	data.AddMember("id", 0, doc.GetAllocator());
	data.AddMember("ip", ip_, doc.GetAllocator());
	data.AddMember("port", 18082, doc.GetAllocator());
	data.AddMember("protocol", "DH-DEVICE_NET_SDK_V6", doc.GetAllocator());
	data.AddMember("gatewayType", "OTHER", doc.GetAllocator());
	std::string outtime_(std::to_string(milliseconds.count() + heartbeat_timeout));
	rapidjson::Value outtime;
	outtime.SetString(outtime_.c_str(), doc.GetAllocator());
	data.AddMember("outtime", outtime, doc.GetAllocator());

	doc.AddMember("data", data, doc.GetAllocator());

	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	doc.Accept(writer);

	return buffer.GetString();
}

std::string gateway_msg::device_sign_in()
{
	rapidjson::Document doc;
	doc.SetObject();

	rapidjson::Value serialNum;
	serialNum.SetString(strSerialNum, doc.GetAllocator());
	doc.AddMember("serialNum", serialNum, doc.GetAllocator());

	rapidjson::Value msgType;
	msgType.SetString("DEVICE_SIGN_IN", doc.GetAllocator());
	doc.AddMember("msgType", msgType, doc.GetAllocator());

	msg_id++;
	rapidjson::Value msgId;
	msgId.SetString((msg_id_b += std::to_string(msg_id)).c_str(), doc.GetAllocator());
	doc.AddMember("msgId", msgId, doc.GetAllocator());
	msg_id_b = "b_";

	rapidjson::Value time;
	time.SetString("2023-08-30T17:22:05.588553100", doc.GetAllocator());
	doc.AddMember("time", time, doc.GetAllocator());

	doc.AddMember("code", 0, doc.GetAllocator());

	rapidjson::Value msg;
	msg.SetString("SUCCESS", doc.GetAllocator());
	doc.AddMember("msg", msg, doc.GetAllocator());

	rapidjson::Value data;
	data.SetObject();
	data.AddMember("id", 20043, doc.GetAllocator());
	data.AddMember("lUserId", 4, doc.GetAllocator());
	data.AddMember("username", "admin", doc.GetAllocator());
	data.AddMember("serialNumber", "DS-7808N-R2(C)0820230227CCRRL33683189WCVU", doc.GetAllocator());
	data.AddMember("name", "test", doc.GetAllocator());
	data.AddMember("charset", "GB2312", doc.GetAllocator());
	data.AddMember("ip", "172.20.0.245", doc.GetAllocator());
	data.AddMember("port", 8000, doc.GetAllocator());
	data.AddMember("manufacturer", "hikvision", doc.GetAllocator());
	data.AddMember("online", 1, doc.GetAllocator());
	data.AddMember("deviceType", 3, doc.GetAllocator());
	data.AddMember("deleted", 0, doc.GetAllocator());
	data.AddMember("password", "rj123456", doc.GetAllocator());
	data.AddMember("createdAt", "2023-08-30T17:22:05", doc.GetAllocator());
	data.AddMember("updatedAt", "2023-08-30T17:22:05", doc.GetAllocator());

	doc.AddMember("data", data, doc.GetAllocator());

	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	doc.Accept(writer);
	return buffer.GetString();
}

bool gateway_msg::pars_device_sync(std::string json, Platform& platform)
{
	rapidjson::Document doc;
	doc.Parse(json.c_str());

	if (!doc.HasParseError())
	{
		if (doc.IsObject())
		{
			if (doc.HasMember("code") && doc["code"].IsInt())
			{
				int code = doc["code"].GetInt();
				//std::cout << "Code: " << code << std::endl;
			}
			if (doc.HasMember("msgId") && doc["msgId"].IsString())
			{
				platform.msgId = doc["msgId"].GetString();
			}

			if (doc.HasMember("data") && doc["data"].IsObject())
			{
				const rapidjson::Value& data = doc["data"];

				if (data.HasMember("deviceId") && data["deviceId"].IsString())
				{
					platform.deviceId = data["deviceId"].GetString();
					//std::cout << "Device ID: " << device << std::endl;
					return true;
				}
			}

			// Parse other fields in a similar way
		}
	}
	else
	{
		std::cout << "JSON parse error" << std::endl;
	}
	return false;
}

bool gateway_msg::pars_device_delete_soft(std::string json, Platform& platform)
{
	return pars_device_sync(json, platform);
}

bool gateway_msg::pars_device_total_sync(std::string json, std::string& msgId)
{
	rapidjson::Document doc;
	doc.Parse(json.c_str());

	if (!doc.IsObject())
	{
		return false;
	}

	if (doc.HasMember("msgId") && doc["msgId"].IsString())
	{
		msgId = doc["msgId"].GetString();
			return true;
	}
	return false;
}

std::string gateway_msg::device_total_sync(std::vector<Platform> platform_list, std::string msgId)
{
	rapidjson::Document document;
	document.SetObject();
	rapidjson::Document::AllocatorType& allocator = document.GetAllocator();

	// 添加 "code", "data", "msg", "msgId", "msgType", "serialNum"
	document.AddMember("code", 0, allocator);

	rapidjson::Value dataArray(rapidjson::kArrayType);
	for (const auto& platform : platform_list)
	{
		if (!platform.soft_del)
		{
			rapidjson::Value platformObject(rapidjson::kObjectType);

			platformObject.AddMember("deviceId", rapidjson::StringRef(platform.deviceId.c_str()), allocator);
			platformObject.AddMember("firmware", rapidjson::StringRef(""), allocator);
			platformObject.AddMember("ip", rapidjson::StringRef(platform.ip.c_str()), allocator);
			platformObject.AddMember("manufacturer", rapidjson::StringRef("dahua"), allocator);
			platformObject.AddMember("model", rapidjson::StringRef("DH-NVR4208-HDS2/H"), allocator);
			platformObject.AddMember("name", rapidjson::StringRef("NVR"), allocator);
			platformObject.AddMember("online", 1, allocator);
			platformObject.AddMember("port", platform.port, allocator);

			// 添加 "time"
			platformObject.AddMember("time", rapidjson::StringRef(getCurrentTime().c_str()), allocator);

			dataArray.PushBack(platformObject, allocator);
		}
	}

	document.AddMember("data", dataArray, allocator);
	document.AddMember("msg", "SUCCESS", allocator);
	document.AddMember("msgId", msgId, allocator);
	document.AddMember("msgType", "DEVICE_TOTAL_SYNC", allocator);
	document.AddMember("serialNum", strSerialNum, allocator);

	// 生成 JSON 字符串
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	document.Accept(writer);

	return buffer.GetString();
}

std::string gateway_msg::channel_sync(std::vector<Device> device_list, Platform& platform)
{
	rapidjson::Document document;
	document.SetObject();

	rapidjson::Document::AllocatorType& allocator = document.GetAllocator();

	// 添加顶层字段
	document.AddMember("serialNum", strSerialNum, allocator);
	document.AddMember("msgType", "CHANNEL_SYNC", allocator);
	document.AddMember("msgId", platform.msgId, allocator);

	// 添加时间
	std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	std::tm timeInfo = *std::localtime(&now);
	std::stringstream ss;
	ss << std::put_time(&timeInfo, "%Y-%m-%d %H:%M:%S");
	std::string formatted_time = ss.str();
	document.AddMember("time", formatted_time, allocator);

	//document.AddMember("time", "2023-08-30T15:45:36.525594700", allocator);
	document.AddMember("code", 0, allocator);
	document.AddMember("msg", "SUCCESS", allocator);

	// 添加嵌套的 data 字段
	rapidjson::Value data(rapidjson::kObjectType);
	data.AddMember("total", device_list.size(), allocator);

	// 添加嵌套的 channelDetailList 数组
	rapidjson::Value channelDetailList(rapidjson::kArrayType);

	for (int i = 0; i < device_list.size(); ++i)
	{
		std::string temp_device_id = platform.deviceId;
		temp_device_id += "#";
		rapidjson::Value channelEntity(rapidjson::kObjectType);
		channelEntity.AddMember("id", temp_device_id += std::to_string(device_list[i].id), allocator);
		channelEntity.AddMember("channelNum", device_list[i].id, allocator);
		channelEntity.AddMember("channelType", 1, allocator);
		channelEntity.AddMember("encodeId", platform.deviceId, allocator);
		channelEntity.AddMember("channelName", boost::locale::conv::to_utf<char>(device_list[i].name, "GBK"), allocator);  // 请添加实际值
		channelEntity.AddMember("manufacturer", "DH", allocator);

		channelEntity.AddMember("ip", device_list[i].ip, allocator);
		channelEntity.AddMember("port", device_list[i].port, allocator);
		channelEntity.AddMember("password", "rj123456", allocator);
		channelEntity.AddMember("ptzType", 0, allocator);
		channelEntity.AddMember("isIpChannel", 1, allocator);

		channelEntity.AddMember("online", device_list[i].online, allocator);
		channelEntity.AddMember("deleted", device_list[i].deleted, allocator);
		//channelEntity.AddMember("createdAt", nullptr, allocator);  // 请添加实际值
		//channelEntity.AddMember("updatedAt", nullptr, allocator);  // 请添加实际值

		// 添加其他字段...

		channelDetailList.PushBack(channelEntity, allocator);
	}

	data.AddMember("channelDetailList", channelDetailList, allocator);
	data.AddMember("num", device_list.size(), allocator);

	// 将 data 添加到 document
	document.AddMember("data", data, allocator);

	// 生成 JSON 字符串
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	document.Accept(writer);

	return buffer.GetString();
}

std::string gateway_msg::response(const std::string& msgId, int code, const std::string& msgType)
{
	// 创建 RapidJSON 文档
	rapidjson::Document document;
	document.SetObject();

	// 添加字段
	rapidjson::Value serialNum(strSerialNum, document.GetAllocator());
	document.AddMember("serialNum", serialNum, document.GetAllocator());

	rapidjson::Value msgTypeValue(msgType.c_str(), document.GetAllocator());
	document.AddMember("msgType", msgTypeValue, document.GetAllocator());

	rapidjson::Value msgIdValue(msgId.c_str(), document.GetAllocator());
	document.AddMember("msgId", msgIdValue, document.GetAllocator());

	// 添加时间
	std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	std::tm timeInfo = *std::localtime(&now);
	std::stringstream ss;
	ss << std::put_time(&timeInfo, "%Y-%m-%d %H:%M:%S");
	std::string formatted_time = ss.str();
	rapidjson::Value time(formatted_time, document.GetAllocator());
	document.AddMember("time", time, document.GetAllocator());

	rapidjson::Value codeValue(code);
	document.AddMember("code", codeValue, document.GetAllocator());

	rapidjson::Value msg("SUCCESS", document.GetAllocator());
	document.AddMember("msg", msg, document.GetAllocator());

	rapidjson::Value data(rapidjson::kNullType);
	document.AddMember("data", data, document.GetAllocator());

	// 使用 StringBuffer 创建 Writer
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

	// 将 JSON 写入缓冲区
	document.Accept(writer);

	return buffer.GetString();
}

bool gateway_msg::pars_ptz_control(std::string json, PtzCMD& ptz)
{
	// 解析JSON
	rapidjson::Document document;
	document.Parse(json.c_str());

	// 检查解析是否成功
	if (document.HasParseError()) {
		std::cerr << "Error parsing JSON." << std::endl;
		return 1;
	}

	// 获取各个字段的值
	int code = document["code"].GetInt();

	std::vector<std::string> tokens;
	// 使用 '#' 作为分隔符拆分字符串
	boost::split(tokens, document["data"]["channelId"].GetString(), boost::is_any_of("#"));
	// 检查是否找到至少两个标记
	if (tokens.size() >= 2)
		ptz.channel = atoi(tokens[1].c_str());

	//ptz.channel = atoi(document["data"]["channelId"].GetString());
	ptz.contrl_cmd = document["data"]["dataMap"]["cmdCode"].GetInt();
	ptz.contrl_param = document["data"]["dataMap"]["cmdValue"].GetInt();
	ptz.device_id = document["data"]["deviceId"].GetString();
	ptz.msgId = document["msgId"].GetString();
	ptz.msgType = document["msgType"].GetString();
	//std::string serialNum = document["serialNum"].GetString();
	//std::string time = document["time"].GetString();
	return false;
}

bool gateway_msg::pars_ptz_preset(std::string json, Preset& p)
{
	rapidjson::Document doc;
	doc.Parse(json);

	if (doc.HasParseError())
	{
		std::cerr << "Error parsing JSON: " << rapidjson::GetParseError_En(doc.GetParseError()) << '\n';
		return false;
	}

	p.msgId = doc["msgId"].GetString();
	p.msgType = doc["msgType"].GetString();
	p.device_id = doc["data"]["deviceId"].GetString();

	std::vector<std::string> tokens;
	// 使用 '#' 作为分隔符拆分字符串
	boost::split(tokens, doc["data"]["channelId"].GetString(), boost::is_any_of("#"));
	// 检查是否找到至少两个标记
	if (tokens.size() >= 2)
	{
		p.channel = atoi(tokens[1].c_str());
		return true;
	}

	return false;
}

std::string gateway_msg::ptz_preset_list(Preset p, std::vector<Preset> presets)
{
	// 初始化 RapidJSON 文档
	rapidjson::Document document;
	document.SetObject();

	// 创建一个 JSON 数组
	rapidjson::Value dataArray(rapidjson::kArrayType);

	// 遍历 Preset 结构体数组
	for (const auto& preset : presets)
	{
		// 创建一个 JSON 对象
		rapidjson::Value presetData(rapidjson::kObjectType);

		// 添加属性
		//presetData.AddMember("msgType", rapidjson::Value(preset.msgType.c_str(), document.GetAllocator()), document.GetAllocator());
		//presetData.AddMember("msgId", rapidjson::Value(preset.msgId.c_str(), document.GetAllocator()), document.GetAllocator());
		//presetData.AddMember("device_id", rapidjson::Value(preset.device_id.c_str(), document.GetAllocator()), document.GetAllocator());
		//presetData.AddMember("channel", preset.channel, document.GetAllocator());
		presetData.AddMember("presetId", preset.index, document.GetAllocator());
		presetData.AddMember("presetName", rapidjson::Value(preset.name.c_str(), document.GetAllocator()), document.GetAllocator());

		// 将 JSON 对象添加到数组
		dataArray.PushBack(presetData, document.GetAllocator());
	}

	// 创建最终的 JSON 对象
	rapidjson::Value finalJson(rapidjson::kObjectType);
	finalJson.AddMember("serialNum", strSerialNum, document.GetAllocator());
	finalJson.AddMember("msgType", "CHANNEL_PTZ_PRESET", document.GetAllocator());
	finalJson.AddMember("msgId", p.msgId, document.GetAllocator());

	// 添加时间
	std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	std::tm timeInfo = *std::localtime(&now);
	std::stringstream ss;
	ss << std::put_time(&timeInfo, "%Y-%m-%d %H:%M:%S");
	std::string formatted_time = ss.str();
	rapidjson::Value time(formatted_time, document.GetAllocator());
	finalJson.AddMember("time", time, document.GetAllocator());
	//finalJson.AddMember("time", "2023-09-13T11:18:44.268041", document.GetAllocator());
	finalJson.AddMember("code", 0, document.GetAllocator());
	finalJson.AddMember("msg", "SUCCESS", document.GetAllocator());
	finalJson.AddMember("data", dataArray, document.GetAllocator());

	// 使用 RapidJSON 的 Writer 将 JSON 输出到字符串缓冲区
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	finalJson.Accept(writer);

	// 返回 JSON 字符串
	return buffer.GetString();
}

bool gateway_msg::pars_channel_play(std::string json, Play& p)
{
	rapidjson::Document doc;
	doc.Parse(json);

	if (doc.HasParseError())
	{
		std::cerr << "Error parsing JSON: " << rapidjson::GetParseError_En(doc.GetParseError()) << '\n';
		return false;
	}

	p.msgType = doc["msgType"].GetString();
	p.msgId = doc["msgId"].GetString();

	const auto& data = doc["data"];
	if (data.IsObject())
	{
		p.streamMode = data["streamMode"].GetInt();
		p.device_id = data["deviceId"].GetString();

		std::vector<std::string> tokens;
		// 使用 '#' 作为分隔符拆分字符串
		boost::split(tokens, data["channelId"].GetString(), boost::is_any_of("#"));
		// 检查是否找到至少两个标记
		if (tokens.size() >= 2)
			p.channel = atoi(tokens[1].c_str());

		p.dispatchUrl = data["dispatchUrl"].GetString();
		p.streamId = data["streamId"].GetString();

		const auto& ssrcInfo = data["ssrcInfo"];
		if (ssrcInfo.IsObject())
		{
			p.ip = ssrcInfo["ip"].GetString();
			p.port = ssrcInfo["port"].GetInt();
			p.ssrc = ssrcInfo["ssrc"].GetString();
			p.sdpIp = ssrcInfo["sdpIp"].GetString();
			p.streamId = ssrcInfo["streamId"].GetString();
			p.mediaServerId = ssrcInfo["mediaServerId"].GetString();
		}
	}
	return true;
}

bool gateway_msg::pars_channel_record_info(std::string json, Record& record)
{
	const char* jsonStr = R"({"code":0,"data":{"channelId":"1700733608331580#0","dataMap":{"startTime":"2023-11-21 00:00:00","endTime":"2023-11-22 23:59:00"},"deviceId":"1700733608331580"},"error":false,"msgId":"71709","msgType":"CHANNEL_RECORD_INFO","serialNum":"29fc51254c0c4809a9f4851f994111a2","time":"2023-11-23 18:14:22"})";

	rapidjson::Document document;
	document.Parse(json);

	if (!document.IsObject())
	{
		std::cerr << "Invalid JSON format." << std::endl;
		return false;
	}

	// Accessing values
	int code = document["code"].GetInt();

	std::vector<std::string> tokens;
	// 使用 '#' 作为分隔符拆分字符串
	boost::split(tokens, document["data"]["channelId"].GetString(), boost::is_any_of("#"));
	// 检查是否找到至少两个标记
	if (tokens.size() >= 2)
		record.channel = atoi(tokens[1].c_str());
	record.device_id = document["data"]["deviceId"].GetString();

	record.startTime = document["data"]["dataMap"]["startTime"].GetString();
	record.endTime = document["data"]["dataMap"]["endTime"].GetString();
	bool error = document["error"].GetBool();
	record.msgId = document["msgId"].GetString();
	record.msgType = document["msgType"].GetString();
	std::string serialNum = document["serialNum"].GetString();
	std::string time = document["time"].GetString();

	return true;
}

std::string gateway_msg::response_record_list(const std::string& msgId, int code, const std::string& msgType, std::vector<Record> list)
{
	rapidjson::Document document;
	document.SetObject();

	rapidjson::Document::AllocatorType& allocator = document.GetAllocator();

	// Adding primitive values
	document.AddMember("serialNum", rapidjson::Value(strSerialNum.c_str(), allocator).Move(), allocator);
	document.AddMember("msgType", rapidjson::Value(msgType.c_str(), allocator).Move(), allocator);
	document.AddMember("msgId", rapidjson::Value(msgId.c_str(), allocator).Move(), allocator);

	// 添加时间
	std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	std::tm timeInfo = *std::localtime(&now);
	std::stringstream ss;
	ss << std::put_time(&timeInfo, "%Y-%m-%d %H:%M:%S");
	std::string formatted_time = ss.str();
	rapidjson::Value time(formatted_time, document.GetAllocator());

	document.AddMember("time", rapidjson::Value(formatted_time, allocator).Move(), allocator);
	document.AddMember("code", code, allocator);
	document.AddMember("msg", rapidjson::Value("SUCCESS", allocator).Move(), allocator);

	// Adding data field with the new format
	rapidjson::Value data(rapidjson::kObjectType);
	data.AddMember("sumNum", static_cast<int>(list.size()), allocator);

	rapidjson::Value recordList(rapidjson::kArrayType);
	for (const auto& record : list)
	{
		rapidjson::Value recordItem(rapidjson::kObjectType);
		recordItem.AddMember("name", rapidjson::Value("", allocator).Move(), allocator);
		recordItem.AddMember("filePath", rapidjson::Value("", allocator).Move(), allocator);
		recordItem.AddMember("fileSize", rapidjson::Value("", allocator).Move(), allocator);
		recordItem.AddMember("address", rapidjson::Value("", allocator).Move(), allocator);

		{
			// 将字符串解析为时间结构
			std::tm timeStruct = {};
			std::istringstream ss(record.startTime);
			ss >> std::get_time(&timeStruct, "%Y%m%d%H%M%S");

			// 将时间结构重新格式化为字符串
			std::ostringstream formattedSS;
			formattedSS << std::put_time(&timeStruct, "%Y-%m-%d %H:%M:%S");

			std::string outputDateTime = formattedSS.str();
			recordItem.AddMember("startTime", rapidjson::Value(outputDateTime.c_str(), allocator).Move(), allocator);
		}

		{
			// 将字符串解析为时间结构
			std::tm timeStruct = {};
			std::istringstream ss(record.endTime);
			ss >> std::get_time(&timeStruct, "%Y%m%d%H%M%S");

			// 将时间结构重新格式化为字符串
			std::ostringstream formattedSS;
			formattedSS << std::put_time(&timeStruct, "%Y-%m-%d %H:%M:%S");

			std::string outputDateTime = formattedSS.str();
			recordItem.AddMember("endTime", rapidjson::Value(outputDateTime.c_str(), allocator).Move(), allocator);
		}

		recordList.PushBack(recordItem, allocator);
	}

	data.AddMember("recordList", recordList, allocator);
	document.AddMember("data", data, allocator);

	// Creating a StringBuffer
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	document.Accept(writer);

	return buffer.GetString();
}

bool gateway_msg::pars_channel_playback(std::string json, Record& record)
{
	const char* jsonStr = R"({
	"serialNum": "eb48104ddf2b4760be123783b3621051",
	"msgType" : "CHANNEL_PLAYBACK",
	"msgId" : "239747",
	"time" : "2023-11-27 09:36:37",
	"code" : 0,
	"msg" : "SUCCESS",
	"data" : {
		"streamMode": 2,
		"deviceId" : "1700815361205907",
		"channelId" : "1700815361205907#0",
		"dispatchUrl" : "http://192.168.16.9:18081/api/media/streamNotify",
		"streamId" : "RECORD_427_170104899715952",
		"ssrcInfo" : {
			"port": 22100,
			"ssrc" : "1102006587",
			"sdpIp" : "124.71.16.209",
			"streamId" : "RECORD_427_170104899715952",
			"mediaServerId" : "LORVY158beilorvy",
			"ip" : "192.168.0.154"
		},
		"msgId": null,
		"startTime" : "2023-11-26 00:00:00",
		"endTime" : "2023-11-26 23:59:59"
	}
		})";

	rapidjson::Document doc;
	doc.Parse(json.c_str());

	if (doc.HasParseError()) {
		std::cerr << "Error parsing JSON: " << rapidjson::GetParseError_En(doc.GetParseError()) << std::endl;
		return false;
	}

	record.msgType = doc["msgType"].GetString();
	record.msgId = doc["msgId"].GetString();
	record.device_id = doc["data"]["deviceId"].GetString();

	std::vector<std::string> tokens;
	// 使用 '#' 作为分隔符拆分字符串
	boost::split(tokens, doc["data"]["channelId"].GetString(), boost::is_any_of("#"));
	// 检查是否找到至少两个标记
	if (tokens.size() >= 2)
		record.channel = atoi(tokens[1].c_str());

	record.startTime = doc["data"]["startTime"].GetString();
	record.endTime = doc["data"]["endTime"].GetString();

	record.streamMode = doc["data"]["streamMode"].GetInt();
	record.dispatchUrl = doc["data"]["dispatchUrl"].GetString();
	record.streamId = doc["data"]["streamId"].GetString();
	record.ip = doc["data"]["ssrcInfo"]["ip"].GetString();
	record.port = doc["data"]["ssrcInfo"]["port"].GetInt();
	record.ssrc = doc["data"]["ssrcInfo"]["ssrc"].GetString();
	record.sdpIp = doc["data"]["ssrcInfo"]["sdpIp"].GetString();
	record.ssrc_streamId = doc["data"]["ssrcInfo"]["streamId"].GetString();
	record.mediaServerId = doc["data"]["ssrcInfo"]["mediaServerId"].GetString();

	return true;
}

bool gateway_msg::pars_channel_stop_play(std::string json, std::string& key)
{
	rapidjson::Document doc;
	doc.Parse(json);

	Record record;
	if (!doc.HasParseError() && doc.IsObject())
	{
		// 解析嵌套的 data 字段
		if (doc.HasMember("data") && doc["data"].IsObject())
		{
			auto& data = doc["data"];
			if (data.HasMember("streamId") && data["streamId"].IsString())
			{
				key = data["streamId"].GetString();
			}
		}
	}

	return true;
}

bool gateway_msg::pars_streamId(std::string json, std::string& streamId)
{
	// 使用 RapidJSON 解析 JSON
	rapidjson::Document document;
	document.Parse(json.c_str());

	// 检查解析是否成功
	if (document.HasParseError())
	{
		std::cerr << "JSON parsing error: " << GetParseError_En(document.GetParseError()) << std::endl;
		return false;
	}

	// 检查是否包含 "data" 字段
	if (document.HasMember("data") && document["data"].IsObject())
	{
		// 获取 "data" 对象
		const rapidjson::Value& data = document["data"];

		// 检查是否包含 "streamId" 字段
		if (data.HasMember("streamId") && data["streamId"].IsString())
		{
			// 提取 "streamId"
			streamId = data["streamId"].GetString();
			return true;
		}
	}

	return false;
}

bool gateway_msg::pars_record_speed(std::string json, float& speed, std::string& streamId)
{
	// 使用 RapidJSON 解析 JSON
	rapidjson::Document document;
	document.Parse(json.c_str());

	// 检查解析是否成功
	if (document.HasParseError())
	{
		std::cerr << "JSON parsing error: " << GetParseError_En(document.GetParseError()) << std::endl;
		return false;
	}

	// 检查是否包含 "data" 字段
	if (document.HasMember("data") && document["data"].IsObject())
	{
		// 获取 "data" 对象
		const rapidjson::Value& data = document["data"];

		// 检查是否包含 "dataMap" 字段
		if (data.HasMember("dataMap") && data["dataMap"].IsObject())
		{
			// 获取 "dataMap" 对象
			const rapidjson::Value& dataMap = data["dataMap"];

			// 检查是否包含 "speed" 字段
			if (dataMap.HasMember("speed") && dataMap["speed"].IsFloat())
			{
				// 提取 "speed"
				speed = dataMap["speed"].GetDouble();
			}
		}

		// 检查是否包含 "streamId" 字段
		if (data.HasMember("streamId") && data["streamId"].IsString())
		{
			// 提取 "streamId"
			streamId = data["streamId"].GetString();
			return true;
		}
	}

	// JSON 结构不符合预期，或者缺少必要的字段
	return false;
}

bool gateway_msg::pars_requst(std::string json, std::string& msgId, std::string& msgType, std::string& deviceId, std::string& channelId)
{
	rapidjson::Document doc;
	doc.Parse(json);

	if (doc.HasParseError())
	{
		std::cerr << "Error parsing JSON: " << rapidjson::GetParseError_En(doc.GetParseError()) << '\n';
		return false;
	}

	doc["code"].GetInt();
	doc["msgId"].GetString();
	doc["msgType"].GetString();
	return true;
}

bool gateway_msg::isMySerialNum(std::string json)
{
	//return true;//业务队列不需要过滤

	rapidjson::Document doc;
	doc.Parse(json.c_str());

	if (!doc.IsObject())
	{
		return false;
	}

	if (doc.HasMember("serialNum") && doc["serialNum"].IsString())
	{
		if (strSerialNum == doc["serialNum"].GetString())
			return true;
	}

	return false;
}

int gateway_msg::get_msg_type(std::string json)
{
	rapidjson::Document doc;
	doc.Parse(json.c_str());

	if (!doc.IsObject())
	{
		return false;
	}

	if (doc.HasMember("msgType") && doc["msgType"].IsString())
	{
		const static boost::unordered_map<std::string, int> msg_type_map{
			{"GATEWAY_SIGN_IN",msg_GATEWAY_SIGN_IN},{"DEVICE_ADD",msg_DEVICE_ADD},{"CHANNEL_SYNC",msg_CHANNEL_SYNC},{"DEVICE_TOTAL_SYNC",msg_DEVICE_TOTAL_SYNC},{"DEVICE_DELETE_SOFT",msg_DEVICE_DELETE_SOFT},
			{"DEVICE_DELETE",msg_DEVICE_DELETE},{"DEVICE_DELETE_RECOVER",msg_DEVICE_DELETE_RECOVER},{"CHANNEL_DELETE_SOFT",msg_CHANNEL_DELETE_SOFT},
			{"CHANNEL_DELETE_RECOVER",msg_CHANNEL_DELETE_RECOVER},{"CHANNEL_PTZ_CONTROL",msg_CHANNEL_PTZ_CONTROL},{"CHANNEL_PTZ_PRESET",msg_CHANNEL_PTZ_PRESET},
			{"CHANNEL_3D_OPERATION",msg_CHANNEL_3D_OPERATION},{"CHANNEL_PLAY",msg_CHANNEL_PLAY},{"CHANNEL_RECORD_INFO",msg_CHANNEL_RECORD_INFO},
			{"CHANNEL_PLAYBACK",msg_CHANNEL_PLAYBACK},{"CHANNEL_STOP_PLAY",msg_CHANNEL_STOP_PLAY},{"DEVICE_RECORD_PAUSE",msg_DEVICE_RECORD_PAUSE},
			{"DEVICE_RECORD_RESUME",msg_DEVICE_RECORD_RESUME},{"DEVICE_RECORD_SPEED",msg_DEVICE_RECORD_SPEED}
		};
		auto it = msg_type_map.find(doc["msgType"].GetString());
		if (it != msg_type_map.end())
			return it->second;
	}
	return false;
}

std::string gateway_msg::device_add(int step, Platform platform)
{
	rapidjson::Document doc;

	if (step == 1)
	{
		// 创建 RapidJSON 文档
		doc.SetObject();

		// 添加时间
		std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		std::tm timeInfo = *std::localtime(&now);
		std::stringstream ss;
		//ss << std::put_time(&timeInfo, "%Y-%m-%dT%H:%M:%S");
		ss << std::put_time(&timeInfo, "%Y-%m-%d %H:%M:%S");
		std::string formatted_time = ss.str();
		rapidjson::Value time;
		time.SetString(formatted_time.c_str(), doc.GetAllocator());
		doc.AddMember("time", time, doc.GetAllocator());

		// 添加其他字段
		int msg_id = 49536;
		doc.AddMember("serialNum", strSerialNum, doc.GetAllocator());
		doc.AddMember("msgType", "DEVICE_ADD", doc.GetAllocator());
		doc.AddMember("msgId", platform.msgId, doc.GetAllocator());
		doc.AddMember("code", platform.login_results, doc.GetAllocator());
		doc.AddMember("msg", platform.login_err_msg, doc.GetAllocator());

		// 添加 data 字段
		//long long llid = boost::lexical_cast<long long>(platform.deviceId.c_str());
		//long long llid = atoll(platform.deviceId.c_str());
		doc.AddMember("data", platform.deviceId, doc.GetAllocator());

		// 将文档转换为 JSON 字符串
		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
		doc.Accept(writer);
	}
	else
	{
		/*
		{"serialNum":"29fc51254c0c4809a9f4851f994111a2","msgType":"DEVICE_SIGN_IN","msgId":"b_2","time":"2023-08-30T17:22:05.588553100","code":0,"msg":"SUCCESS","data":{"id":55555,"lUserId":4,"username":"admin","serialNumber":"DS-7808N-R2(C)0820230227CCRRL33683189WCVU","name":"test","charset":"GB2312","ip":"172.20.0.242","port":37777,"manufacturer":"dh","online":1,"deviceType":3,"deleted":0,"password":"rj123456","createdAt":"2023-11-16T20:10:55","updatedAt":null}}
		*/
		doc.SetObject();

		// 添加键值对到 JSON 对象
		rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

		doc.AddMember("serialNum", strSerialNum, allocator);
		doc.AddMember("msgType", "DEVICE_SIGN_IN", allocator);
		msg_id++;
		doc.AddMember("msgId", msg_id_b += std::to_string(msg_id), allocator);
		msg_id_b = "b_";

		// 添加时间
		std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		std::tm timeInfo = *std::localtime(&now);
		std::stringstream ss;
		//ss << std::put_time(&timeInfo, "%Y-%m-%dT%H:%M:%S");
		ss << std::put_time(&timeInfo, "%Y-%m-%dT%H:%M:%S");
		std::string formatted_time = ss.str();
		rapidjson::Value time;
		doc.AddMember("time", time, allocator);
		doc.AddMember("code", 0, allocator);

		// 创建嵌套的 JSON 对象
		rapidjson::Value dataObj(rapidjson::kObjectType);
		dataObj.AddMember("id", platform.deviceId, allocator);
		dataObj.AddMember("lUserId", 4, allocator);
		dataObj.AddMember("username", platform.username, allocator);
		dataObj.AddMember("serialNumber", "DS-7808N-R2(C)0820230227CCRRL33683189WCVU", allocator);
		dataObj.AddMember("name", "test", allocator);
		dataObj.AddMember("charset", "GB2312", allocator);
		dataObj.AddMember("ip", platform.ip, allocator);
		dataObj.AddMember("port", platform.port, allocator);
		dataObj.AddMember("manufacturer", "dh", allocator);
		dataObj.AddMember("online", 1, allocator);
		dataObj.AddMember("deviceType", 3, allocator);
		dataObj.AddMember("deleted", 0, allocator);
		dataObj.AddMember("password", platform.password, allocator);
		//dataObj.AddMember("createdAt", "2023-08-30T17:22:05", allocator);
		//dataObj.AddMember("updatedAt", "2023-08-30T17:22:05", allocator);

		time.SetString(formatted_time.c_str(), doc.GetAllocator());
		dataObj.AddMember("createdAt", time, allocator);
		dataObj.AddMember("updatedAt", time, allocator);

		// 将嵌套的 JSON 对象添加到主 JSON 对象
		doc.AddMember("msg", "SUCCESS", allocator);
		doc.AddMember("data", dataObj, allocator);

		// 将 JSON 对象转换为字符串
		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
		doc.Accept(writer);
	}

	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	doc.Accept(writer);
	return buffer.GetString();
}

bool gateway_msg::pars_device_add(std::string json, Platform& p)
{/*
 {"code":0,"data":{"dataMap":{"deviceType":2,"password":"rj123456","port":"8
000","ip":"172.20.0.245","deviceId":"34020000001110000245","username":"admin"}},
"error":false,"msgId":"49537","msgType":"DEVICE_ADD","serialNum":"28fc51254c0c4
809a9f4851f994111a1","time":"2023-08-3017:22:27"}
 */
	rapidjson::Document doc;
	doc.Parse(json);

	if (!doc.IsObject()) {
		std::cerr << "Failed to parse JSON." << std::endl;
		return false;
	}

	// 解析 code 字段
	if (doc.HasMember("code") && doc["code"].IsInt()) {
		int code = doc["code"].GetInt();
		//std::cout << "Code: " << code << std::endl;
	}

	// 解析 data 字段
	if (doc.HasMember("data") && doc["data"].IsObject()) {
		const rapidjson::Value& data = doc["data"];

		// 解析 dataMap 字段
		if (data.HasMember("dataMap") && data["dataMap"].IsObject()) {
			const rapidjson::Value& dataMap = data["dataMap"];

			// 解析 deviceType 字段
			if (dataMap.HasMember("deviceType") && dataMap["deviceType"].IsInt()) {
				p.deviceType = dataMap["deviceType"].GetInt();
				//std::cout << "Device Type: " << deviceType << std::endl;
			}
			if (dataMap.HasMember("deviceId") && dataMap["deviceId"].IsString()) {
				//p.deviceId = dataMap["deviceId"].GetString();
				//std::cout << "Device Type: " << deviceType << std::endl;
			}
			if (dataMap.HasMember("ip") && dataMap["ip"].IsString()) {
				p.ip = dataMap["ip"].GetString();
				//std::cout << "Device Type: " << deviceType << std::endl;
			}
			if (dataMap.HasMember("port") && dataMap["port"].IsString()) {
				p.port = atoi(dataMap["port"].GetString());
				//std::cout << "Device Type: " << deviceType << std::endl;
			}
			if (dataMap.HasMember("username") && dataMap["username"].IsString()) {
				p.username = dataMap["username"].GetString();
				//std::cout << "Device Type: " << deviceType << std::endl;
			}
			if (dataMap.HasMember("password") && dataMap["password"].IsString()) {
				p.password = dataMap["password"].GetString();
				//std::cout << "Device Type: " << deviceType << std::endl;
			}
		}
	}

	// 解析其他字段...

	// 解析 time 字段
	if (doc.HasMember("time") && doc["time"].IsString()) {
		std::string time = doc["time"].GetString();
		//std::cout << "Time: " << time << std::endl;
	}

	// 解析 serialNum 字段
	if (doc.HasMember("serialNum") && doc["serialNum"].IsString()) {
		std::string serialNum = doc["serialNum"].GetString();
		//std::cout << "Serial Number: " << serialNum << std::endl;
	}

	// 解析 serialNum 字段
	if (doc.HasMember("msgId") && doc["msgId"].IsString()) {
		p.msgId = doc["msgId"].GetString();
		//std::cout << "Serial Number: " << serialNum << std::endl;
	}

	return true;
}

std::string gateway_msg::device_sync(std::vector<Device> device_list, Platform& platform)
{
	// 创建一个 JSON 对象
	rapidjson::Document document(rapidjson::kObjectType);
	rapidjson::Document::AllocatorType& allocator = document.GetAllocator(); // 只保留一次定义

	document.AddMember("serialNum", strSerialNum, allocator);
	document.AddMember("msgType", "CHANNEL_SYNC", allocator);
	document.AddMember("msgId", platform.msgId, allocator);

	// 添加时间
	std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	std::tm timeInfo = *std::localtime(&now);
	std::stringstream ss;
	ss << std::put_time(&timeInfo, "%Y-%m-%d %H:%M:%S");
	std::string formatted_time = ss.str();
	document.AddMember("time", formatted_time, allocator);

	document.AddMember("code", 0, allocator);
	document.AddMember("msg", "SUCCESS", allocator);

	// 创建 data 对象
	rapidjson::Value data(rapidjson::kObjectType);
	data.AddMember("total", device_list.size(), allocator);

	// 添加数据数组
	rapidjson::Value dataArray(rapidjson::kArrayType);

	for (int i = 0; i < device_list.size(); ++i)
	{
		rapidjson::Value deviceInfo(rapidjson::kObjectType);
		deviceInfo.AddMember("deviceId", device_list[i].deviceId, allocator);
		deviceInfo.AddMember("name", device_list[i].name, allocator);
		deviceInfo.AddMember("manufacturer", device_list[i].manufacturer, allocator);
		deviceInfo.AddMember("model", device_list[i].model, allocator);
		deviceInfo.AddMember("firmware", device_list[i].firmware, allocator);
		deviceInfo.AddMember("ip", device_list[i].ip, allocator);
		deviceInfo.AddMember("port", device_list[i].port, allocator);
		deviceInfo.AddMember("online", device_list[i].online, allocator);
		// 添加其他字段...

		dataArray.PushBack(deviceInfo, allocator);
	}

	document.AddMember("data", dataArray, allocator);

	// 将 JSON 对象转换为字符串
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	document.Accept(writer);

	return  buffer.GetString();
}

bool gateway_msg::pars_gateway_sign(std::string json, std::string& msgType, std::string& mqGetQueue, std::string& mqSetQueue, std::string& mqExchange)
{
	rapidjson::Document document;
	document.Parse(json.c_str());

	if (!document.IsObject()) {
		std::cerr << "JSON parse error" << std::endl;
		return false;
	}

	if (document.HasMember("code") && document["code"].IsInt()) {
		int code = document["code"].GetInt();
	}

	if (document.HasMember("data") && document["data"].IsObject()) {
		const rapidjson::Value& data = document["data"];
		if (data.HasMember("isFirstSignIn") && data["isFirstSignIn"].IsBool()) {
			bool isFirstSignIn = data["isFirstSignIn"].GetBool();
		}
		if (data.HasMember("mqExchange") && data["mqExchange"].IsString()) {
			mqExchange = data["mqExchange"].GetString();
		}
		if (data.HasMember("mqGetQueue") && data["mqGetQueue"].IsString()) {
			mqGetQueue = data["mqGetQueue"].GetString();
			//mqSetQueue = data["mqGetQueue"].GetString();
		}
		if (data.HasMember("mqSetQueue") && data["mqSetQueue"].IsString()) {
			mqSetQueue = data["mqSetQueue"].GetString();
			//mqGetQueue = data["mqSetQueue"].GetString();
		}
		if (data.HasMember("signType") && data["signType"].IsString()) {
			std::string signType = data["signType"].GetString();
		}
	}

	if (document.HasMember("error") && document["error"].IsBool()) {
		bool error = document["error"].GetBool();
	}

	if (document.HasMember("msg") && document["msg"].IsString()) {
		std::string msg = document["msg"].GetString();
	}

	if (document.HasMember("msgId") && document["msgId"].IsString()) {
		std::string msgId = document["msgId"].GetString();
	}

	if (document.HasMember("msgType") && document["msgType"].IsString()) {
		msgType = document["msgType"].GetString();
	}

	if (document.HasMember("serialNum") && document["serialNum"].IsString()) {
		std::string serialNum = document["serialNum"].GetString();
	}

	if (document.HasMember("time") && document["time"].IsString()) {
		std::string time = document["time"].GetString();
	}

	return true;
}