#include <QStringList>
#include <QSet>
#include <QDebug>

#include <tst_Util.h>

#include "DatabaseMigration.h"

using namespace Sportsed::Server;

TEST_CASE("create database") {
	QSqlDatabase db = inMemoryDb();
	REQUIRE_NOTHROW(DatabaseMigration::create(db));
	REQUIRE(db.tables().contains("meta"));
	REQUIRE(db.tables().contains("profile"));
	REQUIRE(db.tables().contains("change"));
}

TEST_CASE("check database") {
	QSqlDatabase db = inMemoryDb();
	REQUIRE_NOTHROW(DatabaseMigration::create(db));
	REQUIRE_NOTHROW(DatabaseMigration::check(db));

	if (db.driverName() != "QSQLITE") {
		SECTION("missing field") {
			REQUIRE_FALSE(db.exec("ALTER TABLE meta DROP COLUMN key").lastError().isValid());
			REQUIRE_THROWS_AS(DatabaseMigration::check(db), DatabaseMigration::DatabaseCheckException);
		}
	}

	SECTION("missing table") {
		QSqlDatabase db2 = inMemoryDb();
		REQUIRE_NOTHROW(DatabaseMigration::create(db2));
		REQUIRE_NOTHROW(DatabaseMigration::check(db2));
		REQUIRE_FALSE(db2.exec("DROP TABLE meta").lastError().isValid());
		REQUIRE_THROWS_AS(DatabaseMigration::check(db2), DatabaseMigration::DatabaseCheckException);
	}
}

TEST_CASE("database version") {
	QSqlDatabase db = inMemoryDb();
	REQUIRE(DatabaseMigration::currentVersion(db) == -1);
	REQUIRE_NOTHROW(DatabaseMigration::create(db));
	REQUIRE(DatabaseMigration::latestVersion() == 1);
	REQUIRE(DatabaseMigration::currentVersion(db) == DatabaseMigration::latestVersion());
}

TEST_CASE("upgrade database") {
	SECTION("no upgrade needed") {
		QSqlDatabase db = inMemoryDb();
		REQUIRE_NOTHROW(DatabaseMigration::create(db));
		REQUIRE(DatabaseMigration::currentVersion(db) == DatabaseMigration::latestVersion());
		REQUIRE_NOTHROW(DatabaseMigration::upgrade(db));
		REQUIRE(DatabaseMigration::currentVersion(db) == DatabaseMigration::latestVersion());
	}

	// TODO add upgrade checks here
}
