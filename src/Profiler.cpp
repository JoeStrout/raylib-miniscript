//
//  Profiler.cpp
//  raylib-miniscript
//
//  Sampling profiler for MiniScript code.  See Profiler.h for an overview.
//

#include "Profiler.h"
#include <algorithm>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <map>
#include <unordered_map>
#include <vector>

using namespace MiniScript;

namespace profiler {
namespace {

const unsigned int kDefaultInterval = 250;

double Now() {
	return std::chrono::duration<double>(
		std::chrono::steady_clock::now().time_since_epoch()).count();
}

// One function, identified by its name and where it was defined.  We copy the
// strings out of the FuncDef so that nothing here depends on MiniScript memory
// once the sample is taken.
struct FuncInfo {
	std::string name;
	std::string loc;    // SourceLoc: "{file} line {n}" of the definition
	std::string file;
	std::string label;  // what the reports show
};

// One stack frame: a function, and the line within it.
struct Frame {
	int func;
	int line;
	bool operator==(const Frame& other) const {
		return func == other.func && line == other.line;
	}
};

struct StackHash {
	size_t operator()(const std::vector<Frame>& stack) const {
		uint64_t h = 1469598103934665603ULL;
		for (const Frame& f : stack) {
			h ^= (uint64_t)(uint32_t)f.func * 31u + (uint64_t)(uint32_t)f.line;
			h *= 1099511628211ULL;
		}
		return (size_t)h;
	}
};

// Cache key for looking up a FuncDef's index without copying its strings: the
// addresses of its name, location and file name.  An address can be reused once
// a FuncDef is freed, so a cache hit is always confirmed by comparing contents.
struct PtrKey {
	const char* name;
	const char* loc;
	const char* file;
	bool operator==(const PtrKey& other) const {
		return name == other.name && loc == other.loc && file == other.file;
	}
};

struct PtrKeyHash {
	size_t operator()(const PtrKey& k) const {
		std::hash<const void*> h;
		return h(k.name) ^ (h(k.loc) * 3) ^ (h(k.file) * 7);
	}
};

struct State {
	bool enabled = false;
	bool finished = false;
	std::string outBase;
	unsigned int interval = kDefaultInterval;
	uint32_t rng = 2463534242u;

	std::vector<FuncInfo> funcs;
	std::unordered_map<PtrKey, int, PtrKeyHash> funcCache;
	std::unordered_map<std::vector<Frame>, double, StackHash> stacks;  // stack -> seconds
	std::vector<Frame> scratch;

	int gcFunc = -1;      // pseudo-function for garbage collection
	int hostFunc = -1;    // pseudo-function for host work between slices
	double lastSliceEnd = -1;
	unsigned long long samples = 0;
	unsigned long long yields = 0;
};

State& S() {
	static State state;
	return state;
}

volatile std::sig_atomic_t gInterrupted = 0;

std::string Sanitize(std::string s) {
	// The folded format separates frames with ';' and lines with '\n'.
	for (char& c : s) {
		if (c == ';') c = ',';
		else if (c == '\n' || c == '\r') c = ' ';
	}
	return s;
}

int AddFunc(const std::string& name, const std::string& loc, const std::string& file) {
	FuncInfo fi;
	fi.name = name;
	fi.loc = loc;
	fi.file = file;
	// "slow (game.ms line 12)"; or "slow (line 12)" for code with no file
	// name; or just "@main" for a main program with no file name.
	std::string where = !loc.empty() ? loc : file;
	const std::string noFile = "(current program) ";
	if (where.compare(0, noFile.size(), noFile) == 0) where = where.substr(noFile.size());
	std::string label = name.empty() ? std::string("(anonymous)") : name;
	if (!where.empty()) label += " (" + where + ")";
	fi.label = Sanitize(label);
	S().funcs.push_back(fi);
	return (int)S().funcs.size() - 1;
}

int AddPseudoFunc(const std::string& label) {
	FuncInfo fi;
	fi.label = label;
	S().funcs.push_back(fi);
	return (int)S().funcs.size() - 1;
}

int FuncIndex(FuncDef f) {
	State& s = S();
	String name = f.Name();
	String loc = f.SourceLoc();
	String file = f.FileName();
	const char* np = name.c_str();
	const char* lp = loc.c_str();
	const char* fp = file.c_str();
	PtrKey key = { np, lp, fp };

	auto it = s.funcCache.find(key);
	if (it != s.funcCache.end()) {
		const FuncInfo& fi = s.funcs[it->second];
		if (fi.name == np && fi.loc == lp && fi.file == fp) return it->second;
	}
	// Cache miss (or a stale entry): find it by content, or add it.
	int index = -1;
	for (int i = 0; i < (int)s.funcs.size(); i++) {
		const FuncInfo& fi = s.funcs[i];
		if (i == s.gcFunc || i == s.hostFunc) continue;
		if (fi.name == np && fi.loc == lp && fi.file == fp) { index = i; break; }
	}
	if (index < 0) index = AddFunc(np, lp, fp);
	s.funcCache[key] = index;
	return index;
}

void AddSample(const std::vector<Frame>& stack, double seconds) {
	State& s = S();
	auto it = s.stacks.find(stack);
	if (it == s.stacks.end()) it = s.stacks.emplace(stack, 0.0).first;
	it->second += seconds;
}

void AddPseudoSample(int func, double seconds) {
	State& s = S();
	s.scratch.clear();
	s.scratch.push_back(Frame{ func, 0 });
	AddSample(s.scratch, seconds);
}

// Record the VM's current call stack, outermost first, charged with `seconds`.
// This mirrors VM::BuildStackTrace: callStack[i].ReturnFunc is the function
// that made the i-th call, and ReturnPC - 1 is the call instruction within it.
void RecordStack(VM& vm, double seconds) {
	State& s = S();
	s.scratch.clear();
	Int32 depth = vm.CallStackDepth();
	for (Int32 i = 1; i < depth; i++) {
		CallInfo ci = vm.GetCallStackFrame(i);
		FuncDef caller = ci.ReturnFunc;
		if (IsNull(caller)) continue;
		Int32 pc = ci.ReturnPC - 1;
		if (pc < 0) pc = 0;
		s.scratch.push_back(Frame{ FuncIndex(caller), caller.GetLineNumber(pc) });
	}
	FuncDef cur = vm.CurrentFunction();
	if (!IsNull(cur)) {
		Int32 pc = vm.PC() - 1;
		if (pc < 0) pc = 0;
		s.scratch.push_back(Frame{ FuncIndex(cur), cur.GetLineNumber(pc) });
	}
	if (s.scratch.empty()) return;
	AddSample(s.scratch, seconds);
	s.samples++;
}

// Instructions to run before the next sample: uniformly spread around the
// interval, so that sampling cannot fall into step with a loop of fixed length.
UInt32 NextBatch() {
	State& s = S();
	uint32_t x = s.rng;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	s.rng = x;
	UInt32 n = s.interval / 2 + x % (s.interval + 1);
	return n < 1 ? 1 : n;   // (0 would mean "no limit" to VM::Run)
}

void OnInterrupt(int) {
	gInterrupted = 1;
	std::signal(SIGINT, SIG_DFL);   // a second Ctrl-C quits immediately
}

struct Row {
	int func;
	int line;
	double self;
	double total;
};

} // anonymous namespace

//--------------------------------------------------------------------------------
// Public API
//--------------------------------------------------------------------------------

void Start(const std::string& outBase, unsigned int interval) {
	State& s = S();
	s.enabled = true;
	s.outBase = outBase;
	s.interval = interval ? interval : kDefaultInterval;
	s.gcFunc = AddPseudoFunc("(garbage collection)");
	s.hostFunc = AddPseudoFunc("(host, between script slices)");
	printf("Profiling enabled: sampling about every %u instructions; results go to %s.folded and %s.txt\n",
		s.interval, outBase.c_str(), outBase.c_str());
}

bool Enabled() {
	return S().enabled;
}

void RunUntilDone(Interpreter& interp, double timeLimit, bool returnEarly) {
	State& s = S();
	VM vm = interp.vm();
	if (!s.enabled || s.finished || IsNull(vm)) {
		interp.RunUntilDone(timeLimit, returnEarly);
		return;
	}

	// This follows Interpreter::RunUntilDone step for step; the only difference
	// is the small batch size, and the sample taken after each batch.
	double start = Now();
	if (s.lastSliceEnd >= 0) AddPseudoSample(s.hostFunc, start - s.lastSliceEnd);
	vm.set_yielding(false);
	while (vm.IsRunning() && !vm.yielding()) {
		double t0 = Now();
		if (t0 - start > timeLimit) break;   // time's up for now
		vm.Run(NextBatch());
		RecordStack(vm, Now() - t0);
		if (!vm.Error().IsNull()) {
			// Report and stop, as RunUntilDone does (via errorOutput).
			Value err = vm.Error();
			interp.set_Error(err);
			TextOutputMethod out = interp.errorOutput();
			if (out) out(ErrorTypes::DescribeError(err), true);
			interp.Stop();
			s.lastSliceEnd = -1;
			return;
		}
		if (vm.yielding()) s.yields++;
		if (returnEarly && vm.yielding()) break;   // waiting for something
	}
	double g0 = Now();
	GCManager::MaybeCollect(false);
	double g1 = Now();
	AddPseudoSample(s.gcFunc, g1 - g0);
	s.lastSliceEnd = vm.IsRunning() ? g1 : -1;
}

void InstallInterruptHandler() {
	std::signal(SIGINT, OnInterrupt);
}

bool Interrupted() {
	return gInterrupted != 0;
}

bool Finish() {
	State& s = S();
	if (!s.enabled || s.finished) return true;
	s.finished = true;

	size_t nf = s.funcs.size();
	std::vector<double> self(nf, 0.0), total(nf, 0.0);
	std::vector<char> seen(nf, 0);
	std::map<std::pair<int, int>, double> lineSelf;
	std::map<std::string, double> folded;
	double grand = 0;

	for (const auto& kv : s.stacks) {
		const std::vector<Frame>& stack = kv.first;
		double t = kv.second;
		if (stack.empty()) continue;
		grand += t;
		self[stack.back().func] += t;
		lineSelf[std::make_pair(stack.back().func, stack.back().line)] += t;
		std::string key;
		for (const Frame& f : stack) {
			// Total time counts a function once per stack, even when recursive.
			if (!seen[f.func]) { seen[f.func] = 1; total[f.func] += t; }
			if (!key.empty()) key += ';';
			key += s.funcs[f.func].label;
		}
		for (const Frame& f : stack) seen[f.func] = 0;
		folded[key] += t;
	}

	bool ok = true;

	// Folded stacks, one function per frame, weights in microseconds.
	std::string foldedPath = s.outBase + ".folded";
	FILE* f = fopen(foldedPath.c_str(), "w");
	if (f) {
		for (const auto& kv : folded) {
			long long us = (long long)(kv.second * 1e6 + 0.5);
			if (us > 0) fprintf(f, "%s %lld\n", kv.first.c_str(), us);
		}
		fclose(f);
	} else {
		printf("Profiler: could not write %s\n", foldedPath.c_str());
		ok = false;
	}

	// The readable report.
	std::vector<Row> funcRows;
	for (size_t i = 0; i < nf; i++) {
		if (total[i] > 0) funcRows.push_back(Row{ (int)i, 0, self[i], total[i] });
	}
	std::sort(funcRows.begin(), funcRows.end(), [](const Row& a, const Row& b) {
		return a.self != b.self ? a.self > b.self : a.total > b.total;
	});
	std::vector<Row> lineRows;
	for (const auto& kv : lineSelf) {
		lineRows.push_back(Row{ kv.first.first, kv.first.second, kv.second, 0 });
	}
	std::sort(lineRows.begin(), lineRows.end(), [](const Row& a, const Row& b) {
		return a.self > b.self;
	});

	double gc = self[s.gcFunc], host = self[s.hostFunc];
	double pct = grand > 0 ? 100.0 / grand : 0;

	std::string reportPath = s.outBase + ".txt";
	f = fopen(reportPath.c_str(), "w");
	if (f) {
		fprintf(f, "MiniScript profile (raylib-miniscript)\n\n");
		fprintf(f, "Time recorded:   %10.3f s\n", grand);
		fprintf(f, "  script:        %10.3f s  (%5.1f%%)\n", grand - gc - host, (grand - gc - host) * pct);
		fprintf(f, "  garbage coll.: %10.3f s  (%5.1f%%)\n", gc, gc * pct);
		fprintf(f, "  host:          %10.3f s  (%5.1f%%)\n", host, host * pct);
		fprintf(f, "Samples:         %10llu  (about every %u instructions)\n", s.samples, s.interval);
		fprintf(f, "Yields:          %10llu\n", s.yields);
		fprintf(f, "\nSelf time is spent in a function's own code (including intrinsics it calls);\n");
		fprintf(f, "total time also includes the MiniScript functions it calls.\n");

		fprintf(f, "\nFunctions by self time\n\n");
		fprintf(f, "   self ms   self%%   total ms  total%%  function\n");
		int n = 0;
		for (const Row& r : funcRows) {
			if (n++ >= 50) break;
			fprintf(f, "%10.2f %6.1f%% %10.2f %6.1f%%  %s\n",
				r.self * 1000, r.self * pct, r.total * 1000, r.total * pct,
				s.funcs[r.func].label.c_str());
		}

		fprintf(f, "\nHottest lines\n\n");
		fprintf(f, "   self ms   self%%  line  in function\n");
		n = 0;
		for (const Row& r : lineRows) {
			if (n++ >= 40 || r.self < 0.000005) break;
			const FuncInfo& fi = s.funcs[r.func];
			if (r.func == s.gcFunc || r.func == s.hostFunc) {
				fprintf(f, "%10.2f %6.1f%%  %s\n", r.self * 1000, r.self * pct, fi.label.c_str());
			} else {
				// Line 0 means the function's entry, before its first line.
				std::string where = fi.file.empty() ? std::string() : fi.file + " ";
				char lineStr[32];
				if (r.line > 0) snprintf(lineStr, sizeof(lineStr), "line %d", r.line);
				else snprintf(lineStr, sizeof(lineStr), "(entry)");
				fprintf(f, "%10.2f %6.1f%%  %s%s  in %s\n", r.self * 1000, r.self * pct,
					where.c_str(), lineStr, fi.label.c_str());
			}
		}
		fclose(f);
	} else {
		printf("Profiler: could not write %s\n", reportPath.c_str());
		ok = false;
	}

	// A short summary on the console.
	printf("\nProfile: %.3f s recorded in %llu samples.  Top functions by self time:\n", grand, s.samples);
	int n = 0;
	for (const Row& r : funcRows) {
		if (n++ >= 8) break;
		printf("  %6.1f%%  %s\n", r.self * pct, s.funcs[r.func].label.c_str());
	}
	if (ok) printf("Full report: %s   Flame graph: %s\n", reportPath.c_str(), foldedPath.c_str());
	fflush(stdout);
	return ok;
}

} // namespace profiler
