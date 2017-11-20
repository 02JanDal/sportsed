#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QPrinterInfo>
#include <QPageSetupDialog>

#include "Automation.h"
#include "AutomationsModel.h"
#include "Report.h"
#include "Generator.h"

namespace Sportsed {
namespace Client {
namespace Printer {

MainWindow::MainWindow(QWidget *parent) :
	ClientMainWindow("printer", parent),
	ui(new Ui::MainWindow),
	m_printer(new QPrinter),
	m_automations(new AutomationsModel(this))
{
	ui->setupUi(this);

	ui->automationsView->setModel(m_automations);

	connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::showAbout);
	connect(ui->actionAboutQt, &QAction::triggered, this, &MainWindow::showAboutQt);
	connect(ui->actionQuit, &QAction::triggered, this, &MainWindow::quit);
	registerActions(ui->actionConnect, ui->actionDisconnect, ui->actionLoadProfile, ui->actionStoreProfile);

	connect(ui->refreshPrintersBtn, &QPushButton::clicked, this, &MainWindow::refreshPrintersClicked);
	connect(ui->printerBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::currentPrinterChanged);
	connect(ui->pageSetupBtn, &QPushButton::clicked, this, &MainWindow::pageSetupClicked);

	connect(ui->printNowBtn, &QPushButton::clicked, this, &MainWindow::printNowClicked);
	connect(ui->newAutomationAddBtn, &QPushButton::clicked, this, &MainWindow::addAutomationClicked);
	connect(ui->reportTypeBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](const int index) {
		ui->startBox->setEnabled(index == 1);
	});

	refreshPrintersClicked();
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::refreshPrintersClicked()
{
	const QString current = ui->printerBox->currentText();
	ui->printerBox->clear();
	ui->printerBox->addItems(QPrinterInfo::availablePrinterNames());
	ui->printerBox->setCurrentText(current);
}
void MainWindow::currentPrinterChanged()
{
	if (ui->printerBox->currentIndex() == -1) {
		ui->pageSetupBtn->setEnabled(false);
		ui->locationLbl->clear();
		ui->descriptionLbl->clear();
		ui->modelLbl->clear();
		ui->tabReports->setEnabled(false);
		ui->tabAutomation->setEnabled(false);
	} else {
		const QPrinterInfo info = QPrinterInfo::printerInfo(ui->printerBox->currentText());
		m_printer->setPrinterName(info.printerName());
		ui->pageSetupBtn->setEnabled(true);
		ui->locationLbl->setText(info.location());
		ui->descriptionLbl->setText(info.description());
		ui->modelLbl->setText(info.makeAndModel());
		ui->tabReports->setEnabled(true);
		ui->tabAutomation->setEnabled(true);
	}
}
void MainWindow::pageSetupClicked()
{
	QPageSetupDialog dlg(m_printer, this);
	dlg.exec();
	ui->printerBox->setCurrentText(m_printer->printerName());
}

void MainWindow::printNowClicked()
{
	Generator(m_printer).generate(currentReport());
}
void MainWindow::addAutomationClicked()
{
	m_automations->add(currentAutomation());
}

Report MainWindow::currentReport() const
{
	Report r(Report::Type(ui->reportTypeBox->currentIndex()));
	if (r.type() == Report::StartsByTime && ui->startBox->currentIndex() != -1) {
		r.setStart(ui->startBox->currentData().value<Common::Record>());
	}
	r.setClassesTogether(ui->classesTogetherBtn->isChecked());
	//r.setClasses(); // TODO: gather from model
	return r;
}
Automation MainWindow::currentAutomation() const
{
	Automation a;
	a.setReport(currentReport());
	if (ui->newAutomationEveryBtn->isChecked()) {
		a.setPrintedEvery(ui->newAutomationEveryEdit->value());
	} else {
		a.setPrintedOnComplete();
		a.setPrintedDelay(ui->newAutomationWaitBtn->isChecked());
		a.setPrintDelay(ui->newAutomationWaitEdit->value());
		a.setPrintedAfter(ui->newAutomationAfterBtn->isChecked() ? ui->newAutomationAfterEdit->time() : QTime());
	}
	return a;
}

void MainWindow::serverConnected() {}
void MainWindow::serverDisconnected() {}

}
}
}
