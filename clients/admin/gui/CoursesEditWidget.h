#pragma once

#include <QWidget>

namespace Sportsed {
namespace Client {
namespace Admin {
namespace Ui {
class CoursesEditWidget;
}

class CoursesEditWidget : public QWidget
{
	Q_OBJECT

public:
	explicit CoursesEditWidget(QWidget *parent = 0);
	~CoursesEditWidget();

private:
	Ui::CoursesEditWidget *ui;
};

}
}
}
