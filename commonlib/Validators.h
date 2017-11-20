#pragma once

#include <jd-util/Exception.h>

class QVariant;

namespace Sportsed {
namespace Common {
class Record;
enum class Table;

DECLARE_EXCEPTION(Validation)
DECLARE_EXCEPTION_X(MissingRequiredField, "Missing required field", ValidationException)
DECLARE_EXCEPTION_X(UnknownField, "Invalid field", ValidationException)
DECLARE_EXCEPTION_X(Coercion, "Invalid types", ValidationException)

class BaseValidator
{
public:
	virtual ~BaseValidator();

	virtual void validateField(const QString &field, const QVariant &value);
	virtual void validateRecord(const Record &record);

	virtual QVariant coerce(const QString &field, const QVariant &value);
	virtual Record coerceRecord(const Record &record);

	static BaseValidator *getValidator(const Table &table);

	enum FieldType
	{
		ID,
		Char,
		String,
		Integer,
		Real,
		Boolean,
		Date,
		Time,
		DateTime,
		IP,
		JSON
	};

protected:
	virtual QHash<QString, FieldType> fields() const = 0;

	int metaType(const QString &field) const;
};

}
}
