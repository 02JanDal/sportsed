#include "ChangeQuery.h"

#include "CapnprotoUtil.h"
#include "Change.h"

namespace Sportsed {
namespace Common {

ChangeQuery::ChangeQuery() {}

ChangeQuery ChangeQuery::fromReader(const Schema::ChangeQuery::Reader &reader)
{
	ChangeQuery query;
	query.m_fromRevision = reader.getFromRevision();
	query.m_tables = readList<TableQuery>(reader.getTables());
	return query;
}

void ChangeQuery::build(Schema::ChangeQuery::Builder builder) const
{
	builder.setFromRevision(m_fromRevision);
	writeList(m_tables, builder, &Schema::ChangeQuery::Builder::initTables);
}

bool ChangeQuery::matches(const Change &change, const Record &record) const
{
	if (change.revision() < m_fromRevision) {
		return false;
	}

	for (const TableQuery &query : m_tables) {
		if (query.table() == change.table()) {
			const bool matches = std::all_of(query.filters().constBegin(), query.filters().constEnd(),
											 [change, record](const TableFilter &filter) {
				if (filter.field() == "id") {
					return filter.value().value<Id>() == change.id();
				} else if (record.values().contains(filter.field())) {
					return filter.value() == record.values().value(filter.field());
				} else {
					// TODO fetch the reset of the record
					return false;
				}
			});
			if (matches) {
				return true;
			}
		}
	}

	return false;
}

bool ChangeQuery::operator==(const ChangeQuery &other) const
{
	return m_fromRevision == other.m_fromRevision && m_tables == other.m_tables;
}

}
}
