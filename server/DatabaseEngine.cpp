#include "DatabaseEngine.h"

#include <QSqlRecord>
#include <QSqlField>
#include <QSqlDriver>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>

#include <jd-util/Formatting.h>

#include "config.h"

using namespace JD::Util;

namespace Sportsed {
namespace Server {

DatabaseEngine::DatabaseEngine(QSqlDatabase &db)
	: m_db(db)
{
}

Common::ChangeResponse DatabaseEngine::changes(const Common::ChangeQuery &query) const
{
	Common::ChangeResponse response;
	response.setQuery(query);

	QString where;
	QVector<QVariant> values;

	QSqlQuery sqlQuery = Database::prepare(QStringLiteral("SELECT type,id,record_id,record_table,fields FROM changes WHERE id > ? AND %1 ORDER BY id ASC LIMIT 100")
										   % where);
	sqlQuery.addBindValue(query.fromRevision());
	for (const QVariant &value : values) {
		sqlQuery.addBindValue(value);
	}
	Database::exec(sqlQuery);

	QVector<Common::Change> changes;
	while (sqlQuery.next()) {
		const QChar typeChar = sqlQuery.value(0).toChar();
		Common::Change::Type type;
		switch (typeChar.toLatin1()) {
		case 'C': type = Common::Change::Create; break;
		case 'U': type = Common::Change::Update; break;
		case 'D': type = Common::Change::Delete; break;
		}

		Common::Change change(type);
		change.setRevision(sqlQuery.value(1).value<Common::Revision>());
		change.setId(sqlQuery.value(2).value<Common::Id>());
		change.setTable(sqlQuery.value(3).toString());
		change.setUpdatedFields(sqlQuery.value(4).toString().split(',').toVector());
		changes.append(change);
	}

	response.setChanges(changes);
	return response;
}

Common::Record DatabaseEngine::create(const Common::Record &record)
{
	if (record.table().isEmpty()) {
		throw ValidationException("Missing table name");
	}
	if (record.id() != 0) {
		throw ValidationException("Attempting to re-create existing record");
	}

	QSqlRecord table = m_db.record(record.table());

#ifdef SPORTSED_DEBUG
	validateValues(record, table, true);
#endif

	QStringList fields;
	QStringList values;
	for (auto it = record.values().constBegin(); it != record.values().constEnd(); ++it) {
		fields.append(m_db.driver()->escapeIdentifier(it.key(), QSqlDriver::FieldName));
		values.append("?");
	}

	QSqlQuery query = Database::prepare(QStringLiteral("INSERT INTO %1 (%2) VALUES (%3)").arg(
											m_db.driver()->escapeIdentifier(record.table(), QSqlDriver::TableName),
											fields.join(','),
											values.join(',')),
										m_db);
	for (const QString &field : fields) {
		query.addBindValue(record.values().value(field));
	}

	Database::TransactionLocker locker(m_db);
	Database::exec(query);
	Common::Record inserted = record;
	inserted.setId(query.lastInsertId().value<Common::Id>());
	inserted.setLatestRevision(insertChange(record.table(), record.id(), Common::Change::Create, inserted));
	locker.commit();

	return inserted;
}

Common::Record DatabaseEngine::read(const QString &table, const Common::Id id)
{
	// TODO: transform into a single query that reads both the record and latest revision id

	QSqlQuery query = Database::prepare(QStringLiteral("SELECT * FROM %1 WHERE id = ?") %
										m_db.driver()->escapeIdentifier(table, QSqlDriver::TableName));
	query.addBindValue(id);
	Database::execOne(query);

	QSqlQuery changeQuery = Database::prepare("SELECT id FROM changes WHERE record_id = ? AND record_table = ? ORDER BY id DESC LIMIT 1");
	changeQuery.addBindValue(id);
	const Common::Revision latestRev = Database::execOne(changeQuery).at(0).value<Common::Revision>();

	Common::Record record;
	record.setId(id);
	record.setLatestRevision(latestRev);
	record.setTable(table);

	const QSqlRecord sqlRecord = query.record();
	QHash<QString, QVariant> values;
	for (int i = 0; i < sqlRecord.count(); ++i) {
		values.insert(sqlRecord.fieldName(i), sqlRecord.value(i));
	}
	record.setValues(values);

	record.setComplete(true);

	return record;
}

Common::Revision DatabaseEngine::update(const Common::Record &record)
{
	if (record.table().isEmpty()) {
		throw ValidationException("Missing table name");
	}
	if (record.id() == 0) {
		throw ValidationException("Attempting to update a missing record");
	}

	QSqlRecord table = m_db.record(record.table());

#ifdef SPORTSED_DEBUG
	validateValues(record, table, false);
#endif

	QStringList fields;
	QVector<QVariant> values;
	for (auto it = record.values().constBegin(); it != record.values().constEnd(); ++it) {
		fields.append(m_db.driver()->escapeIdentifier(it.key(), QSqlDriver::FieldName) + " = ?");
		values.append(it.value());
	}

	QSqlQuery query = Database::prepare(QStringLiteral("UPDATE %1 SET %2 WHERE id = %3")
										% m_db.driver()->escapeIdentifier(record.table(), QSqlDriver::TableName)
										% fields.join(',')
										% record.id(),
										m_db);
	for (const QVariant &value : values) {
		query.addBindValue(value);
	}

	Database::TransactionLocker locker(m_db);
	Database::exec(query);
	const Common::Revision revision = insertChange(record.table(), record.id(), Common::Change::Update, record);
	locker.commit();

	return revision;
}

Common::Revision DatabaseEngine::delete_(const QString &table, const Common::Id id)
{
	QSqlQuery query = Database::prepare(QStringLiteral("DELETE FROM %1 WHERE id = ?").arg(
											m_db.driver()->escapeIdentifier(table, QSqlDriver::TableName)));
	query.addBindValue(id);

	Database::TransactionLocker locker(m_db);
	Database::exec(query);
	const Common::Revision revision = insertChange(table, id, Common::Change::Delete);
	locker.commit();

	return revision;
}

Common::Record DatabaseEngine::complete(const Common::Record &record)
{
	if (record.isComplete()) {
		return record;
	}
	return read(record.table(), record.id());
}

Common::Revision DatabaseEngine::insertChange(const QString &table, const Common::Id id,
											  const Common::Change::Type type, const Common::Record &record)
{
	QChar typeChar;
	switch (type) {
	case Sportsed::Common::Change::Create: typeChar = 'C'; break;
	case Sportsed::Common::Change::Update: typeChar = 'U'; break;
	case Sportsed::Common::Change::Delete: typeChar = 'D'; break;
	}

	QSqlQuery query = Database::prepare("INSERT INTO changes (record_table, record_id, timestamp, fields, type) VALUES (?,?,?,?,?)");
	query.addBindValue(table);
	query.addBindValue(id);
	query.addBindValue(QDateTime::currentMSecsSinceEpoch());
	if (record.values().isEmpty()) {
		query.addBindValue(QVariant(QVariant::String));
	} else {
		query.addBindValue(record.values().keys().join(','));
	}
	query.addBindValue(typeChar);
	Database::exec(query);

	Common::Change change{type};
	change.setId(id);
	change.setRevision(query.lastInsertId().value<Common::Revision>());
	change.setTable(table);
	change.setUpdatedFields(record.values().keys().toVector());
	if (m_changeCb) {
		m_changeCb(change, complete(record));
	}

	return change.revision();
}

void DatabaseEngine::validateValues(const Common::Record &record, const QSqlRecord &sqlRecord, const bool strict)
{
	if (record.values().contains("id")) {
		throw ValidationException("Attempting to explicitly set ID field");
	}

	for (int i = 0; i < sqlRecord.count(); ++i) {
		const QSqlField field = sqlRecord.field(i);
		if (field.isAutoValue() && record.values().contains(field.name())) {
			throw ValidationException(QStringLiteral("Attempting to explicitly set auto-valued field '%1'") % field.name());
		} else if (field.requiredStatus() == QSqlField::Required && !record.values().contains(field.name()) && strict) {
			throw ValidationException(QStringLiteral("Missing required field '%1'") % field.name());
		}
	}

	for (const QString &name : record.values().keys()) {
		if (!sqlRecord.contains(name)) {
			throw ValidationException(QStringLiteral("Attempting to set non-existent field '%1'") % name);
		}
	}
}

}
}
