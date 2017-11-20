#pragma once

#include <QObject>
#include <QHostAddress>
#include <QLoggingCategory>

#include <jd-util/Json.h>

#include <commonlib/Record.h>
#include <commonlib/TableQuery.h>

#include "Async.h"

class QHostAddress;
class QTcpSocket;

namespace Sportsed {
namespace Common {
class MessageSocket;
class ChangeQuery;
class ChangeResponse;
}

namespace Client {
Q_DECLARE_LOGGING_CATEGORY(serverConnection)

class SocketWrapper;

DECLARE_EXCEPTION_X(AlreadyConnected, QStringLiteral("Already connected to server"), ::Exception)
DECLARE_EXCEPTION_X(NotConnected, QStringLiteral("Not connected to server"), ::Exception)

class ServerConnection : public QObject
{
	Q_OBJECT
protected:
	explicit ServerConnection(SocketWrapper *wrapper, QObject *parent = nullptr);

	void connectToServer(const QVariantList &args, const QString &name, const QString &password);

public:
	void disconnectFromServer();

	bool isConnected() const { return m_isConnected; }

signals:
	void status(const QString &msg);
	void connectedChanged(const bool connected);
	void connectionError();

public slots:
	Future<int> version();
	Future<Common::ChangeResponse> changes(const Common::ChangeQuery &query);
	Future<Common::Record> create(const Common::Record &record);
	Future<Common::Record> read(const Common::Table &table, const Common::Id &id);
	Future<Common::Revision> update(const Common::Record &record);
	Future<Common::Revision> delete_(const Common::Table &table, const Common::Id &id);
	Future<Common::Revision> delete_(const Common::Record &record);
	Future<QVector<Common::Record>> find(const Common::TableQuery &query);

	Subscribtion *subscribe(const Common::ChangeQuery &query);

protected:
	bool m_shouldBeConnected = false;
	bool m_authenticated = false;
	SocketWrapper *m_socket;
	QString m_name;
	QString m_password;
	int m_previousState;

	bool m_isConnected = false;
	void recalculateConnected();

	void received(const QByteArray &msg);

	std::shared_ptr<FutureImpl> sendMessage(const QString &cmd, const QJsonValue &value);
	int m_nextMsgId = 0;

	QHash<int, FutureImpl *> m_futures;
	friend class FutureImpl;
	void futureRemoved(const int msgId);

	QHash<int, Subscribtion *> m_subscriptions;
	QVector<Subscribtion *> m_pendingSubscriptions;
};
class TcpServerConnection : public ServerConnection
{
public:
	explicit TcpServerConnection(QObject *parent = nullptr);

	void connectToServer(const QHostAddress &addr, const QString &name, const QString &password);
};
class LocalServerConnection : public ServerConnection
{
public:
	explicit LocalServerConnection(QObject *parent = nullptr);

	void connectToServer(const QString &socketName, const QString &name, const QString &password);
};
class EmbeddedServerConnection : public ServerConnection
{
public:
	explicit EmbeddedServerConnection(QObject *parent = nullptr);

	void connectToServer(const QString &name, const QString &password);
};

}
}
