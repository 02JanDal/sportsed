#include "ChangeResponse.h"

#include <jd-util/Json.h>

using namespace JD::Util;

namespace Sportsed {
namespace Common {

ChangeResponse::ChangeResponse() {}

ChangeResponse ChangeResponse::fromJson(const QJsonObject &obj)
{
	ChangeResponse response;
	response.m_query = Json::ensureIsType<ChangeQuery>(obj, "query");
	response.m_changes = Json::ensureIsArrayOf<Change>(obj, "changes");
	response.m_lastRevision = Json::ensureIsType<Revision>(obj, "last_revision");
	return response;
}

QJsonObject ChangeResponse::toJson() const
{
	return QJsonObject({
						   {"query", m_query.toJson()},
						   {"changes", Json::toJsonArray(m_changes)},
						   {"last_revision", Json::toJson(m_lastRevision)}
					   });
}

}
}
