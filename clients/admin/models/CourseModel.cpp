#include "CourseModel.h"

namespace Sportsed {
namespace Client {
namespace Admin {

CourseModel::CourseModel(ServerConnection *conn, const Common::Id competitionId, QObject *parent)
	: AbstractRecordModel(Common::Table::Course, 1, conn, parent)
{
	setDefault(Common::Record(table(), {
								  {"name", tr("New Course")}
							  }));

	registerField(StageRole, "stage_id");
	registerField(NameRole, "name", tr("Name"));

	setTarget("stage_id>competition_id", competitionId);
}

}
}
}
