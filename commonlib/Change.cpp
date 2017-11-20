#include "Change.h"

#include <jd-util/Functional.h>
#include <jd-util/Json.h>

using namespace Sportsed::Common;
using namespace JD::Util;

namespace Sportsed {
namespace Common {

Change::Change(const Type &type)
	: m_type(type) {}

Change Change::fromJson(const QJsonObject &obj)
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
	change.m_record = Json::ensureIsType<Record>(obj, "record");
	change.m_revision = Json::ensureIsType<Revision>(obj, "revision");
	change.m_updatedFields = Json::ensureIsArrayOf<QString>(obj, "fields");
	return change;
}
QJsonObject Change::toJson() const
{
	QJsonObject obj;
	obj.insert("record", Json::toJson(m_record));
	obj.insert("revision", Json::toJson(m_revision));
	switch (m_type) {
	case Create: obj.insert("type", "create"); break;
	case Update: obj.insert("type", "update"); break;
	case Delete: obj.insert("type", "delete"); break;
	}
	obj.insert("fields", Json::toJsonArray(m_updatedFields));
	return obj;
}

}
}
