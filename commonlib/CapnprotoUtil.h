#pragma once

#include <kj/string.h>
#include <capnp/list.h>

#include <QVector>

class QString;

namespace Sportsed {
namespace Common {

QString toQString(const ::kj::StringPtr &string);
::kj::StringPtr fromQString(const QString &string);

namespace detail {
template <typename Type, typename Reader>
inline Type toType(const Reader &reader) { return Type::fromReader(reader); }
template <>
inline QString toType<QString, capnp::Text::Reader>(const capnp::Text::Reader &reader) { return toQString(reader); }

template <typename Type, typename ListBuilder>
inline void setAtIndex(const uint index, const Type &value, ListBuilder builder)
{
	value.build(builder[index]);
}
template <>
inline void setAtIndex<QString, capnp::List<capnp::Text>::Builder>(const uint index, const QString &value, capnp::List<capnp::Text>::Builder builder)
{
	builder.set(index, fromQString(value));
}
}

template <typename Type, typename ListReader>
QVector<Type> readList(const ListReader &list)
{
	QVector<Type> out;
	for (const auto r : list) {
		out.append(detail::toType<Type>(r));
	}
	return out;
}

template <typename Type, typename Builder, typename Initer>
void writeList(const QVector<Type> &list, Builder builder, Initer initer)
{
	auto listBuilder = (builder.*initer)(uint(list.size()));
	for (int i = 0; i < list.size(); ++i) {
		detail::setAtIndex<Type>(uint(i), list.at(i), listBuilder);
	}
}

}
}
