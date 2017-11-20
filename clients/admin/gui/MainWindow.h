#pragma once

#include <clientlib/ClientMainWindow.h>

namespace Sportsed {
namespace Client {
class RecordObject;

namespace Admin {
namespace Ui {
class MainWindow;
}


class MainWindow : public ClientMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = nullptr);
	~MainWindow();

private:
	Ui::MainWindow *ui;

	RecordObject *m_competition;
	
	void serverConnected() override;
	void serverDisconnected() override;
};

}
}
}
