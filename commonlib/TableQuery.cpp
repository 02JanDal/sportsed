#include "TableQuery.h"

#include <jd-util/Json.h>

#include <QDebug>

using namespace JD::Util;

namespace Sportsed {
namespace Common {

TableFilter::TableFilter() {}

TableFilter::TableFilter(const QString &field, const QVariant &value)
	: m_field(field), m_value(value) {}

TableFilter TableFilter::fromJson(const QJsonObject &obj)
{
	TableFilter filter;
	filter.m_field = Json::ensureString(obj, "field");
	filter.m_value = Json::ensureVariant(obj, "value");
	return filter;
}
QJsonObject TableFilter::toJson() const
{
	return QJsonObject({
						   {"field", m_field},
						   {"value", Json::toJson(m_value)}
					   });
}

bool TableFilter::operator==(const TableFilter &other) const
{
	return m_field == other.m_field && m_value == other.m_value;
}

TableQuery::TableQuery() {}

TableQuery::TableQuery(const Table table, const QVector<TableFilter> &filters)
	: m_table(table), m_filters(filters) {}

TableQuery::TableQuery(const Table table, const TableFilter &filter)
	: TableQuery(table, QVector<TableFilter>() << filter) {}

TableQuery TableQuery::fromJson(const QJsonObject &obj)
{
	TableQuery query;
	query.m_table = Common::fromTableName(Json::ensureString(obj, "table"));
	query.m_filters = Json::ensureIsArrayOf<TableFilter>(obj, "filters");
	return query;
}
QJsonObject TableQuery::toJson() const
{
	return QJsonObject({
						   {"table", Common::tableName(m_table)},
						   {"filters", Json::toJsonArray(m_filters)}
					   });
}

bool TableQuery::operator==(const TableQuery &other) const
{
	return m_table == other.m_table && m_filters == other.m_filters;
}

}
}

QDebug &operator<<(QDebug &dbg, const QVector<Sportsed::Common::TableFilter> &filters)
{
	return dbg << qPrintable('(' + Functional::collection(filters).map([](const Sportsed::Common::TableFilter &f) {
		QString val;
		if (f.value().type() == QVariant::String) {
			val = '"' + f.value().toString() + '"';
		} else {
			val = f.value().toString();
		}
		return f.field() + "=>" + val;
	}).join(QStringLiteral(", ")) + ')');
}
