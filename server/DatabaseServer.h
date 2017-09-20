#pragma once

#include <orm.capnp.h>

#include <QSqlDatabase>

#include "DatabaseEngine.h"

namespace Sportsed {
namespace Server {

class SubscriptionHandler;

class DatabaseServer : public Schema::Database::Server
{
public:
	explicit DatabaseServer(QSqlDatabase &db);
	virtual ~DatabaseServer();

protected:
	::kj::Promise<void> version(VersionContext context) override;
	::kj::Promise<void> changes(ChangesContext context) override;
	::kj::Promise<void> setupSubscription(SetupSubscriptionContext context) override;
	::kj::Promise<void> create(CreateContext context) override;
	::kj::Promise<void> read(ReadContext context) override;
	::kj::Promise<void> update(UpdateContext context) override;
	::kj::Promise<void> delete_(DeleteContext context) override;

private:
	QSqlDatabase m_db;
	DatabaseEngine m_engine;
	SubscriptionHandler *m_subscriptions;
};

}
}
