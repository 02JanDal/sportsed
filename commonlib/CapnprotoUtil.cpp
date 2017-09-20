#include "CapnprotoUtil.h"

#include <QString>

namespace Sportsed {
namespace Common {

QString toQString(const ::kj::StringPtr &string)
{
	return QString::fromUtf8(string.cStr(), int(string.size()));
}

::kj::StringPtr fromQString(const QString &string)
{
	const QByteArray bytes = string.toUtf8();
	return ::kj::StringPtr(bytes.constData(), std::size_t(bytes.size()));
}

}
}
