#pragma once

#include <orm.capnp.h>

#include <QVariant>
#include <QVector>

namespace Sportsed {
namespace Common {

class TableFilter
{
public:
	explicit TableFilter();

	QString field() const { return m_field; }
	void setField(const QString &field) { m_field = field; }

	QVariant value() const { return m_value; }
	void setValue(const QVariant &value) { m_value = value; }

	static TableFilter fromReader(const Schema::TableQuery::Filter::Reader &reader);
	void build(Schema::TableQuery::Filter::Builder builder) const;

	bool operator==(const TableFilter &other) const;

private:
	QString m_field;
	QVariant m_value;
};

class TableQuery
{
public:
	explicit TableQuery();

	QString table() const { return m_table; }
	void setTable(const QString &table) { m_table = table; }

	QVector<TableFilter> filters() const { return m_filters; }
	void setFilters(const QVector<TableFilter> &filters) { m_filters = filters; }

	static TableQuery fromReader(const Schema::TableQuery::Reader &reader);
	void build(Schema::TableQuery::Builder builder) const;

	bool operator==(const TableQuery &other) const;

private:
	QString m_table;
	QVector<TableFilter> m_filters;
};

}
}
