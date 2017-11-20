#pragma once

#include <QWidget>

#include <clientlib/RecordObject.h>

class QAbstractProxyModel;

namespace Sportsed {
namespace Client {
namespace Admin {
namespace Ui {
class CompetitionEditWidget;
}

class CompetitionEditWidget : public QWidget
{
	Q_OBJECT
public:
	explicit CompetitionEditWidget(QWidget *parent = 0);
	~CompetitionEditWidget();

	void setup(ServerConnection *conn, RecordObject *competition);

private slots:
	void disconnectClientClicked();
	void changeServerPasswordClicked();

private:
	Ui::CompetitionEditWidget *ui;

	QAbstractProxyModel *m_clientSearchProxy;
};

}
}
}
