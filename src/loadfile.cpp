//
// loadfile.cpp
// raylib-miniscript
//
// raylib LoadFileData/LoadFileText hooks (web only)
//

#include "loadfile.h"

#ifdef PLATFORM_WEB

#include "raylib.h"
#include <emscripten.h>
#include <emscripten/fetch.h>
#include <cstring>
#include <stdio.h>

EM_ASYNC_JS(unsigned char*, fetchData, (const char *_url, int *_size), {
	const url = UTF8ToString(_url);
	Module.print(`LoadFileData: Fetching ${url} as data...`);
	return await fetch(url).then(async (response)=>{
		if (response.status!==200) {
			Module.print(`LoadFileData: Failed loading ${url} (${response.status} ${response.statusText})`);
			return 0; // nullptr
		}
		const buffer = await response.arrayBuffer();
		let size = buffer.byteLength;
		Module.print(`LoadFileData: Successfully loaded ${size} byte(s)`);
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
	Module.print(`LoadFileText: Fetching ${url} as text...`);
	return await fetch(url).then(async (response)=>{
		if (response.status!==200) {
			Module.print(`LoadFileText: Failed loading ${url} (${response.status} ${response.statusText})`);
			return 0; // nullptr
		}
		const text = await response.text();
		let size = lengthBytesUTF8(text)+1; // null terminator
		Module.print(`LoadFileText: Successfully loaded ${text.length} character(s) (${size} byte(s))`);
		// allocate a buffer and copy the response into it
		let ptr = Module._malloc(size);
		stringToUTF8(text, ptr, size);
		return ptr;
	});
});

void InstallLoadFileHooks() {
	SetLoadFileDataCallback(fetchData);
	SetLoadFileTextCallback(fetchText);
}

#endif // PLATFORM_WEB
