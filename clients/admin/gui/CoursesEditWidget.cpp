#include "CoursesEditWidget.h"
#include "ui_CoursesEditWidget.h"

#include <QSortFilterProxyModel>

#include <clientlib/InterfaceSync.h>
#include <clientlib/RecordObject.h>
#include <jd-util/Util.h>
#include <jd-util/MultiLevelModel.h>
#include <jd-util-gui/ModelUtil.h>

#include "models/CourseModel.h"
#include "models/ControlModel.h"
#include "models/StageModel.h"

namespace Sportsed {
namespace Client {
namespace Admin {

CoursesEditWidget::CoursesEditWidget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::CoursesEditWidget)
{
	ui->setupUi(this);

	m_courseProxies = new JD::Util::MultiLevelModel;
	ui->coursesView->setModel(m_courseProxies->result());
	m_coursesSearchProxy = new QSortFilterProxyModel(this);
	m_coursesSearchProxy->setFilterRole(CourseModel::NameRole);
	m_coursesSearchProxy->setSortRole(Qt::DisplayRole);
	m_coursesSearchProxy->setSortCaseSensitivity(Qt::CaseInsensitive);
	m_coursesSearchProxy->sort(0);
	m_coursesStageProxy = new QSortFilterProxyModel(this);
	m_coursesStageProxy->setFilterRole(CourseModel::StageRole);
	m_courseProxies->append(m_coursesStageProxy);
	m_courseProxies->append(m_coursesSearchProxy);

	JD::Util::enableIfHasSelection(ui->stageBox, ui->coursesView);
	JD::Util::enableIfHasSelection(ui->stageBox, ui->courseAddBtn);
	JD::Util::enableIfHasSelection(ui->stageBox, ui->courseSearchEdit);

	ViewSync::buttons(ui->coursesView, ui->courseAddBtn, ui->courseDeleteBtn);
	ViewSync::sync(ui->coursesView, CourseModel::NameRole, ui->courseNameEdit);
	JD::Util::enableIfHasSelection(ui->coursesView, ui->controlsBox);

	JD::Util::applyProperty(ui->courseSearchEdit, &QLineEdit::text, &QLineEdit::textChanged, m_coursesSearchProxy, &QSortFilterProxyModel::setFilterWildcard);
	connect(ui->stageBox, QOverload<int>::of(&QComboBox::currentIndexChanged), m_coursesStageProxy, [this](int) {
		const QVariant stageId = ui->stageBox->currentData(StageModel::IdRole);
		m_coursesStageProxy->setFilterFixedString(stageId.toString());
		m_courses->setDefaultField("stage_id", stageId);
	});
}

CoursesEditWidget::~CoursesEditWidget()
{
	delete ui;
}

void CoursesEditWidget::setup(ServerConnection *conn, RecordObject *competition)
{
	m_courseProxies->setSourceModel(m_courses = new CourseModel(conn, competition->id(), this));
	ui->stageBox->setModel(new StageModel(conn, competition->id(), this));
}

}
}
}
