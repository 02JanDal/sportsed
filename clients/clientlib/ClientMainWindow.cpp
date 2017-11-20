#include "ClientMainWindow.h"

#include <QTimer>
#include <QAction>
#include <QActionGroup>
#include <QInputDialog>
#include <QProgressDialog>
#include <QMessageBox>
#include <QSettings>
#include <clientlib/ServerConnection.h>
#include <jd-util/Util.h>

#include "ConnectionDialog.h"
#include "CompetitionSelectDialog.h"

namespace Sportsed {
namespace Client {

ClientMainWindow::ClientMainWindow(const QString &profileType, QWidget *parent)
	: JD::Util::MainWindow(parent),
#ifdef SPORTSED_SKIP_AUTH
	  m_conn(new EmbeddedServerConnection(this)),
#else
	  m_conn(new TcpServerConnection(this)),
#endif
	  m_disconnectedActions(new QActionGroup(this)),
	  m_connectedActions(new QActionGroup(this)),
	  m_profileType(profileType)
{
	m_connectedActions->setEnabled(false);
	m_disconnectedActions->setEnabled(true);

	connect(m_conn, &ServerConnection::connectedChanged, this, [this](const bool connected) {
		if (!connected) {
			serverDisconnected();
		}
	});

	JD::Util::applyProperty(m_conn, &ServerConnection::isConnected, &ServerConnection::connectedChanged,
							this, [this](const bool connected) {
		if (connected) {
			CompetitionSelectDialog dlg(m_conn, this);
			if (dlg.exec() == QDialog::Rejected) {
				close();
				return;
			} else {
				setCompetitionId(dlg.competitionId());
			}
			serverConnected();
		}
		m_connectedActions->setEnabled(connected);
		m_disconnectedActions->setEnabled(!connected);
		if (centralWidget()) {
			centralWidget()->setEnabled(connected);
		}
	});

	QTimer::singleShot(1, this, [this]() {
		centralWidget()->setEnabled(false);
		connectClicked();
	});

	QSettings settings;
	if (settings.contains("LastProfile") && !settings.value("LastProfile").toString().isEmpty()) {
		loadProfile(settings.value("LastProfile").toString());
	}
}
ClientMainWindow::~ClientMainWindow()
{
	QSettings settings;
	if (!m_profileName.isEmpty()) {
		settings.setValue("LastProfile", m_profileName);
	}
}

void ClientMainWindow::loadProfile(const QString &name)
{
	try {
		const auto records = waitFor(
					m_conn->find(Common::TableQuery(
									 Common::Table::Profile,
									 {Common::TableFilter("type", m_profileType), Common::TableFilter("name", name)})),
					tr("Loading profile..."), this);
		if (records.isEmpty()) {
			QMessageBox::warning(this, tr("Unknown profile"), tr("There exists no profile with this name"));
		} else {
			m_profileName = name;
			m_profileData = records.first().value("value").toString();
			emit profileChanged();
			setWindowFilePath(name);
		}
	} catch (Exception &e) {
		QMessageBox::warning(this, tr("Loading failed"), tr("Unable to retrieve profile from server:\n\n%1").arg(e.cause()));
	}
}

void ClientMainWindow::registerActions(QAction *connectAct, QAction *disconnectAct, QAction *loadProfileAct, QAction *storeProfileAct)
{
	connect(connectAct, &QAction::triggered, this, &ClientMainWindow::connectClicked);
	connect(disconnectAct, &QAction::triggered, this, &ClientMainWindow::disconnectClicked);
	connect(loadProfileAct, &QAction::triggered, this, &ClientMainWindow::loadProfileClicked);
	connect(storeProfileAct, &QAction::triggered, this, &ClientMainWindow::storeProfileClicked);
	m_connectedActions->addAction(disconnectAct);
	m_connectedActions->addAction(loadProfileAct);
	m_connectedActions->addAction(storeProfileAct);
	m_disconnectedActions->addAction(connectAct);
}

void ClientMainWindow::setCompetitionId(const Common::Id id)
{
	m_competitionId = id;
}

void ClientMainWindow::connectClicked()
{
#ifdef SPORTSED_SKIP_AUTH
	m_conn->connectToServer("debug client", "asdf");
#else
	ConnectionDialog dlg(m_conn, this);
	if (dlg.exec() == ConnectionDialog::Accepted) {
		m_competitionId = dlg.competitionId();
		serverConnected();
	} else {
		close();
	}
#endif
}
void ClientMainWindow::disconnectClicked()
{
	m_conn->disconnectFromServer();
}

void ClientMainWindow::loadProfileClicked()
{
	const QString name = QInputDialog::getText(this,
											   tr("Load profile..."),
											   tr("Enter the name of the profile to load..."),
											   QLineEdit::Normal,
											   m_profileName);
	if (name.isEmpty()) {
		return;
	}

	loadProfile(name);
}
void ClientMainWindow::storeProfileClicked()
{
	const QString name = QInputDialog::getText(this,
											   tr("Store profile..."),
											   tr("Enter the name under which to store the current profile..."),
											   QLineEdit::Normal,
											   m_profileName);
	if (name.isEmpty()) {
		return;
	}

	try {
		QProgressDialog dlg(this);
		dlg.setLabelText(tr("Storing profile..."));
		dlg.setMaximum(0);
		dlg.show();
		const auto records = m_conn->find(Common::TableQuery(
											  Common::Table::Profile,
											  {Common::TableFilter("type", m_profileType), Common::TableFilter("name", name)})).get();
		if (records.isEmpty()) {
			m_conn->create(Common::Record(Common::Table::Profile, {
											  {"type", m_profileType},
											  {"name", name},
											  {"value", m_profileData}
										  })).get();
		} else {
			Common::Record record = records.first();
			record.setValue("value", m_profileData);
			m_conn->update(record).get();
		}

		m_profileName = name;
		emit profileChanged();
		setWindowFilePath(m_profileName);
	} catch (Exception &e) {
		QMessageBox::warning(this, tr("Storage failed"), tr("Unable to store profile to server:\n\n%1").arg(e.cause()));
	}
}

}
}
