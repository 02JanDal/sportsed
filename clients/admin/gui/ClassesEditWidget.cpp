#include "ClassesEditWidget.h"
#include "ui_ClassesEditWidget.h"

namespace Sportsed {
namespace Client {
namespace Admin {

ClassesEditWidget::ClassesEditWidget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::ClassesEditWidget)
{
	ui->setupUi(this);
}

ClassesEditWidget::~ClassesEditWidget()
{
	delete ui;
}

}
}
}
