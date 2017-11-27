#pragma once

#include <QWidget>

class QSortFilterProxyModel;

namespace JD {
namespace Util {
class MultiLevelModel;
}
}

namespace Sportsed {
namespace Client {
class ServerConnection;
class RecordObject;

namespace Admin {
class CourseModel;

namespace Ui {
class CoursesEditWidget;
}

class CoursesEditWidget : public QWidget
{
	Q_OBJECT

public:
	explicit CoursesEditWidget(QWidget *parent = 0);
	~CoursesEditWidget();

	void setup(ServerConnection *conn, RecordObject *competition);

private:
	Ui::CoursesEditWidget *ui;
	QSortFilterProxyModel *m_coursesStageProxy;
	QSortFilterProxyModel *m_coursesSearchProxy;
	JD::Util::MultiLevelModel *m_courseProxies;
	CourseModel *m_courses;
};

}
}
}
