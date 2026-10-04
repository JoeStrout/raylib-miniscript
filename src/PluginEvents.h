//
//  PluginEvents.h
//  raylib-miniscript
//
//  Helpers for plugins that wrap asynchronous SDKs (Steam callbacks, purchases,
//  ads).  The VM and GC belong to the main thread, so SDK callbacks must never
//  build MiniScript values or call into script.  Instead they push plain C++
//  structs onto a PluginEventQueue (from any thread), and an intrinsic such as
//  `steam.poll` drains the queue on the main thread, converting the structs to
//  MiniScript maps only then:
//
//      struct SteamEvent { int kind; double id; };
//      static PluginEventQueue<SteamEvent> events;       // SDK callback: events.Push({1, 42});
//
//      static Value EventToValue(const SteamEvent& e) {
//          ValueDict m;
//          m.SetValue(String("kind"), Value(e.kind));
//          m.SetValue(String("id"), Value(e.id));
//          return DynamicMap(m);
//      }
//      // in the poll intrinsic:  return IntrinsicResult(PluginDrainEvents(events, EventToValue));
//
//  Also here: PluginRooted, for a Value a plugin keeps across frames.  Clear() each
//  one in your shutdown hook (and reset hook, if it holds script closures).
//

#ifndef PLUGINEVENTS_H
#define PLUGINEVENTS_H

#include "miniscript.h"
#include <deque>
#include <mutex>

// True once PluginsShutdown has finished; GC roots are gone by then.  Defined here
// (not in Plugin.cpp) so anything that includes this header links without the engine.
inline bool& PluginsShutDownFlag() { static bool flag = false; return flag; }
inline bool PluginsAreShutDown() { return PluginsShutDownFlag(); }

// A thread-safe, bounded FIFO.  When full, Push drops the OLDEST event (a script
// that never polls must not grow memory without limit) and counts the drop.  That
// suits lossy events; for purchases/entitlements pass capacity 0 (unbounded).
template<typename T>
class PluginEventQueue {
public:
	explicit PluginEventQueue(size_t capacity = 1024) : capacity_(capacity) {}

	// Any thread.
	void Push(const T& event) {
		std::lock_guard<std::mutex> lock(mutex_);
		if (capacity_ > 0 && items_.size() >= capacity_) {
			items_.pop_front();
			dropped_++;
		}
		items_.push_back(event);
	}

	// Oldest first.  Returns false if the queue is empty.
	bool Pop(T& out) {
		std::lock_guard<std::mutex> lock(mutex_);
		if (items_.empty()) return false;
		out = items_.front();
		items_.pop_front();
		return true;
	}

	size_t Size() const {
		std::lock_guard<std::mutex> lock(mutex_);
		return items_.size();
	}

	// How many events were discarded because the queue was full.
	size_t Dropped() const {
		std::lock_guard<std::mutex> lock(mutex_);
		return dropped_;
	}

	void Clear() {
		std::lock_guard<std::mutex> lock(mutex_);
		items_.clear();
	}

private:
	mutable std::mutex mutex_;
	std::deque<T> items_;
	size_t capacity_;
	size_t dropped_ = 0;
};

// Main thread only.  Pops up to `max` events and returns them as a MiniScript
// list, each converted with toValue.  An empty list means nothing is waiting.
template<typename T>
MiniScript::Value PluginDrainEvents(PluginEventQueue<T>& queue,
		MiniScript::Value (*toValue)(const T&), int max = 64) {
	MiniScript::ValueList out;
	T event;
	for (int i = 0; i < max && queue.Pop(event); i++) out.Add(toValue(event));
	return MiniScript::DynamicList(out);
}

// Keeps a Value alive across garbage collections for as long as it is held.  Any
// Value a plugin stores in a static or member (a callback, a cached map) needs
// this; a Value only on the C++ stack during one intrinsic call does not.
class PluginRooted {
public:
	PluginRooted() {}
	~PluginRooted() { Clear(); }
	PluginRooted(const PluginRooted&) = delete;
	PluginRooted& operator=(const PluginRooted&) = delete;

	void Set(MiniScript::Value v) {
		Clear();
		if (PluginsAreShutDown()) return;   // roots are gone; hold nothing rather than a dangling Value
		value_ = v;
		MiniScript::GCManager::AddRoot(value_);
		held_ = true;
	}
	void Clear() {
		if (!held_) return;
		if (!PluginsAreShutDown()) MiniScript::GCManager::RemoveRoot(value_);
		value_ = MiniScript::Value::Null;
		held_ = false;
	}
	MiniScript::Value Get() const { return value_; }

private:
	MiniScript::Value value_;
	bool held_ = false;
};

#endif // PLUGINEVENTS_H
