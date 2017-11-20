#pragma once

#include <QVector>
#include <QJsonObject>

#include "TableQuery.h"
#include "Record.h"

namespace Sportsed {
namespace Common {

class Change;

class ChangeQuery
{
public:
	explicit ChangeQuery(const TableQuery &query = TableQuery(Table::Null), const Revision from = 0);

	Revision fromRevision() const { return m_fromRevision; }
	void setFromRevision(const Revision revision) { m_fromRevision = revision; }

	TableQuery query() const { return m_query; }
	void setQuery(const TableQuery &table) { m_query = table; }

	static ChangeQuery fromJson(const QJsonObject &obj);
	QJsonObject toJson() const;

	bool matches(const Change &change, const Record &record) const;

	bool operator==(const ChangeQuery &other) const;

private:
	Revision m_fromRevision = 0;
	TableQuery m_query;
};

}
}
