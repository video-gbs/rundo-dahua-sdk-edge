#ifndef _MySQLDLL_H
#define _MySQLDLL_H

#ifdef MYSQLDLL_EXPORTS
#define MYSQLDLL_API __declspec(dllexport)
#else
#define MYSQLDLL_API __declspec(dllimport)
#endif

extern "C"
{
     /*
	   功能：
	        连接数据库
		参数
		   int    nChan     通道号 0 ~ 255 
		   char*  szIP      IP
		   int    szPort    端口
		   char*  szUser    用户名
		   char*  szPwd     密码
		   char*  szDBName  数据库名称
         返回值：
		   BOOL   成功 TRUE ,失败 FALSE
	 */
     MYSQLDLL_API BOOL MySQL_Connect(int nChan,char* szIP,int szPort,char* szUser,char* szPwd,char* szDBName); 

     /*
	   功能：
	        断开数据库
		参数
		   int    nChan     通道号
		 返回值
		   void 
	 */
	 MYSQLDLL_API void MySQL_DisConnect(int nChan);

     /*
	   功能：
	        执行SQL语句，往往用在更改，删除
		参数
		   int    nChan    通道号
		   char*  szSQL    SQL语句 
		 返回值
		   BOOL 
	 */
	 MYSQLDLL_API BOOL MySQL_ExecuteSQL(int nChan,char* szSQL,...);

     /*
	   功能：
	        执行SQL命令，
			比如
			MySQL_ExecuteCmd(0,"begin;") ;    开始事物
			MySQL_ExecuteCmd(0,"commit;") ;   递交事物
			MySQL_ExecuteCmd(0,"rollback;") ; 回滚事物 
		参数
		   int    nChan    通道号
		   char*  szCMD    SQL命令
		 返回值
		   BOOL 
	 */
	 MYSQLDLL_API BOOL MySQL_ExecuteCmd(int nChan,char* szCMD);

     /*
	   【注意：废弃该函数，用MySQL_CheckTableHaveRecordBySQL 代替】
	   功能：
	        判断某个SQL是否存在一行数据,不返回行值
		参数
		   int    nChan    通道号
		   char*  szSQL    SQL语句 
		 返回值
		   BOOL 
	 */
	 //MYSQLDLL_API BOOL MySQL_QueryOneRecord(int nChan,char* szSQL) ; 

     /*
	   功能：
	          用于查询记录，包括一条或多条记录集
		参数
		   int    nChan    通道号
		   char*  szSQL    SQL语句 
		 返回值
		   BOOL 
	 */
     MYSQLDLL_API BOOL MySQL_QueryMultiRecord(int nChan,char* szSQL,...) ;

     MYSQLDLL_API BOOL MySQL_QueryMultiRecord2(int nChan,char* szSQL,...) ;

     /*
	   功能：
	              用于获取字段值
		参数
		   int    nChan    通道号
		   int    nOrder   sql语句中字段序号
		 返回值
		   char*  字段值（字符串形式） 
	 */
	 MYSQLDLL_API char*  MySQL_GetFieldValue(int nChan,int nOrder) ;

	 MYSQLDLL_API char*  MySQL_GetFieldValue2(int nChan,int nOrder) ;

     /*
	   功能：
	          用于获取最后一次出错的错误信息
		参数
		   int    nChan    通道号
		 返回值
		   char*  错误信息  
	 */
	 MYSQLDLL_API char*  MySQL_GetLastErrorInfo(int nChan) ;

     /*
	   功能：
	          用于获取最后一次成功执行查询语句的字段总数
		参数
		   int    nChan    通道号
		 返回值
		   int    字段总数
	 */
	 MYSQLDLL_API int  MySQL_GetLastFieldCount(int nChan) ;
	 MYSQLDLL_API int  MySQL_GetLastFieldCount2(int nChan) ;

     /*
	   功能：
	          用于获取最后一次成功执行查询语句的字段名称
		参数
		   int    nChan    通道号
		 返回值
		   char*  字段名称
	 */
	 MYSQLDLL_API char*  MySQL_GetLastFieldName(int nChan,int nFieldOrder) ;
	 MYSQLDLL_API char*  MySQL_GetLastFieldName2(int nChan,int nFieldOrder) ;

	 MYSQLDLL_API char*  MySQL_GetOneFieldValueBySQL(int nChan,char* szSQL,...) ;

	 MYSQLDLL_API BOOL   MySQL_CheckTableHaveRecordBySQL(int nChan,char* szSQL,...) ;
}

#endif