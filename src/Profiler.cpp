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
#include <cstdlib>
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
	bool pseudo = false;  // not a MiniScript function: GC, host, or blocked time
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

	std::map<std::string, int> blockedFuncs;   // label -> pseudo-function
	std::map<std::string, unsigned long long> blockedCalls;   // label -> count
	bool inSlice = false;      // a profiled RunUntilDone is running
	VM sliceVM;                // ...on this VM
	double pendingBlocked = 0; // blocked time already charged, within this batch
	double gcStart = -1;       // when a collection began within this batch, or -1
	double blockedSinceGC = 0; // blocked time charged after gcStart
	unsigned long long collections = 0;

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
	fi.pseudo = true;
	S().funcs.push_back(fi);
	return (int)S().funcs.size() - 1;
}

bool IsPseudo(int func) {
	return S().funcs[func].pseudo;
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
		if (IsPseudo(i)) continue;
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

// Fill s.scratch with the VM's current call stack, outermost first.  This
// mirrors VM::BuildStackTrace: callStack[i].ReturnFunc is the function that
// made the i-th call, and ReturnPC - 1 is the call instruction within it.  The
// VM keeps PC and CurrentFunction current at every instruction, so this is
// also accurate from inside an intrinsic.
void BuildStack(VM& vm) {
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
}

// Record the VM's current call stack, charged with `seconds`.
void RecordStack(VM& vm, double seconds) {
	State& s = S();
	BuildStack(vm);
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

// Called by MiniScript at the start of every collection's mark phase.  Inside
// a profiled batch, that collection almost always comes from `yield` (or
// `wait`), which then ends the batch -- so the time from here to the end of
// the batch is the collection's cost, and it belongs to the yield's call site.
void OnCollect(void*) {
	State& s = S();
	s.collections++;
	if (s.inSlice && s.gcStart < 0) {
		s.gcStart = Now();
		s.blockedSinceGC = 0;
	}
}

double gInterruptTime = 0;   // only touched by OnInterrupt

// The first Ctrl-C asks the run loop to stop and write the report.  A second
// one quits at once, in case the loop is stuck -- but only if it comes well
// after the first, since some tools deliver one Ctrl-C twice (timeout(1)
// signals both the process and its group).
void OnInterrupt(int) {
	double now = Now();
	if (gInterrupted && now - gInterruptTime > 0.5) {
		std::signal(SIGINT, SIG_DFL);
		std::raise(SIGINT);
		return;
	}
	if (!gInterrupted) gInterruptTime = now;
	gInterrupted = 1;
	std::signal(SIGINT, OnInterrupt);   // re-arm (Windows resets handlers)
}

// JSON string literal, quoted and escaped.
std::string JsonString(const std::string& str) {
	std::string out = "\"";
	for (unsigned char c : str) {
		switch (c) {
		case '"':  out += "\\\""; break;
		case '\\': out += "\\\\"; break;
		case '\n': out += "\\n"; break;
		case '\r': out += "\\r"; break;
		case '\t': out += "\\t"; break;
		default:
			if (c < 0x20) {
				char buf[8];
				snprintf(buf, sizeof(buf), "\\u%04x", c);
				out += buf;
			} else {
				out += (char)c;
			}
		}
	}
	return out + "\"";
}

// The line a function was defined on, from its SourceLoc ("{file} line {n}").
int DefinitionLine(const FuncInfo& fi) {
	size_t pos = fi.loc.rfind(" line ");
	if (pos == std::string::npos) return 0;
	return atoi(fi.loc.c_str() + pos + 6);
}

// One speedscope frame, as JSON.
std::string JsonFrame(const std::string& name, const std::string& file, int line) {
	std::string out = "{\"name\":" + JsonString(name);
	if (!file.empty()) out += ",\"file\":" + JsonString(file);
	if (line > 0) out += ",\"line\":" + std::to_string(line);
	return out + "}";
}

// Write a speedscope file (https://www.speedscope.app) holding two profiles
// of the same samples: one frame per function, and one per function and line.
bool WriteSpeedscope(const std::string& path, const std::string& title) {
	State& s = S();
	size_t nf = s.funcs.size();

	// Short names, made unique: "update" twice becomes "update (key.ms)" and
	// "update (game.ms)".  The frame's file and line carry the rest.
	std::map<std::string, int> nameCount;
	for (const FuncInfo& fi : s.funcs) if (!fi.pseudo) nameCount[fi.name]++;
	std::vector<std::string> shortName(nf);
	for (size_t i = 0; i < nf; i++) {
		const FuncInfo& fi = s.funcs[i];
		if (fi.pseudo) shortName[i] = fi.label;
		else if (nameCount[fi.name] > 1) shortName[i] = fi.label;
		else shortName[i] = fi.name.empty() ? std::string("(anonymous)") : fi.name;
	}

	std::vector<std::string> frames;
	std::vector<int> funcFrame(nf);
	for (size_t i = 0; i < nf; i++) {
		const FuncInfo& fi = s.funcs[i];
		funcFrame[i] = (int)frames.size();
		frames.push_back(JsonFrame(shortName[i], fi.file, fi.pseudo ? 0 : DefinitionLine(fi)));
	}
	std::map<std::pair<int, int>, int> lineFrame;
	auto frameForLine = [&](const Frame& fr) -> int {
		if (s.funcs[fr.func].pseudo) return funcFrame[fr.func];
		auto key = std::make_pair(fr.func, fr.line);
		auto it = lineFrame.find(key);
		if (it != lineFrame.end()) return it->second;
		const FuncInfo& fi = s.funcs[fr.func];
		std::string name = shortName[fr.func] + (fr.line > 0 ? " line " + std::to_string(fr.line) : " (entry)");
		int index = (int)frames.size();
		frames.push_back(JsonFrame(name, fi.file, fr.line > 0 ? fr.line : DefinitionLine(fi)));
		lineFrame[key] = index;
		return index;
	};

	// By line: one sample per recorded stack.  By function: stacks that
	// differ only in line numbers are merged.
	std::vector<std::pair<std::vector<int>, double>> byLine;
	std::map<std::vector<int>, double> byFunc;
	double total = 0;
	for (const auto& kv : s.stacks) {
		double us = kv.second * 1e6;
		if (us <= 0 || kv.first.empty()) continue;
		total += us;
		std::vector<int> lineStack, funcStack;
		for (const Frame& fr : kv.first) {
			lineStack.push_back(frameForLine(fr));
			funcStack.push_back(funcFrame[fr.func]);
		}
		byLine.push_back(std::make_pair(lineStack, us));
		byFunc[funcStack] += us;
	}

	FILE* f = fopen(path.c_str(), "w");
	if (!f) return false;
	fprintf(f, "{\"$schema\":\"https://www.speedscope.app/file-format-schema.json\",\n");
	fprintf(f, "\"name\":%s,\n", JsonString(title).c_str());
	fprintf(f, "\"exporter\":\"raylib-miniscript --profile\",\n");
	fprintf(f, "\"activeProfileIndex\":0,\n");
	fprintf(f, "\"shared\":{\"frames\":[\n");
	for (size_t i = 0; i < frames.size(); i++) {
		fprintf(f, "%s%s\n", frames[i].c_str(), i + 1 < frames.size() ? "," : "");
	}
	fprintf(f, "]},\n\"profiles\":[\n");
	auto writeProfile = [&](const char* name, const std::vector<std::pair<std::vector<int>, double>>& samples, bool last) {
		fprintf(f, "{\"type\":\"sampled\",\"name\":%s,\"unit\":\"microseconds\",", JsonString(name).c_str());
		fprintf(f, "\"startValue\":0,\"endValue\":%.3f,\n\"samples\":[\n", total);
		for (size_t i = 0; i < samples.size(); i++) {
			fputc('[', f);
			const std::vector<int>& stack = samples[i].first;
			for (size_t j = 0; j < stack.size(); j++) fprintf(f, j ? ",%d" : "%d", stack[j]);
			fprintf(f, "]%s\n", i + 1 < samples.size() ? "," : "");
		}
		fprintf(f, "],\n\"weights\":[\n");
		for (size_t i = 0; i < samples.size(); i++) {
			fprintf(f, "%.3f%s", samples[i].second, i + 1 < samples.size() ? "," : "");
			if (i % 16 == 15) fputc('\n', f);
		}
		fprintf(f, "]}%s\n", last ? "" : ",");
	};
	std::vector<std::pair<std::vector<int>, double>> byFuncList(byFunc.begin(), byFunc.end());
	writeProfile("By function", byFuncList, false);
	writeProfile("By line", byLine, true);
	fprintf(f, "]}\n");
	bool ok = !ferror(f);
	fclose(f);
	return ok;
}

struct Row {
	int func;
	int line;
	double self;
	double total;
	int tag;    // for line rows: the blocked-time entry this is, or -1
};

// Key for the hottest-lines table: the innermost MiniScript frame, plus the
// blocked-time entry beneath it, if any (so EndDrawing's wait is listed at the
// line that called it, separately from that line's own work).
struct LineKey {
	int func;
	int line;
	int tag;
	bool operator<(const LineKey& o) const {
		if (func != o.func) return func < o.func;
		if (line != o.line) return line < o.line;
		return tag < o.tag;
	}
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
	GCManager::RegisterMarkCallback(&OnCollect, nullptr);
	s.hostFunc = AddPseudoFunc("(host, between script slices)");
	printf("Profiling enabled: sampling about every %u instructions; results go to %s.txt, %s.speedscope.json and %s.folded\n",
		s.interval, outBase.c_str(), outBase.c_str(), outBase.c_str());
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
	s.inSlice = true;
	s.sliceVM = vm;
	while (vm.IsRunning() && !vm.yielding()) {
		double t0 = Now();
		if (t0 - start > timeLimit) break;   // time's up for now
		s.pendingBlocked = 0;
		s.gcStart = -1;
		vm.Run(NextBatch());
		double t1 = Now();
		// A collection during the batch runs from gcStart to the batch's end
		// (less any blocked time after it, which is charged on its own).
		double gc = 0;
		if (s.gcStart >= 0) {
			gc = t1 - s.gcStart - s.blockedSinceGC;
			if (gc < 0) gc = 0;
		}
		// Blocked time inside the batch was charged to its call site already.
		double batch = t1 - t0 - s.pendingBlocked - gc;
		RecordStack(vm, batch > 0 ? batch : 0);
		if (gc > 0) {
			BuildStack(vm);
			s.scratch.push_back(Frame{ s.gcFunc, 0 });
			AddSample(s.scratch, gc);
		}
		s.gcStart = -1;
		if (!vm.Error().IsNull()) {
			s.inSlice = false;
			s.sliceVM = VM();
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
	s.inSlice = false;
	s.sliceVM = VM();
	double g0 = Now();
	GCManager::MaybeCollect(false);
	double g1 = Now();
	AddPseudoSample(s.gcFunc, g1 - g0);
	s.lastSliceEnd = vm.IsRunning() ? g1 : -1;
}

double Clock() {
	return Now();
}

void AddCallTime(const char* label, double seconds) {
	State& s = S();
	if (!s.enabled || s.finished || !s.inSlice || IsNull(s.sliceVM) || seconds <= 0) return;
	auto it = s.blockedFuncs.find(label);
	int func;
	if (it != s.blockedFuncs.end()) func = it->second;
	else func = s.blockedFuncs[label] = AddPseudoFunc(std::string("[") + label + "]");
	BuildStack(s.sliceVM);
	s.scratch.push_back(Frame{ func, 0 });
	AddSample(s.scratch, seconds);
	s.pendingBlocked += seconds;
	if (s.gcStart >= 0) s.blockedSinceGC += seconds;
	s.blockedCalls[label]++;
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
	std::map<LineKey, double> lineSelf;
	std::map<std::string, double> folded;
	double grand = 0;

	for (const auto& kv : s.stacks) {
		const std::vector<Frame>& stack = kv.first;
		double t = kv.second;
		if (stack.empty()) continue;
		grand += t;
		self[stack.back().func] += t;
		const Frame& leaf = stack.back();
		if (stack.size() > 1 && s.funcs[leaf.func].pseudo) {
			const Frame& site = stack[stack.size() - 2];
			lineSelf[LineKey{ site.func, site.line, leaf.func }] += t;
		} else {
			lineSelf[LineKey{ leaf.func, leaf.line, -1 }] += t;
		}
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

	// The same samples for speedscope, with file and line information.
	std::string speedscopePath = s.outBase + ".speedscope.json";
	std::string title = s.outBase;
	size_t slash = title.find_last_of("/\\");
	if (slash != std::string::npos) title = title.substr(slash + 1);
	if (!WriteSpeedscope(speedscopePath, title)) {
		printf("Profiler: could not write %s\n", speedscopePath.c_str());
		ok = false;
	}

	// The readable report.
	std::vector<Row> funcRows;
	for (size_t i = 0; i < nf; i++) {
		if (total[i] > 0) funcRows.push_back(Row{ (int)i, 0, self[i], total[i], -1 });
	}
	std::sort(funcRows.begin(), funcRows.end(), [](const Row& a, const Row& b) {
		return a.self != b.self ? a.self > b.self : a.total > b.total;
	});
	std::vector<Row> lineRows;
	for (const auto& kv : lineSelf) {
		lineRows.push_back(Row{ kv.first.func, kv.first.line, kv.second, 0, kv.first.tag });
	}
	std::sort(lineRows.begin(), lineRows.end(), [](const Row& a, const Row& b) {
		return a.self > b.self;
	});

	double gc = self[s.gcFunc], host = self[s.hostFunc];
	double blocked = 0;
	for (const auto& kv : s.blockedFuncs) blocked += self[kv.second];
	double pct = grand > 0 ? 100.0 / grand : 0;

	std::string reportPath = s.outBase + ".txt";
	f = fopen(reportPath.c_str(), "w");
	if (f) {
		fprintf(f, "MiniScript profile (raylib-miniscript)\n\n");
		double script = grand - gc - host - blocked;
		fprintf(f, "Time recorded:          %10.3f s\n", grand);
		fprintf(f, "  MiniScript code:      %10.3f s  (%5.1f%%)\n", script, script * pct);
		fprintf(f, "  timed raylib calls:   %10.3f s  (%5.1f%%)\n", blocked, blocked * pct);
		fprintf(f, "  garbage collection:   %10.3f s  (%5.1f%%)  %llu collections, %.2f ms each\n",
			gc, gc * pct, s.collections, s.collections ? gc * 1000 / s.collections : 0.0);
		fprintf(f, "  host, between slices: %10.3f s  (%5.1f%%)\n", host, host * pct);
		fprintf(f, "Samples:   %llu  (about every %u instructions)\n", s.samples, s.interval);
		fprintf(f, "Yields:    %llu\n", s.yields);
		auto frames = s.blockedCalls.find("raylib.EndDrawing");
		if (frames != s.blockedCalls.end() && frames->second > 0) {
			fprintf(f, "Frames:    %llu (EndDrawing calls), %.2f ms each\n",
				frames->second, grand * 1000 / frames->second);
		}
		if (!s.blockedFuncs.empty()) {
			std::vector<std::pair<double, std::string>> calls;
			for (const auto& kv : s.blockedFuncs) calls.push_back(std::make_pair(self[kv.second], kv.first));
			std::sort(calls.begin(), calls.end(), [](const std::pair<double, std::string>& a,
				const std::pair<double, std::string>& b) { return a.first > b.first; });
			fprintf(f, "\nTimed raylib calls (exact; charged to the line that made them)\n\n");
			fprintf(f, "    total ms     %%      calls   ms each  call\n");
			for (const auto& c : calls) {
				unsigned long long n = s.blockedCalls[c.second];
				fprintf(f, "%12.2f %5.1f%% %10llu %9.3f  [%s]\n", c.first * 1000, c.first * pct, n,
					n ? c.first * 1000 / n : 0.0, c.second.c_str());
			}
			if (frames != s.blockedCalls.end()) {
				fprintf(f, "\n[raylib.EndDrawing] includes the wait for SetTargetFPS, so most of it is\n");
				fprintf(f, "usually idle time.  The others are mostly raylib submitting batched drawing.\n");
			}
		}
		fprintf(f, "\nSelf time is spent in a function's own code (including intrinsics it calls);\n");
		fprintf(f, "total time also includes the MiniScript functions it calls.  [Bracketed]\n");
		fprintf(f, "entries are timed raylib calls, and (garbage collection) is collection that\n");
		fprintf(f, "MiniScript ran inside the line above it, usually a yield.  A yield always\n");
		fprintf(f, "ends a sample, so a yield's line also gets the work done since the previous\n");
		fprintf(f, "sample (up to about 1.5x the interval, in instructions).\n");

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
			if (IsPseudo(r.func)) {
				fprintf(f, "%10.2f %6.1f%%  %s\n", r.self * 1000, r.self * pct, fi.label.c_str());
			} else {
				// Line 0 means the function's entry, before its first line.
				std::string where = fi.file.empty() ? std::string() : fi.file + " ";
				char lineStr[32];
				if (r.line > 0) snprintf(lineStr, sizeof(lineStr), "line %d", r.line);
				else snprintf(lineStr, sizeof(lineStr), "(entry)");
				std::string tag = r.tag >= 0 ? "  " + s.funcs[r.tag].label : std::string();
				fprintf(f, "%10.2f %6.1f%%  %s%s  in %s%s\n", r.self * 1000, r.self * pct,
					where.c_str(), lineStr, fi.label.c_str(), tag.c_str());
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
	if (ok) {
		printf("Full report: %s\n", reportPath.c_str());
		printf("Flame graph: %s (open at https://www.speedscope.app), or %s\n",
			speedscopePath.c_str(), foldedPath.c_str());
	}
	fflush(stdout);
	return ok;
}

} // namespace profiler
