#include "ConnectionDialog.h"
#include "ui_ConnectionDialog.h"

#include "ServerConnection.h"

namespace Sportsed {
namespace Client {

ConnectionDialog::ConnectionDialog(ServerConnection *conn, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::ConnectionDialog),
	m_conn(conn)
{
	ui->setupUi(this);

	ui->progressBar->hide();
	ui->statusLbl->hide();
	connect(m_conn, &ServerConnection::status, ui->statusLbl, &QLabel::setText);
	connect(m_conn, &ServerConnection::connectedChanged, this, [this](const bool connected) {
		if (connected) {
			accept();
		} else {
			ui->settingsWidget->setEnabled(false);
			ui->progressBar->hide();
			ui->statusLbl->hide();
		}
	});
	connect(m_conn, &ServerConnection::connectionError, this, [this]() {
		ui->settingsWidget->setEnabled(false);
		ui->progressBar->hide();
		ui->statusLbl->hide();
	});

	connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &ConnectionDialog::reject);
	connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &ConnectionDialog::connectClicked);
}

ConnectionDialog::~ConnectionDialog()
{
	delete ui;
}

void ConnectionDialog::connectClicked()
{
	ui->progressBar->show();
	ui->statusLbl->show();
	ui->settingsWidget->setEnabled(false);
	m_conn->connectToServer(QHostAddress(ui->hostEdit->text()), ui->passwordEdit->text());
}

}
}
