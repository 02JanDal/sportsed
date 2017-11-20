#pragma once

#include <QAbstractListModel>
#include <QPair>

#include <commonlib/Record.h>
#include <commonlib/TableQuery.h>

#include "ServerConnection.h"

namespace Sportsed {
namespace Client {
class ServerConnection;

class AbstractRecordModel : public QAbstractListModel
{
	Q_OBJECT
	Q_PROPERTY(bool loading READ isLoading NOTIFY loadingChanged)
public:
	explicit AbstractRecordModel(const Common::Table table, const int columns, ServerConnection *conn, QObject *parent = nullptr);
	virtual ~AbstractRecordModel() {}

	enum
	{
		IdRole = Qt::UserRole
	};

	Common::Table table() const { return m_table; }

	bool isLoading() const { return m_loading; }

	int rowCount(const QModelIndex &) const override;
	int columnCount(const QModelIndex &) const override { return m_columns; }
	QVariant data(const QModelIndex &index, int role) const override;
	bool setData(const QModelIndex &index, const QVariant &value, int role) override;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

	void setDefault(const Common::Record &record);
	void setTarget(const QVector<Common::TableFilter> &target);
	void setTarget(const QString &field, const QVariant &value);

public slots:
	Future<Common::Record> add();
	void remove(const QModelIndex &index);
	void reload();

signals:
	void loadingChanged(const bool loading);

protected:
	virtual QString header(const int column) const;
	virtual QVariant value(const Common::Record &record, const int column, const int role) const;
	virtual bool setValue(Common::Record &record, const int column, const QVariant &value, const int role);
	virtual QVector<int> roleForFields(const QVector<QString> &fields) const;

	void registerField(const int role, const QString &field, const QString &header = QString());

private slots:
	void subscriptionsTriggered(const Common::ChangeResponse &res);

private:
	Common::Table m_table;
	int m_columns;
	ServerConnection *m_conn;

	bool m_loading = false;
	bool m_dirty = false;

	Common::Record m_defaultRecord;
	QVector<Common::TableFilter> m_target;

	QVector<Common::Record> m_rows;
	Subscribtion *m_subscription = nullptr;

	/// Add or update the given record
	void set(const Common::Record &record);

	QVector<QPair<QString, QString>> m_registeredColumns;
	QHash<int, QString> m_roleToField;
	QHash<QString, int> m_fieldToRole;
};

}
}
