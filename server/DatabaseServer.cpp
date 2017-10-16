#include "DatabaseServer.h"

#include <QTcpSocket>

#include <jd-util/Json.h>

#include "commonlib/ChangeQuery.h"
#include "commonlib/ChangeResponse.h"
#include "commonlib/TcpMessageSocket.h"
#include "DatabaseMigration.h"

using namespace JD::Util;

namespace Sportsed {
namespace Server {

class Connection : public QObject
{
	Q_OBJECT
public:
	explicit Connection(qintptr handle, QObject *parent)
		: QObject(parent), socket(new Common::TcpMessageSocket(this))
	{
		socket->setSocketDescriptor(handle);

		connect(socket, &QTcpSocket::stateChanged, this, [this](const QTcpSocket::SocketState state) {
			switch (state) {
			case QAbstractSocket::UnconnectedState:
				deleteLater();
				break;
			case QAbstractSocket::HostLookupState: break;
			case QAbstractSocket::ConnectingState: break;
			case QAbstractSocket::ConnectedState: break;
			case QAbstractSocket::BoundState: break;
			case QAbstractSocket::ListeningState: break;
			case QAbstractSocket::ClosingState:
				emit disconnected();
				break;
			}
		});
		connect(socket, &Common::TcpMessageSocket::message, this, &Connection::message);
	}

	Common::TcpMessageSocket *socket;

	bool authenticated = false;

	QHash<int, Common::ChangeQuery> subscriptions;
	int nextSubscriptionId = 1;

	void send(const QByteArray &msg)
	{
		const int size = msg.size();
		QByteArray sizeData(4, 0);
		sizeData[0] = static_cast<char>(size >> 24);
		sizeData[1] = static_cast<char>(size >> 16);
		sizeData[2] = static_cast<char>(size >> 8);
		sizeData[3] = static_cast<char>(size >> 0);
		socket->write(sizeData);
		socket->write(msg);
	}

signals:
	void message(const QByteArray &data);
	void disconnected();
};

DatabaseServer::DatabaseServer(QSqlDatabase &db, const QString &password)
	: m_db(db), m_password(password), m_engine(db)
{
	m_engine.setChangeCallback([this](const Common::Change &change, const Common::Record &record) {
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
	});
}

DatabaseServer::~DatabaseServer()
{
	m_db.close();
}

void DatabaseServer::incomingConnection(qintptr handle)
{
	Connection *conn = new Connection(handle, this);
	connect(conn, &Connection::disconnected, this, [this, conn]() {
		m_connections.removeAt(m_connections.indexOf(conn));
	});
	connect(conn, &Connection::message, this, [this, conn](const QByteArray &data) {
		int msgId = -1;
		try {
			const QJsonObject msg = Json::ensureObject(Json::ensureDocument(data));
			msgId = Json::ensureInteger(msg, "id");
			const QString cmd = Json::ensureString(msg, "cmd");
			QJsonValue value;

			if (cmd == "authenticate") {
				if (m_password == Json::ensureString(msg, "data")) {
					conn->authenticated = true;
					value = true;
				} else {
					throw Exception("Invalid password");
				}
			}

			if (!conn->authenticated && cmd != "version") {
				throw Exception("Not authorized");
			}

			if (cmd == "version") {
				value = DatabaseMigration::currentVersion(m_db);
			} else if (cmd == "changes") {
				const Common::ChangeQuery query = Json::ensureIsType<Common::ChangeQuery>(msg, "data");
				value = m_engine.changes(query).toJson();
			} else if (cmd == "create") {
				const Common::Record record = Json::ensureIsType<Common::Record>(msg, "data");
				const Common::Record inserted = m_engine.create(record);
				value = QJsonObject({
										{"id", Json::toJson(inserted.id())},
										{"revision", Json::toJson(inserted.latestRevision())}
									});
			} else if (cmd == "read") {
				const QJsonObject obj = Json::ensureObject(msg, "data");
				const Common::Record record = m_engine.read(
							Common::fromTableName(Json::ensureString(obj, "table")),
							Json::ensureIsType<Common::Id>(obj, "id")
							);
				value = record.toJson();
			} else if (cmd == "update") {
				const Common::Record record = Json::ensureIsType<Common::Record>(msg, "data");
				const Common::Revision revision = m_engine.update(record);
				value = Json::toJson(revision);
			} else if (cmd == "delete") {
				const QJsonObject obj = Json::ensureObject(msg, "data");
				const Common::Revision revision = m_engine.delete_(
							Common::fromTableName(Json::ensureString(obj, "table")),
							Json::ensureIsType<Common::Id>(obj, "id")
							);
				value = Json::toJson(revision);
			} else if (cmd == "find") {
				const Common::TableQuery query = Json::ensureIsType<Common::TableQuery>(msg, "data");
				value = Json::toJsonArray(m_engine.find(query));
			} else if (cmd == "subscribe") {
				const Common::ChangeQuery query = Json::ensureIsType<Common::ChangeQuery>(msg, "data");
				const int id = conn->nextSubscriptionId;
				conn->subscriptions.insert(id, query);
				conn->nextSubscriptionId += 1;
				value = id;
			} else if (cmd == "unsubscribe") {
				if (msg.value("data").isObject()) {
					const Common::ChangeQuery query = Json::ensureIsType<Common::ChangeQuery>(msg, "data");
					QVector<int> ids;
					QMutableHashIterator<int, Common::ChangeQuery> it(conn->subscriptions);
					while (it.hasNext()) {
						it.next();
						if (it.value() == query) {
							ids.append(it.key());
							it.remove();
						}
					}
					value = Json::toJsonArray(ids);
				} else {
					const int id = Json::ensureInteger(msg, "data");
					if (!conn->subscriptions.contains(id)) {
						throw Exception("Subscribtion ID does not exist");
					}
					conn->subscriptions.remove(id);
					value = Json::toJsonArray(QVector<int>() << id);
				}
			} else {
				throw Exception("Unknown command");
			}

			const QJsonObject reply = QJsonObject({
													  {"cmd", "success"},
													  {"data", value},
													  {"reply_to", msgId}
												  });
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

}
}

#include "DatabaseServer.moc"
