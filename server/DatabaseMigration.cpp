#include "DatabaseMigration.h"

#include <QSqlDatabase>
#include <QSqlRecord>
#include <QSqlQuery>
#include <QVector>
#include <QVariant>
#include <QDebug>

#include <jd-util-sql/DatabaseUtil.h>
#include <jd-util/Functional.h>
#include <jd-util/TermUtil.h>

using namespace JD::Util;

namespace Sportsed {
namespace Server {

struct Table
{
	template <typename... Fields>
	explicit Table(const QString &name, Fields... fields)
		: sql("CREATE TABLE %1 (__id__, %2)" % name % QStringList({fields...}).join(", ").replace("FK", "__fk__")) {}
	QString sql;
};

static QVector<QString> createStatements(QSqlDatabase &db)
{
	Database::Dialect dialect(db);

	const QString idField = dialect.idField(Database::Dialect::Big);
	const QString fkFieldType = dialect.fkFieldType(Database::Dialect::Big);

	const QVector<Table> tables = {
		Table("meta", "key VARCHAR(64) NOT NULL", "value VARCHAR(256)"),
		Table("change", "type CHAR(1) NOT NULL", "record_id INT NOT NULL", "record_table VARCHAR(32) NOT NULL",
			  "timestamp INT NOT NULL", "fields VARCHAR(256) DEFAULT NULL"),
		Table("profile", "type VARCHAR(16) NOT NULL", "name VARCHAR(64) NOT NULL", "value TEXT NOT NULL"),
		Table("client", "name VARCHAR(64) NOT NULL", "ip VARCHAR(32) NOT NULL"),
		Table("competition", "name VARCHAR(128) NOT NULL", "sport VARCHAR(64) NOT NULL"),
		Table("stage", "competition_id FK NOT NULL", "name VARCHAR(128) NOT NULL", "type VARCHAR(64) NOT NULL", "discipline VARCHAR(64) NOT NULL", "date DATE NOT NULL", "in_totals BOOLEAN NOT NULL DEFAULT TRUE"),
		Table("control", "stage_id FK", "name VARCHAR(5) NOT NULL", "special VARCHAR(256) NOT NULL DEFAULT ''"),
		Table("course", "stage_id FK", "name VARCHAR(64) NOT NULL"),
		Table("course_control", "control_id FK", "course_id FK", "'order' INTEGER NOT NULL", "distance_from_previous DOUBLE"),
		Table("class", "stage_id FK", "name VARCHAR(64)")
	};

	return Functional::collection(tables).map([idField, fkFieldType](const Table &t) {
		return QString(t.sql).replace("__id__", idField).replace("__fk__", fkFieldType);
	});
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
		fields.append(statement.mid(nextComma+1, nextSpace - nextComma).remove('\'').trimmed());
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

void DatabaseMigration::prepare(QSqlDatabase &db, const bool forceMigrate)
{
	const int version = currentVersion(db);
	if (version == -1) {
		qInfo() << Term::fg(Term::Blue, "Database schema does not exist, it will now be created.");
		create(db);
		qInfo() << Term::fg(Term::Green, "Database schema successfully created.");
	} else if (version < latestVersion()) {
		if (forceMigrate || Term::askBoolean("Database schema is out-of-date. Migrate to newer version?")) {
			upgrade(db);
		} else {
			throw Exception("Refusing to operate on old database schema.");
		}
	} else if (version > latestVersion()) {
		throw Exception("Database schema is of newer version than this program is familiar with. Please update sportsed_server to a newer version.");
	} else {
		check(db);
		qInfo() << Term::fg(Term::Green, "Database schema check successful, schema is up-to-date.");
	}
}

}
}
