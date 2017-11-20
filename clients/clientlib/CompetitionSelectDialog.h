#pragma once

#include <QDialog>

#include <commonlib/Record.h>

namespace Sportsed {
namespace Client {
class ServerConnection;

namespace Ui {
class CompetitionSelectDialog;
}

class CompetitionSelectDialog : public QDialog
{
	Q_OBJECT
public:
	explicit CompetitionSelectDialog(ServerConnection *conn, QWidget *parent = 0);
	~CompetitionSelectDialog();

	Common::Id competitionId() const { return m_compId; }

private slots:
	void refreshExisting();

private:
	Ui::CompetitionSelectDialog *ui;
	ServerConnection *m_conn;

	Common::Id m_compId;
};

}
}
