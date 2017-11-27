#include "DatabaseEngine.h"

#include <QSqlRecord>
#include <QSqlField>
#include <QSqlDriver>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
#include <QDebug>

#include <jd-util/Formatting.h>

#include "config.h"

using namespace JD::Util;

namespace Sportsed {
namespace Server {

static QPair<QString, QVector<QVariant>> whereForQuery(const Common::TableQuery &query)
{
	QStringList joins;
	QStringList items;
	items.append("1 = 1"); // eliminates the need for special casing an empty query in the caller
	QVector<QVariant> values;
	for (const Common::TableFilter &filter : query.filters()) {
		QString op;
		switch (filter.op()) {
		case Sportsed::Common::TableFilter::Equal: op = '='; break;
		case Sportsed::Common::TableFilter::NotEqual: op = "<>"; break;
		case Sportsed::Common::TableFilter::Less: op = '<'; break;
		case Sportsed::Common::TableFilter::LessEqual: op = "<="; break;
		case Sportsed::Common::TableFilter::Greater: op = ">"; break;
		case Sportsed::Common::TableFilter::GreaterEqual: op = ">="; break;
		}

		if (filter.field().contains('>')) {
			// special filtering in the format table_a_id>table_b_id>table_c_id that will use joins to match the table_a_id field
			// in the current table against the id field in table_a, table_b_id in table_a against table_b.id etc. and table_c_id
			// against the given value

			const QStringList fields = filter.field().split('>');
			QString join;
			for (int i = 0; i < fields.size(); ++i) {
				if (i != (fields.size()-1)) {
					const QString field = fields.at(i);
					const QString table = QString(field).remove("_id");
					const QString prevTable = i == 0 ? Common::tableName(query.table()) : QString(fields.at(i-1)).remove("_id");
					join += QStringLiteral(" JOIN %1 ON %1.id = %2.%3").arg(table, prevTable, field);
				}
			}
			joins.append(join);
			items.append(QStringLiteral("%1.%2 %3 ?").arg(QString(fields.at(fields.size()-2)).remove("_id"), fields.last(), op));
		} else {
			// normal filtering
			items.append(QStringLiteral("%1.%2 %3 ?").arg(Common::tableName(query.table()), filter.field(), op));
		}
		values.append(filter.value());
	}
	return qMakePair(joins.join(" ") + " WHERE " + items.join(" AND "), values);
}

DatabaseEngine::DatabaseEngine(QSqlDatabase &db)
	: m_db(db)
{
	if (db.tables().isEmpty()) {
		throw ValidationException("Database %1 (connection: %2) contains no fields" % m_db.databaseName() % m_db.connectionName());
	}
	Database::exec(db.exec(QStringLiteral("DELETE FROM %1 WHERE 1=1").arg(Common::tableName(Common::Table::Client))));
	Database::exec(db.exec(QStringLiteral("DELETE FROM %1 WHERE record_table = %2").arg(
							   Common::tableName(Common::Table::Change),
							   db.driver()->escapeIdentifier(Common::tableName(Common::Table::Client), QSqlDriver::TableName))));
}

Common::ChangeResponse DatabaseEngine::changes(const Common::ChangeQuery &query)
{
	Common::ChangeResponse response;
	response.setQuery(query);

	QString where = "1=0";
	QVector<QVariant> values;
	// TODO: extend filter implementation to all fields
	// QUESTION: should this filter apply to all fields? what if a field has changed?
	if (!query.query().isNull()) {
		where += " OR (record_table = ?";
		values.append(Common::tableName(query.query().table()));
		for (const Common::TableFilter &filter : query.query().filters()) {
			if (filter.field() == "id") {
				where += " AND record_id = ?";
				values.append(filter.value());
			}
		}
		where += ")";
	}

	QSqlQuery sqlQuery = Database::prepare(QStringLiteral("SELECT type,id,record_id,record_table,fields FROM %1 WHERE id > ? AND (%2) ORDER BY id ASC LIMIT 100")
										   % m_db.driver()->escapeIdentifier(Common::tableName(Common::Table::Change), QSqlDriver::TableName)
										   % where,
										   m_db);
	sqlQuery.addBindValue(query.fromRevision());
	for (const QVariant &value : values) {
		sqlQuery.addBindValue(value);
	}

	// TODO would it be possible to get a read-only lock?
	Database::TransactionLocker locker(m_db);
	const auto latest = Database::execOne(m_db.exec("SELECT id FROM %1 ORDER BY id DESC LIMIT 1"
													% m_db.driver()->escapeIdentifier(Common::tableName(Common::Table::Change), QSqlDriver::TableName)));
	Database::exec(sqlQuery);
	locker.rollback(); // no changes done, so might just as well roll back

	QVector<Common::Change> changes;
	while (sqlQuery.next()) {
		const QChar typeChar = sqlQuery.value(0).toString().at(0);
		Common::Change::Type type;
		switch (typeChar.toLatin1()) {
		case 'C': type = Common::Change::Create; break;
		case 'U': type = Common::Change::Update; break;
		case 'D': type = Common::Change::Delete; break;
		default: throw Database::DatabaseException("Unknown change type '%1'" % QString(typeChar));
		}

		Common::Change change(type);
		change.setRevision(sqlQuery.value(1).value<Common::Revision>());
		change.setRecord(read(Common::fromTableName(sqlQuery.value(3).toString()),
							  sqlQuery.value(2).value<Common::Id>(),
							  true));
		change.setUpdatedFields(sqlQuery.value(4).toString().split(',', QString::SkipEmptyParts).toVector());
		changes.append(change);
	}

	response.setChanges(changes);
	response.setLastRevision(latest.at(0).value<Common::Revision>());
	return response;
}

Common::Record DatabaseEngine::create(const Common::Record &record)
{
	if (record.isNull()) {
		throw ValidationException("Cannot create null record");
	}
	if (record.id() != 0) {
		throw ValidationException("Attempting to re-create existing record");
	}

#ifdef SPORTSED_DEBUG
	QSqlRecord table = m_db.record(Common::tableName(record.table()));
	validateValues(record, table, true);
#endif

	QStringList fields;
	QStringList values;
	for (auto it = record.values().constBegin(); it != record.values().constEnd(); ++it) {
		fields.append(m_db.driver()->escapeIdentifier(it.key(), QSqlDriver::FieldName));
		values.append("?");
	}

	QSqlQuery query = Database::prepare(QStringLiteral("INSERT INTO %1 (%2) VALUES (%3)").arg(
											m_db.driver()->escapeIdentifier(Common::tableName(record.table()), QSqlDriver::TableName),
											fields.join(','),
											values.join(',')),
										m_db);
	for (const QVariant &val : record.values()) {
		query.addBindValue(val);
	}


	Database::TransactionLocker locker(m_db);
	Database::exec(query);

	Common::Record inserted = record;
	inserted.setId(query.lastInsertId().value<Common::Id>());
	insertChange(Common::tableName(inserted.table()), inserted.id(), Common::Change::Create, inserted);
	locker.commit();

	return complete(inserted);
}

Common::Record DatabaseEngine::read(const Common::Table &table, const Common::Id id, const bool includeDeleted)
{
	const QVector<Common::Record> rows = find(Common::TableQuery(table, Common::TableFilter("id", id)), includeDeleted);
	if (rows.size() == 0) {
		throw Database::DoesntExistException();
	}
	return rows.first();
}

Common::Revision DatabaseEngine::update(const Common::Record &record)
{
	if (record.isNull()) {
		throw ValidationException("Cannot update null record");
	}
	if (record.id() == 0) {
		throw ValidationException("Attempting to update a missing record");
	}

	QSqlRecord table = m_db.record(Common::tableName(record.table()));

#ifdef SPORTSED_DEBUG
	validateValues(record, table, false);
#endif

	QStringList fields;
	QVector<QVariant> values;
	for (auto it = record.values().constBegin(); it != record.values().constEnd(); ++it) {
		fields.append(m_db.driver()->escapeIdentifier(it.key(), QSqlDriver::FieldName) + " = ?");
		values.append(it.value());
	}

	QSqlQuery query = Database::prepare(QStringLiteral("UPDATE %1 SET %2 WHERE id = %3 AND _deleted_ = 0")
										% m_db.driver()->escapeIdentifier(Common::tableName(record.table()), QSqlDriver::TableName)
										% fields.join(',')
										% record.id(),
										m_db);
	for (const QVariant &value : values) {
		query.addBindValue(value);
	}

	Database::TransactionLocker locker(m_db);
	Database::exec(query);
	const Common::Revision revision = insertChange(Common::tableName(record.table()), record.id(), Common::Change::Update, record);
	locker.commit();

	return revision;
}

Common::Revision DatabaseEngine::delete_(const Common::Table &table, const Common::Id id)
{
	Common::Record record(table);
	record.setId(id);
	record = complete(record);

	QSqlQuery query = Database::prepare("UPDATE %1 SET _deleted_ = 1 WHERE id = ?"
										% m_db.driver()->escapeIdentifier(Common::tableName(table), QSqlDriver::TableName),
										m_db);
	query.addBindValue(id);

	Database::TransactionLocker locker(m_db);
	Database::exec(query);
	const Common::Revision revision = insertChange(Common::tableName(table),
												   id,
												   Common::Change::Delete,
												   record);
	locker.commit();

	return revision;
}

QVector<Common::Record> DatabaseEngine::find(const Common::TableQuery &query, const bool includeDeleted)
{
	const auto where = whereForQuery(query);

	const QString tableName = m_db.driver()->escapeIdentifier(Common::tableName(query.table()), QSqlDriver::TableName);
	QSqlQuery sql = Database::prepare(
				QStringLiteral("SELECT %1.*, (SELECT change.id FROM change WHERE change.record_id = %1.id AND change.record_table = %1 ORDER BY change.id DESC LIMIT 1) AS _latest_revision_ FROM %1 %2 %3") %
				tableName %
				where.first %
				(includeDeleted ? "" : QStringLiteral(" AND %1._deleted_ = 0").arg(tableName)),
				m_db);
	for (const QVariant &val : where.second) {
		sql.addBindValue(val);
	}
	Database::exec(sql);

	QVector<Common::Record> records;
	while (sql.next()) {
		Common::Record record;
		record.setTable(query.table());

		const QSqlRecord sqlRecord = sql.record();
		QHash<QString, QVariant> values;
		for (int i = 0; i < sqlRecord.count(); ++i) {
			if (sqlRecord.fieldName(i) == "id") {
				record.setId(sqlRecord.value(i).value<Common::Id>());
			} else if (sqlRecord.fieldName(i) == "_latest_revision_") {
				record.setLatestRevision(sqlRecord.value(i).value<Common::Revision>());
			} else if (!sqlRecord.fieldName(i).startsWith('_')) {
				values.insert(sqlRecord.fieldName(i), sqlRecord.value(i));
			}
		}
		record.setValues(values);

		record.setComplete(true);
		records.append(record);
	}

	return records;
}

Common::Record DatabaseEngine::complete(const Common::Record &record)
{
	if (record.isComplete()) {
		return record;
	}
	if (record.isNull()) {
		throw ValidationException("Attempting to complete a null record");
	}
	if (!record.isPersisted()) {
		throw ValidationException("Record does not exist in database (missing id)");
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

	QSqlQuery query = Database::prepare("INSERT INTO %1 (record_table, record_id, timestamp, fields, type) VALUES (?,?,?,?,?)"
										% Common::tableName(Common::Table::Change), m_db);
	query.addBindValue(table);
	query.addBindValue(id);
	query.addBindValue(QDateTime::currentMSecsSinceEpoch());
	if (record.values().isEmpty() || type != Common::Change::Type::Update) {
		query.addBindValue(QVariant(QVariant::String));
	} else {
		query.addBindValue(record.values().keys().join(','));
	}
	query.addBindValue(typeChar);
	Database::exec(query);

	Common::Change change{type};
	change.setRevision(query.lastInsertId().value<Common::Revision>());
	change.setRecord(complete(record));
	if (type == Common::Change::Type::Update) {
		change.setUpdatedFields(record.values().keys().toVector());
	}
	if (m_changeCb) {
		m_changeCb(change);
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
		if (field.name().startsWith('_')) {
			continue;
		}
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
