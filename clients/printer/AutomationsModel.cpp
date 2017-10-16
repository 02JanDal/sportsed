#include "AutomationsModel.h"

namespace Sportsed {
namespace Client {
namespace Printer {

AutomationsModel::AutomationsModel(QObject *parent)
	: QAbstractListModel(parent) {}

void AutomationsModel::add(const Automation &automation)
{
	beginInsertRows(QModelIndex(), m_automations.size(), m_automations.size());
	m_automations.append(automation);
	m_lastPrinted.append(QTime());
	endInsertRows();
}
void AutomationsModel::remove(const QModelIndex &index)
{
	const int row = index.row();
	beginRemoveRows(QModelIndex(), row, row);
	m_automations.removeAt(row);
	m_lastPrinted.removeAt(row);
	endRemoveRows();
}

int AutomationsModel::columnCount(const QModelIndex &) const
{
	return 2;
}
int AutomationsModel::rowCount(const QModelIndex &) const
{
	return m_automations.size();
}

QVariant AutomationsModel::data(const QModelIndex &index, int role) const
{
	if (role == Qt::DisplayRole) {
		if (index.column() == 0) {
			return m_automations.at(index.row()).brief();
		} else if (index.column() == 1) {
			const QTime time = m_lastPrinted.at(index.row());
			if (time.isNull()) {
				return QString();
			} else {
				return time.toString("HH:mm:ss");
			}
		}
	} else if (role == Qt::ToolTipRole) {
		return m_automations.at(index.row()).fullDescription();
	}

	return QVariant();
}

QVariant AutomationsModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
		if (section == 0) {
			return tr("Automation");
		} else if (section == 1) {
			return tr("Last Printed");
		}
	}

	return QVariant();
}

}
}
}
