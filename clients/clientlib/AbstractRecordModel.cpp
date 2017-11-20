#include "AbstractRecordModel.h"

#include <jd-util/Functional.h>
#include <commonlib/ChangeQuery.h>
#include <commonlib/ChangeResponse.h>

namespace Sportsed {
namespace Client {

AbstractRecordModel::AbstractRecordModel(const Common::Table table, const int columns, ServerConnection *conn, QObject *parent)
	: QAbstractListModel(parent), m_table(table), m_columns(columns), m_conn(conn)
{
	setDefault(Common::Record(m_table));
	connect(conn, &ServerConnection::connectedChanged, this, [this](const bool connected) {
		if (connected) {
			reload();
		}
	});
}

int AbstractRecordModel::rowCount(const QModelIndex &) const
{
	if (!m_subscription && !m_loading) {
		qWarning() << "Attempting to retrieve data from not yet reload()'d model, did you forget to call reload()?";
	}
	return m_rows.size();
}

QVariant AbstractRecordModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid() || index.row() >= m_rows.size()) {
		return QVariant();
	} else if (role == IdRole) {
		return m_rows.at(index.row()).id();
	} else {
		return value(m_rows.at(index.row()), index.column(), role);
	}
}
bool AbstractRecordModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (!index.isValid() || index.row() >= m_rows.size()) {
		return false;
	} else {
		if (index.data(role) == value) {
			return true;
		}

		const int row = index.row();
		Common::Record original = m_rows.at(row);
		if (setValue(m_rows[row], index.column(), value, role)) {
			const QVector<QString> changed = m_rows.at(row).changesBetween(original);
			emit dataChanged(this->index(row, 0), this->index(row, m_columns), roleForFields(changed));

			Common::Record changes(original.table());
			changes.setId(original.id());
			for (const QString &field : changed) {
				changes.setValue(field, m_rows[row].value(field));
			}
			m_conn->update(changes).then([](){}, [this, original]() {
				set(original);
			});
			return true;
		} else {
			return false;
		}
	}
}
QVariant AbstractRecordModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (section >= 0 && section < m_columns && orientation == Qt::Horizontal && role == Qt::DisplayRole) {
		return header(section);
	} else {
		return QVariant();
	}
}

void AbstractRecordModel::setDefault(const Common::Record &record)
{
	Q_ASSERT_X(record.table() == m_table, "AbstractRecordModel::setDefault", "default record has wrong table");
	m_defaultRecord = record;
}

void AbstractRecordModel::setTarget(const QVector<Common::TableFilter> &target)
{
	if (m_target == target) {
		return;
	}

	m_target = target;
	m_dirty = true;
	reload();
}
void AbstractRecordModel::setTarget(const QString &field, const QVariant &value)
{
	setTarget(QVector<Common::TableFilter>() << Common::TableFilter(field, value));
}

Future<Common::Record> AbstractRecordModel::add()
{
	return m_conn->create(m_defaultRecord);
}
void AbstractRecordModel::remove(const QModelIndex &index)
{
	beginRemoveRows(QModelIndex(), index.row(), index.row());
	const Common::Record rec = m_rows.takeAt(index.row());
	endRemoveRows();

	// BUG: what happens if we change the target between the removal call and the error?
	// add the record back if the removal fails
	m_conn->delete_(rec).then([](){}, [this, rec]() {
		set(rec);
	});
}

void AbstractRecordModel::reload()
{
	if (!m_conn->isConnected()) {
		QMetaObject::Connection *connection = new QMetaObject::Connection;
		*connection = connect(m_conn, &ServerConnection::connectedChanged, this, [this, connection](const bool connected) {
			if (connected) {
				reload();
				disconnect(*connection);
				delete connection;
			}
		});
		return;
	}

	m_loading = true;
	emit loadingChanged(m_loading);
	m_conn->find(Common::TableQuery(m_table, m_target)).then([this](const QVector<Common::Record> &records) {
		m_loading = false;
		emit loadingChanged(m_loading);
		beginResetModel();
		m_rows = records;
		endResetModel();

		const Common::Revision latest = std::max_element(m_rows.cbegin(), m_rows.cend(), [](const Common::Record &a, const Common::Record &b) {
			return a.latestRevision() < b.latestRevision();
		})->latestRevision();

		if (m_subscription) {
			delete m_subscription;
		}
		m_subscription = m_conn->subscribe(Common::ChangeQuery(Common::TableQuery(m_table, m_target), latest));
		connect(m_subscription, &Subscribtion::triggered, this, &AbstractRecordModel::subscriptionsTriggered);
	}, [this]() {
		m_loading = false;
		emit loadingChanged(m_loading);
	});
}

void AbstractRecordModel::subscriptionsTriggered(const Common::ChangeResponse &res)
{
	for (const Common::Change &change : res.changes()) {
		if (change.type() == Common::Change::Create || change.type() == Common::Change::Update) {
			set(change.record());
		} else if (change.type() == Common::Change::Delete) {
			for (int i = 0; i < m_rows.size(); ++i) {
				if (m_rows.at(i).id() == change.record().id()) {
					beginRemoveRows(QModelIndex(), i, i);
					m_rows.removeAt(i);
					endRemoveRows();
				}
			}
		}
	}
}

void AbstractRecordModel::set(const Common::Record &record)
{
	for (int i = 0; i < m_rows.size(); ++i) {
		if (m_rows.at(i).id() == record.id()) {
			const QVector<QString> changes = record.changesBetween(m_rows.at(i));
			m_rows[i] = record;
			emit dataChanged(index(i, 0), index(i, m_columns), roleForFields(changes));
			return;
		}
	}

	// didn't find it, add it
	beginInsertRows(QModelIndex(), m_rows.size(), m_rows.size());
	m_rows.append(record);
	endInsertRows();
}

void AbstractRecordModel::registerField(const int role, const QString &field, const QString &header)
{
	m_roleToField.insert(role, field);
	m_fieldToRole.insert(field, role);
	if (!header.isNull()) {
		m_registeredColumns.append(qMakePair(field, header));
	}
}
QString AbstractRecordModel::header(const int column) const
{
	return m_registeredColumns.at(column).second;
}
QVariant AbstractRecordModel::value(const Common::Record &record, const int column, const int role) const
{
	if (role == Qt::DisplayRole || role == Qt::EditRole) {
		return record.value(m_registeredColumns.at(column).first);
	} else if (m_roleToField.contains(role)) {
		return record.value(m_roleToField.value(role));
	} else {
		return QVariant();
	}
}
bool AbstractRecordModel::setValue(Common::Record &record, const int column, const QVariant &value, const int role)
{
	if (role == Qt::EditRole && column < m_registeredColumns.size()) {
		record.setValue(m_registeredColumns.at(column).first, value);
		return true;
	} else if (m_roleToField.contains(role)) {
		record.setValue(m_roleToField.value(role), value);
		return true;
	} else {
		return false;
	}
}
QVector<int> AbstractRecordModel::roleForFields(const QVector<QString> &fields) const
{
	return JD::Util::Functional::map(fields, [this](const QString &field) { return m_fieldToRole.value(field); });
}

}
}
