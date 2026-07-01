<a href="https://codeberg.org/greergan/SlimTS">
  <img src="https://raw.githubusercontent.com/greergan/SlimTS/master/assets/slimts_logo.png" width="75" alt="SlimTS Logo">
</a>

# SlimCommonHttpUrl

A lightweight, WHATWG-oriented HTTP URL implementation in modern C++.  
Acts as a validating, backing store for the [SlimTS](https://codeberg.org/greergan/SlimTS) Javascript URL object.  
Part of the [SlimCommon](https://codeberg.org/greergan/SlimCommon) library.  
Built using [SlimLibraryPackager](https://codeberg.org/greergan/SlimLibraryPackager).  
CI/CD supplied by unified workflows provided by [SlimLibraryPackager](https://codeberg.org/greergan/SlimLibraryPackager).  

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Core API](#core-api)
  - [hints_map](#hints_map)
  - [UrlParseMode enum](#urlparsemode-enum)
  - [ErrorStatus enum](#errorstatus-enum)
  - [UrlParseException](#urlparseexception)
  - [URL class](#url-class)
  - [Constructors and object lifetime](#constructors-and-object-lifetime)
  - [Static methods](#static-methods)
  - [Getters](#getters)
- [Building](#building)
- [Dependencies](#dependencies)
  - [required_packages](#required_packages)
- [Examples](#examples)

## Overview

This library provides a strict, validation-heavy HTTP URL parser with:
- WHATWG URL Standard compliant parsing behavior
- Hints-assisted parsing to avoid redundant scanning
- Zero dynamic allocation in validation paths (where possible)
- Explicit status reporting via [`ErrorStatus`](https://codeberg.org/greergan/SlimCommonHttp)
- Strict parsing over permissive recovery
- Minimal runtime overhead in hot paths
- Heavy use of `noexcept`

[↑ Top](#table-of-contents)

## Features

| Feature | Description |
|--------|-------------|
| WHATWG compatibility | Getter surface mirrors the browser `URL` API |
| Supported schemes | `http`, `https`, `ws`, `wss`, `file` |
| Hints map | Pre-computed parse offsets passed in to skip redundant scanning |
| Static pre-check | `can_parse` validates and populates hints without allocating a URL object |
| Path-only mode | `UrlParseMode::PATH` skips scheme/authority parsing for bare path inputs |
| Userinfo | Username and password parsed per WHATWG — both default to empty string |
| Query parameters | `searchParams()` exposes structured access via [SlimCommonHttpUrlSearchParams](https://codeberg.org/greergan/SlimCommonHttpUrlSearchParams) |
| Immutable after construction | No setters on `URL` — parse-once design |
| Zero-copy reads | `URL` getters return `string_view` into owned storage |
| Error model | Strong enum-based status reporting via `ErrorStatus` (from [SlimCommonHttp](https://codeberg.org/greergan/SlimCommonHttp)) |

[↑ Top](#table-of-contents)

## Core API

### hints_map

```cpp
using hints_map = std::unordered_map<std::string, std::pair<int, int>>;
```

`hints_map` is an alias for a map of named parse offsets into the URL string. Each entry maps a component name to a `{start, length}` pair describing its byte range within the input. Hints are populated by `can_parse` and consumed by the hinted constructor to skip redundant scanning on construction.

[↑ Top](#table-of-contents)

### UrlParseMode enum

```cpp
enum struct UrlParseMode : uint8_t { FULL, PATH };
```

Controls how much of the input the parser processes.

| Value | Description |
|-------|-------------|
| `FULL` | Default behaviour — parses the complete URL including scheme, authority, path, query, and fragment |
| `PATH` | Skips scheme and authority parsing entirely; treats the input as a bare path (and optional query/fragment) only |

Use `PATH` mode when the input is known to be a path string rather than a full URL, avoiding unnecessary validation overhead for scheme and host components.

[↑ Top](#table-of-contents)

### ErrorStatus enum

`ErrorStatus` is the scoped enum provided by [SlimCommonHttp](https://codeberg.org/greergan/SlimCommonHttp).

Values relevant to URL parsing:

| Value | Meaning |
|-------|---------|
| `UrlStringEmpty` | Input string is empty |
| `UrlInvalidControlCharacter` | Input contains a control character |
| `UrlSchemeInvalidCharacter` | Scheme contains a disallowed character |
| `UrlSchemeUnsupported` | Scheme is not one of the supported values |
| `UrlSchemeDelimiterMissing` | `://` delimiter not found after scheme |
| `UrlBodyInvalidCharacter` | URL body contains a disallowed character (e.g. unencoded space) |
| `UrlHostMissing` | No host found after authority delimiter or `@` |
| `UrlHostInvalidStart` | Host begins with a non-alphanumeric character |
| `UrlHostInvalidCharacter` | Host contains a disallowed character |
| `UrlPortInvalidCharacter` | Port contains a non-digit character |
| `UrlFilePathMissing` | `file://` URL has no path |
| `UrlFilePathTrailingSlash` | `file://` URL path ends with `/` |
| `OK` | No error; the operation succeeded |

Every value has a human-readable description, retrievable without allocation:

```cpp
[[nodiscard]] constexpr std::string_view error::status::to_string(ErrorStatus status) noexcept;
```

```cpp
slim::common::http::hints_map hints;
ErrorStatus e = slim::common::http::URL::can_parse("not a url", hints);
if (e != ErrorStatus::OK) {
    std::cerr << error::status::to_string(e) << '\n';
}
```

[↑ Top](#table-of-contents)

### UrlParseException

`UrlParseException` is thrown by the `URL` constructors on parse failure. It carries the `ErrorStatus` value that caused the failure.

[↑ Top](#table-of-contents)

### URL class

```cpp
slim::common::http::URL u("https://user:pass@example.com:8080/path?q=1#section");
```

### Constructors and object lifetime

| Form | Description |
|------|-------------|
| `URL()` | Default constructor, produces an empty URL |
| `URL(std::string_view s)` | Construct and parse from string. Throws `UrlParseException` on failure |
| `URL(std::string_view s, const hints_map& hints)` | Construct using pre-computed parse hints. No validation — caller must ensure hints are valid |
| `URL(std::string_view s, UrlParseMode mode)` | Construct with an explicit parse mode. Use `UrlParseMode::PATH` to skip scheme and authority parsing and treat the input as a bare path. Throws `UrlParseException` on failure |

Copy construction and copy assignment are not declared; the compiler-generated defaults apply. Move construction and move assignment are supported via the compiler-generated defaults.

[↑ Top](#table-of-contents)

### Static methods

| Method | Description |
|--------|-------------|
| `static ErrorStatus can_parse(std::string_view s, hints_map& hints) noexcept` | Validates the URL string and populates parse hints without constructing a URL object |

Use `can_parse` when you need to validate a URL before committing to construction, or when you want to reuse the hints across multiple operations.

[↑ Top](#table-of-contents)

### Getters

All getters are `const noexcept`. Per the WHATWG standard, all components default to empty string when absent.

| Method | Returns |
|--------|---------|
| `protocol() const noexcept` | Scheme without colon (e.g. `"https"`) |
| `username() const noexcept` | Username component of userinfo (empty string if absent) |
| `password() const noexcept` | Password component of userinfo (empty string if absent) |
| `host() const noexcept` | Host and port (e.g. `"example.com:8080"`); port omitted if absent |
| `hostname() const noexcept` | Host without port (e.g. `"example.com"`) |
| `port() const noexcept` | Port as string (e.g. `"8080"`); empty if absent |
| `origin() const noexcept` | Scheme + host + port, excluding default ports 80/443 (e.g. `"https://example.com:8080"`) |
| `pathname() const noexcept` | Path (e.g. `"/path"`); empty if absent |
| `search() const noexcept` | Query string without leading `?` (e.g. `"q=1"`); empty if absent |
| `searchParams() const noexcept` | Returns a `std::shared_ptr<`[`UrlSearchParams`](https://codeberg.org/greergan/SlimCommonHttpUrlSearchParams)`>` giving structured, mutable access to the query parameters |
| `hash() const noexcept` | Fragment without leading `#` (e.g. `"section"`); empty if absent |
| `href() const noexcept` | Full original URL string as supplied to the constructor |

[↑ Top](#table-of-contents)

## Building

This library is built using [SlimLibraryPackager](https://codeberg.org/greergan/SlimLibraryPackager). See that repository for build instructions.

[↑ Top](#table-of-contents)

## Dependencies

### required_packages

External package dependencies for this library are declared in the [`required_packages`](required_packages) file at the repository root. This file is read by [SlimLibraryPackager](https://codeberg.org/greergan/SlimLibraryPackager) during the build process to resolve dependencies and install them if not present.

```
# PackageName [minVersion [maxVersion]]
SlimCommonHttp 0.2.0
SlimCommonUtilities 0.12.0
SlimCommonHttpUrlSearchParam
SlimCommonHttpUrlSearchParams
```

- [SlimCommonHttp](https://codeberg.org/greergan/SlimCommonHttp)
- [SlimCommonUtilities](https://codeberg.org/greergan/SlimCommonUtilities)
- [SlimCommonHttpUrlSearchParam](https://codeberg.org/greergan/SlimCommonHttpUrlSearchParam)
- [SlimCommonHttpUrlSearchParams](https://codeberg.org/greergan/SlimCommonHttpUrlSearchParams)

[↑ Top](#table-of-contents)

## Examples

```cpp
// Simple construction
slim::common::http::URL u("https://example.com/path?q=1#section");

std::cout << u.protocol()  << '\n'; // -> "https"
std::cout << u.hostname()  << '\n'; // -> "example.com"
std::cout << u.pathname()  << '\n'; // -> "/path"
std::cout << u.search()    << '\n'; // -> "q=1"
std::cout << u.hash()      << '\n'; // -> "section"
```

```cpp
// Pre-validation with hints
slim::common::http::hints_map hints;
ErrorStatus e = slim::common::http::URL::can_parse("https://example.com/path", hints);
if (e != ErrorStatus::OK) {
    std::cerr << error::status::to_string(e) << '\n';
    return e;
}

slim::common::http::URL u("https://example.com/path", hints);
```

```cpp
// Full URL with userinfo
try {
    slim::common::http::URL u("https://user:pass@example.com:8080/path?q=1#section");

    std::cout << u.href()      << '\n'; // -> "https://user:pass@example.com:8080/path?q=1#section"
    std::cout << u.protocol()  << '\n'; // -> "https"
    std::cout << u.username()  << '\n'; // -> "user"
    std::cout << u.password()  << '\n'; // -> "pass"
    std::cout << u.origin()    << '\n'; // -> "https://example.com:8080"
    std::cout << u.host()      << '\n'; // -> "example.com:8080"
    std::cout << u.hostname()  << '\n'; // -> "example.com"
    std::cout << u.port()      << '\n'; // -> "8080"
    std::cout << u.pathname()  << '\n'; // -> "/path"
    std::cout << u.search()    << '\n'; // -> "q=1"
    std::cout << u.hash()      << '\n'; // -> "section"
}
catch (const slim::common::http::UrlParseException& e) {
    std::cerr << "URL error: " << e.what() << '\n';
}
catch (const std::exception& e) {
    std::cerr << "Unexpected error: " << e.what() << '\n';
}
```

```cpp
// Username only (no password)
slim::common::http::URL u("https://user@example.com/path");
std::cout << u.username() << '\n'; // -> "user"
std::cout << u.password() << '\n'; // -> ""
```

```cpp
// Path-only mode — skips scheme and authority parsing
slim::common::http::URL u("/api/v1/resource?filter=active#results", slim::common::http::UrlParseMode::PATH);
std::cout << u.pathname() << '\n'; // -> "/api/v1/resource"
std::cout << u.search()   << '\n'; // -> "filter=active"
std::cout << u.hash()     << '\n'; // -> "results"
```

```cpp
// file:// URL
try {
    slim::common::http::URL u("file:///home/user/document.txt");
    std::cout << u.protocol() << '\n'; // -> "file"
    std::cout << u.pathname() << '\n'; // -> "/home/user/document.txt"
}
catch (const slim::common::http::UrlParseException& e) {
    std::cerr << "URL error: " << e.what() << '\n';
}
```

```cpp
// Reading query parameters via searchParams()
slim::common::http::URL u("https://example.com/search?q=cats&tag=cute&tag=fluffy");

auto params = u.searchParams(); // std::shared_ptr<UrlSearchParams>

std::cout << params->has("q") << '\n'; // -> 1 (true)

if (auto q = params->get("q")) {
    std::cout << q->get_value() << '\n'; // -> "cats"
}
```

[↑ Top](#table-of-contents)
