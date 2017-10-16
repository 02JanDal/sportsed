#pragma once

#include <QVariant>
#include <QVector>

#include "Record.h"

namespace Sportsed {
namespace Common {

class TableFilter
{
public:
	explicit TableFilter();
	explicit TableFilter(const QString &field, const QVariant &value);

	QString field() const { return m_field; }
	void setField(const QString &field) { m_field = field; }

	QVariant value() const { return m_value; }
	void setValue(const QVariant &value) { m_value = value; }

	static TableFilter fromJson(const QJsonObject &obj);
	QJsonObject toJson() const;

	bool operator==(const TableFilter &other) const;

private:
	QString m_field;
	QVariant m_value;
};

class TableQuery
{
public:
	explicit TableQuery();
	explicit TableQuery(const Table table, const QVector<TableFilter> &filters = {});
	explicit TableQuery(const Table table, const TableFilter &filter);

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
