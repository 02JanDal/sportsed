#include "DatabaseServer.h"

#include "commonlib/ChangeQuery.h"
#include "commonlib/ChangeResponse.h"
#include "commonlib/CapnprotoUtil.h"
#include "DatabaseMigration.h"

namespace Sportsed {
namespace Server {

class SubscriberImpl;

class SubscriptionHandler
{
public:
	explicit SubscriptionHandler() {}

	using Subscription = std::pair<SubscriberImpl *, Common::ChangeQuery>;

	void emitChange(const Common::Change &change, const Common::Record &record);
	void subscribe(const Subscription &subscription)
	{
		m_subscriptions.append(subscription);
	}
	void unsubscribe(const Subscription &subscription)
	{
		m_subscriptions.removeAll(subscription);
	}
	void unsubscribe(const SubscriberImpl *subscriber)
	{
		QMutableVectorIterator<Subscription> it{m_subscriptions};
		while (it.hasNext()) {
			if (it.next().first == subscriber) {
				it.remove();
			}
		}
	}

private:
	QVector<Subscription> m_subscriptions;
};

class UnsubscriptionHandle : public Schema::Handle::Server
{
public:
	explicit UnsubscriptionHandle(SubscriptionHandler *handler, const SubscriptionHandler::Subscription &subscription)
		: m_handler(handler), m_subscription(subscription) {}
	virtual ~UnsubscriptionHandle();

private:
	SubscriptionHandler *m_handler;
	const SubscriptionHandler::Subscription m_subscription;
};

class SubscriberImpl : public Schema::Subscriber::Server
{
public:
	explicit SubscriberImpl(SubscriptionHandler *handler, Schema::ChangeCallback::Client client)
		: m_handler(handler), m_client(client) {}
	virtual ~SubscriberImpl()
	{
		m_handler->unsubscribe(this);
	}

	void call(const Common::ChangeResponse &response)
	{
		auto req = m_client.callRequest();
		response.build(req.initChanges());
		req.send().detach([](kj::Exception&&) {});
	}

protected:
	::kj::Promise<void> subscribeTo(SubscribeToContext context) override;

private:
	SubscriptionHandler *m_handler;
	Schema::ChangeCallback::Client m_client;
};

::kj::Promise<void> SubscriberImpl::subscribeTo(Schema::Subscriber::Server::SubscribeToContext context)
{
	const auto subscription = std::make_pair<SubscriberImpl *, Common::ChangeQuery>(this, Common::ChangeQuery::fromReader(context.getParams().getQuery()));
	m_handler->subscribe(subscription);
	context.getResults().setHandle(kj::heap<UnsubscriptionHandle>(m_handler, subscription));
	return kj::READY_NOW;
}

void SubscriptionHandler::emitChange(const Common::Change &change, const Common::Record &record)
{
	for (const Subscription &sub : m_subscriptions) {
		if (sub.second.matches(change, record)) {
			Common::ChangeResponse response;
			response.setChanges(QVector<Common::Change>() << change);
			response.setQuery(sub.second);
			response.setLastRevision(change.revision());
			sub.first->call(response);
		}
	}
}

UnsubscriptionHandle::~UnsubscriptionHandle()
{
	m_handler->unsubscribe(m_subscription);
}

DatabaseServer::DatabaseServer(QSqlDatabase &db)
	: m_db(db), m_engine(db), m_subscriptions(new SubscriptionHandler)
{
	m_engine.setChangeCallback([this](const Common::Change &c, const Common::Record &r) { m_subscriptions->emitChange(c, r); });
}

DatabaseServer::~DatabaseServer()
{
	m_db.close();
}

::kj::Promise<void> DatabaseServer::version(VersionContext context)
{
	context.getResults().setNumber(DatabaseMigration::currentVersion(m_db));
	return kj::READY_NOW;
}

::kj::Promise<void> DatabaseServer::changes(ChangesContext context)
{
	const Common::ChangeQuery query = Common::ChangeQuery::fromReader(context.getParams().getQuery());
	const Common::ChangeResponse response = m_engine.changes(query);
	response.build(context.getResults().initResponse());
	return kj::READY_NOW;
}

::kj::Promise<void> DatabaseServer::setupSubscription(SetupSubscriptionContext context)
{
	context.getResults().setSubscriber(kj::heap<SubscriberImpl>(m_subscriptions, context.getParams().getCb()));
	return kj::READY_NOW;
}

::kj::Promise<void> DatabaseServer::create(CreateContext context)
{
	const Common::Record record = Common::Record::fromReader(context.getParams().getRecord());
	const Common::Record inserted = m_engine.create(record);
	context.getResults().setId(inserted.id());
	context.getResults().setRevision(inserted.latestRevision());
	return kj::READY_NOW;
}

::kj::Promise<void> DatabaseServer::read(ReadContext context)
{
	const Common::Record record = m_engine.read(
				Common::toQString(context.getParams().getTable()),
				context.getParams().getId());
	record.build(context.getResults().initRecord());
	return kj::READY_NOW;
}

::kj::Promise<void> DatabaseServer::update(UpdateContext context)
{
	const Common::Record record = Common::Record::fromReader(context.getParams().getRecord());
	const Common::Revision revision = m_engine.update(record);
	context.getResults().setRevision(revision);
	return kj::READY_NOW;
}

::kj::Promise<void> DatabaseServer::delete_(DeleteContext context)
{
	const Common::Revision revision = m_engine.delete_(
				Common::toQString(context.getParams().getTable()),
				context.getParams().getId()
				);
	context.getResults().setRevision(revision);
	return kj::READY_NOW;
}

}
}
