#include "CompetitorsEditWidget.h"
#include "ui_CompetitorsEditWidget.h"

namespace Sportsed {
namespace Client {
namespace Admin {

CompetitorsEditWidget::CompetitorsEditWidget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::CompetitorsEditWidget)
{
	ui->setupUi(this);
}

CompetitorsEditWidget::~CompetitorsEditWidget()
{
	delete ui;
}

}
}
}
