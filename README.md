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
  - [ErrorStatus enum](#errorstatus-enum)
  - [HttpHeaderException](#httpheaderexception)
  - [URL class](#url-class)
  - [Constructors and object lifetime](#constructors-and-object-lifetime)
  - [Static methods](#static-methods)
  - [Getters](#getters)
- [Building](#building)
- [Dependencies](#dependencies)
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
| Hints map | Pre-computed parse offsets passed in to skip redundant scanning |
| Static pre-check | `can_parse` validates and populates hints without allocating a URL object |
| Immutable after construction | No setters — parse-once design |
| Zero-copy reads | All getters return `string_view` into owned storage |
| Error model | Strong enum-based status reporting via `ErrorStatus` (from [SlimCommonHttp](https://codeberg.org/greergan/SlimCommonHttp)) |

[↑ Top](#table-of-contents)

## Core API

### ErrorStatus enum

`ErrorStatus` is the scoped enum provided by [SlimCommonHttp](https://codeberg.org/greergan/SlimCommonHttp).

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

### HttpHeaderException

`HttpHeaderException` is the exception class provided by [SlimCommonHttp](https://codeberg.org/greergan/SlimCommonHttp).

[↑ Top](#table-of-contents)

### URL class

```cpp
slim::common::http::URL u("https://user:pass@example.com:8080/path?q=1#section");
```

### Constructors and object lifetime

| Form | Description |
|------|-------------|
| `URL()` | Default constructor, produces an empty URL |
| `URL(std::string_view s)` | Construct and parse from string. Throws `HttpHeaderException` on failure |
| `URL(std::string_view s, const hints_map& hints)` | Construct using pre-computed parse hints. Throws `HttpHeaderException` on failure |
| `URL(const URL&)` | Deleted — copies are not allowed |
| `URL& operator=(const URL&)` | Deleted — copies are not allowed |
| `URL(URL&&) noexcept` | Move construction is supported |
| `URL& operator=(URL&&) noexcept` | Move assignment is supported |

[↑ Top](#table-of-contents)

### Static methods

| Method | Description |
|--------|-------------|
| `static ErrorStatus can_parse(std::string_view s, hints_map& hints) noexcept` | Validates the URL string and populates parse hints without constructing a URL object |

Use `can_parse` when you need to validate a URL before committing to construction, or when you want to reuse the hints across multiple operations.

[↑ Top](#table-of-contents)

### Getters

All getters are `const noexcept` and return `std::string_view` into the URL's owned storage.

| Method | Returns |
|--------|---------|
| `hash() const noexcept` | Fragment identifier (e.g. `#section`) |
| `host() const noexcept` | Host and port (e.g. `example.com:8080`) |
| `hostname() const noexcept` | Host without port (e.g. `example.com`) |
| `href() const noexcept` | Full serialized URL |
| `origin() const noexcept` | Scheme + host + port (e.g. `https://example.com:8080`) |
| `password() const noexcept` | Password component of userinfo |
| `pathname() const noexcept` | Path (e.g. `/path`) |
| `port() const noexcept` | Port as string (e.g. `8080`) |
| `protocol() const noexcept` | Scheme with colon (e.g. `https:`) |
| `search() const noexcept` | Query string with leading `?` (e.g. `?q=1`) |
| `searchParams() const noexcept` | Serialized search parameters |
| `username() const noexcept` | Username component of userinfo |

[↑ Top](#table-of-contents)

## Building

This library is built using [SlimLibraryPackager](https://codeberg.org/greergan/SlimLibraryPackager). See that repository for build instructions.

[↑ Top](#table-of-contents)

## Dependencies

External package dependencies for this library are declared in the [`required_packages`](https://codeberg.org/greergan/SlimCommonHttpUrl/src/branch/master/required_packages) file at the repository root. This file is read by [SlimLibraryPackager](https://codeberg.org/greergan/SlimLibraryPackager) during the build process to resolve dependencies and install them if not present.

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

std::cout << u.protocol()  << '\n'; // -> "https:"
std::cout << u.hostname()  << '\n'; // -> "example.com"
std::cout << u.pathname()  << '\n'; // -> "/path"
std::cout << u.search()    << '\n'; // -> "?q=1"
std::cout << u.hash()      << '\n'; // -> "#section"
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
// Exception-based usage
try {
    slim::common::http::URL u("https://user:pass@example.com:8080/path?q=1#section");

    std::cout << u.href()     << '\n'; // -> "https://user:pass@example.com:8080/path?q=1#section"
    std::cout << u.origin()   << '\n'; // -> "https://example.com:8080"
    std::cout << u.host()     << '\n'; // -> "example.com:8080"
    std::cout << u.username() << '\n'; // -> "user"
    std::cout << u.password() << '\n'; // -> "pass"
}
catch (const slim::common::http::HttpHeaderException& e) {
    std::cerr << "URL error: " << e.what() << '\n';
}
catch (const std::exception& e) {
    std::cerr << "Unexpected error: " << e.what() << '\n';
}
```

[↑ Top](#table-of-contents)
