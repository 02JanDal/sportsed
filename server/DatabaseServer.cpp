#include "DatabaseServer.h"

#include <QTcpSocket>
#include <QLocalSocket>

#include <jd-util/Json.h>

#include "commonlib/ChangeQuery.h"
#include "commonlib/ChangeResponse.h"
#include "commonlib/MessageSocket.h"
#include "DatabaseMigration.h"

using namespace JD::Util;

namespace Sportsed {
namespace Server {
Q_LOGGING_CATEGORY(server, "sportsed.server.server")

class Connection : public QObject
{
	Q_OBJECT
public:
	explicit Connection(QIODevice *socketDevice, QObject *parent)
		: QObject(parent), socket(new Common::MessageSocket(socketDevice))
	{
		socket->setParent(this);
		connect(socket, &Common::MessageSocket::message, this, &Connection::message);
	}

	bool authenticated = false;
	QString name;
	Common::Id clientRecordId;

	Common::MessageSocket *socket;

	QHash<int, Common::ChangeQuery> subscriptions;
	int nextSubscriptionId = 1;

	virtual void send(const QByteArray &msg)
	{
		socket->send(msg);
	}
	virtual QString address() const = 0;

	Common::Record asClientRecord() const
	{
		return Common::Record(Common::Table::Client, QHash<QString, QVariant>({
								  {"name", name},
								  {"ip", address()}
							  }));
	}

signals:
	void message(const QByteArray &data);
	void disconnected();
};

class TcpConnection : public Connection
{
	static QTcpSocket *makeSocket(qintptr handle, QObject *parent)
	{
		QTcpSocket *socket = new QTcpSocket(parent);
		socket->setSocketDescriptor(handle);
		return socket;
	}
public:
	explicit TcpConnection(qintptr handle, QObject *parent)
		: Connection(makeSocket(handle, this), parent),
		  tcpSocket(qobject_cast<QTcpSocket *>(socket->device()))
	{
		connect(tcpSocket, &QTcpSocket::stateChanged, this, [this](const QTcpSocket::SocketState state) {
			if (state == QAbstractSocket::UnconnectedState) {
				emit disconnected();
				deleteLater();
			}
		});
	}

	QString address() const override { return tcpSocket->peerAddress().toString(); }

	QTcpSocket *tcpSocket;
};
class LocalConnection : public Connection
{
	static QLocalSocket *makeSocket(quintptr handle, QObject *parent)
	{
		QLocalSocket *socket = new QLocalSocket(parent);
		socket->setSocketDescriptor(static_cast<qintptr>(handle));
		return socket;
	}
public:
	explicit LocalConnection(quintptr handle, QObject *parent)
		: Connection(makeSocket(handle, parent), parent),
		  localSocket(qobject_cast<QLocalSocket *>(socket->device()))
	{
		connect(localSocket, &QLocalSocket::stateChanged, this, [this](const QLocalSocket::LocalSocketState state) {
			qDebug(server) << state;
			if (state == QLocalSocket::UnconnectedState) {
				emit disconnected();
				deleteLater();
			}
		});
	}

	QString address() const override { return tr("local"); }

	QLocalSocket *localSocket;
};
class EmbeddedConnection : public Connection
{
	ByteArraySender *sender;
public:
	explicit EmbeddedConnection(ByteArraySender *s, QObject *parent)
		: Connection(nullptr, parent), sender(s)
	{
		connect(sender, &ByteArraySender::serverReceived, this, &EmbeddedConnection::message);
	}


	void send(const QByteArray &msg) override
	{
		sender->sendToClient(msg);
	}
	QString address() const override { return tr("Embedded"); }
};

DatabaseServer::DatabaseServer(QSqlDatabase &db, const QString &password)
	: m_db(db), m_password(password), m_engine(db)
{
	m_engine.setChangeCallback([this](const Common::Change &change) { handleChange(change); });

	m_commands.insert("version", [this](QJsonValue, DatabaseEngine &, Connection *) -> QJsonValue {
		return DatabaseMigration::currentVersion(m_db);
	});
	m_commands.insert("changes", [](const QJsonValue &data, DatabaseEngine &engine, Connection *) {
		const Common::ChangeQuery query = Json::ensureIsType<Common::ChangeQuery>(data);
		return engine.changes(query).toJson();
	});
	m_commands.insert("create", [](const QJsonValue &data, DatabaseEngine &engine, Connection *) {
		const Common::Record record = Json::ensureIsType<Common::Record>(data);
		const Common::Record inserted = engine.create(record);
		return inserted.toJson();
	});
	m_commands.insert("read", [](const QJsonValue &data, DatabaseEngine &engine, Connection *) {
		const QJsonObject obj = Json::ensureObject(data);
		const Common::Record record = engine.read(
					Common::fromTableName(Json::ensureString(obj, "table")),
					Json::ensureIsType<Common::Id>(obj, "id")
					);
		return record.toJson();
	});
	m_commands.insert("update", [](const QJsonValue &data, DatabaseEngine &engine, Connection *) {
		const Common::Record record = Json::ensureIsType<Common::Record>(data);
		const Common::Revision revision = engine.update(record);
		return Json::toJson(revision);
	});
	m_commands.insert("delete", [](const QJsonValue &data, DatabaseEngine &engine, Connection *) {
		const QJsonObject obj = Json::ensureObject(data);
		const Common::Revision revision = engine.delete_(
					Common::fromTableName(Json::ensureString(obj, "table")),
					Json::ensureIsType<Common::Id>(obj, "id")
					);
		return Json::toJson(revision);
	});
	m_commands.insert("find", [](const QJsonValue &data, DatabaseEngine &engine, Connection *) {
		const Common::TableQuery query = Json::ensureIsType<Common::TableQuery>(data);
		return Json::toJsonArray(engine.find(query));
	});
	m_commands.insert("subscribe", [](const QJsonValue &data, DatabaseEngine &engine, Connection *conn) {
		const Common::ChangeQuery query = Json::ensureIsType<Common::ChangeQuery>(data);
		const int id = conn->nextSubscriptionId;
		conn->subscriptions.insert(id, query);
		conn->nextSubscriptionId += 1;
		return QJsonObject({
							   {"subscription", id},
							   {"changes", engine.changes(query).toJson()}
						   });
	});
	m_commands.insert("unsubscribe", [](const QJsonValue &data, DatabaseEngine &, Connection *conn) {
		if (data.isObject()) {
			const Common::ChangeQuery query = Json::ensureIsType<Common::ChangeQuery>(data);
			QVector<int> ids;
			QMutableHashIterator<int, Common::ChangeQuery> it(conn->subscriptions);
			while (it.hasNext()) {
				it.next();
				if (it.value() == query) {
					ids.append(it.key());
					it.remove();
				}
			}
			return Json::toJsonArray(ids);
		} else {
			const int id = Json::ensureInteger(data);
			if (!conn->subscriptions.contains(id)) {
				throw Exception("Subscribtion ID does not exist");
			}
			conn->subscriptions.remove(id);
			return Json::toJsonArray(QVector<int>() << id);
		}
	});
}

DatabaseServer::~DatabaseServer()  {}

void DatabaseServer::addConnection(Connection *conn, QObject *slotCtxt)
{
	qCDebug(server) << "new connection from" << conn->address();
	QObject::connect(conn, &Connection::disconnected, slotCtxt, [this, conn]() {
		m_connections.removeAt(m_connections.indexOf(conn));
		qCDebug(server) << "client" << conn->address() << "disconnected";

		try {
			m_engine.delete_(Common::Table::Client, conn->clientRecordId);
		} catch (Exception &e) {
			qCCritical(server) << e.cause();
		}
	});
	QObject::connect(conn, &Connection::message, slotCtxt, [this, conn](const QByteArray &data) {
		int msgId = -1;
		try {
			const QJsonObject msg = Json::ensureObject(Json::ensureDocument(data));
			qCDebug(server) << "received" << msg;
			msgId = Json::ensureInteger(msg, "msgId");
			const QString cmd = Json::ensureString(msg, "cmd");
			QJsonValue value;

			if (cmd == "authenticate") {
				const QJsonObject auth = Json::ensureObject(msg, "data");
#ifndef SPORTSED_SKIP_AUTH
				if (m_password != Json::ensureString(auth, "pwd")) {
					throw Exception("Invalid password");
				}
#endif
				conn->name = Json::ensureString(auth, "name");
				conn->authenticated = true;
				value = true;

				conn->clientRecordId = m_engine.create(conn->asClientRecord()).id();
			} else if (m_commands.contains(cmd)) {
				if (!conn->authenticated && cmd != "version") {
					throw Exception("Not authorized");
				}
				value = m_commands[cmd](Json::ensureValue(msg, "data"), m_engine, conn);
			} else {
				throw Exception("Unknown command %1" % cmd);
			}

			const QJsonObject reply = QJsonObject({
													  {"cmd", "reply"},
													  {"data", value},
													  {"reply_to", msgId}
												  });
			qCDebug(server) << "sending" << reply;
			conn->send(Json::toText(reply));
		} catch (Exception &e) {
			const QJsonObject msg = QJsonObject({
													{"cmd", "error"},
													{"data", e.cause()},
													{"reply_to", msgId}
												});
			conn->send(Json::toText(msg));
		}
	});
	m_connections.append(conn);
}

void DatabaseServer::handleChange(const Common::Change &change)
{
	const Common::Record &record = change.record();
	for (Connection *conn : m_connections) {
		for (auto it = conn->subscriptions.constBegin(); it != conn->subscriptions.constEnd(); ++it) {
			if (it.value().matches(change, record)) {
				Common::ChangeResponse response;
				response.setChanges(QVector<Common::Change>() << change);
				response.setQuery(it.value());
				response.setLastRevision(change.revision());

				const QJsonObject msg = QJsonObject({
														{"cmd", "changes"},
														{"reply_to", it.key()},
														{"data", response.toJson()}
													});
				conn->send(Json::toText(msg));
			}
		}
	}
}

TcpDatabaseServer::TcpDatabaseServer(QSqlDatabase &db, const QString &password)
	: QTcpServer(nullptr), DatabaseServer(db, password)
{
	connect(this, &TcpDatabaseServer::acceptError, this, [this]() {
		qCCritical(server) << "server listening error:" << errorString();
	});
}

bool TcpDatabaseServer::listen()
{
	return QTcpServer::listen(QHostAddress::Any, 4829);
}

void TcpDatabaseServer::incomingConnection(qintptr handle)
{
	addConnection(new TcpConnection(handle, this), this);
}

LocalDatabaseServer::LocalDatabaseServer(QSqlDatabase &db, const QString &password)
	: QLocalServer(nullptr), DatabaseServer(db, password) {}

bool LocalDatabaseServer::listen()
{
	QLocalServer::removeServer(m_socketName);
	return QLocalServer::listen(m_socketName);
}

void LocalDatabaseServer::incomingConnection(quintptr socketDescriptor)
{
	addConnection(new LocalConnection(socketDescriptor, this), this);
}

ByteArraySender::ByteArraySender(QObject *parent) : QObject(parent) {}
ByteArraySender *ByteArraySender::instance = nullptr;

EmbeddedDatabaseServer::EmbeddedDatabaseServer(QSqlDatabase &db, const QString &password)
	: QObject(nullptr), DatabaseServer(db, password) {}
ByteArraySender *EmbeddedDatabaseServer::setup()
{
	ByteArraySender *sender = new ByteArraySender(this);
	addConnection(new EmbeddedConnection(sender, this), this);
	return sender;
}
bool EmbeddedDatabaseServer::listen()
{
	return true;
}

}
}

#include "DatabaseServer.moc"
