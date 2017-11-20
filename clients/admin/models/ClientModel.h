#pragma once

#include <clientlib/AbstractRecordModel.h>

namespace Sportsed {
namespace Client {
namespace Admin {

class ClientModel : public AbstractRecordModel
{
	Q_OBJECT
public:
	explicit ClientModel(ServerConnection *conn, QObject *parent = nullptr);

	enum
	{
		NameRole = Qt::UserRole + 100,
		IPRole
	};

private:
	bool setValue(Common::Record &record, const int column, const QVariant &value, const int role) override;
};

}
}
}
