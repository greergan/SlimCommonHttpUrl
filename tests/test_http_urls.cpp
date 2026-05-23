#include <catch2/catch_test_macros.hpp>
#include <slim/common/http/url.h>
#include <slim/SlimValue.hpp>

using slim::common::http::URL;

namespace {
	bool has_error_key(slim::SlimValue& result, const std::string& key) {
		if(!result.has_map("errors")) return false;
		for(auto& [k, v] : result.get_map("errors").get())
			if(k == key) return true;
		return false;
	}
}

// -----------------------------------------------------------------------------
// Valid http:// URLs
// -----------------------------------------------------------------------------

TEST_CASE("http:// - host only", "[http][valid]") {
	auto result = URL::can_parse("http://example.com");
	CHECK_FALSE(result.has_error());
}

TEST_CASE("http:// - host with path", "[http][valid]") {
	auto result = URL::can_parse("http://example.com/path");
	CHECK_FALSE(result.has_error());
}

TEST_CASE("http:// - host with path and query string", "[http][valid]") {
	auto result = URL::can_parse("http://example.com/path?q=hello");
	CHECK_FALSE(result.has_error());
}

TEST_CASE("http:// - host with path, query string, and fragment", "[http][valid]") {
	auto result = URL::can_parse("http://example.com/path?q=hello#section");
	CHECK_FALSE(result.has_error());
}

TEST_CASE("http:// - host with explicit port", "[http][valid]") {
	auto result = URL::can_parse("http://example.com:8080/path");
	CHECK_FALSE(result.has_error());
}

TEST_CASE("http:// - fragment only (no path or query)", "[http][valid]") {
	auto result = URL::can_parse("http://example.com#section");
	CHECK_FALSE(result.has_error());
}

// -----------------------------------------------------------------------------
// Invalid http:// URLs
// -----------------------------------------------------------------------------

TEST_CASE("http:// - no host (bare http://)", "[http][invalid]") {
	// authority is empty; whether this is an error depends on host validation
	// — recorded here so expected behaviour is explicit
	auto result = URL::can_parse("http://");
	CHECK(result.has_error());
}

TEST_CASE("http:// - space in URL", "[http][invalid]") {
	auto result = URL::can_parse("http://example.com/path with spaces");
	CHECK(result.has_error());
	CHECK(has_error_key(result, "invalid_character"));
}
