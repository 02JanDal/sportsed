#include "CompetitionEditWidget.h"
#include "ui_CompetitionEditWidget.h"

#include <QAbstractProxyModel>

#include <jd-util-gui/ModelUtil.h>
#include <jd-util-gui/GuiUtil.h>
#include <clientlib/InterfaceSync.h>

#include "models/StageModel.h"
#include "models/ClientModel.h"

using namespace JD;

namespace Sportsed {
namespace Client {
namespace Admin {

CompetitionEditWidget::CompetitionEditWidget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::CompetitionEditWidget)
{
	ui->setupUi(this);
	ui->stageTypeEdit->addItem(tr("Single"), QStringLiteral("single"));
	ui->stageTypeEdit->addItem(tr("Relay"), QStringLiteral("relay"));
	setEnabled(false);

	connect(ui->clientDisconnectBtn, &QPushButton::clicked, this, &CompetitionEditWidget::disconnectClientClicked);
	connect(ui->changeClientPasswordBtn, &QPushButton::clicked, this, &CompetitionEditWidget::changeServerPasswordClicked);

	ui->clientsView->setModel(m_clientSearchProxy = Util::searchableModel(ui->clientSearchEdit));
	Util::enableIfHasSelection(ui->clientsView, ui->clientDisconnectBtn);

	Util::enableIfHasSelection(ui->stageBox, ui->stageWidget);
	ViewSync::sync(ui->stageBox, StageModel::NameRole, ui->stageNameEdit);
	ViewSync::sync(ui->stageBox, StageModel::TypeRole, ui->stageTypeEdit);
	ViewSync::sync(ui->stageBox, StageModel::DisciplineRole, ui->stageDisciplineBox);
	ViewSync::sync(ui->stageBox, StageModel::DateRole, ui->stageDateEdit);
	ViewSync::sync(ui->stageBox, StageModel::InTotalsRole, ui->stageTotalesBox);
}

CompetitionEditWidget::~CompetitionEditWidget()
{
	delete ui;
}

void CompetitionEditWidget::setup(ServerConnection *conn, RecordObject *competition)
{
	ItemSync::sync(competition, "name", ui->nameEdit);
	ItemSync::sync(competition, "sport", ui->sportBox);

	m_clientSearchProxy->setSourceModel(new ClientModel(conn, this));
	ui->stageBox->setModel(new StageModel(conn, competition->id(), this));
	ViewSync::buttons(ui->stageBox, ui->stageAddBtn, ui->stageRemoveBtn);

	setEnabled(true);
}

void CompetitionEditWidget::disconnectClientClicked()
{
	JD::Util::Gui::notImplemented();
}

void CompetitionEditWidget::changeServerPasswordClicked()
{
	JD::Util::Gui::notImplemented();
}

}
}
}
