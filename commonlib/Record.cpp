#include "Record.h"

#include <jd-util/Json.h>
#include <QDebug>

#include "Validators.h"

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
	return BaseValidator::getValidator(record.m_table)->coerceRecord(record);
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

QVector<QString> Record::changesBetween(const Record &record) const
{
	QVector<QString> changes;
	for (auto it = m_values.cbegin(); it != m_values.cend(); ++it) {
		if (it.value() != record.value(it.key())) {
			changes.append(it.key());
		}
	}
	return changes;
}

bool Record::operator==(const Record &other) const
{
	return m_table == other.m_table && m_id == other.m_id &&
			m_values == other.m_values && m_latestRevision == other.m_latestRevision;
}

QString tableName(const Table table)
{
	switch (table) {
	case Table::Null: throw SerializeNullTableNameException();
	case Table::Meta: return "meta";
	case Table::Change: return "change";
	case Table::Profile: return "profile";
	case Table::Client: return "client";
	case Table::Competition: return "competition";
	case Table::Stage: return "stage";
	case Table::Course: return "course";
	case Table::Control: return "control";
	case Table::CourseControl: return "course_control";
	case Table::Class: return "class";
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
	} else if (str == "client") {
		return Table::Client;
	} else if (str == "competition") {
		return Table::Competition;
	} else if (str == "stage") {
		return Table::Stage;
	} else if (str == "course") {
		return Table::Course;
	} else if (str == "control") {
		return Table::Control;
	} else if (str == "course_control") {
		return Table::CourseControl;
	} else if (str == "class") {
		return Table::Class;
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
	case Table::Client: dbg.nospace() << "Client"; break;
	case Table::Competition: dbg.nospace() << "Competition"; break;
	case Table::Stage: dbg.nospace() << "Stage"; break;
	case Table::Course: dbg.nospace() << "Course"; break;
	case Table::Control: dbg.nospace() << "Control"; break;
	case Table::CourseControl: dbg.nospace() << "CourseControl"; break;
	case Table::Class: dbg.nospace() << "Class"; break;
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
