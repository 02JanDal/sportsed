#include "DatabaseMigration.h"

#include <QSqlDatabase>
#include <QSqlRecord>
#include <QSqlQuery>
#include <QVector>
#include <QVariant>

#include <jd-util-sql/DatabaseUtil.h>

using namespace JD::Util;

namespace Sportsed {
namespace Server {

static QVector<QString> createStatements(QSqlDatabase &db)
{
	Database::Dialect dialect(db);

	const QString idField = dialect.idField(Database::Dialect::Big);

	return {
		"CREATE TABLE meta (%1, key VARCHAR(64) NOT NULL, value VARCHAR(256))" % idField,
		"CREATE TABLE change (%1, type CHAR(1) NOT NULL, record_id INT NOT NULL, record_table VARCHAR(32), timestamp INT NOT NULL, fields VARCHAR(256) DEFAULT NULL)"
							   % idField,
		"CREATE TABLE profile (%1, type VARCHAR(16) NOT NULL, name VARCHAR(64) NOT NULL, value TEXT NOT NULL)" % idField
	};
}
static QVector<QString> extractFields(const QString &statement)
{
	QVector<QString> fields;
	const int firstParan = statement.indexOf('(');
	fields.append(statement.mid(firstParan+1, statement.indexOf(' ', firstParan)-firstParan).trimmed());

	int index = 0;
	while (index < statement.size()) {
		const int nextComma = statement.indexOf(',', index);
		if (nextComma == -1) {
			break;
		}
		const int nextSpace = statement.indexOf(' ', nextComma+2);
		fields.append(statement.mid(nextComma+1, nextSpace - nextComma).trimmed());
		index = nextSpace;
	}

	return fields;
}

void DatabaseMigration::create(QSqlDatabase &db)
{
	Database::TransactionLocker locker(db);

	for (const QString &statement : createStatements(db)) {
		Database::exec(db.exec(statement));
	}

	 Database::exec(db.exec(QStringLiteral("INSERT INTO meta (key, value) VALUES ('version', %1)") % latestVersion()));

	locker.commit();
}

void DatabaseMigration::check(QSqlDatabase &db)
{
	const int preambleSize = QStringLiteral("CREATE TABLE ").size();

	const QStringList tables = db.tables();
	for (const QString &statement : createStatements(db)) {
		const QString name = statement.mid(preambleSize, statement.indexOf(' ', preambleSize+1) - preambleSize).trimmed();
		if (!tables.contains(name)) {
			throw DatabaseCheckException("Missing table '%1'" % name);
		}
		const QSqlRecord record = db.record(name);
		const QVector<QString> fields = extractFields(statement);
		for (const QString &field : fields) {
			if (!record.contains(field)) {
				throw DatabaseCheckException("Table '%1' is missing the following field: '%2'" % name % field);
			}
		}
	}
}

void DatabaseMigration::upgrade(QSqlDatabase &db)
{
	Database::TransactionLocker locker(db);

	const int from = currentVersion(db);
	const int to = latestVersion();
	if (from >= to) {
		return;
	}

	Database::exec(db.exec(QStringLiteral("UPDATE meta SET value = %1 WHERE key = 'version'") % latestVersion()));

	locker.commit();
}

int16_t DatabaseMigration::currentVersion(QSqlDatabase &db)
{
	if (db.record("meta").isEmpty()) {
		return -1;
	}
	return Database::execOne(db.exec("SELECT value FROM meta WHERE key = 'version'")).first().value<int16_t>();
}

int DatabaseMigration::latestVersion()
{
	return 1;
}

}
}
