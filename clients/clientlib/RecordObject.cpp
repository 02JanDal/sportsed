#include "RecordObject.h"

#include <commonlib/ChangeQuery.h>
#include <commonlib/ChangeResponse.h>

namespace Sportsed {
namespace Client {

// FIXME: guard against risks of race conditions

RecordObject::RecordObject(const Common::Record &rec, ServerConnection *conn)
	: QObject(conn), m_record(rec), m_conn(conn)
{
	if (rec.isPersisted()) {
		setupChangeSubscription(rec.latestRevision());
	}
}

RecordObject::RecordObject(const Common::Table &table, ServerConnection *conn)
	: QObject(conn), m_record(table), m_conn(conn) {}

void RecordObject::create()
{
	if (m_record.isPersisted()) {
		throw Exception("Already exists");
	}
	m_conn->create(m_record).then([this](const Common::Record &rec) {
		const QVector<QString> changes = rec.changesBetween(m_record);
		m_record = rec;
		emit updated(changes);
		emit created();
		setupChangeSubscription(rec.latestRevision());
	}, [this](const Exception &e) {
		emit error(e.cause());
	});
}

void RecordObject::setValue(const QString &field, const QVariant &value)
{
	setValues({{field, value}});
}
void RecordObject::setValues(const QHash<QString, QVariant> &values)
{
	Common::Record original = m_record;
	for (auto it = values.cbegin(); it != values.cend(); ++it) {
		m_record.setValue(it.key(), it.value());
	}
	emit updated(original.changesBetween(m_record));
	m_conn->update(m_record).then([](){}, [this, original](const Exception &e) {
		const QVector<QString> changes = original.changesBetween(m_record);
		m_record = original;
		emit updated(changes);
		emit error(e.cause());
	});
}

void RecordObject::delete_()
{
	if (!m_record.isPersisted()) {
		throw Exception("Cannot delete non-existing record");
	}
	m_conn->delete_(m_record).then([this]() {
		if (m_subscription) {
			delete m_subscription;
		}
		m_record.unsetId();
		emit deleted();
	}, [this](const Exception &e) {
		emit error(e.cause());
	});
}

void RecordObject::setupChangeSubscription(const Common::Revision &from)
{
	m_subscription = m_conn->subscribe(Common::ChangeQuery(
										   Common::TableQuery(m_record.table(), Common::TableFilter("id", m_record.id())),
										   from
										   ));
	connect(m_subscription, &Subscribtion::triggered, this, [this](const Common::ChangeResponse &res) {
		for (const Common::Change &change : res.changes()) {
			if (change.type() == Common::Change::Create) {
				// handled in RecordObject::create
			} else if (change.type() == Common::Change::Update) {
				m_record = change.record();
				emit updated(change.updatedFields());
			} else if (change.type() == Common::Change::Delete) {
				m_record.unsetId();
				emit deleted();
			}
		}
	});
}

}
}
