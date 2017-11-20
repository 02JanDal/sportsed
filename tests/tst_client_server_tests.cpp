#undef CATCH_CONFIG_MAIN
#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

#include <QSignalSpy>
#include <QDeadlineTimer>
#include <QTest>
#include <jd-util/Logging.h>
#include <QTemporaryFile>

// server
#include <DatabaseServer.h>
#include <DatabaseMigration.h>
#include <tst_Util.h>

// client
#include <clientlib/ServerConnection.h>

#pragma clang diagnostic ignored "-Wused-but-marked-unused"

using namespace Sportsed;

class TestSetup
{
public:
	explicit TestSetup() : m_db(inMemoryDb())
	{
		Server::DatabaseMigration::create(m_db);
		m_server = new Server::LocalDatabaseServer(m_db, "foobar");
		m_server->setSocketName("tst_client_server_tests");
	}

	void startServer()
	{
		REQUIRE(m_server->listen());
	}
	void restartServer()
	{
		m_server->close();
		delete m_server;
		qDebug() << "-- SERVER DOWN --";
		m_server = new Server::LocalDatabaseServer(m_db, "foobar");
		m_server->setSocketName("tst_client_server_tests");
		startServer();
	}

	std::unique_ptr<Client::LocalServerConnection> createClient()
	{
		std::unique_ptr<Client::LocalServerConnection> client = std::make_unique<Client::LocalServerConnection>();
		QSignalSpy statusSpy(client.get(), &Client::ServerConnection::status);
		QSignalSpy connectedSpy(client.get(), &Client::ServerConnection::connectedChanged);
		client->connectToServer("tst_client_server_tests", QStringLiteral("client %1") % m_numClients++, "foobar");

		if (connectedSpy.isEmpty()) {
			REQUIRE(connectedSpy.wait());
		}
		REQUIRE_FALSE(statusSpy.isEmpty());
		REQUIRE(connectedSpy.last().first().toBool());

		return client;
	}

private:
	QSqlDatabase m_db;
	Server::LocalDatabaseServer *m_server;
	int m_numClients = 0;
};

TEST_CASE("client-server-connection") {
	qRegisterMetaType<Common::ChangeResponse>();

	SECTION("basic") {
		TestSetup setup;
		setup.startServer();
		QTest::qWait(100);
		auto clientA = setup.createClient();
		auto clientB = setup.createClient();
	}
	SECTION("reconnection") {
		TestSetup setup;
		setup.startServer();
		auto clientA = setup.createClient();
		QSignalSpy statusSpy(clientA.get(), &Client::ServerConnection::status);
		setup.restartServer();

		// wait max 5s until we have reconnected
		QDeadlineTimer timer(5000);
		while (!timer.hasExpired()) {
			if (!statusSpy.isEmpty() && statusSpy.last().first().toString() == "Connected") {
				break;
			}
			statusSpy.wait(int(timer.remainingTime()));
		}
		REQUIRE_FALSE(timer.hasExpired());
		REQUIRE(clientA->isConnected());
	}
	SECTION("clients-list") {
		TestSetup setup;
		setup.startServer();
		auto clientA = setup.createClient();
		auto clientB = setup.createClient();

		const QVector<Common::Record> clientsABefore = clientA->find(Common::TableQuery(Common::Table::Client)).get();
		const QVector<Common::Record> clientsBBefore = clientB->find(Common::TableQuery(Common::Table::Client)).get();
		REQUIRE(clientsABefore == clientsBBefore);
		REQUIRE(clientsABefore.size() == 2);
		REQUIRE(clientsABefore.at(1).value("ip").toString() == "local");

		Client::Subscribtion *clientAChanges = clientA->subscribe(Common::ChangeQuery(Common::TableQuery(Common::Table::Client)));
		QSignalSpy changesSpy(clientAChanges, &Client::Subscribtion::triggered);

		// for some reason QSignalSpy gives invalid variants, despite the qRegisterMetaType call above, so we only use it for
		// waiting and do collection of arguments manually
		QVector<Common::ChangeResponse> responses;
		QObject::connect(clientAChanges, &Client::Subscribtion::triggered, [&responses](const Common::ChangeResponse &cr) {
			responses.append(cr);
		});

		// ----- add a client ----- //
		auto clientC = setup.createClient();

		if (changesSpy.isEmpty()) {
			REQUIRE(changesSpy.wait());
		}

		const QVector<Common::Record> clientsAAfterAdd = clientA->find(Common::TableQuery(Common::Table::Client)).get();
		const QVector<Common::Record> clientsBAfterAdd = clientB->find(Common::TableQuery(Common::Table::Client)).get();
		const QVector<Common::Record> clientsCAfterAdd = clientC->find(Common::TableQuery(Common::Table::Client)).get();

		REQUIRE(clientsAAfterAdd != clientsABefore);
		REQUIRE(clientsAAfterAdd == clientsBAfterAdd);
		REQUIRE(clientsAAfterAdd == clientsCAfterAdd);
		REQUIRE(clientsAAfterAdd.size() == 3);
		REQUIRE(changesSpy.size() == 1);
		const QVector<Common::Change> changesAfterAdd = responses.first().changes();
		REQUIRE(changesAfterAdd.first().type() == Common::Change::Create);
		REQUIRE(changesAfterAdd.first().record().table() == Common::Table::Client);
		REQUIRE(changesAfterAdd.first().record() == clientsCAfterAdd.last());

		// ----- removing a client ----- //
		clientB.reset();

		const QVector<Common::Record> clientsAAfterRemove = clientA->find(Common::TableQuery(Common::Table::Client)).get();
		const QVector<Common::Record> clientsCAfterRemove = clientC->find(Common::TableQuery(Common::Table::Client)).get();
		REQUIRE(clientsAAfterRemove != clientsAAfterAdd);
		REQUIRE(clientsAAfterRemove == clientsCAfterRemove);
		REQUIRE(clientsAAfterRemove.size() == 2);
		REQUIRE(changesSpy.size() == 2);
		const QVector<Common::Change> changesAfterRemove = responses.last().changes();
		REQUIRE(changesAfterRemove.first().type() == Common::Change::Delete);
		REQUIRE(changesAfterRemove.first().record().table() == Common::Table::Client);
		REQUIRE(changesAfterRemove.first().record() == clientsABefore.last());
	}
	SECTION("full-workflow", "[!mayfail]") {
		// FIXME: for some reason the QSignalSpy creation below fails because Common::ChangeResponse is not registered with the meta type system, despite the call to qRegisterMetaType?
		TestSetup setup;
		setup.startServer();
		QTest::qWait(100);
		auto clientA = setup.createClient();
		auto clientB = setup.createClient();

		Client::Subscribtion *sub = clientA->subscribe(Common::ChangeQuery(Common::TableQuery(Common::Table::Profile)));
		QSignalSpy changesSpy(sub, &Client::Subscribtion::triggered);

		// for some reason QSignalSpy gives invalid variants, despite the qRegisterMetaType call above, so we only use it for
		// waiting and do collection of arguments manually
		QVector<Common::ChangeResponse> responses;
		QObject::connect(sub, &Client::Subscribtion::triggered, [&responses](const Common::ChangeResponse &cr) {
			responses.append(cr);
		});

		// create from client A
		Common::Record rec = clientB->create(Common::Record(Common::Table::Meta, {{"key", "test"}, {"value", "foo"}})).get();

		// update from client B
		rec.setValue("value", "bar");
		const Common::Revision updateRev = clientB->update(rec).get();

		// delete from client A
		const Common::Revision deleteRev = clientA->delete_(rec).get();

		// make sure clientA got all changes
		QDeadlineTimer deadline(5000);
		while (changesSpy.size() < 3 && !deadline.hasExpired()) {
			REQUIRE(changesSpy.wait(int(deadline.remainingTime())));
			qDebug() << changesSpy;
		}
		REQUIRE(changesSpy.size() == 3);
		REQUIRE(responses.at(0).lastRevision() == rec.latestRevision());
		REQUIRE(responses.at(1).lastRevision() == updateRev);
		REQUIRE(responses.at(2).lastRevision() == deleteRev);
		REQUIRE(responses.at(0).changes().size() == 1);
		REQUIRE(responses.at(1).changes().size() == 1);
		REQUIRE(responses.at(2).changes().size() == 1);
		REQUIRE(responses.at(0).changes().first().type() == Common::Change::Create);
		REQUIRE(responses.at(1).changes().first().type() == Common::Change::Update);
		REQUIRE(responses.at(2).changes().first().type() == Common::Change::Delete);
		REQUIRE(responses.at(0).changes().first().revision() == rec.latestRevision());
		REQUIRE(responses.at(1).changes().first().revision() == updateRev);
		REQUIRE(responses.at(2).changes().first().revision() == deleteRev);
	}
}

int main(int argc, char *argv[])
{
	JD::Util::installLogFormatter();

	QCoreApplication app(argc, argv);
	int result = Catch::Session().run(argc, argv);
	return (result < 0xff ? result : 0xff);
}
