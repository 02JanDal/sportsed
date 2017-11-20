#include <QCoreApplication>
#include <QCommandLineParser>
#include <QSqlDatabase>
#include <QSqlError>
#include <QDir>

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
	parser.addOption(QCommandLineOption("debug", "Use a debugging friendly db setup"));

	parser.process(app);

	QString driverName;
	const QString dbType = parser.isSet("debug") ? "sqlite" : parser.value("db-type").toLower();
	if (dbType == "psql") {
		driverName = "QPSQL";
	} else if (dbType == "mysql") {
		driverName = "QMYSQL";
	} else if (dbType == "sqlite") {
		driverName = "QSQLITE";
	} else {
		qCritical() << Term::fg(Term::Red, "Invalid database type '%1', should be one of 'psql', 'mysql' or 'sqlite'\n" % dbType);
		return -1;
	}

	QSqlDatabase db = QSqlDatabase::addDatabase(driverName);
	if (parser.isSet("debug")) {
		db.setDatabaseName(QDir::current().absoluteFilePath("sportsed_debug.sqlite"));
	} else {
		db.setHostName(parser.value("db-host"));
		db.setPort(parser.value("db-port").toInt());
		db.setUserName(parser.value("db-user"));
		db.setPassword(parser.value("db-pass"));
		db.setDatabaseName(parser.value("db-name"));
	}
	if (!db.open()) {
		qCritical() << Term::fg(Term::Red, "Unable to connect to database: ") << Term::fg(Term::Magenta, db.lastError().text());
		return -1;
	}

	try {
		DatabaseMigration::prepare(db, false);
	} catch (Exception &e) {
		qCritical() << Term::fg(Term::Red, e.cause() + '\n');
		return -1;
	}

	TcpDatabaseServer server(db, parser.value("password"));
	if (!server.listen()) {
		qCritical() << Term::fg(Term::Red, server.errorString());
		return -1;
	}
	qInfo() << Term::fg(Term::Green, QStringLiteral("Server is now listening on 0.0.0.0:%1") % server.serverPort());

	return app.exec();
}
