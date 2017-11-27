#include "ChangeQuery.h"

#include <jd-util/Json.h>

#include "Change.h"

using namespace JD::Util;

namespace Sportsed {
namespace Common {

ChangeQuery::ChangeQuery(const TableQuery &query, const Revision from)
	: m_fromRevision(from), m_query(query) {}

ChangeQuery ChangeQuery::fromJson(const QJsonObject &obj)
{
	ChangeQuery query;
	query.m_fromRevision = Json::ensureIsType<Revision>(obj, "from_revision");
	query.m_query = Json::ensureIsType<TableQuery>(obj, "table");
	return query;
}
QJsonObject ChangeQuery::toJson() const
{
	return QJsonObject({
						   {"from_revision", Json::toJson(m_fromRevision)},
						   {"table", Json::toJson(m_query)}
					   });
}

bool ChangeQuery::matches(const Change &change, const Record &record) const
{
	if (change.revision() < m_fromRevision) {
		return false;
	}

	if (m_query.table() == record.table()) {
		const bool matches = std::all_of(m_query.filters().constBegin(), m_query.filters().constEnd(),
										 [change, record](const TableFilter &filter) {
			if (filter.field() == "id") {
				return filter.value().value<Id>() == record.id();
			} else if (record.values().contains(filter.field())) {
				const QVariant a = filter.value();
				const QVariant b = record.value(filter.field());
				switch (filter.op()) {
				case Sportsed::Common::TableFilter::Equal: return a == b;
				case Sportsed::Common::TableFilter::NotEqual: return a != b;
				case Sportsed::Common::TableFilter::Less: return a < b;
				case Sportsed::Common::TableFilter::LessEqual: return a <= b;
				case Sportsed::Common::TableFilter::Greater: return a > b;
				case Sportsed::Common::TableFilter::GreaterEqual: return a >= b;
				}
			} else if (filter.field().contains('>')) {
				return true; // include all "multi-level" fields since we are not able to determine if they should be excluded
			} else {
				return false;
			}
		});
		if (matches) {
			return true;
		}
	}

	return false;
}

bool ChangeQuery::operator==(const ChangeQuery &other) const
{
	return m_fromRevision == other.m_fromRevision && m_query == other.m_query;
}

}
}
