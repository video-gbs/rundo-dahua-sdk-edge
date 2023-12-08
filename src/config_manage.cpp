#include <iostream>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include "config_manage.h"
#include "log.h"

bool config_manager::add_loacl_media_config(char* path)
{
	{
		std::string strConfigFile = path;
		std::string::size_type pos = strConfigFile.find(".exe");
		if (std::string::npos != pos)
		{
			strConfigFile = strConfigFile.substr(0, pos);
		}
		strConfigFile += ".ini";
		//strConfigFile += ".xml";

		strcpy(_apppatch, strConfigFile.c_str());

		LOG_CONSOLE("��ʼ���ر��������ļ� %s", _apppatch);
		if (!ParseConfigFile_ini(strConfigFile))
			//if (!ParseConfigFile(strConfigFile))
		{
			LOG_ERROR("���ر��������ļ�ʧ�� %s", _apppatch);
		}
	}

	return false;
}

bool config_manager::add_media_config(Config& config)
{
	std::lock_guard<std::mutex>lg(_media_config_map_mtx);

	sprintf(key_, "%s", config.mediaServerId.c_str());
	auto it = _media_config_map.find(key_);
	if (it != _media_config_map.end())
	{
		LOG_INFO("�����ļ��Ѵ���,����Ҫ�ظ�����");
		return true;
	}
	else
	{
		LOG_CONSOLE("���������ļ����");
		return _media_config_map.insert(std::make_pair(key_, config)).second;
	}

}

Config config_manager::get_media_config1()
{
	Config config;
	std::lock_guard<std::mutex>lg(_media_config_map_mtx);
	auto it = _media_config_map.find(key_);
	if (it != _media_config_map.end())
	{
		config = it->second;
	}
	return config;
}

bool config_manager::get_media_config(Config& config)
{
	std::lock_guard<std::mutex>lg(_media_config_map_mtx);
	auto it = _media_config_map.find(key_);
	if (it != _media_config_map.end())
	{
		config = it->second;
		return true;
	}
	return false;
}


bool config_manager::ParseConfigFile_ini(const std::string& strFilePath)
{
	try
	{
		Config scheduler;
		boost::property_tree::ptree pt;
		boost::property_tree::ini_parser::read_ini(strFilePath, pt);

		// ��ptree�ж�ȡÿ���ֶε�ֵ�������ṹ���Ӧ�ĳ�Ա����
		scheduler.mediaServerId = pt.get<std::string>("mediaServerId", scheduler.mediaServerId);
		scheduler.httpIp = pt.get<std::string>("httpIp", scheduler.httpIp);
		scheduler.httpPort = pt.get<unsigned int>("httpPort", scheduler.httpPort);

		scheduler.MQIp = pt.get<std::string>("MQIp", scheduler.MQIp);
		scheduler.MQPort = pt.get<unsigned int>("MQPort", scheduler.MQPort);
		scheduler.MQUser = pt.get<std::string>("MQUser", scheduler.MQUser);
		scheduler.MQPsw = pt.get<std::string>("MQPsw", scheduler.MQPsw);

		scheduler.heartbeatTimeout = pt.get<unsigned int>("heartbeatTimeout", scheduler.heartbeatTimeout);
		scheduler.MQListenTimeout = pt.get<unsigned int>("MQListenTimeout", scheduler.MQListenTimeout);

		return add_media_config(scheduler);
	}
	catch (const std::exception& ex)
	{
		LOG_ERROR("Failed to read INI file:%s ", ex.what());
		return false;
	}
	return false;
}
