#pragma once
#include<queue>
#include<mutex>
#include<condition_variable>
#include<fstream>
#include<thread>
#include<chrono>
#include"_public.h"
#include"_mysql.h"
using namespace std;
using namespace chrono;

class ConnectionPool
{
public:
	//建立一个数据库连接池
	static ConnectionPool* getConnectPool();
	//关闭拷贝构造
	ConnectionPool(const ConnectionPool& obj) = delete;
	ConnectionPool& operator=(const ConnectionPool& obj) = delete;
	//取出一个数据库连接
	shared_ptr<connection> getConnection();
	~ConnectionPool();
private:
	ConnectionPool();
	bool LoadConfig();
	void addConnection(); //在连接队列中增加连接
	void produceConnection();  //生产数据库连接
	void recycleConnection();  //销毁数据库连接

	string m_ip;
	string m_user;
	string m_passwd;
	string m_dbName;
	int m_port;
	int m_minSize;  //连接池中的最小连接数
	int m_maxSize;  //连接池中的最大连接数
	int m_timeout;  //超时机制
	int m_maxIdleTime;  //最长空闲时间

	queue<connection*> m_connectionQ; //用于存放连接的容器
	mutex m_mutexQ;
	condition_variable m_cond;
};

