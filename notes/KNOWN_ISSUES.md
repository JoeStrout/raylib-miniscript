# Known Issues

Bugs found but not yet fixed.  Remove an entry once its fix lands.

Tests: each entry has a test that checks the fixed behavior (so it fails until
the fix lands) in `tests/known_issues_tests.cpp`
(`cmake --build build --target known_issues_tests && ./build/known_issues_tests`),
or, for the ones that need a window, `tests/known_issues_window.ms`
(`./build/raylib-miniscript tests/known_issues_window.ms`, run by hand).

None at the moment.
