#include <tst_Util.h>
#include <QDebug>
#include <QDate>
#include <jd-util-sql/DatabaseUtil.h>

#include "DatabaseEngine.h"
#include "DatabaseMigration.h"

using namespace Sportsed::Server;
using namespace Sportsed::Common;

QSqlDatabase database()
{
	QSqlDatabase db = inMemoryDb();
	DatabaseMigration::create(db);
	return db;
}

Record createRecord()
{
	Record in(Table::Profile);
	in.setValue("type", "asdf");
	in.setValue("name", "foo");
	in.setValue("value", "{}");
	return in;
}

TEST_CASE("crud operations & change recording") {
	SECTION("create") {
		QSqlDatabase db = database();
		DatabaseEngine e(db);
		Record in = createRecord();
		REQUIRE_FALSE(in.isPersisted());
		Record out;
		REQUIRE_NOTHROW(e.create(in)); // get IDs later that are not 0
		REQUIRE_NOTHROW(out = e.create(in));
		REQUIRE(out.table() == Table::Profile);
		REQUIRE(out.isComplete());
		REQUIRE_FALSE(out.isNull());
		REQUIRE(out.value("type") == "asdf");
		REQUIRE(out.value("name") == "foo");
		REQUIRE(out.value("value") == "{}");
		REQUIRE(out.id() > 0);
		REQUIRE(out.latestRevision() > 0);
		REQUIRE(out.isPersisted());

		auto changes = e.changes(ChangeQuery({TableQuery(Table::Profile)}));
		REQUIRE(changes.changes().size() == 2);
		REQUIRE(changes.lastRevision() == out.latestRevision());
		REQUIRE(changes.changes()[1].record().id() == out.id());
		REQUIRE(changes.changes()[1].type() == Change::Create);
		REQUIRE(changes.changes()[1].revision() == out.latestRevision());
		REQUIRE(changes.changes()[1].record().table() == Table::Profile);
		REQUIRE(changes.changes()[1].updatedFields().isEmpty());

		QSqlQuery q1 = db.exec("SELECT * FROM profile");
		REQUIRE(q1.next());
		REQUIRE(q1.next());
		REQUIRE(q1.value("type") == "asdf");
		REQUIRE_FALSE(q1.next());

		QSqlQuery q2 = db.exec("SELECT * FROM change");
		REQUIRE(q2.next());
		REQUIRE(q2.next());
		REQUIRE_FALSE(q2.next());
	}
	SECTION("read") {
		QSqlDatabase db = database();
		DatabaseEngine e(db);
		Record in = createRecord();
		REQUIRE_NOTHROW(e.create(in));

		Record out;
		REQUIRE_NOTHROW(out = e.read(Table::Profile, 1));
		REQUIRE(out.isPersisted());
		REQUIRE_FALSE(out.isNull());
		REQUIRE(out.isComplete());
		REQUIRE(out.id() == 1);
		REQUIRE(out.table() == Table::Profile);
		REQUIRE(out.latestRevision() == 1);
		REQUIRE(out.values() == in.values());
	}
	SECTION("update") {
		QSqlDatabase db = database();
		DatabaseEngine e(db);
		Record in = createRecord();
		REQUIRE_NOTHROW(e.create(in));

		Record update(in.table());
		update.setId(1);
		update.setValue("value", "[]");
		REQUIRE(e.update(update) > 1);

		Record updated = e.read(Table::Profile, 1);
		REQUIRE(updated.latestRevision() == 2);
		REQUIRE(updated.value("value") == "[]");

		auto changes = e.changes(ChangeQuery({TableQuery(Table::Profile)}));
		REQUIRE(changes.lastRevision() == updated.latestRevision());
		REQUIRE(changes.changes().size() == 2);
		REQUIRE(changes.changes()[0].type() == Change::Create);
		REQUIRE(changes.changes()[1].type() == Change::Update);
		REQUIRE(changes.changes()[1].record().id() == 1);
		REQUIRE(changes.changes()[1].record().table() == Table::Profile);
		REQUIRE(changes.changes()[1].updatedFields() == QVector<QString>({"value"}));
	}
	SECTION("delete") {
		QSqlDatabase db = database();
		DatabaseEngine e(db);
		Record in = createRecord();
		REQUIRE_NOTHROW(e.create(in));

		REQUIRE_NOTHROW(e.delete_(Table::Profile, 1) > 1);
		REQUIRE_THROWS_AS(e.read(Table::Profile, 1), JD::Util::Database::DoesntExistException);

		auto changes = e.changes(ChangeQuery({TableQuery(Table::Profile)}));
		REQUIRE(changes.lastRevision() == 2);
		REQUIRE(changes.changes().size() == 2);
		REQUIRE(changes.changes()[0].type() == Change::Create);
		REQUIRE(changes.changes()[1].type() == Change::Delete);
		REQUIRE(changes.changes()[1].record().id() == 1);
		REQUIRE(changes.changes()[1].record().table() == Table::Profile);
		REQUIRE(changes.changes()[1].updatedFields().isEmpty());
	}
	SECTION("change callback") {
		QSqlDatabase db = database();
		DatabaseEngine e(db);
		Record in = createRecord();

		QVector<Change> changes;
		auto cb = [&changes](const Change &c) {
			changes.append(c);
		};
		e.setChangeCallback(cb);

		REQUIRE_NOTHROW(e.create(in));
		REQUIRE(changes.size() == 1);
		REQUIRE(changes.at(0).type() == Change::Create);

		REQUIRE_NOTHROW(e.delete_(in.table(), 1));
		REQUIRE(changes.size() == 2);
		REQUIRE(changes.at(0).record().table() == Table::Profile);
	}
}

TEST_CASE("searching") {
	QSqlDatabase db = database();
	DatabaseEngine e(db);
	Record ina = createRecord();
	ina.setValue("name", "a");
	Record inb = createRecord();
	inb.setValue("name", "b");
	Record inc = createRecord();
	inc.setValue("name", "c");
	REQUIRE_NOTHROW(e.create(ina));
	REQUIRE_NOTHROW(e.create(inb));
	REQUIRE_NOTHROW(e.create(inc));

	const QVector<Record> resultA = e.find(TableQuery(Table::Profile, TableFilter("name", "a")));
	REQUIRE(resultA.size() == 1);
	REQUIRE(resultA.at(0).values() == ina.values());

	const QVector<Record> resultAll = e.find(TableQuery(Table::Profile, TableFilter("value", "{}")));
	REQUIRE(resultAll.size() == 3);

	const auto compA = e.create(Record(Table::Competition, {{"name", "comp a"}, {"sport", "Orienteering"}}));
	const auto compB = e.create(Record(Table::Competition, {{"name", "comp b"}, {"sport", "Orienteering"}}));
	const auto stage1 = e.create(Record(Table::Stage, {
											{"name", "stage 1"},
											{"date", QDate::currentDate()},
											{"discipline", "Middle"},
											{"in_totals", true},
											{"type", "Relay"},
											{"competition_id", compA.id()}
										}));
	const auto stage2 = e.create(Record(Table::Stage, {
											{"name", "stage 2"},
											{"date", QDate::currentDate()},
											{"discipline", "Middle"},
											{"in_totals", true},
											{"type", "Relay"},
											{"competition_id", compA.id()}
										}));
	const auto stage3 = e.create(Record(Table::Stage, {
											{"name", "stage 3"},
											{"date", QDate::currentDate()},
											{"discipline", "Middle"},
											{"in_totals", true},
											{"type", "Relay"},
											{"competition_id", compB.id()}
										}));
	const auto courseA = e.create(Record(Table::Course, {{"stage_id", stage1.id()}, {"name", "course a"}}));
	const auto courseB = e.create(Record(Table::Course, {{"stage_id", stage1.id()}, {"name", "course b"}}));
	const auto courseC = e.create(Record(Table::Course, {{"stage_id", stage2.id()}, {"name", "course c"}}));
	const auto courseD = e.create(Record(Table::Course, {{"stage_id", stage3.id()}, {"name", "course d"}}));
	const QVector<Record> coursesX = e.find(TableQuery(Table::Course, TableFilter("stage_id>competition_id", compA.id())));
	const QVector<Record> coursesY = e.find(TableQuery(Table::Course, TableFilter("stage_id>competition_id", compB.id())));
	REQUIRE(coursesX.size() == 3);
	REQUIRE(coursesX == (QVector<Record>() << courseA << courseB << courseC));
	REQUIRE(coursesY == (QVector<Record>() << courseD));

}

TEST_CASE("completing") {
	QSqlDatabase db = database();
	DatabaseEngine e(db);
	Record in = createRecord();
	REQUIRE_NOTHROW(e.create(in));

	Record incomplete(in.table());
	incomplete.setId(1);
	REQUIRE_FALSE(incomplete.isComplete());
	REQUIRE_FALSE(incomplete.isNull());

	Record completed = e.complete(incomplete);
	REQUIRE(completed.isComplete());
	REQUIRE(completed.values() == in.values());
}
