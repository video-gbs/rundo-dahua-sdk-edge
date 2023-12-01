#pragma once
#include <string>
#include <Windows.h>
#include <boost/serialization/singleton.hpp>
#include "common.h"
#include "x64_mysql/MySQLDLL.h"

#define  DB_Chan1   0
class mysql_manage
{
public:
	mysql_manage();
	~mysql_manage();

	bool connetDB();
	void disconnetDB();
	//²éÕÒ
	bool Query_sql(Device& device, char* sql, ...);
	//Ôö£¬É¾£¬¸Ä
	bool Execute_sql(char* sql, ...);
private:
};
typedef boost::serialization::singleton<mysql_manage> mysql_manage_singleton;
