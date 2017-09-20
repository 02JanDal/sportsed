@0xdc294cfad6677162;

using Cxx = import "/capnp/c++.capnp";
$Cxx.namespace("Sportsed::Schema");

struct Record {
	table @0 :Text;
	id @1 :UInt64;
	latestRevision @2 :UInt64;
	fields @3 :List(Field);
	struct Field {
		name @0 :Text;
		value :union {
			bool @1 :Bool;
			text @2 :Text;
			integer @3 :Int64;
			pointer @4 :AnyPointer;
		}
	}
	complete @4 :Bool;

	interface Subscriber {
		subscribeTo @0 () -> (handle :Handle);
	}
	subscriber @5 :Subscriber;
}

enum ChangeType {
	create @0;
	update @1;
	delete @2;
}

struct Change {
	table @0 :Text;
	id @1 :UInt64;
	revision @2 :UInt64;
	type @3 :ChangeType;
	updatedFields @4 :List(Text);

	interface RecordReader {
		read @0 () -> (record :Record);
	}
	reader @5 :RecordReader;
}

struct TableQuery {
	table @0 :Text;
	filters @1 :List(Filter);
	struct Filter {
		field @0 :Text;
		value :union {
			bool @1 :Bool;
			text @2 :Text;
			integer @3 :Int64;
		}
	}
}

struct ChangeQuery {
	fromRevision @0 :UInt64;
	tables @1 :List(TableQuery);
}
struct ChangeResponse {
	query @0 :ChangeQuery;
	changes @1 :List(Change);
	lastRevision @2 :UInt64;
}

interface Database {
	version @0 () -> (number :Int16); # database schema version
	changes @1 (query :ChangeQuery) -> (response :ChangeResponse);
	setupSubscription @2 (cb :ChangeCallback) -> (subscriber :Subscriber);

	create @3 (record :Record) -> (id :UInt64, revision :UInt64);
	read @4 (table :Text, id :UInt64) -> (record :Record);
	update @5 (record :Record) -> (revision :UInt64);
	delete @6 (table :Text, id :UInt64) -> (revision :UInt64);
}

interface Subscriber {
	subscribeTo @0 (query :ChangeQuery) -> (handle :Handle);
}

interface ChangeCallback {
	call @0 (changes :ChangeResponse);
}

interface Handle {}
