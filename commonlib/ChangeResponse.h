#pragma once

#include <orm.capnp.h>

#include <QVector>

#include "ChangeQuery.h"
#include "Change.h"
#include "Record.h"

namespace Sportsed {
namespace Common {

class ChangeResponse
{
public:
	explicit ChangeResponse();

	ChangeQuery query() const { return m_query; }
	void setQuery(const ChangeQuery &query) { m_query = query; }

	QVector<Change> changes() const { return m_changes; }
	void setChanges(const QVector<Change> &changes) { m_changes = changes; }

	Revision lastRevision() const { return m_lastRevision; }
	void setLastRevision(const Revision revision) { m_lastRevision = revision; }

	static ChangeResponse fromReader(const Schema::ChangeResponse::Reader &reader);
	void build(Schema::ChangeResponse::Builder builder) const;

private:
	ChangeQuery m_query;
	QVector<Change> m_changes;
	Revision m_lastRevision;
};

}
}
