<a href="https://codeberg.org/greergan/SlimTS">
  <img src="https://raw.githubusercontent.com/greergan/SlimTS/master/assets/slimts_logo.png" width="75" alt="SlimTS Logo">
</a>

# SlimCommonHttpUrl

A lightweight, WHATWG-oriented HTTP URL implementation in modern C++.  
Acts as a validating, backing store for the [SlimTS](https://codeberg.org/greergan/SlimTS) Javascript URL object.  
Part of the [SlimCommon](https://codeberg.org/greergan/SlimCommon) library.  
Built using [SlimLibraryPackager](https://codeberg.org/greergan/SlimLibraryPackager).  
CI/CD supplied by unified workflows provided by [SlimLibraryPackager](https://codeberg.org/greergan/SlimLibraryPackager).  

[!IMPORTANT]
**Required: Complete SearchParams parsing, best to create a SearchParam/SearchParams class similar to Header/Headers**

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Core API](#core-api)
  - [ErrorStatus enum](#errorstatus-enum)
  - [UrlParseException](#urlparseexception)
  - [URL class](#url-class)
  - [Constructors and object lifetime](#constructors-and-object-lifetime)
  - [Static methods](#static-methods)
  - [Getters](#getters)
  - [Friend classes](#friend-classes)
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
| Userinfo | Username and password parsed per WHATWG — both default to empty string |
| Immutable after construction | No setters — parse-once design |
| Zero-copy reads | All getters return `string_view` into owned storage |
| Error model | Strong enum-based status reporting via `ErrorStatus` (from [SlimCommonHttp](https://codeberg.org/greergan/SlimCommonHttp)) |

[↑ Top](#table-of-contents)

## Core API

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

Copy construction and copy assignment are not declared; the compiler-generated defaults apply. Move construction and move assignment are supported via the compiler-generated defaults.

[↑ Top](#table-of-contents)

### Static methods

| Method | Description |
|--------|-------------|
| `static ErrorStatus can_parse(std::string_view s, hints_map& hints) noexcept` | Validates the URL string and populates parse hints without constructing a URL object |

Use `can_parse` when you need to validate a URL before committing to construction, or when you want to reuse the hints across multiple operations.

[↑ Top](#table-of-contents)

### Getters

All getters are `const noexcept` and return `std::string_view` into the URL's owned storage. Per the WHATWG standard, all components default to empty string when absent.

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
| `searchParams() const noexcept` | Serialized search parameters (not yet implemented) |
| `hash() const noexcept` | Fragment without leading `#` (e.g. `"section"`); empty if absent |
| `href() const noexcept` | Full original URL string as supplied to the constructor |

[↑ Top](#table-of-contents)

### Friend classes

```cpp
friend class Request;
```

[↑ Top](#table-of-contents)

## Building

This library is built using [SlimLibraryPackager](https://codeberg.org/greergan/SlimLibraryPackager). See that repository for build instructions.

[↑ Top](#table-of-contents)

## Dependencies

### required_packages

External package dependencies for this library are declared in the [`required_packages`](required_packages) file at the repository root. This file is read by [SlimLibraryPackager](https://codeberg.org/greergan/SlimLibraryPackager) during the build process to resolve dependencies and install them if not present.

```
SlimCommonHttp 0.2.0
SlimCommonUtilities 0.12.0
```

- [SlimCommonHttp](https://codeberg.org/greergan/SlimCommonHttp)
- [SlimCommonUtilities](https://codeberg.org/greergan/SlimCommonUtilities)

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

[↑ Top](#table-of-contents)
