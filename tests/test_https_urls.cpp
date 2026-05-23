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
// Valid https:// URLs
// -----------------------------------------------------------------------------

TEST_CASE("https:// - host only", "[https][valid]") {
	auto result = URL::can_parse("https://example.com");
	CHECK_FALSE(result.has_error());
}

TEST_CASE("https:// - host with path", "[https][valid]") {
	auto result = URL::can_parse("https://example.com/path");
	CHECK_FALSE(result.has_error());
}

TEST_CASE("https:// - host with path and query string", "[https][valid]") {
	auto result = URL::can_parse("https://example.com/path?q=hello");
	CHECK_FALSE(result.has_error());
}

TEST_CASE("https:// - host with path, query string, and fragment", "[https][valid]") {
	auto result = URL::can_parse("https://example.com/path?q=hello#section");
	CHECK_FALSE(result.has_error());
}

TEST_CASE("https:// - host with explicit port", "[https][valid]") {
	auto result = URL::can_parse("https://example.com:8443/path");
	CHECK_FALSE(result.has_error());
}

TEST_CASE("https:// - fragment only (no path or query)", "[https][valid]") {
	auto result = URL::can_parse("https://example.com#section");
	CHECK_FALSE(result.has_error());
}

// -----------------------------------------------------------------------------
// Invalid https:// URLs
// -----------------------------------------------------------------------------

TEST_CASE("https:// - no host (bare https://)", "[https][invalid]") {
	auto result = URL::can_parse("https://");
	CHECK(result.has_error());
}

TEST_CASE("https:// - space in URL", "[https][invalid]") {
	auto result = URL::can_parse("https://example.com/path with spaces");
	CHECK(result.has_error());
	CHECK(has_error_key(result, "invalid_character"));
}
