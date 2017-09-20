#pragma once

#include <jd-util/Exception.h>

class QSqlDatabase;

namespace Sportsed {
namespace Server {
namespace DatabaseMigration {

DECLARE_EXCEPTION(DatabaseMigration)
DECLARE_EXCEPTION(DatabaseCheck)

void create(QSqlDatabase &db);
void check(QSqlDatabase &db);
void upgrade(QSqlDatabase &db);
int16_t currentVersion(QSqlDatabase &db);
int latestVersion();

}
}
}
