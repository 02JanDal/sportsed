#pragma once

#include <jd-util-gui/MainWindow.h>
#include <QProgressDialog>

#include "ServerConnection.h"

class QActionGroup;

namespace Sportsed {
namespace Client {

class ClientMainWindow : public JD::Util::MainWindow
{
	Q_OBJECT
public:
	explicit ClientMainWindow(const QString &profileType, QWidget *parent);
	virtual ~ClientMainWindow();

	template <typename T>
	static T waitFor(Future<T> future, const QString &msg = QString(), QWidget *parent = nullptr)
	{
		if (!future.hasValue()) {
			QProgressDialog dlg(parent);
			dlg.setLabelText(msg);
			dlg.setMaximum(0);
			dlg.setMinimum(0);
			future.then([&dlg]() { dlg.accept(); });
			dlg.exec();
		}
		return future.get();
	}

	void loadProfile(const QString &name);

signals:
	void profileChanged();

protected:
	void registerActions(QAction *connect, QAction *disconnectAct, QAction *loadProfileAct, QAction *storeProfileAct);
	virtual void serverConnected() = 0;
	virtual void serverDisconnected() = 0;

	void setCompetitionId(const Common::Id id);
	Common::Id competitionId() const { return m_competitionId; }
	ServerConnection *conn() const { return m_conn; }

private slots:
	void connectClicked();
	void disconnectClicked();
	void loadProfileClicked();
	void storeProfileClicked();

private:
#ifdef SPORTSED_SKIP_AUTH
	EmbeddedServerConnection *m_conn;
#else
	TcpServerConnection *m_conn;
#endif
	QActionGroup *m_disconnectedActions;
	QActionGroup *m_connectedActions;

	Common::Id m_competitionId;

	QString m_profileType;
	QString m_profileName;
	QString m_profileData;
};


}
}
