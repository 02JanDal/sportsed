#pragma once

#include <clientlib/AbstractRecordModel.h>

namespace Sportsed {
namespace Client {
namespace Admin {

class StageModel : public AbstractRecordModel
{
	Q_OBJECT
public:
	explicit StageModel(ServerConnection *conn, const Common::Id competitionId, QObject *parent = nullptr);

	enum
	{
		CompetitionRole = Qt::UserRole + 100,
		NameRole,
		TypeRole,
		DisciplineRole,
		DateRole,
		InTotalsRole
	};

private:
	QVariant value(const Common::Record &record, const int column, const int role) const override;
};

}
}
}
