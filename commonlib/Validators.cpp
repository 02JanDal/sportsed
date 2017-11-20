#include "Validators.h"

#include <QHostAddress>

#include <jd-util/Formatting.h>
#include <cmath>

#include "Record.h"

namespace Sportsed {
namespace Common {

namespace detail {

QT_WARNING_PUSH
QT_WARNING_DISABLE_CLANG("-Wglobal-constructors")
using Fields = QHash<QString, BaseValidator::FieldType>;
static Fields meta = {
	{"key", BaseValidator::String},
	{"value", BaseValidator::String}
};
static Fields change = {
	{"type", BaseValidator::Char},
	{"record_id", BaseValidator::ID},
	{"record_table", BaseValidator::String},
	{"timestamp", BaseValidator::Integer},
	{"fields", BaseValidator::String}
};
static Fields profile = {
	{"type", BaseValidator::String},
	{"name", BaseValidator::String},
	{"value", BaseValidator::JSON}
};
static Fields client = {
	{"name", BaseValidator::String},
	{"ip", BaseValidator::IP}
};
static Fields competition = {
	{"name", BaseValidator::String},
	{"sport", BaseValidator::String}
};
static Fields stage = {
	{"competition_id", BaseValidator::ID},
	{"name", BaseValidator::String},
	{"type", BaseValidator::String},
	{"discipline", BaseValidator::String},
	{"date", BaseValidator::Date},
	{"in_totals", BaseValidator::Boolean},
};
static Fields control = {
	{"stage_id", BaseValidator::ID},
	{"name", BaseValidator::String},
	{"special", BaseValidator::String}
};
static Fields course = {
	{"stage_id", BaseValidator::ID},
	{"name", BaseValidator::String}
};
static Fields courseControl = {
	{"control_id", BaseValidator::ID},
	{"course_id", BaseValidator::ID},
	{"order", BaseValidator::Integer},
	{"distance_from_previous", BaseValidator::Real}
};
static Fields class_ = {
	{"stage_id", BaseValidator::ID},
	{"name", BaseValidator::String}
};
QT_WARNING_POP

#define MAKE_VALIDATOR(name, global) class name ## Validator : public BaseValidator { QHash<QString, FieldType> fields() const override { return global; } };
MAKE_VALIDATOR(Meta, meta)
MAKE_VALIDATOR(Change, change)
MAKE_VALIDATOR(Profile, profile)
MAKE_VALIDATOR(Client, client)
MAKE_VALIDATOR(Competition, competition)
MAKE_VALIDATOR(Stage, stage)
MAKE_VALIDATOR(Control, control)
MAKE_VALIDATOR(Course, course)
MAKE_VALIDATOR(CourseControl, courseControl)
MAKE_VALIDATOR(Class, class_)
#undef MAKE_VALIDATOR

inline bool isInteger(const QVariant &value) {
	const QVariant::Type type = value.type();
	return type == QVariant::Char || type == QVariant::Int || type == QVariant::UInt || type == QVariant::LongLong || type == QVariant::ULongLong;
}
inline bool isReal(const QVariant &value) {
	const QVariant::Type type = value.type();
	return isInteger(value) || type == QVariant::Double;
}

using ValidatorsHash = QHash<Table, BaseValidator *>;
Q_GLOBAL_STATIC(ValidatorsHash, validators)
}

BaseValidator::~BaseValidator() {}

void BaseValidator::validateField(const QString &field, const QVariant &value)
{
	if (!fields().contains(field)) {
		throw UnknownFieldException();
	}

	const FieldType type = fields().value(field);
	switch (type) {
	case Sportsed::Common::BaseValidator::ID: [[clang::fallthrough]];
	case Sportsed::Common::BaseValidator::Integer:
		if (!detail::isInteger(value)) {
			throw ValidationException("Not an integer");
		}
		break;
	case Sportsed::Common::BaseValidator::Char:
		if (value.type() != QVariant::Char) {
			throw ValidationException("Not a char");
		}
		break;
	case Sportsed::Common::BaseValidator::String:
		if (value.type() != QVariant::String) {
			throw ValidationException("Not a string");
		}
		break;
	case Sportsed::Common::BaseValidator::Real:
		if (!detail::isReal(value)) {
			throw ValidationException("Not a real number");
		}
		break;
	case Sportsed::Common::BaseValidator::Boolean:
		if (value.type() != QVariant::Bool) {
			throw ValidationException("Not a bool");
		}
		break;
	case Sportsed::Common::BaseValidator::Date:
		if (value.type() != QVariant::Date) {
			throw ValidationException("Not a date");
		}
		break;
	case Sportsed::Common::BaseValidator::Time:
		if (value.type() != QVariant::Time) {
			throw ValidationException("Not a time");
		}
		break;
	case Sportsed::Common::BaseValidator::DateTime:
		if (value.type() != QVariant::DateTime) {
			throw ValidationException("Not a date/time");
		}
		break;
	case Sportsed::Common::BaseValidator::IP:
		if (value.type() != QVariant::String) {
			throw ValidationException("Not an IP");
		} else {
			QHostAddress addr;
			if (!addr.setAddress(value.toString())) {
				throw ValidationException("Not an IP");
			}
		}
		break;
	case Sportsed::Common::BaseValidator::JSON:
		if (value.type() != qMetaTypeId<QJsonObject>()) {
			throw ValidationException("Not JSON");
		}
		break;
	}
}

void BaseValidator::validateRecord(const Record &record)
{
	for (const QString &field : fields().keys()) {
		if (!record.values().contains(field)) {
			throw MissingRequiredFieldException("Missing required field '%1'" % field);
		}
	}

	for (auto it = record.values().cbegin(); it != record.values().cend(); ++it) {
		validateField(it.key(), it.value());
	}
}

QVariant BaseValidator::coerce(const QString &field, const QVariant &value)
{
	if (!fields().contains(field)) {
		throw UnknownFieldException();
	}
	QVariant res(value);
	if (!res.convert(metaType(field))) {
		throw CoercionException(QStringLiteral("Unable to convert from %1 to %2")
								% value.typeName()
								% QMetaType::typeName(metaType(field)));
	}
	return res;
}

Record BaseValidator::coerceRecord(const Record &record)
{
	Record out(record.table());
	out.setId(record.id());
	out.setLatestRevision(record.latestRevision());
	for (const QString &field : record.values().keys()) {
		out.setValue(field, coerce(field, record.value(field)));
	}
	return out;
}

BaseValidator *BaseValidator::getValidator(const Table &table)
{
	if (!detail::validators->contains(table)) {
		switch (table) {
		case Table::Null: return nullptr;
		case Table::Meta: detail::validators->insert(table, new detail::MetaValidator); break;
		case Table::Change: detail::validators->insert(table, new detail::ChangeValidator); break;
		case Table::Profile: detail::validators->insert(table, new detail::ProfileValidator); break;
		case Table::Client: detail::validators->insert(table, new detail::ClientValidator); break;
		case Table::Competition: detail::validators->insert(table, new detail::CompetitionValidator); break;
		case Table::Stage: detail::validators->insert(table, new detail::StageValidator); break;
		case Table::Control: detail::validators->insert(table, new detail::ControlValidator); break;
		case Table::Course: detail::validators->insert(table, new detail::CourseValidator); break;
		case Table::CourseControl: detail::validators->insert(table, new detail::CourseControlValidator); break;
		case Table::Class: detail::validators->insert(table, new detail::ClassValidator); break;
		}
	}
	return detail::validators->value(table);
}

int BaseValidator::metaType(const QString &field) const
{
	switch (fields().value(field)) {
	case Sportsed::Common::BaseValidator::ID: return qMetaTypeId<Common::Id>();
	case Sportsed::Common::BaseValidator::Char: return QMetaType::QChar;
	case Sportsed::Common::BaseValidator::String: return QMetaType::QString;
	case Sportsed::Common::BaseValidator::Integer: return QMetaType::LongLong;
	case Sportsed::Common::BaseValidator::Real: return QMetaType::Double;
	case Sportsed::Common::BaseValidator::Boolean: return QMetaType::Bool;
	case Sportsed::Common::BaseValidator::Date: return QMetaType::QDate;
	case Sportsed::Common::BaseValidator::Time: return QMetaType::QTime;
	case Sportsed::Common::BaseValidator::DateTime: return QMetaType::QDateTime;
	case Sportsed::Common::BaseValidator::IP: return QMetaType::QString;
	case Sportsed::Common::BaseValidator::JSON: return QMetaType::QJsonDocument;
	}
}

}
}
