#pragma once

#include <QObject>

#include <commonlib/Record.h>
#include "ServerConnection.h"

namespace Sportsed {
namespace Client {

class RecordObject : public QObject
{
	Q_OBJECT
public:
	explicit RecordObject(const Common::Record &rec, ServerConnection *conn);
	explicit RecordObject(const Common::Table &table, ServerConnection *conn);

	Common::Id id() const { return m_record.id(); }
	Common::Record record() const { return m_record; }
	QVariant value(const QString &field) const { return m_record.value(field); }

	template <typename T>
	T value(const QString &field) const { return m_record.value(field).value<T>(); }

public slots:
	void create();
	void setValue(const QString &field, const QVariant &value);
	void setValues(const QHash<QString, QVariant> &values);
	void delete_();

signals:
	void created();
	void updated(const QVector<QString> &fields);
	void deleted();
	void error(const QString &error);

private:
	Common::Record m_record;
	ServerConnection *m_conn;
	Subscribtion *m_subscription = nullptr;

	void setupChangeSubscription(const Common::Revision &from);
};

}
}
