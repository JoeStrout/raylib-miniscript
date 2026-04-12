//
//  HttpModule.cpp
//  raylib-miniscript
//
//  Implements http.post(url, data, headers) for MiniScript scripts.
//
//  API (matches https://miniscript.org/wiki/Http.post):
//    http.post(url, data="", headers=null)
//      url     - string URL (http:// or https://)
//      data    - string (sent as-is) or map (form-encoded)
//      headers - optional map of extra request headers
//    Returns the response body as a string for text/JSON responses.
//
//  macOS/Linux: libcurl (system library, uses OS TLS — zero extra deps).
//  Windows:     WinHTTP (built into Windows, uses SChannel TLS — zero extra deps).
//  Web:         emscripten_fetch (already enabled with -sFETCH=1).
//

#include "HttpModule.h"
#include "MiniscriptInterpreter.h"
#include "MiniscriptIntrinsics.h"
#include "MiniscriptErrors.h"
#include "MiniscriptTypes.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#ifdef PLATFORM_WEB
  #include <emscripten/fetch.h>
  #include <map>
#elif defined(_WIN32)
  #include <windows.h>
  #include <winhttp.h>
  #pragma comment(lib, "winhttp.lib")
#else
  #include <curl/curl.h>
#endif

using namespace MiniScript;

//----------------------------------------------------------------------
// Shared helpers
//----------------------------------------------------------------------

// URL-encode a byte string (for form data values).
static std::string UrlEncode(const char* src, size_t len) {
	static const char hex[] = "0123456789ABCDEF";
	std::string out;
	out.reserve(len * 3);
	for (size_t i = 0; i < len; i++) {
		unsigned char c = (unsigned char)src[i];
		if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
			(c >= '0' && c <= '9') ||
			c == '-' || c == '_' || c == '.' || c == '~') {
			out += (char)c;
		} else if (c == ' ') {
			out += '+';
		} else {
			out += '%';
			out += hex[c >> 4];
			out += hex[c & 0xF];
		}
	}
	return out;
}

// Convert a MiniScript map to application/x-www-form-urlencoded body.
static std::string MapToFormData(ValueDict dict) {
	std::string result;
	ValueDictIterator iter = dict.GetIterator();
	while (!iter.Done()) {
		String k = iter.Key().ToString();
		String v = iter.Value().ToString();
		std::string ek = UrlEncode(k.c_str(), (size_t)k.LengthB());
		std::string ev = UrlEncode(v.c_str(), (size_t)v.LengthB());
		if (!result.empty()) result += '&';
		result += ek + '=' + ev;
		iter.Next();
	}
	return result;
}

//----------------------------------------------------------------------
// Desktop implementation
//----------------------------------------------------------------------

#ifndef PLATFORM_WEB

// Perform a synchronous HTTP/HTTPS POST.
// Returns the response body, or raises RuntimeException on error.
static std::string DoHttpPost(const char* url,
							  const std::string& body,
							  const std::vector<std::string>& headers);

//--------------------------------------------------
// Windows: WinHTTP
//--------------------------------------------------
#ifdef _WIN32

static std::wstring ToWide(const char* s) {
	if (!s || !*s) return L"";
	int n = MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
	std::wstring out(n - 1, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, s, -1, &out[0], n);
	return out;
}

static std::string DoHttpPost(const char* urlStr,
							  const std::string& body,
							  const std::vector<std::string>& headers) {
	std::wstring wUrl = ToWide(urlStr);

	// Crack URL into components.
	wchar_t wHost[256] = {}, wPath[2048] = {};
	URL_COMPONENTS uc = {};
	uc.dwStructSize        = sizeof(uc);
	uc.lpszHostName        = wHost;
	uc.dwHostNameLength    = (DWORD)(sizeof(wHost) / sizeof(wchar_t));
	uc.lpszUrlPath         = wPath;
	uc.dwUrlPathLength     = (DWORD)(sizeof(wPath) / sizeof(wchar_t));
	if (!WinHttpCrackUrl(wUrl.c_str(), 0, 0, &uc)) {
		RuntimeException(String("http.post: invalid URL: ") + urlStr).raise();
	}
	bool isHttps = (uc.nScheme == INTERNET_SCHEME_HTTPS);

	HINTERNET hSession = WinHttpOpen(L"raylib-miniscript/1.0",
		WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
		WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (!hSession) RuntimeException(String("http.post: WinHttpOpen failed")).raise();

	HINTERNET hConnect = WinHttpConnect(hSession, wHost, uc.nPort, 0);
	if (!hConnect) {
		WinHttpCloseHandle(hSession);
		RuntimeException(String("http.post: WinHttpConnect failed")).raise();
	}

	DWORD flags = isHttps ? WINHTTP_FLAG_SECURE : 0;
	HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST",
		wPath[0] ? wPath : L"/",
		nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
	if (!hRequest) {
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);
		RuntimeException(String("http.post: WinHttpOpenRequest failed")).raise();
	}

	// Add custom headers.
	if (!headers.empty()) {
		std::wstring wHeaders;
		for (const auto& h : headers) {
			wHeaders += ToWide(h.c_str());
			wHeaders += L"\r\n";
		}
		WinHttpAddRequestHeaders(hRequest, wHeaders.c_str(), (DWORD)wHeaders.size(),
			WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);
	}

	// Send request with body.
	BOOL ok = WinHttpSendRequest(hRequest,
		WINHTTP_NO_ADDITIONAL_HEADERS, 0,
		(LPVOID)body.c_str(), (DWORD)body.size(), (DWORD)body.size(), 0);
	if (!ok) {
		WinHttpCloseHandle(hRequest);
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);
		RuntimeException(String("http.post: WinHttpSendRequest failed")).raise();
	}

	ok = WinHttpReceiveResponse(hRequest, nullptr);
	if (!ok) {
		WinHttpCloseHandle(hRequest);
		WinHttpCloseHandle(hConnect);
		WinHttpCloseHandle(hSession);
		RuntimeException(String("http.post: WinHttpReceiveResponse failed")).raise();
	}

	// Read response body.
	std::string responseBody;
	DWORD available = 0;
	while (WinHttpQueryDataAvailable(hRequest, &available) && available > 0) {
		std::vector<char> buf(available);
		DWORD bytesRead = 0;
		if (!WinHttpReadData(hRequest, buf.data(), available, &bytesRead)) break;
		responseBody.append(buf.data(), bytesRead);
	}

	WinHttpCloseHandle(hRequest);
	WinHttpCloseHandle(hConnect);
	WinHttpCloseHandle(hSession);
	return responseBody;
}

//--------------------------------------------------
// macOS / Linux: libcurl
//--------------------------------------------------
#else

static size_t CurlWriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
	std::string* buf = static_cast<std::string*>(userdata);
	buf->append(ptr, size * nmemb);
	return size * nmemb;
}

static std::string DoHttpPost(const char* urlStr,
							  const std::string& body,
							  const std::vector<std::string>& headers) {
	struct curl_slist* headerList = nullptr;
	for (const auto& h : headers) {
		headerList = curl_slist_append(headerList, h.c_str());
	}

	CURL* curl = curl_easy_init();
	if (!curl) {
		curl_slist_free_all(headerList);
		RuntimeException(String("http.post: failed to initialize curl")).raise();
	}

	std::string responseBody;
	curl_easy_setopt(curl, CURLOPT_URL, urlStr);
	curl_easy_setopt(curl, CURLOPT_POST, 1L);
	curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
	curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)body.size());
	if (headerList) curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerList);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);

	CURLcode res = curl_easy_perform(curl);
	curl_slist_free_all(headerList);
	curl_easy_cleanup(curl);

	if (res != CURLE_OK) {
		RuntimeException(String("http.post: ") + curl_easy_strerror(res)).raise();
	}
	return responseBody;
}

#endif // _WIN32 / macOS+Linux

//--------------------------------------------------
// Shared intrinsic (calls DoHttpPost)
//--------------------------------------------------

static IntrinsicResult intrinsic_http_post(Context* context, IntrinsicResult partialResult) {
	String url = context->GetVar("url").ToString();
	Value dataVal = context->GetVar("data");
	Value headersVal = context->GetVar("headers");

	// Build request body.
	std::string body;
	bool injectFormType = false;
	if (dataVal.type == ValueType::Map) {
		body = MapToFormData(dataVal.GetDict());
		injectFormType = true;
	} else if (!dataVal.IsNull()) {
		String s = dataVal.ToString();
		body.assign(s.c_str(), (size_t)s.LengthB());
	}

	// Collect header lines.
	std::vector<std::string> headers;
	if (injectFormType) {
		headers.push_back("Content-Type: application/x-www-form-urlencoded");
	}
	if (headersVal.type == ValueType::Map) {
		ValueDict hmap = headersVal.GetDict();
		ValueDictIterator iter = hmap.GetIterator();
		while (!iter.Done()) {
			headers.push_back(
				std::string(iter.Key().ToString().c_str()) +
				": " +
				std::string(iter.Value().ToString().c_str()));
			iter.Next();
		}
	}

	try {
		std::string responseBody = DoHttpPost(url.c_str(), body, headers);
		return IntrinsicResult(String(responseBody.c_str(), (long)responseBody.size()));
	} catch (MiniscriptException& e) {
		return IntrinsicResult(e.message);
	}
}

//----------------------------------------------------------------------
// Web implementation (emscripten_fetch)
//----------------------------------------------------------------------

#else // PLATFORM_WEB

struct HttpFetchState {
	emscripten_fetch_t* fetch = nullptr;
	bool completed = false;
	int status = 0;
	std::string body;
	std::vector<std::string> headerStrings; // kept alive for the fetch duration
	std::vector<const char*> headerPtrs;    // null-terminated k,v,k,v,...,null
};

static std::map<long, HttpFetchState> activeHttpFetches;
static long nextHttpFetchId = 1;

static void http_fetch_done(emscripten_fetch_t* fetch) {
	for (auto& pair : activeHttpFetches) {
		if (pair.second.fetch == fetch) {
			pair.second.completed = true;
			pair.second.status = fetch->status;
			break;
		}
	}
}

static IntrinsicResult intrinsic_http_post(Context* context, IntrinsicResult partialResult) {
	// State 2: fetch has completed — collect the response.
	if (!partialResult.Done()) {
		long fetchId = (long)partialResult.Result().DoubleValue();
		auto it = activeHttpFetches.find(fetchId);
		if (it == activeHttpFetches.end()) {
			return IntrinsicResult(String("http.post: internal error (fetch not found)"));
		}
		HttpFetchState& state = it->second;
		if (!state.completed) return partialResult; // still in flight

		emscripten_fetch_t* fetch = state.fetch;
		activeHttpFetches.erase(it);

		std::string resp;
		if (fetch->numBytes > 0) resp.assign(fetch->data, (size_t)fetch->numBytes);
		emscripten_fetch_close(fetch);
		return IntrinsicResult(String(resp.c_str(), (long)resp.size()));
	}

	// State 1: start the fetch.
	String url = context->GetVar("url").ToString();
	Value dataVal = context->GetVar("data");
	Value headersVal = context->GetVar("headers");

	long fetchId = nextHttpFetchId++;
	HttpFetchState& state = activeHttpFetches[fetchId]; // default-construct in place

	bool injectFormType = false;
	if (dataVal.type == ValueType::Map) {
		state.body = MapToFormData(dataVal.GetDict());
		injectFormType = true;
	} else if (!dataVal.IsNull()) {
		String s = dataVal.ToString();
		state.body.assign(s.c_str(), (size_t)s.LengthB());
	}

	// Build header strings first, then pointers (avoids dangling ptrs on realloc).
	if (injectFormType) {
		state.headerStrings.push_back("Content-Type");
		state.headerStrings.push_back("application/x-www-form-urlencoded");
	}
	if (headersVal.type == ValueType::Map) {
		ValueDict hmap = headersVal.GetDict();
		ValueDictIterator iter = hmap.GetIterator();
		while (!iter.Done()) {
			state.headerStrings.push_back(iter.Key().ToString().c_str());
			state.headerStrings.push_back(iter.Value().ToString().c_str());
			iter.Next();
		}
	}
	for (const auto& s : state.headerStrings) state.headerPtrs.push_back(s.c_str());
	state.headerPtrs.push_back(nullptr);

	emscripten_fetch_attr_t attr;
	emscripten_fetch_attr_init(&attr);
	strcpy(attr.requestMethod, "POST");
	attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
	attr.onsuccess = http_fetch_done;
	attr.onerror = http_fetch_done;
	attr.requestData = state.body.c_str();
	attr.requestDataSize = (size_t)state.body.size();
	attr.requestHeaders = state.headerPtrs.data();

	state.fetch = emscripten_fetch(&attr, url.c_str());

	return IntrinsicResult(Value((double)fetchId), false);
}

#endif // PLATFORM_WEB

//----------------------------------------------------------------------
// Module registration
//----------------------------------------------------------------------

static ValueDict httpMap;

static IntrinsicResult intrinsic_http(Context* context, IntrinsicResult partialResult) {
	return IntrinsicResult(httpMap);
}

void AddHttpIntrinsics() {
	Intrinsic *i;

	// Send an HTTP POST request; returns the response body as a string
	i = Intrinsic::Create("");
	i->AddParam("url", "");
	i->AddParam("data");
	i->AddParam("headers");
	i->code = intrinsic_http_post;
	httpMap.SetValue(String("post"), i->GetFunc());

	Intrinsic* httpFunc = Intrinsic::Create("http");
	httpFunc->code = intrinsic_http;
}
