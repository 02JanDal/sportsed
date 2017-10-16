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
	T waitFor(const Future<T> &future, const QString &msg = QString())
	{
		QProgressDialog dlg;
		dlg.setLabelText(msg);
		dlg.setMaximum(0);
		dlg.setMinimum(0);
		dlg.show();
		return future.get();
	}

	void loadProfile(const QString &name);

signals:
	void profileChanged();

protected:
	void registerActions(QAction *connect, QAction *disconnectAct, QAction *loadProfileAct, QAction *storeProfileAct);

private slots:
	void connectClicked();
	void disconnectClicked();
	void loadProfileClicked();
	void storeProfileClicked();

private:
	ServerConnection *m_conn;
	QActionGroup *m_disconnectedActions;
	QActionGroup *m_connectedActions;

	QString m_profileType;
	QString m_profileName;
	QString m_profileData;
};



}
}
