#pragma once
#include <string>
#include <unordered_map>
#include <mutex>
#include <boost/serialization/singleton.hpp>
#include "common.h"

class config_manager
{

public:
	config_manager() {};
	~config_manager() {};

	//读取本地配置文件到缓存
	bool add_loacl_media_config(char* path);

	Config get_media_config1();
	bool get_media_config(Config& config);
private:
	//本地配置相关
	bool ParseConfigFile_ini(const std::string& strFilePath);
	bool add_media_config(Config& config);

private:
	std::unordered_map<std::string, Config> _media_config_map;
	std::mutex _media_config_map_mtx;
	char _apppatch[265];
	char key_[64];
};
typedef boost::serialization::singleton<config_manager> config_manager_singleton;//配置管理单例