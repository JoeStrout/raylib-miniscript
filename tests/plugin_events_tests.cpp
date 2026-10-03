//
//  plugin_events_tests.cpp
//  raylib-miniscript
//
//  Tests for src/PluginEvents.h: the thread-safe event queue, draining it into a
//  MiniScript list, and PluginRooted.
//
//  Build:  cmake --build build --target plugin_events_tests
//  Run:    ./build/plugin_events_tests
//

#include "miniscript.h"
#include "PluginEvents.h"

#include <stdio.h>
#include <thread>
#include <vector>

// Plugin.cpp is not part of this test; the engine defines this.
bool PluginsAreShutDown() { return false; }

using namespace MiniScript;

static int failures = 0;
static int checks = 0;

static void ok(bool condition, const char* what) {
	checks++;
	if (condition) {
		printf("ok   %s\n", what);
	} else {
		printf("FAIL %s\n", what);
		failures++;
	}
}

struct Ev { int kind; double id; };

static Value EvToValue(const Ev& e) {
	ValueDict m;
	m.SetValue(String("kind"), Value(e.kind));
	m.SetValue(String("id"), Value(e.id));
	return DynamicMap(m);
}

static void testFifo() {
	PluginEventQueue<Ev> q;
	ok(q.Size() == 0, "new queue is empty");
	Ev e;
	ok(!q.Pop(e), "Pop on an empty queue returns false");
	q.Push({1, 10});
	q.Push({2, 20});
	q.Push({3, 30});
	ok(q.Size() == 3, "Size counts pushes");
	ok(q.Pop(e) && e.id == 10, "Pop returns the oldest first");
	ok(q.Pop(e) && e.id == 20, "then the next");
	q.Clear();
	ok(q.Size() == 0 && !q.Pop(e), "Clear empties the queue");
}

static void testBound() {
	PluginEventQueue<Ev> q(4);
	for (int i = 1; i <= 10; i++) q.Push({0, (double)i});
	ok(q.Size() == 4, "queue never exceeds its capacity");
	ok(q.Dropped() == 6, "drops are counted");
	Ev e;
	ok(q.Pop(e) && e.id == 7, "the oldest events were the ones dropped");
}

static void testThreads() {
	PluginEventQueue<Ev> q(0);   // 0 = unbounded
	const int kThreads = 8, kPer = 5000;
	std::vector<std::thread> threads;
	for (int t = 0; t < kThreads; t++) {
		threads.emplace_back([&q, t] { for (int i = 0; i < kPer; i++) q.Push({t, (double)i}); });
	}
	for (std::thread& th : threads) th.join();
	ok(q.Size() == (size_t)(kThreads * kPer), "no events lost across 8 pushing threads");
	// Per-thread order must be preserved.
	std::vector<double> last(kThreads, -1);
	bool ordered = true;
	Ev e;
	while (q.Pop(e)) {
		if (e.id <= last[e.kind]) ordered = false;
		last[e.kind] = e.id;
	}
	ok(ordered, "each thread's events stay in order");
}

static void testDrain() {
	PluginEventQueue<Ev> q;
	Value empty = PluginDrainEvents(q, EvToValue);
	ok(empty.Type() == ValueType::List && empty.GetList().Count() == 0, "draining an empty queue gives an empty list");
	for (int i = 0; i < 100; i++) q.Push({i, (double)i * 2});
	Value first = PluginDrainEvents(q, EvToValue, 30);
	ValueList list = first.GetList();
	ok(list.Count() == 30, "drain honors its max");
	ok(q.Size() == 70, "and leaves the rest queued");
	Value m = list[5];
	ok(m.Type() == ValueType::Map
		&& m.GetDict().Lookup(String("kind"), Value::Null).IntValue() == 5
		&& m.GetDict().Lookup(String("id"), Value::Null).DoubleValue() == 10,
		"events arrive as maps, in order");
	Value rest = PluginDrainEvents(q, EvToValue, 1000);
	ok(rest.GetList().Count() == 70 && q.Size() == 0, "a large max drains everything");
}

static void testRooted() {
	PluginRooted r;
	Value s = String("kept across collections");
	r.Set(s);
	GCManager::FullCollectGarbage();
	ok(r.Get().ToString() == "kept across collections", "a rooted Value survives a collection");
	r.Set(Value(42));   // replaces (and un-roots) the first
	ok(r.Get().IntValue() == 42, "Set replaces the held value");
	r.Clear();
	r.Clear();          // harmless twice
	ok(r.Get().IsNull(), "Clear releases the value");
}

int main() {
	GCManager::Init();
	value_init_constants();
	ErrorTypes::Init();

	testFifo();
	testBound();
	testThreads();
	testDrain();
	testRooted();

	printf("\n%d checks, %d failures\n", checks, failures);
	return failures ? 1 : 0;
}
