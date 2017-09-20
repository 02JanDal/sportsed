#pragma once

#include <QVector>
#include <orm.capnp.h>
#include <msgpack.hpp>

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

	QString table() const { return m_table; }
	void setTable(const QString &table) { m_table = table; }

	Id id() const { return m_id; }
	void setId(const Id id) { m_id = id; }

	Revision revision() const { return m_revision; }
	void setRevision(const Revision revision) { m_revision = revision; }

	Type type() const { return m_type; }

	QVector<QString> updatedFields() const { return m_updatedFields; }
	void setUpdatedFields(const QVector<QString> &fields) { m_updatedFields = fields; }

	static Change unpack(const msgpack::unpacker)

	static Change fromReader(const Schema::Change::Reader &reader);
	void build(Schema::Change::Builder builder) const;

private:
	QString m_table;
	Id m_id;
	Revision m_revision;
	Type m_type;
	QVector<QString> m_updatedFields;
};

}
}
