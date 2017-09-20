#pragma once

#include <QVector>

#include "TableQuery.h"
#include "Record.h"

namespace Sportsed {
namespace Common {

class Change;

class ChangeQuery
{
public:
	explicit ChangeQuery();

	Revision fromRevision() const { return m_fromRevision; }
	void setFromRevision(const Revision revision) { m_fromRevision = revision; }

	QVector<TableQuery> tables() const { return m_tables; }
	void setTables(const QVector<TableQuery> &tables) { m_tables = tables; }

	static ChangeQuery fromReader(const Schema::ChangeQuery::Reader &reader);
	void build(Schema::ChangeQuery::Builder builder) const;

	bool matches(const Change &change, const Record &record) const;

	bool operator==(const ChangeQuery &other) const;

private:
	Revision m_fromRevision;
	QVector<TableQuery> m_tables;
};

}
}
