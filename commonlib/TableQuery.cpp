#include "TableQuery.h"

#include "CapnprotoUtil.h"

namespace Sportsed {
namespace Common {

TableFilter::TableFilter() {}

TableFilter TableFilter::fromReader(const Schema::TableQuery::Filter::Reader &reader)
{
	TableFilter filter;
	filter.m_field = toQString(reader.getField());

	auto valueReader = reader.getValue();
	switch (valueReader.which()) {
	case Schema::TableQuery::Filter::Value::BOOL:
		filter.m_value = valueReader.getBool();
		break;
	case Schema::TableQuery::Filter::Value::INTEGER:
		filter.m_value = valueReader.getInteger();
		break;
	case Schema::TableQuery::Filter::Value::TEXT:
		filter.m_value = toQString(valueReader.getText());
		break;
	}

	return filter;
}
void TableFilter::build(Schema::TableQuery::Filter::Builder builder) const
{
	builder.setField(fromQString(m_field));
	switch (m_value.type()) {
	case QVariant::Bool:
		builder.getValue().setBool(m_value.toBool());
		break;
	case QVariant::String:
		builder.getValue().setText(fromQString(m_value.toString()));
		break;
	default:
		builder.getValue().setInteger(m_value.toLongLong());
		break;
	}
}

bool TableFilter::operator==(const TableFilter &other) const
{
	return m_field == other.m_field && m_value == other.m_value;
}

TableQuery::TableQuery() {}

TableQuery TableQuery::fromReader(const Schema::TableQuery::Reader &reader)
{
	TableQuery query;
	query.m_table = toQString(reader.getTable());
	query.m_filters = readList<TableFilter>(reader.getFilters());
	return query;
}

void TableQuery::build(Schema::TableQuery::Builder builder) const
{
	builder.setTable(fromQString(m_table));
	writeList(m_filters, builder, &Schema::TableQuery::Builder::initFilters);
}

bool TableQuery::operator==(const TableQuery &other) const
{
	return m_table == other.m_table && m_filters == other.m_filters;
}

}
}
