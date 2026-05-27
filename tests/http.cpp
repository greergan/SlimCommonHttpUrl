#include <catch2/catch_test_macros.hpp>
#include <slim/common/http/url.h>
#include <slim/SlimValue.hpp>

using slim::common::http::URL;

// -----------------------------------------------------------------------------
// Valid http:// URLs
// -----------------------------------------------------------------------------

TEST_CASE("http:// - host only", "[http][valid]") {
	auto url = URL("http://example.com");
	CHECK(url.protocol() == "http");
	CHECK(url.host() == "example.com");
	CHECK(url.hostname() == "example.com");
	CHECK(url.origin() == "http://example.com");
	CHECK(url.port().empty());
	CHECK(url.pathname().empty());
	CHECK(url.search().empty());
	CHECK(url.hash().empty());
}

TEST_CASE("http:// - host with / as path", "[http][valid]") {
	auto url = URL("http://example.com/");
	CHECK(url.protocol() == "http");
	CHECK(url.host() == "example.com");
	CHECK(url.hostname() == "example.com");
	CHECK(url.origin() == "http://example.com");
	CHECK(url.port().empty());
	CHECK(url.pathname() == "/");
	CHECK(url.search().empty());
	CHECK(url.hash().empty());
}

TEST_CASE("http:// - host with path", "[http][valid]") {
	auto url = URL("http://example.com/path");
	CHECK(url.protocol() == "http");
	CHECK(url.host() == "example.com");
	CHECK(url.hostname() == "example.com");
	CHECK(url.origin() == "http://example.com");
	CHECK(url.port().empty());
	CHECK(url.pathname() == "/path");
	CHECK(url.search().empty());
	CHECK(url.hash().empty());
}

TEST_CASE("http:// - host with path and query string", "[http][valid]") {
	auto url = URL("http://example.com/path?q=hello");
}

TEST_CASE("http:// - host with path, query string, and fragment", "[http][valid]") {
	auto url = URL("http://example.com/path?q=hello#section");
}

TEST_CASE("http:// - host with explicit port and / as path", "[http][valid]") {
	auto url = URL("http://example.com:8080/");
	CHECK(url.protocol() == "http");
	CHECK(url.host() == "example.com:8080");
	CHECK(url.hostname() == "example.com");
	CHECK(url.origin() == "http://example.com:8080");
	CHECK(url.port() == "8080");
	CHECK(url.pathname() == "/");
	CHECK(url.search().empty());
	CHECK(url.hash().empty());
}

TEST_CASE("http:// - host with explicit port and / as path with fragment", "[http][valid]") {
	auto url = URL("http://example.com:8080/#abc");
	CHECK(url.protocol() == "http");
	CHECK(url.host() == "example.com:8080");
	CHECK(url.hostname() == "example.com");
	CHECK(url.origin() == "http://example.com:8080");
	CHECK(url.port() == "8080");
	CHECK(url.pathname() == "/");
	CHECK(url.search().empty());
	CHECK(url.hash() == "abc");
}

TEST_CASE("http:// - host with explicit port and path", "[http][valid]") {
	auto url = URL("http://example.com:8080/path");
	CHECK(url.protocol() == "http");
	CHECK(url.host() == "example.com:8080");
	CHECK(url.hostname() == "example.com");
	CHECK(url.origin() == "http://example.com:8080");
	CHECK(url.port() == "8080");
	CHECK(url.pathname() == "/path");
	CHECK(url.search().empty());
	CHECK(url.hash().empty());
}

TEST_CASE("http:// - host with explicit port, path and fragment", "[http][valid]") {
	auto url = URL("http://example.com:8080/path#section");
	CHECK(url.protocol() == "http");
	CHECK(url.host() == "example.com:8080");
	CHECK(url.hostname() == "example.com");
	CHECK(url.origin() == "http://example.com:8080");
	CHECK(url.port() == "8080");
	CHECK(url.pathname() == "/path");
	CHECK(url.search().empty());
	CHECK(url.hash() == "section");
}

TEST_CASE("http:// - host with explicit port, path and search", "[http][valid]") {
	auto url = URL("http://example.com:8080/path?q=hello");
}

TEST_CASE("http:// - host with explicit port, path, search and fragment", "[http][valid]") {
	auto url = URL("http://example.com:8080/path?q=hello#section");
}

// -----------------------------------------------------------------------------
// Invalid http:// URLs
// -----------------------------------------------------------------------------

TEST_CASE("http:// - no host (bare http://)", "[http][invalid]") {
	auto url = URL("http://");
}

TEST_CASE("http:// - scheme starting with digit is rejected", "[http][scheme][regression]") {
	auto url = URL("1http://example.com");
}

TEST_CASE("http:// - space in URL", "[http][invalid]") {
	auto url = URL("http://example.com/path with spaces");
}

TEST_CASE("http:// - empty scheme is an error", "[http][scheme]") {
	auto url = URL("://example.com");
}

TEST_CASE("http:// - only first error field is recorded when host is invalid", "[http][errors]") {
	auto url = URL("http://:abc/path");
}

TEST_CASE("http:// - root path slash is valid and path coord is set", "[http][regression]") {
	auto url = URL("http://example.com/");
}

TEST_CASE("http:// - host with port and no path is valid", "[http][regression]") {
	auto url = URL("http://example.com:80");
	CHECK(url.protocol() == "http");
	CHECK(url.host() == "example.com:80");
	CHECK(url.hostname() == "example.com");
	CHECK(url.origin() == "http://example.com");
	CHECK(url.port() == "80");
	CHECK(url.pathname().empty());
	CHECK(url.search().empty());
	CHECK(url.hash().empty());
}

TEST_CASE("http:// - port followed directly by query string is valid", "[http][regression]") {
	auto url = URL("http://example.com:8080?q=1");
}

TEST_CASE("http:// - port followed directly by bare fragment is valid", "[http][regression]") {
	auto url = URL("http://example.com:8080#");
	CHECK(url.protocol() == "http");
	CHECK(url.host() == "example.com:8080");
	CHECK(url.hostname() == "example.com");
	CHECK(url.origin() == "http://example.com:8080");
	CHECK(url.port() == "8080");
	CHECK(url.pathname().empty());
	CHECK(url.search().empty());
	CHECK(url.hash().empty());
}

TEST_CASE("http:// - bare fragment '#' with no fragment text is valid", "[http][regression]") {
	auto url = URL("http://example.com#");
	CHECK(url.protocol() == "http");
	CHECK(url.host() == "example.com");
	CHECK(url.hostname() == "example.com");
	CHECK(url.origin() == "http://example.com");
	CHECK(url.port().empty());
	CHECK(url.pathname().empty());
	CHECK(url.search().empty());
	CHECK(url.hash().empty());
}

TEST_CASE("http:// - fragment '#' with section is valid", "[http][regression]") {
	auto url = URL("http://example.com#section");
	CHECK(url.protocol() == "http");
	CHECK(url.host() == "example.com");
	CHECK(url.hostname() == "example.com");
	CHECK(url.origin() == "http://example.com");
	CHECK(url.port().empty());
	CHECK(url.pathname().empty());
	CHECK(url.search().empty());
	CHECK(url.hash() == "section");
}

TEST_CASE("http:// - host with port and fragment '#' with section is valid", "[http][regression]") {
	auto url = URL("http://example.com:8090#section");
	CHECK(url.protocol() == "http");
	CHECK(url.host() == "example.com:8090");
	CHECK(url.hostname() == "example.com");
	CHECK(url.origin() == "http://example.com:8090");
	CHECK(url.port() == "8090");
	CHECK(url.pathname().empty());
	CHECK(url.search().empty());
	CHECK(url.hash() == "section");
}