#pragma once

#include <QSqlDatabase>

#include <jd-util/Exception.h>
#include <jd-util-sql/DatabaseUtil.h>

#include "commonlib/ChangeQuery.h"
#include "commonlib/ChangeResponse.h"
#include "commonlib/Record.h"

namespace Sportsed {
namespace Server {

DECLARE_EXCEPTION(Validation)

class DatabaseEngine
{
public:
	explicit DatabaseEngine(QSqlDatabase &db);

	Common::ChangeResponse changes(const Common::ChangeQuery &query);

	Common::Record create(const Common::Record &record);
	Common::Record read(const Common::Table &table, const Common::Id id);
	Common::Revision update(const Common::Record &record);
	Common::Revision delete_(const Common::Table &table, const Common::Id id);

	QVector<Common::Record> find(const Common::TableQuery &query);

	Common::Record complete(const Common::Record &record);

	using ChangeCallback = std::function<void(Common::Change)>;
	void setChangeCallback(const ChangeCallback &cb) { m_changeCb = cb; }

private:
	QSqlDatabase m_db;
	ChangeCallback m_changeCb;

	Common::Revision insertChange(const QString &table, const Common::Id id,
								  const Common::Change::Type type, const Common::Record &record);
	void validateValues(const Common::Record &record, const QSqlRecord &sqlRecord, const bool strict);
};

}
}
