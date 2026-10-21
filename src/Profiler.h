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
//  Time is wall time, so slow intrinsics (drawing, file I/O, a vsync wait in
//  EndDrawing) are charged to the script line that called them, or to a line
//  shortly after it within the same batch.  Batches that end at a `yield` are
//  recorded like any other.  Garbage collection and the host's own work between
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

/// Write the report files and print a short summary to stdout.  Safe to call
/// more than once; only the first call writes.  Returns false on a write error.
bool Finish();

} // namespace profiler

#endif // PROFILER_H
