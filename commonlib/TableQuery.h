#pragma once

#include <QVariant>
#include <QVector>

#include "Record.h"

namespace Sportsed {
namespace Common {

class TableFilter
{
public:
	enum Operator
	{
		Equal,
		NotEqual,
		Less,
		LessEqual,
		Greater,
		GreaterEqual
	};

	explicit TableFilter();
	explicit TableFilter(const QString &field, const QVariant &value);
	explicit TableFilter(const QString &field, const Operator op, const QVariant &value);

	QString field() const { return m_field; }
	void setField(const QString &field) { m_field = field; }

	Operator op() const { return m_op; }
	void setOp(const Operator op) { m_op = op; }

	QVariant value() const { return m_value; }
	void setValue(const QVariant &value) { m_value = value; }

	static TableFilter fromJson(const QJsonObject &obj);
	QJsonObject toJson() const;

	bool operator==(const TableFilter &other) const;

private:
	QString m_field;
	Operator m_op = Equal;
	QVariant m_value;
};

class TableQuery
{
public:
	explicit TableQuery();
	explicit TableQuery(const Table table, const QVector<TableFilter> &filters = {});
	explicit TableQuery(const Table table, const TableFilter &filter);

	bool isNull() const { return m_table == Table::Null; }

	Table table() const { return m_table; }
	void setTable(const Table &table) { m_table = table; }

	QVector<TableFilter> filters() const { return m_filters; }
	void setFilters(const QVector<TableFilter> &filters) { m_filters = filters; }

	static TableQuery fromJson(const QJsonObject &obj);
	QJsonObject toJson() const;

	bool operator==(const TableQuery &other) const;

private:
	Table m_table;
	QVector<TableFilter> m_filters;
};

}
}

QDebug &operator<<(QDebug &dbg, const QVector<Sportsed::Common::TableFilter> &filters);
