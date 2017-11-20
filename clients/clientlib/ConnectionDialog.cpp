#include "ConnectionDialog.h"
#include "ui_ConnectionDialog.h"

#include <QMessageBox>
#include <QInputDialog>

#include "ClientMainWindow.h"
#include "ServerConnection.h"

namespace Sportsed {
namespace Client {

ConnectionDialog::ConnectionDialog(TcpServerConnection *conn, QWidget *parent) :
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
			if (ui->settingsStackedWidget->currentWidget() == ui->connectionOptionsWidget) {
				ui->settingsStackedWidget->setCurrentWidget(ui->competitionSelectionWidget);
				refreshCompetitionsClicked();
			} else if (ui->competitionsBox->currentIndex() != -1) {
				accept();
			}
		} else {
			ui->buttonBox->button(QDialogButtonBox::Open)->setEnabled(true);
			ui->settingsStackedWidget->setEnabled(true);
			ui->progressBar->hide();
			ui->statusLbl->hide();
		}
	});
	connect(m_conn, &ServerConnection::connectionError, this, [this]() {
		ui->buttonBox->button(QDialogButtonBox::Open)->setEnabled(true);
		ui->settingsStackedWidget->setEnabled(false);
		ui->progressBar->hide();
		ui->statusLbl->hide();
	});

	connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &ConnectionDialog::reject);
	connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &ConnectionDialog::connectClicked);
	connect(ui->refreshCompetitionsButton, &QPushButton::clicked, this, &ConnectionDialog::refreshCompetitionsClicked);
	connect(ui->createCompetitionBtn, &QPushButton::clicked, this, &ConnectionDialog::createCompetitionClicked);

	ui->hostEdit->setText("localhost");
	ui->nameEdit->setText("debug client");
	ui->passwordEdit->setText("");
	connectClicked();
}

ConnectionDialog::~ConnectionDialog()
{
	delete ui;
}

void ConnectionDialog::connectClicked()
{
	ui->progressBar->show();
	ui->statusLbl->show();
	ui->settingsStackedWidget->setEnabled(false);
	ui->buttonBox->button(QDialogButtonBox::Open)->setEnabled(false);
	try {
		QHostAddress addr = QHostAddress(ui->hostEdit->text());
		if (ui->hostEdit->text().toLower() == "localhost") {
			addr = QHostAddress::LocalHostIPv6;
		}
		m_conn->connectToServer(addr, ui->nameEdit->text(), ui->passwordEdit->text());
	} catch (Exception &e) {
		QMessageBox::critical(this, tr("Connection error"), tr("Connection error: %1") % e.cause());
	}
}

void ConnectionDialog::refreshCompetitionsClicked()
{
	if (!m_conn->isConnected()) {
		return;
	}

	const auto competitions = ClientMainWindow::waitFor(
				m_conn->find(Common::TableQuery(Common::Table::Competition)),
				tr("Loading competitions..."), this);

	ui->competitionsBox->clear();
	for (const auto &comp : competitions) {
		ui->competitionsBox->addItem(comp.value("name").toString(), comp.id());
	}

	if (ui->competitionsBox->count() > 0) {
		ui->competitionsBox->setCurrentIndex(0);
		ui->buttonBox->button(QDialogButtonBox::Open)->click();
	}
}

void ConnectionDialog::createCompetitionClicked()
{
	if (!m_conn->isConnected()) {
		return;
	}

	bool ok = false;
	const QString name = QInputDialog::getText(this, tr("New Competition"), tr("Please give the competition a name"),
											   QLineEdit::Normal, QString(), &ok);
	if (!ok) {
		return;
	}

	Common::Record newComp(Common::Table::Competition);
	newComp.setValue("name", name);
	const auto record = ClientMainWindow::waitFor(
				m_conn->create(newComp), tr("Creating competition %1...") % name, this);
	m_compId = record.id();
	accept();
}

}
}
