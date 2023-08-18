#include "GetCols.h"

GetCols::GetCols()
{
	initdata(0); // 调用成员变量初始化函数
}

void GetCols::initdata(int mod)
{
	if (mod == 0 || mod == 1)
	{
		m_allcount = 0;
		m_maxcollen = 0;
		m_vallcols.clear();
		m_allcols.clear();
	}
	if (mod == 0 || mod == 2)
	{
		m_pkcount = 0;
		m_pkcols.clear();
		m_vpkcols.clear();
	}
}

bool GetCols::allcols(connection* conn, char* tablename)
{
	initdata(1);
	struct st_columns columns;

	sqlstatement stmt;
	stmt.connect(conn);
	stmt.prepare("select lower(column_name),lower(data_type),character_maximum_length from information_schema.COLUMNS where table_name=:1");

	stmt.bindin(1, tablename, 30); //绑定表名
	stmt.bindout(1, columns.colname, 30);  //从结果中绑定字段名
	stmt.bindout(2, columns.datatype, 30); //从结果中绑定字段类型
	stmt.bindout(3, &columns.collen);

	if (stmt.execute() != 0) return false;

	while (true)
	{
		memset(&columns, 0, sizeof(struct st_columns));

		if (stmt.next() != 0) break;

		// 统一数据类型，统一各类型数据类型
		// 列的数据类型，分为number、date和char三大类。统一数据类型
		if (strcmp(columns.datatype, "char") == 0)    strcpy(columns.datatype, "char");
		if (strcmp(columns.datatype, "varchar") == 0) strcpy(columns.datatype, "char");

		if (strcmp(columns.datatype, "datetime") == 0)  strcpy(columns.datatype, "date");
		if (strcmp(columns.datatype, "timestamp") == 0) strcpy(columns.datatype, "date");

		if (strcmp(columns.datatype, "tinyint") == 0)   strcpy(columns.datatype, "number");
		if (strcmp(columns.datatype, "smallint") == 0)  strcpy(columns.datatype, "number");
		if (strcmp(columns.datatype, "mediumint") == 0) strcpy(columns.datatype, "number");
		if (strcmp(columns.datatype, "int") == 0)       strcpy(columns.datatype, "number");
		if (strcmp(columns.datatype, "integer") == 0)   strcpy(columns.datatype, "number");
		if (strcmp(columns.datatype, "bigint") == 0)    strcpy(columns.datatype, "number");
		if (strcmp(columns.datatype, "numeric") == 0)   strcpy(columns.datatype, "number");
		if (strcmp(columns.datatype, "decimal") == 0)   strcpy(columns.datatype, "number");
		if (strcmp(columns.datatype, "float") == 0)     strcpy(columns.datatype, "number");
		if (strcmp(columns.datatype, "double") == 0)    strcpy(columns.datatype, "number");

		// 如果业务有需要，可以修改上面的代码，增加对更多数据类型的支持。
		// 如果字段的数据类型不在上面列出来的中，忽略它。
		if ((strcmp(columns.datatype, "char") != 0) &&
			(strcmp(columns.datatype, "date") != 0) &&
			(strcmp(columns.datatype, "number") != 0)) continue;

		// 如果字段类型是date，把长度设置为19。yyyy-mm-dd hh:mi:ss
		if (strcmp(columns.datatype, "date") == 0) columns.collen = 19;

		// 如果字段类型是number，把长度设置为20。
		if (strcmp(columns.datatype, "number") == 0) columns.collen = 20;

		m_allcols += columns.colname; m_allcols += ",";

		m_vallcols.push_back(columns);

		if (m_maxcollen < columns.collen) m_maxcollen = columns.collen;

		m_allcount++;
	}

	// 删除m_allcols最后一个多余的逗号。
	if (m_allcount > 0) m_allcols.pop_back();

	return true;
}

bool GetCols::pkcols(connection* conn, char* tablename)
{
	initdata(2);
	struct st_columns pk_columns;  //主键字段

	sqlstatement stmt;
	stmt.connect(conn);
	stmt.prepare("select lower(column_name),seq_in_index from information_schema.STATISTICS where table_name=:1 and index_name='primary' order by seq_in_index");
	stmt.bindin(1, tablename, 30);     //绑定表名
	stmt.bindout(1, pk_columns.colname, 30);   //主键字段
	stmt.bindout(2, &pk_columns.pkseq);        //主键顺序

	if (stmt.execute() != 0) return false;

	while (true)
	{
		memset(&pk_columns, 0, sizeof(struct st_columns));

		if (stmt.next() != 0) break;

		m_pkcols += pk_columns.colname; m_pkcols += ",";

		m_vpkcols.push_back(pk_columns);

		m_pkcount++;
	}
	// 删除m_pkcols最后一个多余的逗号。
	if (m_pkcount > 0)  m_pkcols.pop_back();

	return true;
}

bool GetCols::bindseq()
{
	if (m_vallcols.empty() || m_vpkcols.empty())
		return false;
	int flag = 0;

	for (int ii = 0; ii < m_vpkcols.size(); ii++)
		for (int jj = 0; jj < m_vallcols.size(); jj++)
			if (strcmp(m_vpkcols[ii].colname, m_vallcols[jj].colname) == 0)
			{
				// 更新m_vallcols容器中的pkseq。
				m_vallcols[jj].pkseq = m_vpkcols[ii].pkseq;
				flag = 1;
				break;
			}
	if (flag == 0)
		return false;
	else
		return true;
}
