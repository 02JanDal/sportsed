#pragma once

#include <QWidget>

namespace Sportsed {
namespace Client {
namespace Admin {
namespace Ui {
class CompetitorsEditWidget;
}

class CompetitorsEditWidget : public QWidget
{
	Q_OBJECT

public:
	explicit CompetitorsEditWidget(QWidget *parent = 0);
	~CompetitorsEditWidget();

private:
	Ui::CompetitorsEditWidget *ui;
};

}
}
}
