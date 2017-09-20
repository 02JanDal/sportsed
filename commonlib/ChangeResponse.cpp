#include "ChangeResponse.h"

#include "CapnprotoUtil.h"

namespace Sportsed {
namespace Common {

ChangeResponse::ChangeResponse() {}

ChangeResponse ChangeResponse::fromReader(const Schema::ChangeResponse::Reader &reader)
{
	ChangeResponse response;
	response.m_query = ChangeQuery::fromReader(reader.getQuery());
	response.m_changes = readList<Change>(reader.getChanges());
	response.m_lastRevision = reader.getLastRevision();
	return response;
}

void ChangeResponse::build(Schema::ChangeResponse::Builder builder) const
{
	m_query.build(builder.initQuery());
	writeList(m_changes, builder, &Schema::ChangeResponse::Builder::initChanges);
	builder.setLastRevision(m_lastRevision);
}

}
}
