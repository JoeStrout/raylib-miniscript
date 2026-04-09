//
//  HttpModule.h
//  raylib-miniscript
//
//  HTTP module: exposes http.post(url, data, headers) to MiniScript scripts.
//  Desktop uses libcurl (system library); web uses emscripten_fetch.
//

#ifndef HTTPMODULE_H
#define HTTPMODULE_H

/// Register the `http` intrinsic and its methods into the MiniScript environment.
void AddHttpIntrinsics();

#endif // HTTPMODULE_H
