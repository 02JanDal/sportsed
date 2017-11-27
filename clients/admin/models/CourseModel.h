#pragma once

#include <clientlib/AbstractRecordModel.h>

namespace Sportsed {
namespace Client {
namespace Admin {

class CourseModel : public AbstractRecordModel
{
	Q_OBJECT
public:
	explicit CourseModel(ServerConnection *conn, const Common::Id competitionId, QObject *parent = nullptr);

	enum
	{
		StageRole = Qt::UserRole + 100,
		NameRole
	};

private:
	bool isEditable(const int) const override { return true; }
};

}
}
}
