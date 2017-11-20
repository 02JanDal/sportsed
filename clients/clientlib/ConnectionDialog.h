#pragma once

#include <QDialog>

#include <commonlib/Record.h>

namespace Sportsed {
namespace Client {

namespace Ui {
class ConnectionDialog;
}

class TcpServerConnection;

// TODO: this is essentially a wizard, make it a QWizard
// TODO: add support for LocalServerConnection
class ConnectionDialog : public QDialog
{
	Q_OBJECT

public:
	explicit ConnectionDialog(TcpServerConnection *conn, QWidget *parent = nullptr);
	~ConnectionDialog();

	Common::Id competitionId() const { return m_compId; }

private:
	void connectClicked();
	void refreshCompetitionsClicked();
	void createCompetitionClicked();

private:
	Ui::ConnectionDialog *ui;
	TcpServerConnection *m_conn;

	Common::Id m_compId;
};

}
}
