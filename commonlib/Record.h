#pragma once

#include <orm.capnp.h>

#include <QHash>
#include <QVariant>

namespace Sportsed {
namespace Common {

using Id = unsigned long long;
using Revision = unsigned long long;

class Record
{
public:
	explicit Record();

	QString table() const { return m_table; }
	void setTable(const QString &table) { m_table = table; }

	Id id() const { return m_id; }
	void setId(const Id id) { m_id = id; }

	Revision latestRevision() const { return m_latestRevision; }
	void setLatestRevision(const Revision revision) { m_latestRevision = revision; }

	QHash<QString, QVariant> values() const { return m_values; }
	void setValues(const QHash<QString, QVariant> &values) { m_values = values; }

	bool isComplete() const { return m_complete; }
	void setComplete(const bool complete) { m_complete = complete; }

	static Record fromReader(const Schema::Record::Reader &reader);
	void build(Schema::Record::Builder builder) const;

private:
	QString m_table;
	Id m_id = 0;
	Revision m_latestRevision = 0;
	QHash<QString, QVariant> m_values;
	bool m_complete = false;
};

}
}
