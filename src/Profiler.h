//
//  Profiler.h
//  raylib-miniscript
//
//  A sampling profiler for MiniScript code, enabled with `--profile <name>`.
//
//  In profile mode the host drives the VM in small batches of instructions
//  instead of RunUntilDone's large ones.  After each batch it records the full
//  call stack (by function, and the line within each) together with the wall
//  time the batch took.  When the run ends it writes:
//
//    <name>.folded   flame-graph input ("folded stacks", time in microseconds),
//                    which opens directly in https://www.speedscope.app or
//                    Brendan Gregg's flamegraph.pl
//    <name>.txt      a readable report: functions by self and total time, and
//                    the hottest source lines
//
//  Time is wall time.  An intrinsic's time normally lands on the line where the
//  batch containing it ends, which is usually the call site or a few lines on.
//  Calls that can take a long time time themselves with a TimedScope, and are
//  charged exactly to the calling line, as a "[raylib.EndDrawing]" entry
//  beneath it: EndDrawing (which also waits out the frame for SetTargetFPS),
//  WaitTime, SwapScreenBuffer, and the Begin/End mode calls that flush raylib's
//  draw batch (texture, 2D, 3D, shader, blend and scissor modes).  Batches that
//  end at a `yield` are recorded like any other.
//
//  Garbage collection is timed too.  MiniScript collects inside `yield` and
//  `wait`; a mark callback notes when each collection starts, and the time
//  from there to the end of the batch is charged to the calling line as
//  "(garbage collection)".  Garbage collection and the host's own work between
//  slices appear as separate entries, so the totals add up to the time profiled.
//
//  This needs nothing from MiniScript beyond its public VM API; when profiling
//  is off, the host's run loop is unchanged.
//

#ifndef PROFILER_H
#define PROFILER_H

#include "miniscript.h"
#include <string>

namespace profiler {

/// Turn profiling on.  outBase is the output path without extension; interval
/// is the mean number of VM instructions per sample (0 = default).
void Start(const std::string& outBase, unsigned int interval = 0);

/// True once Start has been called.
bool Enabled();

/// Profiling replacement for interp.RunUntilDone(timeLimit, returnEarly).  It
/// runs, yields, reports errors and collects garbage exactly as that does, and
/// records samples as it goes.
void RunUntilDone(MiniScript::Interpreter& interp, double timeLimit, bool returnEarly);

/// Catch Ctrl-C (SIGINT) so that an interrupted run still writes its report.
/// The main loop should poll Interrupted() and exit normally when it is set.
void InstallInterruptHandler();
bool Interrupted();

/// Charge `seconds` spent inside a host call to the script's current call
/// stack, under a "[label]" entry, and keep it out of the batch in progress.
/// Ignored unless a profiled slice is running.
void AddCallTime(const char* label, double seconds);

/// Seconds on a monotonic clock.
double Clock();

/// Times the enclosing block and reports it with AddCallTime.  Costs one
/// check when profiling is off.  Use inside intrinsics that may be slow:
///     profiler::TimedScope timed("raylib.EndDrawing");
struct TimedScope {
	const char* label;
	double start;
	explicit TimedScope(const char* label) : label(label), start(Enabled() ? Clock() : -1) {}
	~TimedScope() { if (start >= 0) AddCallTime(label, Clock() - start); }
	TimedScope(const TimedScope&) = delete;
	TimedScope& operator=(const TimedScope&) = delete;
};

/// Write the report files and print a short summary to stdout.  Safe to call
/// more than once; only the first call writes.  Returns false on a write error.
bool Finish();

} // namespace profiler

#endif // PROFILER_H
