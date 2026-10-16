//
// loadfile.cpp
// raylib-miniscript
//
// raylib LoadFileData/LoadFileText hooks (web only): the in-memory file system
// (a preloaded file package) first, then fetch()
//

#include "loadfile.h"

#ifdef PLATFORM_WEB

#include "raylib.h"
#include <emscripten.h>
#include <emscripten/fetch.h>
#include <cstring>
#include <cstdlib>
#include <stdio.h>
#include <sys/stat.h>

EM_ASYNC_JS(unsigned char*, fetchData, (const char *_url, int *_size), {
	const url = UTF8ToString(_url);
	return await fetch(url).then(async (response)=>{
		if (response.status!==200) {
			console.warn(`LoadFileData: Failed loading ${url} (${response.status} ${response.statusText})`);
			return 0; // nullptr
		}
		const buffer = await response.arrayBuffer();
		let size = buffer.byteLength;
		// set the size variable
		(new Int32Array(HEAP8.buffer, _size))[0] = size;
		// allocate a buffer and copy the response into it
		let ptr = Module._malloc(size);
		(new Uint8Array(HEAP8.buffer, ptr, size)).set(new Uint8Array(buffer, 0, size));
		return ptr;
	});
});

EM_ASYNC_JS(char*, fetchText, (const char *_url), {
	const url = UTF8ToString(_url);
	return await fetch(url).then(async (response)=>{
		if (response.status!==200) {
			console.warn(`LoadFileText: Failed loading ${url} (${response.status} ${response.statusText})`);
			return 0; // nullptr
		}
		const text = await response.text();
		let size = lengthBytesUTF8(text)+1; // null terminator
		// allocate a buffer and copy the response into it
		let ptr = Module._malloc(size);
		stringToUTF8(text, ptr, size);
		return ptr;
	});
});

bool WebPackageLoaded() {
	static int loaded = -1;
	if (loaded < 0) {
		struct stat st;
		loaded = (stat("assets", &st) == 0 && S_ISDIR(st.st_mode)) ? 1 : 0;
	}
	return loaded == 1;
}

unsigned char* ReadPackagedFile(const char* path, int* size) {
	if (size) *size = 0;
	struct stat st;
	if (stat(path, &st) != 0 || !S_ISREG(st.st_mode)) return nullptr;
	FILE* f = fopen(path, "rb");
	if (f == nullptr) return nullptr;
	unsigned char* data = (unsigned char*)malloc((size_t)st.st_size + 1);
	if (data == nullptr) { fclose(f); return nullptr; }
	size_t got = fread(data, 1, (size_t)st.st_size, f);
	fclose(f);
	data[got] = '\0';
	if (size) *size = (int)got;
	return data;
}

// The package first, then the network; with a package loaded, a missing file
// is just missing.  (Plain C++, so a packaged read never unwinds asyncify.)
static unsigned char* loadFileDataHook(const char* path, int* size) {
	unsigned char* data = ReadPackagedFile(path, size);
	if (data != nullptr) return data;
	if (WebPackageLoaded()) { *size = 0; return nullptr; }
	return fetchData(path, size);
}

static char* loadFileTextHook(const char* path) {
	unsigned char* data = ReadPackagedFile(path, nullptr);
	if (data != nullptr) return (char*)data;
	if (WebPackageLoaded()) return nullptr;
	return fetchText(path);
}

void InstallLoadFileHooks() {
	SetLoadFileDataCallback(loadFileDataHook);
	SetLoadFileTextCallback(loadFileTextHook);
}

#endif // PLATFORM_WEB
