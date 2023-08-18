#pragma once
#include<iostream>
#include<mysql.h>
#include<chrono>
using namespace std;
using namespace chrono;

// 把文件filename加载到buffer中，必须确保buffer足够大。
// 成功返回文件的大小，文件不存在或为空返回0。  
unsigned long filetobuf(const char* filename, char* buffer);

// 把buffer中的内容写入文件filename，size为buffer中有效内容的大小。
// 成功返回true，失败返回false。
bool buftofile(const char* filename, char* buffer, unsigned long size);


class MysqlConn
{
public:
	// 初始化数据库连接
	MysqlConn();
	// 释放数据库连接
	~MysqlConn();
	// 连接数据库
	bool connect(string user, string passwd, string dbName, string ip, unsigned short port = 3306);
	// 更新数据库：insert,update,delete
	bool update(string sql);
	// 查询数据库
	bool select(string sql);
	// 遍历查询得到的结果集
	bool next();
	// 得到结果集中的字段值
	string value(int index);
	// 事务操作
	bool transaction();
	// 提交事务
	bool commit();
	// 事务回滚
	bool rollback();
	// 刷新起始的空闲时间点
	void refreshAliveTime();
	// 计算连接存活的总时长
	long long getAliveTime();

private:
	MYSQL* m_conn = nullptr;
	MYSQL_RES* m_result = nullptr;
	MYSQL_ROW m_row = nullptr;

	void freeResult();//将SQL获得结果集，进行资源释放

	steady_clock::time_point m_alivetime;

};