#include "StageModel.h"

namespace Sportsed {
namespace Client {
namespace Admin {

StageModel::StageModel(ServerConnection *conn, const Common::Id competitionId, QObject *parent)
	: AbstractRecordModel(Common::Table::Stage, 2, conn, parent)
{
	setDefault(Common::Record(table(), {
								  {"competition_id", competitionId},
								  {"name", QString()},
								  {"type", "single"},
								  {"discipline", tr("Middle")},
								  {"date", QDate::currentDate()},
								  {"in_totals", true}
							  }));

	registerField(CompetitionRole, "competition_id");
	registerField(NameRole, "name", tr("Name"));
	registerField(TypeRole, "type", tr("Type"));
	registerField(DisciplineRole, "discipline", tr("Discipline"));
	registerField(DateRole, "date", tr("Date"));
	registerField(InTotalsRole, "in_totals", tr("In totals"));

	setTarget("competition_id", competitionId);
}

QVariant StageModel::value(const Common::Record &record, const int column, const int role) const
{
	if (role == Qt::DisplayRole && column == 4) {
		return record.value("in_totals").toBool() ? tr("Yes") : tr("No");
	} else {
		return AbstractRecordModel::value(record, column, role);
	}
}

}
}
}
