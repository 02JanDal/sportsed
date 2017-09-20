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

void DatabaseMigration::create(QSqlDatabase &db)
{
	Database::Dialect dialect(db);

	Database::TransactionLocker locker(db);
	Database::exec(db.exec("CREATE TABLE meta (key VARCHAR(64) NOT NULL, value VARCHAR(256))"));
	Database::exec(db.exec("CREATE TABLE changes (%1, type CHAR(1) NOT NULL, record_id INT NOT NULL, record_table VARCHAR(32), fields VARCHAR(256) DEFAULT NULL)"
						   % dialect.idField(Database::Dialect::Big)));
	locker.commit();
}

void DatabaseMigration::check(QSqlDatabase &db)
{
	const QStringList tables = db.tables();
	if (!tables.contains("meta") ||
			!tables.contains("changes")) {
		throw DatabaseCheckException("Missing table(s)");
	}

	const QSqlRecord meta = db.record("meta");
	if (!meta.contains("key") ||
			!meta.contains("value")) {
		throw DatabaseCheckException("Table 'meta' missing fields");
	}

	const QSqlRecord changes = db.record("changes");
	if (!changes.contains("id") ||
			!changes.contains("record_id") ||
			!changes.contains("record_table") ||
			!changes.contains("fields")) {
		throw DatabaseCheckException("Table 'changes' missing fields");
	}
}

void DatabaseMigration::upgrade(QSqlDatabase &db)
{
	const int from = currentVersion(db);
	const int to = latestVersion();
	if (from >= to) {
		return;
	}
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
