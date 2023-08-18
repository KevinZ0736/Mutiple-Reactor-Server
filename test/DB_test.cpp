#include<iostream>
#include<memory>
#include"MysqlConn.h"
using namespace std;

//单线程：使用/不使用连接池
void op1(int begin,int end)
{
	for (int i = begin; i < end; i++)
	{
		MysqlConn conn;
		conn.connect("root", "kevin", "testdb", "124.222.175.81");
		char sql[1024] = { 0 };
		sprintf(sql, "insert into person values(%d,%d,'man','tom-%d')", i);
		conn.update(sql);
	}
}

int query()
{
	MysqlConn conn;
	conn.connect("root", "kevin", "testdb", "124.222.175.81");
	string sql = "insert into person values(5,25,'man','tom')";
	bool flag = conn.update(sql);
	cout << "flag value:" << flag << endl;

	sql = "select * from person";
	conn.select(sql);
	while (conn.next())
	{
		cout << conn.value(0) << ","
			<< conn.value(1) << ","
			<< conn.value(2) << ","
			<< conn.value(3) << endl;
	}
	return 0;
}

int main()
{
	query();
	return 0;
}