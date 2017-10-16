#pragma once

#include <clientlib/ClientMainWindow.h>

#include <QPrinter>

namespace Sportsed {
namespace Client {
namespace Printer {
namespace Ui {
class MainWindow;
}

class Report;
class Automation;
class AutomationsModel;

class MainWindow : public ClientMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = nullptr);
	~MainWindow();

public slots:
	void refreshPrintersClicked();
	void currentPrinterChanged();
	void pageSetupClicked();

	void printNowClicked();
	void addAutomationClicked();

private:
	Ui::MainWindow *ui;
	QPrinter *m_printer;
	AutomationsModel *m_automations;

	Report currentReport() const;
	Automation currentAutomation() const;
};

}
}
}
