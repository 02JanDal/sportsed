#include "ServerConnection.h"

#include <jd-util/Json.h>
#include <QEventLoop>

#include <commonlib/TcpMessageSocket.h>
#include <commonlib/ChangeQuery.h>
#include <commonlib/ChangeResponse.h>

using namespace JD::Util;

namespace Sportsed {
namespace Client {

ServerConnection::ServerConnection(QObject *parent)
	: QObject(parent), m_socket(new Common::TcpMessageSocket(this))
{
	connect(m_socket, &Common::TcpMessageSocket::message, this, &ServerConnection::message);
	connect(m_socket, &QTcpSocket::stateChanged, this, [this](const QTcpSocket::SocketState state) {
		switch (state) {
		case QAbstractSocket::UnconnectedState:
			emit connectedChanged(isConnected());
			if (m_shouldBeConnected) {
				emit status("Reconnecting...");
				m_socket->connectToHost(m_address, 4829, QTcpSocket::ReadWrite);
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
			emit status(tr("Authenticated..."));
			m_authenticated = false;
			Future<bool>(sendMessage("authenticate", m_password))
					.then([this](const bool) {
				m_authenticated = true;
				emit status(tr("Connected"));
				emit connectedChanged(isConnected());
			}, [this]() {
				m_shouldBeConnected = false;
				m_socket->disconnectFromHost();
				emit status(tr("Authentication error"));
				emit connectionError();
			});
			break;
		case QAbstractSocket::BoundState: break;
		case QAbstractSocket::ListeningState: break;
		case QAbstractSocket::ClosingState:
			emit status(tr("Disconnecting..."));
			break;
		}
	});
}

void ServerConnection::connectToServer(const QHostAddress &addr, const QString &password)
{
	if (m_shouldBeConnected) {
		throw AlreadyConnectedException();
	}

	m_shouldBeConnected = true;
	m_socket->connectToHost(addr, 4829, QTcpSocket::ReadWrite);
	m_password = password;
}
void ServerConnection::disconnectFromServer()
{
	m_shouldBeConnected = false;
	m_socket->disconnectFromHost();
}

bool ServerConnection::isConnected() const
{
	return m_socket->state() == QTcpSocket::ConnectedState && m_authenticated;
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
	return Future<Common::Record>(sendMessage("create", record.toJson()));
}
Future<Common::Record> ServerConnection::read(const Common::Table &table, const Common::Id &id)
{
	return Future<Common::Record>(sendMessage("read", QJsonObject({
																	  {"table", Common::tableName(table)},
																	  {"id", Json::toJson(id)}
																  })));
}
Future<Common::Revision> ServerConnection::update(const Common::Record &record)
{
	return Future<Common::Revision>(sendMessage("update", record.toJson()));
}
Future<Common::Revision> ServerConnection::delete_(const Common::Table &table, const Common::Id &id)
{
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
	return Future<QVector<Common::Record>>(sendMessage("find", query.toJson()));
}

Subscribtion *ServerConnection::subscribe(const Common::ChangeQuery &query)
{
	Subscribtion *sub = new Subscribtion(this);
	m_pendingSubscriptions.append(sub);
	auto fut = Future<int>(sendMessage("subscribe", query.toJson()));
	fut.then([this, sub](const int subscriptionId) {
		if (m_pendingSubscriptions.contains(sub)) {
			m_subscriptions.insert(subscriptionId, sub);
			m_pendingSubscriptions.removeAll(sub);
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
				sendMessage("unsubscribe", id);
			}
		}
	});

	return sub;
}

void ServerConnection::message(const QByteArray &msg)
{
	try {
		const QJsonObject obj = Json::ensureObject(Json::ensureDocument(msg));
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

FutureImpl::FutureImpl(ServerConnection *conn, const int msgId)
	: m_conn(conn), m_msgId(msgId) {}

FutureImpl::~FutureImpl()
{
	m_conn->futureRemoved(m_msgId);
}

QJsonValue FutureImpl::get()
{
	if (!m_value) {
		m_loop = std::make_unique<QEventLoop>();
		m_loop->exec();
		m_loop.reset();
	}

	const QJsonObject obj = *m_value;
	if (Json::ensureString(obj, "cmd") == "error") {
		throw FutureResultException(Json::ensureString(obj, "data"));
	}
	return Json::ensureValue(obj, "data");
}

void FutureImpl::then(const Callback &cb)
{
	m_cb = cb;
	if (m_value) {
		m_cb();
	}
}

void FutureImpl::receivedMessage(const QJsonObject &obj)
{
	m_value = obj;
	if (m_loop) {
		m_loop->exit();
	}

	if (m_cb) {
		try {
			m_cb();
		} catch (...) {}
	}
}

Subscribtion::Subscribtion(QObject *parent)
	: QObject(parent) {}

}
}
