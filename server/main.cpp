#include <QCoreApplication>
#include <QCommandLineParser>
#include <QSqlDatabase>
#include <QSqlError>

#include <jd-util/TermUtil.h>
#include <jd-util/Logging.h>
#include <iostream>

#include "DatabaseServer.h"
#include "DatabaseMigration.h"
#include "config.h"

using namespace JD::Util;
using namespace Sportsed::Server;

int main(int argc, char **argv)
{
	installLogFormatter();

	QCoreApplication app(argc, argv);
	app.setApplicationName("sportsed_server");
	app.setApplicationVersion(SPORTSED_VERSION);
	app.setOrganizationName("JanDalheimer");
	app.setOrganizationDomain("02jandal.xyz");

	QCommandLineParser parser;
	parser.addHelpOption();
	parser.addVersionOption();
	parser.addOption(QCommandLineOption({"p", "port"}, "Which port to listen on", "PORT", "7384"));
	parser.addOption(QCommandLineOption("password", "Password to access the server", "PASSWORD", "password"));
	parser.addOption(QCommandLineOption("db-type", "Type of database used (one of: psql, mysql, sqlite)", "TYPE", "psql"));
	parser.addOption(QCommandLineOption("db-host", "Adress of the database to connect to", "IP", "localhost"));
	parser.addOption(QCommandLineOption("db-port", "Port of the database to connect to", "PORT", "5432"));
	parser.addOption(QCommandLineOption("db-user", "Username for authenticating with the database", "USERNAME", "root"));
	parser.addOption(QCommandLineOption("db-pass", "Password for authenticating with the database", "PASSWORD", ""));
	parser.addOption(QCommandLineOption("db-name", "Name of the database to use", "NAME", "sportsed"));

	parser.process(app);

	QString driverName;
	const QString dbType = parser.value("db-type").toLower();
	if (dbType == "psql") {
		driverName = "QPSQL";
	} else if (dbType == "mysql") {
		driverName = "QMYSQL";
	} else if (dbType == "sqlite") {
		driverName = "QSQLITE";
	} else {
		std::cerr << Term::fg(Term::Red, "Invalid database type '%1', should be one of 'psql', 'mysql' or 'sqlite'\n" % dbType);
		return -1;
	}

	QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL");
	db.setHostName(parser.value("db-host"));
	db.setPort(parser.value("db-port").toInt());
	db.setUserName(parser.value("db-user"));
	db.setPassword(parser.value("db-pass"));
	db.setDatabaseName(parser.value("db-name"));
	if (!db.open()) {
		std::cerr << Term::fg(Term::Red, "Unable to connect to database: ") << Term::fg(Term::Magenta, db.lastError().text()) << std::endl;
		return -1;
	}

	try {
		const int version = DatabaseMigration::currentVersion(db);
		if (version == -1) {
			std::cout << Term::fg(Term::Blue, "Database schema does not exist, it will now be created.\n");
			DatabaseMigration::create(db);
			std::cout << Term::fg(Term::Green, "Database schema successfully created.\n");
		} else if (version < DatabaseMigration::latestVersion()) {
			if (Term::askBoolean("Database schema is out-of-date. Migrate to newer version?")) {
				DatabaseMigration::upgrade(db);
			} else {
				throw Exception("Refusing to operate on old database schema.");
			}
		} else if (version > DatabaseMigration::latestVersion()) {
			throw Exception("Database schema is of newer version than this program is familiar with. Please update sportsed_server to a newer version.");
		} else {
			DatabaseMigration::check(db);
			std::cout << Term::fg(Term::Green, "Database schema check successful, schema is up-to-date.\n");
		}
	} catch (Exception &e) {
		std::cerr << Term::fg(Term::Red, e.cause() + '\n');
		return -1;
	}

	DatabaseServer server(db, parser.value("password"));
	if (!server.listen(QHostAddress::Any, 4829)) {
		std::cerr << Term::fg(Term::Red, server.errorString() + '\n');
		return -1;
	}

	return app.exec();
}
