#include "ClientModel.h"

namespace Sportsed {
namespace Client {
namespace Admin {

ClientModel::ClientModel(ServerConnection *conn, QObject *parent)
	: AbstractRecordModel(Common::Table::Client, 2, conn, parent)
{
	registerField(NameRole, "name", tr("Name"));
	registerField(IPRole, "ip", tr("IP"));

	reload();
}

bool ClientModel::setValue(Common::Record &, const int, const QVariant &, const int)
{
	return false;
}

}
}
}
