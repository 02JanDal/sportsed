#include <catch.hpp>

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

namespace Catch {
template <>
struct StringMaker<QString>
{
	static std::string convert(const QString &str) { return '"' + str.toStdString() + '"'; }
};

template <typename T>
struct StringMaker<QSet<T>>
{
	static std::string convert(const QSet<T> &set)
	{
		if (set.isEmpty()) {
			return "{}";
		}
		std::string str = "{";
		for (const T &t : set) {
			str += StringMaker<T>::convert(t) + ", ";
		}
		str.erase(str.size()-2);
		return str + "}";
	}
};
}

QSqlDatabase inMemoryDb()
{
	QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE",
												QStringLiteral("db_conn_%1").arg(Catch::getCurrentNanosecondsSinceEpoch()));
	db.setDatabaseName(":memory:");
	REQUIRE(db.open());
	REQUIRE(db.isOpen());
	return db;
}
