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

	Table table() const { return m_table; }
	void setTable(const Table &table) { m_table = table; }

	Id id() const { return m_id; }
	void setId(const Id id) { m_id = id; }

	Revision revision() const { return m_revision; }
	void setRevision(const Revision revision) { m_revision = revision; }

	Type type() const { return m_type; }

	QVector<QString> updatedFields() const { return m_updatedFields; }
	void setUpdatedFields(const QVector<QString> &fields) { m_updatedFields = fields; }

	static Change fromJson(const QJsonObject &obj);
	QJsonObject toJson() const;

private:
	Table m_table;
	Id m_id;
	Revision m_revision;
	Type m_type;
	QVector<QString> m_updatedFields;
};

}
}
