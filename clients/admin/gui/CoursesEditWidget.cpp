#include "CoursesEditWidget.h"
#include "ui_CoursesEditWidget.h"

namespace Sportsed {
namespace Client {
namespace Admin {

CoursesEditWidget::CoursesEditWidget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::CoursesEditWidget)
{
	ui->setupUi(this);
}

CoursesEditWidget::~CoursesEditWidget()
{
	delete ui;
}

}
}
}
