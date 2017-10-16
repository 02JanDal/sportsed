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
	explicit ChangeQuery(const QVector<TableQuery> &query = {}, const Revision from = 0);

	Revision fromRevision() const { return m_fromRevision; }
	void setFromRevision(const Revision revision) { m_fromRevision = revision; }

	QVector<TableQuery> tables() const { return m_tables; }
	void setTables(const QVector<TableQuery> &tables) { m_tables = tables; }

	static ChangeQuery fromJson(const QJsonObject &obj);
	QJsonObject toJson() const;

	bool matches(const Change &change, const Record &record) const;

	bool operator==(const ChangeQuery &other) const;

private:
	Revision m_fromRevision = 0;
	QVector<TableQuery> m_tables;
};

}
}
