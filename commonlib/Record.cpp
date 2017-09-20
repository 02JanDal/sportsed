#include "Record.h"

#include "CapnprotoUtil.h"

namespace Sportsed {
namespace Common {

Record::Record() {}

Record Record::fromReader(const Schema::Record::Reader &reader)
{
	Record record;
	record.m_table = toQString(reader.getTable());
	record.m_id = reader.getId();
	record.m_latestRevision = reader.getLatestRevision();
	for (const auto r : reader.getFields()) {
		QVariant value;
		switch (r.getValue().which()) {
		case Schema::Record::Field::Value::BOOL:
			value = r.getValue().getBool();
			break;
		case Schema::Record::Field::Value::TEXT:
			value = toQString(r.getValue().getText());
			break;
		case Schema::Record::Field::Value::INTEGER:
			value = r.getValue().getInteger();
			break;
		case Schema::Record::Field::Value::POINTER:
			// TODO sending lists and structs
			break;
		}

		record.m_values.insert(toQString(r.getName()), value);
	}

	return record;
}
void Record::build(Schema::Record::Builder builder) const
{
	builder.setTable(fromQString(m_table));
	builder.setId(m_id);
	builder.setLatestRevision(m_latestRevision);
	auto listBuilder = builder.initFields(uint(m_values.size()));
	const auto keys = m_values.keys();
	for (int i = 0; i < keys.size(); ++i) {
		auto fieldBuilder = listBuilder[uint(i)];
		fieldBuilder.setName(fromQString(keys.at(i)));
		const QVariant value = m_values.value(keys.at(i));
		switch (value.type()) {
		case QVariant::String:
			fieldBuilder.getValue().setText(fromQString(value.toString()));
			break;
		case QVariant::Bool:
			fieldBuilder.getValue().setBool(value.toBool());
			break;
		default:
			fieldBuilder.getValue().setInteger(value.toLongLong());
			break;
			// TODO: list and structs
		}
	}
}

}
}
