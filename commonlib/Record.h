#pragma once

#include <QHash>
#include <QVariant>

#include <jd-util/Exception.h>

#if __has_include(<optional>)
#include <optional>
#elif defined(__cpp_lib_experimental_optional)
#include <experimental/optional>
namespace std {
using experimental::optional;
}
#else
#error Missing required header optional
#endif

namespace Sportsed {
namespace Common {

using Id = unsigned long long;
using Revision = unsigned long long;

enum class Table {
	Null,
	Meta,
	Change,
	Profile
};

DECLARE_EXCEPTION(SerializeNullTableName)
DECLARE_EXCEPTION(InvalidTableName)

QString tableName(const Table table);
Table fromTableName(const QString &str);

class Record
{
public:
	explicit Record(const Table table = Table::Null, const QHash<QString, QVariant> &values = {});

	bool isNull() const { return m_table == Table::Null; }
	bool isPersisted() const { return bool(m_id); }

	Table table() const { return m_table; }
	void setTable(const Table &table) { m_table = table; }

	Id id() const { return m_id.value_or(0); }
	void setId(const Id id) { m_id = id; }

	Revision latestRevision() const { return m_latestRevision; }
	void setLatestRevision(const Revision revision) { m_latestRevision = revision; }

	QVariant value(const QString &name) const { return m_values.value(name); }
	void setValue(const QString &name, const QVariant &value) { m_values.insert(name, value); }

	QHash<QString, QVariant> values() const { return m_values; }
	void setValues(const QHash<QString, QVariant> &values) { m_values = values; }

	bool isComplete() const { return m_complete; }
	void setComplete(const bool complete) { m_complete = complete; }

	static Record fromJson(const QJsonObject &obj);
	QJsonValue toJson() const;

private:
	Table m_table;
	std::optional<Id> m_id;
	Revision m_latestRevision = 0;
	QHash<QString, QVariant> m_values;
	bool m_complete = false;
};

}
}

QDebug operator<<(QDebug dbg, const Sportsed::Common::Record &r);

Q_DECLARE_METATYPE(Sportsed::Common::Record)
