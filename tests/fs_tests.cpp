//
//  fs_tests.cpp
//  raylib-miniscript
//
//  Tests for the virtual file system layer (src/FileSystem.cpp).
//
//  Step 1 of the sandboxing plan has no script-visible surface, so this is the
//  only way to exercise it until the file module is routed through it.  The
//  cases that matter most are the path-translation ones: each of them is an
//  escape that has bitten somebody's sandbox before.
//
//  Build:  cmake --build build --target fs_tests
//  Run:    ./build/fs_tests
//

#include "FileSystem.h"

#include <stdio.h>
#include <string.h>
#include <string>
#include <unistd.h>
#include <sys/stat.h>

using MiniScript::String;

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

static void eq(const String& actual, const char* expected, const char* what) {
	checks++;
	if (strcmp(actual.c_str(), expected) == 0) {
		printf("ok   %s\n", what);
	} else {
		printf("FAIL %s (got \"%s\", expected \"%s\")\n", what, actual.c_str(), expected);
		failures++;
	}
}

//--------------------------------------------------------------------------------
// A scratch directory tree to mount
//--------------------------------------------------------------------------------

static std::string scratch;

static void writeFile(const std::string& path, const char* contents) {
	FILE* f = fopen(path.c_str(), "wb");
	if (f == NULL) { printf("FAIL could not create %s\n", path.c_str()); failures++; return; }
	fwrite(contents, 1, strlen(contents), f);
	fclose(f);
}

static void buildScratch() {
	char tmpl[] = "/tmp/fs_tests_XXXXXX";
	scratch = mkdtemp(tmpl);

	mkdir((scratch + "/usr").c_str(), 0755);
	mkdir((scratch + "/usr/sub").c_str(), 0755);
	mkdir((scratch + "/usr2").c_str(), 0755);
	mkdir((scratch + "/sys").c_str(), 0755);
	mkdir((scratch + "/outside").c_str(), 0755);

	writeFile(scratch + "/usr/hello.txt", "hello\nworld\n");
	writeFile(scratch + "/usr/sub/deep.txt", "deep");
	writeFile(scratch + "/usr2/other.txt", "other");
	writeFile(scratch + "/sys/lib.ms", "// system");
	writeFile(scratch + "/outside/secret.txt", "SECRET");
	// Stands in for the boot script: it lives beside the disks, not on one.
	writeFile(scratch + "/boot.ms", "// the shell's own source");

	// A symlink inside /usr pointing back out of it.  Without the realpath
	// containment check, this is a straight walk out of the sandbox.
	symlink((scratch + "/outside").c_str(), (scratch + "/usr/escape").c_str());
}

//--------------------------------------------------------------------------------
// Tests
//--------------------------------------------------------------------------------

static void testPathHelpers() {
	printf("\n-- path helpers --\n");
	eq(fs::PathCombine(String("/usr"), String("a.txt")), "/usr/a.txt", "PathCombine adds a separator");
	eq(fs::PathCombine(String("/usr/"), String("a.txt")), "/usr/a.txt", "PathCombine does not double a separator");
	eq(fs::PathCombine(String("/"), String("/usr")), "/usr", "PathCombine of root with an absolute path");
	eq(fs::GetFileName(String("/usr/sub/deep.txt")), "deep.txt", "GetFileName");
	eq(fs::StripFileName(String("/usr/sub/deep.txt")), "/usr/sub", "StripFileName");
	eq(fs::StripFileName(String("/usr")), "/", "StripFileName at the root");
}

static void testPassthroughBeforeLatch() {
	printf("\n-- unsandboxed passthrough --\n");
	ok(!fs::IsSandboxed(), "starts unsandboxed");

	// Host paths must survive untouched, or every non-Mini-Micro host breaks.
	String out;
	ok(fs::ResolvePath(String("assets/main.ms"), out), "relative host path resolves");
	eq(out, "assets/main.ms", "relative host path is unchanged");
	ok(fs::ResolvePath(String("C:\\Windows\\x.txt"), out), "backslash path is allowed when unsandboxed");

	// And a real read through the layer works with no mounts at all.
	String text;
	ok(fs::ReadText(String((scratch + "/usr/hello.txt").c_str()), text), "reads a host path with no mounts");
	eq(text, "hello\nworld\n", "contents of the host read");
}

// Mounts must work before the latch, or a host application could not load its
// own resources from /hw during boot -- which is the whole point of step 2.
static void testMountsWorkBeforeLatch() {
	printf("\n-- mounts before the latch --\n");
	ok(!fs::IsSandboxed(), "still unsandboxed");

	String text;
	ok(fs::ReadText(String("/sys/lib.ms"), text), "a mount resolves before the latch");
	eq(text, "// system", "contents came from the mount");

	// ...and host paths still work alongside them.
	ok(fs::ReadText(String((scratch + "/outside/secret.txt").c_str()), text),
		"host paths still reachable before the latch");

	// Read-only is a property of the mount, not of the latch.
	ok(!fs::WriteText(String("/sys/nope.txt"), String("x")).empty(),
		"a read-only mount refuses writes before the latch too");

	// The real path handed to raylib's loaders resolves through the mount.
	String host;
	ok(fs::HostPath(String("/sys/lib.ms"), host), "HostPath resolves a mount");
	ok(strstr(host.c_str(), "/sys/lib.ms") != NULL, "HostPath returned a real path");
	ok(fs::HostPath(String("assets/main.ms"), host), "HostPath passes a host path through");
	eq(host, "assets/main.ms", "unchanged for a non-mount path");

	// All three overloads, including a bare literal -- which would be ambiguous
	// between the String and Value forms without the const char* one.  This call
	// is here to fail at compile time if that overload ever goes away.
	ok(fs::HostPath("/sys/lib.ms", host), "HostPath accepts a literal");
	ok(fs::HostPath(MiniScript::Value(String("/sys/lib.ms")), host), "HostPath accepts a Value");
}

static void testNormalization() {
	printf("\n-- path normalization --\n");
	String out;

	ok(!fs::ResolvePath(String("/usr/a\\b.txt"), out), "backslash separator rejected");

	ok(fs::ResolvePath(String("/usr/./sub/../hello.txt"), out), "'.' and '..' fold");
	eq(out, "/usr/hello.txt", "folded to /usr/hello.txt");

	ok(fs::ResolvePath(String("/usr//sub///deep.txt"), out), "empty components collapse");
	eq(out, "/usr/sub/deep.txt", "collapsed to /usr/sub/deep.txt");

	ok(!fs::ResolvePath(String("/usr/../../etc/passwd"), out), "'..' above the root rejected");
	ok(!fs::ResolvePath(String("/../etc/passwd"), out), "leading '..' rejected");

	ok(fs::ResolvePath(String("/usr/.."), out), "'..' back to the root is fine");
	eq(out, "/", "resolved to /");

	// Folding is lexical, so this never touches the host -- it just names a
	// mount that is not there.
	ok(fs::ResolvePath(String("/usr/../etc/passwd"), out), "folds to a non-mount path");
	eq(out, "/etc/passwd", "folded to /etc/passwd");
	String rel;
	ok(fs::GetMount(out, rel) == nullptr, "/etc has no mount");
}

static void testCwd() {
	printf("\n-- virtual cwd --\n");
	eq(fs::Cwd(), "/", "cwd starts at the root");
	ok(fs::SetCwd(String("/usr/sub")), "setdir to an existing directory");
	eq(fs::Cwd(), "/usr/sub", "cwd updated");

	String out;
	ok(fs::ResolvePath(String("deep.txt"), out), "relative path resolves against cwd");
	eq(out, "/usr/sub/deep.txt", "resolved against cwd");

	ok(fs::ResolvePath(String("../hello.txt"), out), "relative path with '..'");
	eq(out, "/usr/hello.txt", "resolved above cwd but inside the mount");

	ok(!fs::SetCwd(String("/usr/hello.txt")), "setdir to a file fails");
	ok(!fs::SetCwd(String("/nope")), "setdir to a missing directory fails");
	ok(fs::SetCwd(String("/")), "setdir back to the root");
}

static void testMountBoundaries() {
	printf("\n-- mount boundaries --\n");
	String rel;

	ok(fs::GetMount(String("/usr/hello.txt"), rel) != nullptr, "/usr resolves to a mount");
	eq(rel, "hello.txt", "mount-relative path has no leading slash");

	ok(fs::GetMount(String("/usr"), rel) != nullptr, "bare /usr resolves");
	eq(rel, "", "bare mount has an empty relative path");

	// /usr2 is its own mount, and its real directory sits beside /usr's.  A
	// prefix test without a component boundary would confuse the two.
	fs::Backend* usr = fs::GetMount(String("/usr/x"), rel);
	fs::Backend* usr2 = fs::GetMount(String("/usr2/x"), rel);
	ok(usr != usr2, "/usr root does not match /usr2");

	ok(fs::GetMount(String("/"), rel) == nullptr, "the virtual root is not itself a mount");
	ok(fs::GetMount(String("/nope/x"), rel) == nullptr, "unknown mount");
}

static void testSymlinkContainment() {
	printf("\n-- symlink containment --\n");

	// The lexical folding cannot catch this one: /usr/escape is a legitimate
	// path component, and only realpath() reveals where it lands.
	String text;
	ok(!fs::ReadText(String("/usr/escape/secret.txt"), text), "symlink out of /usr rejected");
	ok(!fs::Exists(String("/usr/escape/secret.txt")), "escaped path does not exist");

	// The same file by its real host path is equally invisible: there is no
	// mount there, and no way to name one.
	ok(!fs::Exists(String((scratch + "/outside/secret.txt").c_str())), "host path to the same file does not exist");
	ok(!fs::Exists(String("/etc/passwd")), "/etc/passwd does not exist");

	// A disk is a named subdirectory, never the directory holding them.  The
	// boot script and the shell's own source sit beside the disks, and nothing
	// on any disk can name its way up to them.
	ok(!fs::Exists(String("/boot.ms")), "a sibling of the mounts is unreachable");
	ok(!fs::Exists(String("/sys/../boot.ms")), "and cannot be reached by climbing out of one");
	ok(!fs::Exists(String("/hw/../boot.ms")), "nor out of the hidden disk");
}

static void testReadOnlyEnforcement() {
	printf("\n-- read-only enforcement --\n");

	// Every mutating operation, not just opening for write.
	ok(!fs::WriteText(String("/sys/new.txt"), String("x")).empty(), "writeText to /sys refused");
	ok(!fs::MakeDir(String("/sys/newdir")).empty(), "makedir in /sys refused");
	ok(!fs::Delete(String("/sys/lib.ms")).empty(), "delete in /sys refused");
	ok(!fs::MoveOrCopy(String("/usr/hello.txt"), String("/sys/hello.txt"), false, false).empty(),
		"copy into /sys refused");
	ok(fs::Exists(String("/sys/lib.ms")), "/sys/lib.ms survived");

	// ...but /sys is still perfectly readable.
	String text;
	ok(fs::ReadText(String("/sys/lib.ms"), text), "/sys is readable");
	eq(text, "// system", "contents of /sys/lib.ms");

	// Resolving for raylib's loaders needs the same enforcement.  HostPath has
	// no idea what its caller will do with the path, so a destination must ask
	// for HostPathForWrite -- otherwise raylib writes into a read-only mount
	// with the resolver's blessing.
	String host;
	ok(fs::HostPath(String("/sys/lib.ms"), host), "HostPath reads from a read-only mount");
	ok(!fs::HostPathForWrite(String("/sys/lib.ms"), host), "HostPathForWrite refuses a read-only mount");
	ok(!fs::HostPathForWrite(String("/sys/new.txt"), host), "...for a new file there too");
	ok(fs::HostPathForWrite(String("/usr/new.txt"), host), "HostPathForWrite allows a writable mount");
	ok(!fs::HostPathForWrite(String("/etc/passwd"), host), "HostPathForWrite refuses a non-mount");
}

static void testListing() {
	printf("\n-- listing --\n");
	std::vector<String> names;
	ok(fs::ListDir(String("/"), names), "listing the root works");

	bool sawUsr = false, sawUsr2 = false, sawSys = false, sawHw = false;
	for (size_t i = 0; i < names.size(); i++) {
		const char* n = names[i].c_str();
		if (strcmp(n, "usr") == 0) sawUsr = true;
		if (strcmp(n, "usr2") == 0) sawUsr2 = true;
		if (strcmp(n, "sys") == 0) sawSys = true;
		if (strcmp(n, "hw") == 0) sawHw = true;
	}
	ok(sawUsr && sawUsr2 && sawSys, "root lists the visible mounts");
	ok(!sawHw, "hidden /hw is absent from the root listing");

	// Hidden does not mean inaccessible -- /hw is undocumented, not secret.
	ok(fs::Exists(String("/hw/bezel.png")), "/hw is readable even though it is unlisted");

	fs::FileInfo info;
	ok(fs::GetInfo(String("/"), info) && info.isDirectory, "the root reports as a directory");
}

static void testOpenFileModes() {
	printf("\n-- OpenFile modes --\n");
	fs::Resolved r;

	ok(fs::Resolve(String("/usr/hello.txt"), r), "resolve /usr/hello.txt");
	{
		fs::OpenFile f(r, String("r"));
		ok(f.error.empty() && f.IsOpen(), "open \"r\" on an existing file");
		String line;
		ok(f.ReadLine(line), "readLine");
		eq(line, "hello", "first line");
		ok(f.ReadLine(line) && strcmp(line.c_str(), "world") == 0, "second line");
		ok(!f.ReadLine(line), "readLine at end returns nothing");
		ok(f.IsAtEnd(), "atEnd after reading everything");
		f.Close();
		ok(!f.IsOpen(), "closed");
	}

	{
		fs::Resolved missing;
		ok(fs::Resolve(String("/usr/nope.txt"), missing), "resolve a missing file");
		fs::OpenFile f(missing, String("r"));
		ok(!f.error.empty(), "open \"r\" on a missing file is an error");
	}

	{
		fs::Resolved missing;
		fs::Resolve(String("/usr/created.txt"), missing);
		fs::OpenFile f(missing, String("w"));
		ok(f.error.empty(), "open \"w\" on a missing file succeeds");
	}

	{
		fs::Resolved bin;
		fs::Resolve(String("/usr/hello.txt"), bin);
		fs::OpenFile f(bin, String("rb"));
		ok(!f.error.empty(), "binary mode rejected");
	}

	{
		fs::Resolved bad;
		fs::Resolve(String("/usr/hello.txt"), bad);
		fs::OpenFile f(bad, String("q"));
		ok(!f.error.empty(), "invalid mode rejected");
	}

	// Read-only disk: the handle opens, but writing through it does not.
	{
		fs::Resolved sys;
		fs::Resolve(String("/sys/lib.ms"), sys);
		fs::OpenFile f(sys, String("r+"));
		f.Write(String("clobber"));
		ok(!f.error.empty(), "write to a handle on a read-only disk refused");
		f.Close();
		String text;
		fs::ReadText(String("/sys/lib.ms"), text);
		eq(text, "// system", "/sys/lib.ms unchanged");
	}
}

static void testOpenFilePersistence() {
	printf("\n-- OpenFile persistence --\n");
	fs::Resolved r;
	fs::Resolve(String("/usr/written.txt"), r);

	{
		fs::OpenFile f(r, String("w"));
		f.Write(String("alpha\n"));
		f.Write(String("beta\n"));
		ok(f.error.empty(), "wrote two lines");
		// Not yet on disk: the buffer flushes on close, not on write.
		ok(!fs::Exists(String("/usr/written.txt")), "nothing persisted before close");
		f.Close();
	}
	ok(fs::Exists(String("/usr/written.txt")), "persisted on close");

	String text;
	fs::ReadText(String("/usr/written.txt"), text);
	eq(text, "alpha\nbeta\n", "contents round-tripped");

	// A handle destroyed without close still saves, because ~OpenFile calls
	// Close.  This is a deliberate divergence from Mini Micro 1, whose OpenFile
	// (Assets/Scripts/FileUtils.cs) has only an explicit Close -- no finalizer,
	// no IDisposable -- so a dropped handle there loses its writes.
	//
	// We diverge because a script's RawData/FileHandle now lives in a GCHandle,
	// so dropping the last reference runs a finalizer that can flush; losing the
	// data instead would be silent.  The cost, accepted knowingly: the save
	// happens when the sweep runs, not at a point the script chooses, so a
	// write-drop-read sequence depends on GC timing.  close() remains the
	// supported way to finish with a file -- it is the only one that is
	// deterministic and the only one where the script can see a write error.
	{
		fs::Resolved r2;
		fs::Resolve(String("/usr/dropped.txt"), r2);
		fs::OpenFile f(r2, String("w"));
		f.Write(String("saved anyway"));
	}
	ok(fs::Exists(String("/usr/dropped.txt")), "a dropped handle persists via the destructor");
	String dropped;
	fs::ReadText(String("/usr/dropped.txt"), dropped);
	eq(dropped, "saved anyway", "...with its contents intact");

	// Position and seeking, over the in-memory buffer.
	{
		fs::Resolved r3;
		fs::Resolve(String("/usr/written.txt"), r3);
		fs::OpenFile f(r3, String("r+"));
		ok(f.Position() == 0, "position starts at 0");
		String chars;
		ok(f.ReadChars(5, chars), "readChars");
		eq(chars, "alpha", "read 5 code points");
		ok(f.Position() == 5, "position advanced");
		f.SetPosition(0);
		ok(f.Position() == 0, "position is settable");
		String all;
		ok(f.ReadToEnd(all), "readToEnd");
		eq(all, "alpha\nbeta\n", "read everything back");
		f.Close();
	}
}

static void testUtf8Reads() {
	printf("\n-- UTF-8 code point reads --\n");
	fs::Resolved r;
	fs::Resolve(String("/usr/utf8.txt"), r);
	{
		fs::OpenFile f(r, String("w"));
		f.Write(String("aéπ𝄞z"));   // 1, 2, 3, and 4 byte encodings
		f.Close();
	}
	fs::Resolved r2;
	fs::Resolve(String("/usr/utf8.txt"), r2);
	fs::OpenFile f(r2, String("r"));
	String s;
	ok(f.ReadChars(4, s), "readChars across mixed encodings");
	eq(s, "aéπ𝄞", "read 4 code points, not 4 bytes");
	ok(f.ReadToEnd(s) && strcmp(s.c_str(), "z") == 0, "the rest follows");
	f.Close();
}

static void testMoveAndCopy() {
	printf("\n-- move and copy --\n");

	// Within one mount: a rename.
	ok(fs::MoveOrCopy(String("/usr/written.txt"), String("/usr/moved.txt"), true, false).empty(),
		"move within a mount");
	ok(!fs::Exists(String("/usr/written.txt")), "source is gone");
	ok(fs::Exists(String("/usr/moved.txt")), "destination is there");

	// Across mounts: the bytes are copied.
	ok(fs::MoveOrCopy(String("/usr/moved.txt"), String("/usr2/copied.txt"), false, false).empty(),
		"copy across mounts");
	ok(fs::Exists(String("/usr/moved.txt")), "source survives a copy");
	String text;
	ok(fs::ReadText(String("/usr2/copied.txt"), text), "destination readable");
	eq(text, "alpha\nbeta\n", "bytes arrived intact");

	ok(!fs::MoveOrCopy(String("/usr/moved.txt"), String("/usr2/copied.txt"), false, false).empty(),
		"copy over an existing file refused without overwrite");
	ok(fs::MoveOrCopy(String("/usr/moved.txt"), String("/usr2/copied.txt"), false, true).empty(),
		"copy over an existing file allowed with overwrite");
}

static void testLatchIsOneWay() {
	printf("\n-- the latch --\n");
	ok(fs::IsSandboxed(), "still sandboxed");
	// There is deliberately no way to test leaving: no such function exists.
	// If one is ever added, this comment is the thing that should have stopped it.
}

//--------------------------------------------------------------------------------

int main() {
	MiniScript::GCManager::Init();
	MiniScript::value_init_constants();
	MiniScript::ErrorTypes::Init();

	buildScratch();
	printf("scratch: %s\n", scratch.c_str());

	testPathHelpers();
	testPassthroughBeforeLatch();

	// Everything past here runs sandboxed.
	fs::Mount(String("sys"), fs::RealDirBackend::Open(String((scratch + "/sys").c_str()), false));
	fs::Mount(String("usr"), fs::RealDirBackend::Open(String((scratch + "/usr").c_str()), true));
	fs::Mount(String("usr2"), fs::RealDirBackend::Open(String((scratch + "/usr2").c_str()), true));
	fs::Mount(String("hw"), fs::RealDirBackend::Open(String((scratch + "/sys").c_str()), false), /*listed*/ false);
	// /hw shares the sys directory here purely so there is something to read.
	writeFile(scratch + "/sys/bezel.png", "PNG");

	testMountsWorkBeforeLatch();

	fs::EnterSandbox();

	testNormalization();
	testCwd();
	testMountBoundaries();
	testSymlinkContainment();
	testReadOnlyEnforcement();
	testListing();
	testOpenFileModes();
	testOpenFilePersistence();
	testUtf8Reads();
	testMoveAndCopy();
	testLatchIsOneWay();

	fs::CloseAllMounts();

	printf("\n%d checks, %d failures\n", checks, failures);
	return failures == 0 ? 0 : 1;
}
