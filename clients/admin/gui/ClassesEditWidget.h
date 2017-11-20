#pragma once

#include <QWidget>

namespace Sportsed {
namespace Client {
namespace Admin {
namespace Ui {
class ClassesEditWidget;
}

class ClassesEditWidget : public QWidget
{
	Q_OBJECT

public:
	explicit ClassesEditWidget(QWidget *parent = 0);
	~ClassesEditWidget();

private:
	Ui::ClassesEditWidget *ui;
};

}
}
}
