#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <clientlib/RecordObject.h>

namespace Sportsed {
namespace Client {
namespace Admin {

MainWindow::MainWindow(QWidget *parent) :
	ClientMainWindow("admin", parent),
	ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::showAbout);
	connect(ui->actionAboutQt, &QAction::triggered, this, &MainWindow::showAboutQt);
	connect(ui->actionQuit, &QAction::triggered, this, &MainWindow::quit);
	registerActions(ui->actionConnect, ui->actionDisconnect, ui->actionLoadProfile, ui->actionStoreProfile);
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::serverConnected()
{
	m_competition = new RecordObject(
				waitFor(conn()->read(Common::Table::Competition, competitionId()), tr("Loading competition"), this),
				conn());
	ui->competitionTab->setup(conn(), m_competition);
	ui->coursesTab->setup(conn(), m_competition);
}
void MainWindow::serverDisconnected()
{
}

}
}
}
