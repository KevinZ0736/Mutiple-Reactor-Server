#include "MysqlConn.h"

MysqlConn::MysqlConn()
{
	m_conn = mysql_init(nullptr);
	mysql_set_character_set(m_conn, "utf8");
}

MysqlConn::~MysqlConn()
{
	if (m_conn)
	{
		mysql_close(m_conn);
	}
	freeResult();
}

bool MysqlConn::connect(string user, string passwd, string dbName, string ip, unsigned short port)
{
	MYSQL* ptr = mysql_real_connect(m_conn, ip.c_str(), user.c_str(), passwd.c_str(), dbName.c_str(), port, nullptr, 0);
	return ptr != nullptr;
}

bool MysqlConn::update(string sql)
{
	//返回0证明执行成功了
	if (mysql_query(m_conn, sql.c_str()))
	{
		return false;
	}
	return true;
}

bool MysqlConn::select(string sql)
{
	freeResult();
	if (mysql_query(m_conn, sql.c_str()))
	{
		return false;
	}
	m_result = mysql_store_result(m_conn);
	return true;
}

bool MysqlConn::next()
{
	if (m_result)
	{
		m_row = mysql_fetch_row(m_result);
		if (m_row != nullptr)
		{
			return true;
		}
	}

	return false;
}

string MysqlConn::value(int index)
{
	int rowCount = mysql_num_fields(m_result);
	if (index >= rowCount || index < 0)
	{
		return string();
	}
	char* val = m_row[index];
	unsigned long length = mysql_fetch_lengths(m_result)[index];//获取结果集的长度，防止获取的是二进制字符，导致只识别到/0就结束了
	return string(val, length);
}

bool MysqlConn::transaction()
{
	return mysql_autocommit(m_conn, false);//第二个参数，确定是手动提交还是自动
}

bool MysqlConn::commit()
{
	return mysql_commit(m_conn);
}

bool MysqlConn::rollback()
{
	return mysql_rollback(m_conn);
}

void MysqlConn::refreshAliveTime()
{
	m_alivetime = steady_clock::now();
}

long long MysqlConn::getAliveTime()
{
	nanoseconds res = steady_clock::now() - m_alivetime; //基本单位是纳秒
	milliseconds millsec = duration_cast<milliseconds>(res);
	return millsec.count();
}

void MysqlConn::freeResult()
{
	if (m_result)
	{
		mysql_free_result(m_result);
		m_result = nullptr;
	}
}



