#include "CompetitionSelectDialog.h"
#include "ui_CompetitionSelectDialog.h"

#include <jd-util/Util.h>

#include "ServerConnection.h"
#include "ClientMainWindow.h"

using namespace JD::Util;

namespace Sportsed {
namespace Client {

CompetitionSelectDialog::CompetitionSelectDialog(ServerConnection *conn, QWidget *parent) :
	QDialog(parent),
	ui(new Ui::CompetitionSelectDialog),
	m_conn(conn)
{
	ui->setupUi(this);

	QPushButton *openBtn = ui->buttonBox->button(QDialogButtonBox::Open);

	connect(ui->buttonBox, &QDialogButtonBox::accepted, this, [this, openBtn]() {
		openBtn->setEnabled(false);
		if (ui->newBtn->isChecked()) {
			const Common::Record rec = ClientMainWindow::waitFor(
						m_conn->create(Common::Record(Common::Table::Competition, {
														  {"name", ui->nameEdit->text()},
														  {"sport", QString()}
													  })));
			m_compId = rec.id();
		} else {
			m_compId = ui->competitionsBox->currentData().value<Common::Id>();
		}
		accept();
		openBtn->setEnabled(true);
	});
	connect(ui->refreshBtn, &QPushButton::clicked, this, &CompetitionSelectDialog::refreshExisting);

	auto updateUiState = [this, openBtn]() {
		openBtn->setEnabled(
					ui->newBtn->isChecked() || (ui->competitionsBox->currentIndex() != -1));
		ui->nameEdit->setEnabled(ui->newBtn->isChecked());
		ui->competitionsBox->setEnabled(ui->existingBtn->isChecked());
		ui->refreshBtn->setEnabled(ui->existingBtn->isChecked());
	};
	connect(ui->newBtn, &QRadioButton::toggled, this, updateUiState);
	connect(ui->competitionsBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, updateUiState);

	refreshExisting();
}

CompetitionSelectDialog::~CompetitionSelectDialog()
{
	delete ui;
}

void CompetitionSelectDialog::refreshExisting()
{
	const QVector<Common::Record> competitions = ClientMainWindow::waitFor(
				m_conn->find(Common::TableQuery(Common::Table::Competition)));
	ui->competitionsBox->clear();
	for (const Common::Record &comp : competitions) {
		ui->competitionsBox->addItem(comp.value("name").toString(), comp.id());
	}
}

}
}
