#include "Change.h"

#include <jd-util/Functional.h>
#include <jd-util/Json.h>

using namespace Sportsed::Common;
using namespace JD::Util;

namespace Sportsed {
namespace Common {

Change::Change(const Type &type)
	: m_type(type) {}

Change Change::parse(const QJsonObject &obj)
{
	Type t;
	const QString type = Json::ensureString(obj, "type");
	if (type == "create") {
		t = Create;
	} else if (type == "update") {
		t = Update;
	} else if (type == "delete") {
		t = Delete;
	}

	Change change = Change(t);
	change.m_id = Json::ensureIsType<Id>(obj, "id");
	change.m_revision = Json::ensureIsType<Revision>(obj, "revision");
	change.m_table = Json::ensureString(obj, "table");
	change.m_updatedFields = Json::ensureIsArrayOf<QString>(obj, "fields");
	return change;
}
QJsonObject Change::toJson() const
{
	QJsonObject obj;
	obj.insert("id", m_id);
	obj.insert("revision", m_revision);
	obj.insert("table", m_table);
	switch (m_type) {
	case Create: obj.insert("type", "create"); break;
	case Update: obj.insert("type", "update"); break;
	case Delete: obj.insert("type", "delete"); break;
	}
	obj.insert("fields", Json::toJson(m_updatedFields));
	return obj;
}

}
}
