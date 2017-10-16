#pragma once

#include <QSqlDatabase>
#include <QTcpServer>

#include "DatabaseEngine.h"

namespace Sportsed {
namespace Server {

class Connection;

class DatabaseServer : public QTcpServer
{
public:
	explicit DatabaseServer(QSqlDatabase &db, const QString &password);
	virtual ~DatabaseServer() override;

protected:
	void incomingConnection(qintptr handle) override;

private:
	QSqlDatabase m_db;
	QString m_password;
	DatabaseEngine m_engine;

	QVector<Connection *> m_connections;
};

}
}
