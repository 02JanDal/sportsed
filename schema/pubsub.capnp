@0xa0da60e6f9ad92d3;

struct SignalPayload {
	entries @0 :List(Entry);
	struct Entry {
		key @0 :Text;
		value :union {
			bool @1 :Bool;
			text @2 :Text;
			integer @3 :Int64;
			pointer @4 :AnyPointer;
		}
	}
}

interface Channel {
	subscribe @0 (cb :ChannelCallback) -> (handle :Handle);
}

interface ChannelCallback {
	call @0 (value :SignalPayload);
}

interface Handle {}
