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
	   ���ܣ�
	        �������ݿ�
		����
		   int    nChan     ͨ���� 0 ~ 255 
		   char*  szIP      IP
		   int    szPort    �˿�
		   char*  szUser    �û���
		   char*  szPwd     ����
		   char*  szDBName  ���ݿ�����
         ����ֵ��
		   BOOL   �ɹ� TRUE ,ʧ�� FALSE
	 */
     MYSQLDLL_API BOOL MySQL_Connect(int nChan,char* szIP,int szPort,char* szUser,char* szPwd,char* szDBName); 

     /*
	   ���ܣ�
	        �Ͽ����ݿ�
		����
		   int    nChan     ͨ����
		 ����ֵ
		   void 
	 */
	 MYSQLDLL_API void MySQL_DisConnect(int nChan);

     /*
	   ���ܣ�
	        ִ��SQL��䣬�������ڸ��ģ�ɾ��
		����
		   int    nChan    ͨ����
		   char*  szSQL    SQL��� 
		 ����ֵ
		   BOOL 
	 */
	 MYSQLDLL_API BOOL MySQL_ExecuteSQL(int nChan,char* szSQL,...);

     /*
	   ���ܣ�
	        ִ��SQL���
			����
			MySQL_ExecuteCmd(0,"begin;") ;    ��ʼ����
			MySQL_ExecuteCmd(0,"commit;") ;   �ݽ�����
			MySQL_ExecuteCmd(0,"rollback;") ; �ع����� 
		����
		   int    nChan    ͨ����
		   char*  szCMD    SQL����
		 ����ֵ
		   BOOL 
	 */
	 MYSQLDLL_API BOOL MySQL_ExecuteCmd(int nChan,char* szCMD);

     /*
	   ��ע�⣺�����ú�������MySQL_CheckTableHaveRecordBySQL ���桿
	   ���ܣ�
	        �ж�ĳ��SQL�Ƿ����һ������,��������ֵ
		����
		   int    nChan    ͨ����
		   char*  szSQL    SQL��� 
		 ����ֵ
		   BOOL 
	 */
	 //MYSQLDLL_API BOOL MySQL_QueryOneRecord(int nChan,char* szSQL) ; 

     /*
	   ���ܣ�
	          ���ڲ�ѯ��¼������һ���������¼��
		����
		   int    nChan    ͨ����
		   char*  szSQL    SQL��� 
		 ����ֵ
		   BOOL 
	 */
     MYSQLDLL_API BOOL MySQL_QueryMultiRecord(int nChan,char* szSQL,...) ;

     MYSQLDLL_API BOOL MySQL_QueryMultiRecord2(int nChan,char* szSQL,...) ;

     /*
	   ���ܣ�
	              ���ڻ�ȡ�ֶ�ֵ
		����
		   int    nChan    ͨ����
		   int    nOrder   sql������ֶ����
		 ����ֵ
		   char*  �ֶ�ֵ���ַ�����ʽ�� 
	 */
	 MYSQLDLL_API char*  MySQL_GetFieldValue(int nChan,int nOrder) ;

	 MYSQLDLL_API char*  MySQL_GetFieldValue2(int nChan,int nOrder) ;

     /*
	   ���ܣ�
	          ���ڻ�ȡ���һ�γ���Ĵ�����Ϣ
		����
		   int    nChan    ͨ����
		 ����ֵ
		   char*  ������Ϣ  
	 */
	 MYSQLDLL_API char*  MySQL_GetLastErrorInfo(int nChan) ;

     /*
	   ���ܣ�
	          ���ڻ�ȡ���һ�γɹ�ִ�в�ѯ�����ֶ�����
		����
		   int    nChan    ͨ����
		 ����ֵ
		   int    �ֶ�����
	 */
	 MYSQLDLL_API int  MySQL_GetLastFieldCount(int nChan) ;
	 MYSQLDLL_API int  MySQL_GetLastFieldCount2(int nChan) ;

     /*
	   ���ܣ�
	          ���ڻ�ȡ���һ�γɹ�ִ�в�ѯ�����ֶ�����
		����
		   int    nChan    ͨ����
		 ����ֵ
		   char*  �ֶ�����
	 */
	 MYSQLDLL_API char*  MySQL_GetLastFieldName(int nChan,int nFieldOrder) ;
	 MYSQLDLL_API char*  MySQL_GetLastFieldName2(int nChan,int nFieldOrder) ;

	 MYSQLDLL_API char*  MySQL_GetOneFieldValueBySQL(int nChan,char* szSQL,...) ;

	 MYSQLDLL_API BOOL   MySQL_CheckTableHaveRecordBySQL(int nChan,char* szSQL,...) ;
}

#endif