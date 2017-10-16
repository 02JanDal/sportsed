#include "Record.h"

#include <jd-util/Json.h>
#include <QDebug>

using namespace JD::Util;

namespace Sportsed {
namespace Common {

Record::Record(const Table table, const QHash<QString, QVariant> &values) : m_table(table), m_values(values) {}

Record Record::fromJson(const QJsonObject &obj)
{
	Record record(fromTableName(Json::ensureString(obj, "table")));
	if (obj.value("id").isDouble()) {
		record.m_id = Json::ensureIsType<Id>(obj, "id");
	}
	record.m_latestRevision = Json::ensureIsType<Revision>(obj, "latest_revision");
	record.m_values = Json::ensureIsHashOf<QVariant>(obj, "values");
	return record;
}
QJsonValue Record::toJson() const
{
	if (m_table == Table::Null) {
		return QJsonValue();
	}
	return QJsonObject({
						   {"table", tableName(m_table)},
						   {"id", bool(m_id) ? Json::toJson(m_id.value_or(0)) : QJsonValue()},
						   {"latest_revision", Json::toJson(m_latestRevision)},
						   {"values", Json::toJsonObject(m_values)}
					   });
}

QString tableName(const Table table)
{
	switch (table) {
	case Table::Null: throw SerializeNullTableNameException();
	case Table::Meta: return "meta";
	case Table::Change: return "change";
	case Table::Profile: return "profile";
	}
}
Table fromTableName(const QString &str)
{
	if (str == "meta") {
		return Table::Meta;
	} else if (str == "change") {
		return Table::Change;
	} else if (str == "profile") {
		return Table::Profile;
	} else {
		throw InvalidTableNameException();
	}
}

}
}

QDebug operator<<(QDebug dbg, const Sportsed::Common::Record &r)
{
	using namespace Sportsed::Common;

	QDebugStateSaver saver(dbg);

	if (r.isNull()) {
		return dbg << "Record(Null)";
	}
	dbg.nospace() << "Record(";
	switch (r.table()) {
	case Table::Null: break;
	case Table::Meta: dbg.nospace() << "Meta"; break;
	case Table::Change: dbg.nospace() << "Change"; break;
	case Table::Profile: dbg.nospace() << "Profile"; break;
	}

	if (!r.isComplete()) {
		dbg.nospace() << ", InComplete";
	}

	if (r.isPersisted()) {
		dbg.nospace() << ", id=" << r.id();
	} else {
		dbg.nospace() << ", NotPersisted";
	}
	if (r.latestRevision() != 0) {
		dbg.nospace() << ", rev=" << r.latestRevision();
	}

	dbg.nospace() << ", values=" << r.values();
	dbg.nospace() << ")";

	return dbg;
}
