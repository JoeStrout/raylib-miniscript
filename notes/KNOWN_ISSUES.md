# Known Issues

Bugs found but not yet fixed.  Remove an entry once its fix lands.

Tests: each entry has a test that checks the fixed behavior (so it fails until
the fix lands) in `tests/known_issues_tests.cpp`
(`cmake --build build --target known_issues_tests && ./build/known_issues_tests`),
or, for the ones that need a window, `tests/known_issues_window.ms`
(`./build/raylib-miniscript tests/known_issues_window.ms`, run by hand).

The entries below were found during the 2026-11-15 review of the API notes
(`WithNote`).  All are confirmed by failing tests.

## GetCodepointPrevious reads before the string

`GetCodepointPrevious` (src/RText.cpp) passes the start of `text.c_str()`, so
raylib walks backward into memory before the string.  The result is meaningless
and the read is out of bounds.  It probably needs an index or offset parameter
(or to point at the end of the string).

## TakeScreenshot doesn't resolve its path

`TakeScreenshot` (src/RCore.cpp) passes the script's filename straight to
raylib without `fs::HostPath` / `HostPathForWrite`, unlike the other file
functions.  It is refused once sandboxed, so this matters only in unsandboxed runs.
