#include "ConnectionPool.h"

ConnectionPool::ConnectionPool()
{
	//加载配置文件
	if (!LoadConfig())
	{
		return;
	}

	for (int x = 0; x < m_maxSize; x++)
	{
		connection* conn = new connection;
		conn->connecttodb(m_user, m_passwd, m_dbName, m_ip, m_port, "utf8");
		m_connectionQ.push(conn);
	}
	thread producer(&ConnectionPool::produceConnection, this);  //生产者线程
	thread recycler(&ConnectionPool::recycleConnection, this); //消费者线程

	producer.detach();  //调用detach()成员函数分离子线程，子线程退出时，系统将自动回收资源。
	recycler.detach();
}

ConnectionPool::~ConnectionPool()
{
	while (!m_connectionQ.empty())
	{
		connection* conn = m_connectionQ.front();
		m_connectionQ.pop();
		delete conn;
	}
}

//它们在类的命名空间中定义，可以直接通过类名访问，而不需要创建类的实例。
ConnectionPool* ConnectionPool::getConnectPool()
{
	static ConnectionPool pool;//由于静态成员是与类关联而不是与类的实例关联的，因此只需在类中进行一次声明。
	return &pool;
}

shared_ptr<connection> ConnectionPool::getConnection()
{
	unique_lock<mutex> locker(m_mutexQ);
	while (m_connectionQ.empty())
	{
		if (cv_status::timeout == m_cond.wait_for(locker, milliseconds(m_timeout)))
		{
			if (m_connectionQ.empty()) {
				continue;
			}
		}
	}
	//采用shared_ptr智能指针，自己编写删除函数，实现在指针周期结束时，并不释放资源，而是更新连接时间，重新放入到池子中
	shared_ptr<connection> connptr(m_connectionQ.front(), [this](connection* conn) {
		lock_guard<mutex> locker(m_mutexQ);
		if (conn->m_cda.rc == 0)
		{
			conn->commit();
		}
		else {
			conn->rollback();
		}
		conn->refreshAliveTime();
		m_connectionQ.push(conn);
		});
	m_connectionQ.pop();//由于没有连接总数的限制，使得每次唤醒生产者，只要不满100就会一直生产，与返还的连接叠加后，会使得总数一直增加（不考虑时限情况）
	m_cond.notify_all();
	return connptr;
}

bool ConnectionPool::LoadConfig()
{
	ifstream Conf;
	Conf.open("/usr/project/Project_win/Database/DBconfig.xml", ios::in);
	if (Conf.is_open() == false)
	{
		return false;
	}
	string buffer, config;
	while (getline(Conf, buffer))
	{
		config = config + buffer;
	}
	const char* conf = config.c_str();
	if ((GetXMLBuffer(conf, "ip", m_ip)) &&
		(GetXMLBuffer(conf, "port", &m_port)) &&
		(GetXMLBuffer(conf, "userName", m_user)) &&
		(GetXMLBuffer(conf, "password", m_passwd)) &&
		(GetXMLBuffer(conf, "dbName", m_dbName)) &&
		(GetXMLBuffer(conf, "minSize", &m_minSize)) &&
		(GetXMLBuffer(conf, "maxSize", &m_maxSize)) &&
		(GetXMLBuffer(conf, "maxIdleTime", &m_maxIdleTime)) &&
		(GetXMLBuffer(conf, "timeout", &m_timeout)) == false)
	{
		return false;
	}
	return true;
}

void ConnectionPool::addConnection()
{
	connection* conn = new connection;
	conn->connecttodb(m_user, m_passwd, m_dbName, m_ip, m_port, "utf8");
	conn->refreshAliveTime();
	m_connectionQ.push(conn);
}

void ConnectionPool::produceConnection()
{
	while (true)
	{
		unique_lock<mutex> locker(m_mutexQ);
		while (m_connectionQ.size() > m_minSize)
		{
			m_cond.wait(locker);
		}
		if (m_connectionQ.size() < m_maxSize)
		{
			addConnection();
			m_cond.notify_all(); //生产了一个连接，唤醒阻塞的等待连接的线程
		}
	}
}

void ConnectionPool::recycleConnection()
{
	while (true)
	{
		this_thread::sleep_for(chrono::milliseconds(500));
		lock_guard<mutex> locker(m_mutexQ);
		while (m_connectionQ.size() > m_minSize)
		{
			connection* conn = m_connectionQ.front();
			if (conn->getAliveTime() >= m_maxIdleTime)
			{
				m_connectionQ.pop();
				delete conn;
			}
			else {
				break;
			}
		}
	}
}
