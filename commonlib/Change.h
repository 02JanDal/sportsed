#pragma once

#include <QVector>
#include <QJsonObject>

#include "Record.h"

namespace Sportsed {
namespace Common {

class Change
{
public:
	enum Type
	{
		Create,
		Update,
		Delete
	};

	explicit Change(const Type &type = Create);

	Record record() const { return m_record; }
	void setRecord(const Record &rec) { m_record = rec; }

	Revision revision() const { return m_revision; }
	void setRevision(const Revision revision) { m_revision = revision; }

	Type type() const { return m_type; }

	QVector<QString> updatedFields() const { return m_updatedFields; }
	void setUpdatedFields(const QVector<QString> &fields) { m_updatedFields = fields; }

	static Change fromJson(const QJsonObject &obj);
	QJsonObject toJson() const;

private:
	Record m_record;
	Revision m_revision;
	Type m_type;
	QVector<QString> m_updatedFields;
};

}
}
