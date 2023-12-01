#include "mysql_manage.h"
#include "log.h"

mysql_manage::mysql_manage()
{
}

mysql_manage::~mysql_manage()
{
	disconnetDB();
}

bool mysql_manage::connetDB()
{
	std::string db_ip;
	int db_port = 3306;
	std::string db_user;
	std::string db_psw;
	std::string db_name;
	bool bRet = MySQL_Connect(DB_Chan1, (char*)db_ip.c_str(), db_port, (char*)db_user.c_str(), (char*)db_psw.c_str(), (char*)db_name.c_str());
	if (bRet)
	{
		LOG_INFO("登陆数据库%s:%d:%s 成功", db_ip.c_str(), db_port, db_user.c_str());
	}
	else
	{
		LOG_ERROR("登陆数据库%s:%d:%s 失败", db_ip.c_str(), db_port, db_user.c_str());
	}
	return bRet;
}

void mysql_manage::disconnetDB()
{
	MySQL_DisConnect(DB_Chan1);
}

bool mysql_manage::Query_sql(Device& device, char* sql, ...)
{
	LOG_INFO("Query_sql:%s", sql);

	char szSQL2[8192] = { 0 };
	va_list list;
	va_start(list, sql);
	vsprintf_s(szSQL2, sql, list);
	va_end(list);

	//查库获取设备信息
	char szBuffer[1024] = { 0 };

	while (MySQL_QueryMultiRecord2(DB_Chan1, sql))
	{
		device.deviceId = MySQL_GetFieldValue2(DB_Chan1, 0);

		sprintf(szBuffer, "%s - %s -  %s -  %s - %s -  %s - ", MySQL_GetFieldValue2(DB_Chan1, 0), MySQL_GetFieldValue2(DB_Chan1, 1), MySQL_GetFieldValue2(DB_Chan1, 2),
			MySQL_GetFieldValue2(DB_Chan1, 3), MySQL_GetFieldValue2(DB_Chan1, 4), MySQL_GetFieldValue2(DB_Chan1, 5));
	}
	return false;
}

bool mysql_manage::Execute_sql(char* sql, ...)
{
	LOG_INFO("Execute_sql:%s", sql);

	char szSQL2[8192] = { 0 };
	va_list list;
	va_start(list, sql);
	vsprintf_s(szSQL2, sql, list);
	va_end(list);

	return MySQL_ExecuteSQL(DB_Chan1, sql);
}