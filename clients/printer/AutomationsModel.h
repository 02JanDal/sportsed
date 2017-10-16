#pragma once

#include <QAbstractListModel>

#include "Automation.h"

namespace Sportsed {
namespace Client {
namespace Printer {

class AutomationsModel : public QAbstractListModel
{
	Q_OBJECT
public:
	explicit AutomationsModel(QObject *parent = nullptr);

	void add(const Automation &automation);
	void remove(const QModelIndex &index);

	int columnCount(const QModelIndex &parent) const override;
	int rowCount(const QModelIndex &parent) const override;
	QVariant data(const QModelIndex &index, int role) const override;
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;

private:
	QVector<Automation> m_automations;
	QVector<QTime> m_lastPrinted;
};

}
}
}
