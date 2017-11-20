#include "ServerConnection.h"

#include <jd-util/Json.h>
#include <QEventLoop>
#include <QLocalSocket>
#include <QTimer>

#include <commonlib/MessageSocket.h>
#include <commonlib/ChangeQuery.h>
#include <commonlib/ChangeResponse.h>
#include <commonlib/Validators.h>

#ifdef SPORTSED_SKIP_AUTH
#include <server/DatabaseServer.h>
#endif

Q_DECLARE_METATYPE(QHostAddress)

using namespace JD::Util;

namespace Sportsed {
namespace Client {
Q_LOGGING_CATEGORY(serverConnection, "sportsed.client.connection")

class SocketWrapper : public Common::MessageSocket
{
	Q_OBJECT
public:
	explicit SocketWrapper(QIODevice *device = nullptr) : Common::MessageSocket(device) {}

	virtual QAbstractSocket::SocketState state() const = 0;
	virtual QAbstractSocket::SocketError error() const = 0;
	virtual QString errorString() const = 0;
	virtual void disconnectFromHost() = 0;
	Q_INVOKABLE virtual void connectToHost() = 0;

	void setConnectionArguments(const QVariantList &args) { m_connectionArgs = args; }

signals:
	void errored();
	void stateChanged();
	void connected();

protected:
	QVariantList m_connectionArgs;
};
class TcpSocketWrapper : public SocketWrapper
{
	QTcpSocket *m_socket = nullptr;
public:
	explicit TcpSocketWrapper() : SocketWrapper() {}

	QAbstractSocket::SocketState state() const override { return m_socket ? m_socket->state() : QAbstractSocket::UnconnectedState; }
	QAbstractSocket::SocketError error() const override { return m_socket ? m_socket->error() : QAbstractSocket::UnknownSocketError; }
	void connectToHost() override
	{
		if (m_socket) {
			if (state() != QAbstractSocket::UnconnectedState) {
				throw Exception("Attempting to connect to already connected socket");
			} else {
				delete m_socket;
			}
		}
		m_socket = new QTcpSocket(this);
		connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error), this, &TcpSocketWrapper::errored);
		connect(m_socket, &QTcpSocket::stateChanged, this, &TcpSocketWrapper::stateChanged);
		connect(m_socket, &QTcpSocket::connected, this, &TcpSocketWrapper::connected);
		setDevice(m_socket);
		m_socket->connectToHost(m_connectionArgs.at(0).value<QHostAddress>(), m_connectionArgs.at(1).value<quint16>());
	}
	void disconnectFromHost() override { m_socket->disconnectFromHost(); }
	QString errorString() const override { return m_socket->errorString(); }
};
class LocalSocketWrapper : public SocketWrapper
{
	QLocalSocket *m_socket = nullptr;
public:
	explicit LocalSocketWrapper() : SocketWrapper() {}

	QAbstractSocket::SocketState state() const override
	{
		if (!m_socket) {
			return QAbstractSocket::UnconnectedState;
		}
		switch (m_socket->state()) {
		case QLocalSocket::UnconnectedState: return QAbstractSocket::UnconnectedState;
		case QLocalSocket::ConnectingState: return QAbstractSocket::ConnectingState;
		case QLocalSocket::ConnectedState: return QAbstractSocket::ConnectedState;
		case QLocalSocket::ClosingState: return QAbstractSocket::ClosingState;
		}
	}
	QAbstractSocket::SocketError error() const override
	{
		if (!m_socket) {
			return QAbstractSocket::UnknownSocketError;
		}
		switch (m_socket->error()) {
		case QLocalSocket::ConnectionRefusedError: return QAbstractSocket::ConnectionRefusedError;
		case QLocalSocket::PeerClosedError: return QAbstractSocket::RemoteHostClosedError;
		case QLocalSocket::ServerNotFoundError: return QAbstractSocket::HostNotFoundError;
		case QLocalSocket::SocketAccessError: return QAbstractSocket::SocketAccessError;
		case QLocalSocket::SocketResourceError: return QAbstractSocket::SocketResourceError;
		case QLocalSocket::SocketTimeoutError: return QAbstractSocket::SocketTimeoutError;
		case QLocalSocket::DatagramTooLargeError: return QAbstractSocket::DatagramTooLargeError;
		case QLocalSocket::ConnectionError: return QAbstractSocket::NetworkError;
		case QLocalSocket::UnsupportedSocketOperationError: return QAbstractSocket::UnsupportedSocketOperationError;
		case QLocalSocket::UnknownSocketError: return QAbstractSocket::UnknownSocketError;
		case QLocalSocket::OperationError: return QAbstractSocket::OperationError;
		}
	}
	void connectToHost() override
	{
		if (m_socket) {
			if (state() != QAbstractSocket::UnconnectedState) {
				throw Exception("Attempting to connect to already connected socket");
			} else {
				delete m_socket;
			}
		}
		m_socket = new QLocalSocket(this);
		connect(m_socket, QOverload<QLocalSocket::LocalSocketError>::of(&QLocalSocket::error), this, &LocalSocketWrapper::errored);
		connect(m_socket, &QLocalSocket::stateChanged, this, [this]() {emit stateChanged();});
		connect(m_socket, &QLocalSocket::connected, this, &LocalSocketWrapper::connected);
		setDevice(m_socket);
		m_socket->connectToServer(m_connectionArgs.at(0).toString());
	}
	void disconnectFromHost() override { m_socket->disconnectFromServer(); }
	QString errorString() const override { return m_socket->errorString(); }
};
class EmbeddedSocketWrapper : public SocketWrapper
{
public:
	explicit EmbeddedSocketWrapper() : SocketWrapper()
	{
		connect(Server::ByteArraySender::instance, &Server::ByteArraySender::clientReceived,
				this, &EmbeddedSocketWrapper::message);
	}

	QAbstractSocket::SocketState state() const override
	{
		return m_connected ? QAbstractSocket::ConnectedState : QAbstractSocket::UnconnectedState;
	}
	QAbstractSocket::SocketError error() const override { return QAbstractSocket::UnknownSocketError; }
	void connectToHost() override
	{
		m_connected = true;
		emit stateChanged();
		emit connected();
	}
	void disconnectFromHost() override {}
	QString errorString() const override { return QString(); }

	void send(const QByteArray &msg) override { Server::ByteArraySender::instance->sendToServer(msg); }

	bool m_connected = false;
};

ServerConnection::ServerConnection(SocketWrapper *wrapper, QObject *parent)
	: QObject(parent), m_socket(wrapper)
{
	m_socket->setParent(this);

	connect(m_socket, &SocketWrapper::message, this, &ServerConnection::received);
	connect(m_socket, &SocketWrapper::stateChanged, this, [this]() {
		const QAbstractSocket::SocketState state = m_socket->state();
		qDebug(serverConnection) << state;
		switch (state) {
		case QAbstractSocket::UnconnectedState:
			recalculateConnected();
			if ((m_previousState == QAbstractSocket::ClosingState || m_previousState == QAbstractSocket::ConnectedState) && m_shouldBeConnected) {
				emit status("Reconnecting...");
				QMetaObject::invokeMethod(m_socket, "connectToHost", Qt::QueuedConnection);
			} else {
				emit status("Disconnected");
			}
			break;
		case QAbstractSocket::HostLookupState:
			emit status(tr("Looking up host name..."));
			break;
		case QAbstractSocket::ConnectingState:
			emit status(tr("Connecting..."));
			break;
		case QAbstractSocket::ConnectedState:
			emit status(tr("Authenticating..."));
			m_authenticated = false;

			// somewhat weird behavior in QLocalSocket: the new openMode gets set after the state change is advertised,
			// this means that we can't actually send data yet...
			// to work around this we use the connected() signal instead (see below), rather than authenticating here
			break;
		case QAbstractSocket::BoundState: break;
		case QAbstractSocket::ListeningState: break;
		case QAbstractSocket::ClosingState:
			emit status(tr("Disconnecting..."));
			break;
		}
		m_previousState = state;
	});
	connect(m_socket, &SocketWrapper::connected, this, [this]() {
		Future<bool>(sendMessage("authenticate", QJsonObject({
																 {"name", m_name},
																 {"pwd", m_password}
															 })))
				.then([this](const bool) {
			m_authenticated = true;
			emit status(tr("Connected"));
			recalculateConnected();
		}, [this]() {
			m_shouldBeConnected = false;
			m_socket->disconnectFromHost();
			emit status(tr("Authentication error"));
			emit connectionError();
		});
	});
	connect(m_socket, &SocketWrapper::errored, this, [this]() {
		if (m_socket->error() != QAbstractSocket::RemoteHostClosedError) {
			qCWarning(serverConnection) << m_socket->errorString();
		}
	});
	connect(this, &ServerConnection::status, this, [this](const QString &msg) {
		qCDebug(serverConnection) << qPrintable(msg);
	});

	m_previousState = m_socket->state();
}

void ServerConnection::connectToServer(const QVariantList &args, const QString &name, const QString &password)
{
	if (m_shouldBeConnected) {
		emit connectedChanged(false);
		throw AlreadyConnectedException();
	}

	m_shouldBeConnected = true;
	m_name = name;
	m_password = password;
	m_socket->setConnectionArguments(args);
	m_socket->connectToHost();
}
void ServerConnection::disconnectFromServer()
{
	m_shouldBeConnected = false;
	m_socket->disconnectFromHost();
}

Future<int> ServerConnection::version()
{
	return Future<int>(sendMessage("version", QJsonValue()));
}

Future<Common::ChangeResponse> ServerConnection::changes(const Common::ChangeQuery &query)
{
	return Future<Common::ChangeResponse>(sendMessage("changes", query.toJson()));
}

Future<Common::Record> ServerConnection::create(const Common::Record &record)
{
	Common::BaseValidator::getValidator(record.table())->validateRecord(record);
	qCInfo(serverConnection) << "CREATE" << Common::tableName(record.table()) << record.values();
	return Future<Common::Record>(sendMessage("create", record.toJson()));
}
Future<Common::Record> ServerConnection::read(const Common::Table &table, const Common::Id &id)
{
	qCInfo(serverConnection) << "READ" << Common::tableName(table) << id;
	return Future<Common::Record>(sendMessage("read", QJsonObject({
																	  {"table", Common::tableName(table)},
																	  {"id", Json::toJson(id)}
																  })));
}
Future<Common::Revision> ServerConnection::update(const Common::Record &record)
{
	for (auto it = record.values().cbegin(); it != record.values().cend(); ++it) {
		Common::BaseValidator::getValidator(record.table())->validateField(it.key(), it.value());
	}
	qCInfo(serverConnection) << "UPDATE" << Common::tableName(record.table()) << record.id() << record.values();
	return Future<Common::Revision>(sendMessage("update", record.toJson()));
}
Future<Common::Revision> ServerConnection::delete_(const Common::Table &table, const Common::Id &id)
{
	qCInfo(serverConnection) << "DELETE" << Common::tableName(table) << id;
	return Future<Common::Revision>(sendMessage("delete", QJsonObject({
																		  {"table", Common::tableName(table)},
																		  {"id", Json::toJson(id)}
																	  })));
}
Future<Common::Revision> ServerConnection::delete_(const Common::Record &record)
{
	return delete_(record.table(), record.id());
}

Future<QVector<Common::Record> > ServerConnection::find(const Common::TableQuery &query)
{
	qCInfo(serverConnection) << "FIND" << Common::tableName(query.table()) << query.filters();
	return Future<QVector<Common::Record>>(sendMessage("find", query.toJson()));
}

Subscribtion *ServerConnection::subscribe(const Common::ChangeQuery &query)
{
	Subscribtion *sub = new Subscribtion(this);
	m_pendingSubscriptions.append(sub);
	auto fut = Future<QJsonObject>(sendMessage("subscribe", query.toJson()));
	fut.then([this, sub, query](const QJsonObject &obj) {
		const int subscriptionId = Json::ensureInteger(obj, "subscription");
		qCInfo(serverConnection) << qPrintable(QStringLiteral("SUBSCRIBEd(%1)").arg(subscriptionId))
								 << Common::tableName(query.query().table()) << query.query().filters();
		if (m_pendingSubscriptions.contains(sub)) {
			m_subscriptions.insert(subscriptionId, sub);
			m_pendingSubscriptions.removeAll(sub);
			emit sub->triggered(Json::ensureIsType<Common::ChangeResponse>(obj, "changes"));
		} else {
			sendMessage("unsubscribe", subscriptionId);
		}
	});
	connect(sub, &Subscribtion::destroyed, this, [this, sub]() {
		if (m_pendingSubscriptions.contains(sub)) {
			m_pendingSubscriptions.removeAll(sub);
		} else {
			const int id = m_subscriptions.key(sub, -1);
			if (id != -1) {
				m_subscriptions.remove(id);
				qCInfo(serverConnection) << qPrintable(QStringLiteral("UNSUBSCRIBE(%1)").arg(id));
				sendMessage("unsubscribe", id);
			}
		}
	});

	return sub;
}

void ServerConnection::recalculateConnected()
{
	const bool newValue = m_socket->state() == QTcpSocket::ConnectedState && m_authenticated;
	if (newValue != m_isConnected) {
		m_isConnected = newValue;
		emit connectedChanged(newValue);
	}
}

void ServerConnection::received(const QByteArray &msg)
{
	try {
		const QJsonObject obj = Json::ensureObject(Json::ensureDocument(msg));
		qCDebug(serverConnection) << "reply recv" << obj;
		const QString cmd = Json::ensureString(obj, "cmd");
		if (cmd == "changes") {
			const int subscribtion = Json::ensureInteger(obj, "reply_to");
			const Common::ChangeResponse changes = Json::ensureIsType<Common::ChangeResponse>(obj, "data");
			if (m_subscriptions.contains(subscribtion)) {
				emit m_subscriptions.value(subscribtion)->triggered(changes);
			}
		} else if (cmd == "reply" || cmd == "error") {
			const int msgId = Json::ensureInteger(obj, "reply_to");
			if (m_futures.contains(msgId)) {
				m_futures.value(msgId)->receivedMessage(obj);
			}
		}
	} catch (Exception &) {

	}
}

std::shared_ptr<FutureImpl> ServerConnection::sendMessage(const QString &cmd, const QJsonValue &value)
{
	if (m_socket->state() != QTcpSocket::ConnectedState || (!m_authenticated && cmd != "authenticate")) {
		throw NotConnectedException();
	}

	const int id = m_nextMsgId++;
	const QJsonObject msg = QJsonObject({
											{"cmd", cmd},
											{"msgId", id},
											{"data", value}
										});
	qCDebug(serverConnection) << "msg send" << msg;
	// cannot use std::make_shared since FutureImpl is fully private
	auto future = std::shared_ptr<FutureImpl>(new FutureImpl(this, id));
	m_futures.insert(id, future.get());
	m_socket->send(Json::toText(msg));
	return future;
}

void ServerConnection::futureRemoved(const int msgId)
{
	m_futures.remove(msgId);
}

TcpServerConnection::TcpServerConnection(QObject *parent)
	: ServerConnection(new TcpSocketWrapper, parent) {}

void TcpServerConnection::connectToServer(const QHostAddress &addr, const QString &name, const QString &password)
{
	ServerConnection::connectToServer({qVariantFromValue(addr), 4829}, name, password);
}

LocalServerConnection::LocalServerConnection(QObject *parent)
	: ServerConnection(new LocalSocketWrapper, parent) {}
void LocalServerConnection::connectToServer(const QString &socketName, const QString &name, const QString &password)
{
	ServerConnection::connectToServer({socketName}, name, password);
}

EmbeddedServerConnection::EmbeddedServerConnection(QObject *parent)
	: ServerConnection(new EmbeddedSocketWrapper, parent) {}

void EmbeddedServerConnection::connectToServer(const QString &name, const QString &password)
{
	ServerConnection::connectToServer({}, name, password);
}

}
}

#include "ServerConnection.moc"
