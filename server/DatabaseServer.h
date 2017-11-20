#pragma once

#include <QSqlDatabase>
#include <QTcpServer>
#include <QLocalServer>
#include <QLoggingCategory>

#include "DatabaseEngine.h"

namespace Sportsed {
namespace Server {

class Connection;

Q_DECLARE_LOGGING_CATEGORY(server)

class DatabaseServer
{
public:
	explicit DatabaseServer(QSqlDatabase &db, const QString &password);
	virtual ~DatabaseServer();

	virtual bool listen() = 0;

protected:
	void addConnection(Connection *conn, QObject *slotCtxt);

private:
	QSqlDatabase m_db;
	QString m_password;
	DatabaseEngine m_engine;

	QVector<Connection *> m_connections;

	QHash<QString, std::function<QJsonValue(QJsonValue, DatabaseEngine&, Connection *)>> m_commands;

	void handleChange(const Common::Change &change);
};

class TcpDatabaseServer : public QTcpServer, public DatabaseServer
{
public:
	explicit TcpDatabaseServer(QSqlDatabase &db, const QString &password);
	virtual ~TcpDatabaseServer() {}

	bool listen() override;

protected:
	void incomingConnection(qintptr handle) override;
};

class LocalDatabaseServer : public QLocalServer, public DatabaseServer
{
public:
	explicit LocalDatabaseServer(QSqlDatabase &db, const QString &password);
	virtual ~LocalDatabaseServer() {}

	void setSocketName(const QString &name) { m_socketName = name; }
	bool listen() override;

protected:
	void incomingConnection(quintptr socketDescriptor) override;

private:
	QString m_socketName;
};

class ByteArraySender : public QObject
{
	Q_OBJECT
public:
	explicit ByteArraySender(QObject *parent = nullptr);

	static ByteArraySender *instance;

public slots:
	void sendToClient(const QByteArray &data) { emit clientReceived(data); }
	void sendToServer(const QByteArray &data) { emit serverReceived(data); }

signals:
	void clientReceived(const QByteArray &data);
	void serverReceived(const QByteArray &data);
};
class EmbeddedDatabaseServer : public QObject, DatabaseServer
{
public:
	explicit EmbeddedDatabaseServer(QSqlDatabase &db, const QString &password);

	ByteArraySender *setup();
	bool listen() override;
};

}
}
