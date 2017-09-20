#include <catch.hpp>

#include <QSqlDatabase>

#include "DatabaseMigration.h"

QSqlDatabase inMemoryDb()
{
	QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
	db.setDatabaseName(":memory:");
	return db;
}

SECTION("create database") {
	REQUIRE_NOTHROW
}
