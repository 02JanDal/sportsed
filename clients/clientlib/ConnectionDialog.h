#pragma once

#include <QDialog>

namespace Sportsed {
namespace Client {

namespace Ui {
class ConnectionDialog;
}

class ServerConnection;

class ConnectionDialog : public QDialog
{
	Q_OBJECT

public:
	explicit ConnectionDialog(ServerConnection *conn, QWidget *parent = nullptr);
	~ConnectionDialog();

private:
	void connectClicked();

private:
	Ui::ConnectionDialog *ui;
	ServerConnection *m_conn;
};

}
}
