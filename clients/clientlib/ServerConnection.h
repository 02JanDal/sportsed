#pragma once

#include <QObject>
#include <QHostAddress>

#include <jd-util/Json.h>
#include <experimental/optional>
#include <functional>

#include <commonlib/Record.h>
#include <commonlib/TableQuery.h>

class QHostAddress;
class QEventLoop;

namespace Sportsed {
namespace Common {
class TcpMessageSocket;
class ChangeQuery;
class ChangeResponse;
}

namespace Client {

class ServerConnection;

DECLARE_EXCEPTION(FutureResult)

class FutureImpl
{
public:
	~FutureImpl();

private:
	friend class ServerConnection;
	template <typename T> friend class Future;

	using Callback = std::function<void()>;

	explicit FutureImpl(ServerConnection *conn, const int msgId);

	QJsonValue get();
	void then(const Callback &cb);

	void receivedMessage(const QJsonObject &obj);

private:
	ServerConnection *m_conn;
	int m_msgId;
	std::unique_ptr<QEventLoop> m_loop;
	Callback m_cb;

	std::experimental::optional<QJsonObject> m_value;
};

template <typename T>
class Future
{
public:
	explicit Future(const std::shared_ptr<FutureImpl> &impl) : m_impl(impl) {}

	T get() const { return JD::Util::Json::ensureIsType<T>(m_impl->get()); }

	void then(const std::function<void(T)> &cb, const std::function<void()> &errorCb = {})
	{
		m_impl->then([this, cb, errorCb]() {
			T val;
			bool threw = false;
			try {
				val = get();
			} catch (...) {
				threw = true;
				if (errorCb) {
					errorCb();
				}
			}
			if (threw) {
				cb(val);
			}
		});
	}

private:
	std::shared_ptr<FutureImpl> m_impl;
};

class Subscribtion : public QObject
{
	Q_OBJECT
public:
	explicit Subscribtion(QObject *parent = nullptr);

signals:
	void triggered(const Common::ChangeResponse &changes);
};

DECLARE_EXCEPTION(AlreadyConnected)
DECLARE_EXCEPTION(NotConnected)

class ServerConnection : public QObject
{
	Q_OBJECT
public:
	explicit ServerConnection(QObject *parent = nullptr);

	void connectToServer(const QHostAddress &addr, const QString &password);
	void disconnectFromServer();

	bool isConnected() const;

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

private:
	bool m_shouldBeConnected = false;
	bool m_authenticated = false;
	Common::TcpMessageSocket *m_socket;
	QHostAddress m_address;
	QString m_password;

	void message(const QByteArray &msg);

	std::shared_ptr<FutureImpl> sendMessage(const QString &cmd, const QJsonValue &value);
	int m_nextMsgId = 0;

	QHash<int, FutureImpl *> m_futures;
	friend class FutureImpl;
	void futureRemoved(const int msgId);

	QHash<int, Subscribtion *> m_subscriptions;
	QVector<Subscribtion *> m_pendingSubscriptions;
};

}
}
