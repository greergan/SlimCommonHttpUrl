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
// Empty input
// -----------------------------------------------------------------------------

TEST_CASE("empty string", "[invalid][scheme]") {
	auto result = URL::can_parse("");
	CHECK(result.has_error());
	CHECK(has_error_key(result, "url"));
}

// -----------------------------------------------------------------------------
// Malformed scheme
// -----------------------------------------------------------------------------

TEST_CASE("scheme only - no delimiter", "[invalid][scheme]") {
	// 'a' — no ':' found
	auto result = URL::can_parse("a");
	CHECK(result.has_error());
	CHECK(has_error_key(result, "scheme"));
}

TEST_CASE("scheme with colon - no slashes", "[invalid][scheme]") {
	// 'a:' — ':' present but not followed by '//'
	auto result = URL::can_parse("a:");
	CHECK(result.has_error());
	CHECK(has_error_key(result, "scheme"));
}

TEST_CASE("scheme with colon and one slash", "[invalid][scheme]") {
	// 'a:/' — ':' present but only one slash follows
	auto result = URL::can_parse("a:/");
	CHECK(result.has_error());
	CHECK(has_error_key(result, "scheme"));
}

TEST_CASE("scheme with '://' but no host or path", "[invalid][scheme]") {
	// 'a://' — delimiter present but scheme 'a' is not in the supported set
	auto result = URL::can_parse("a://");
	CHECK(result.has_error());
	CHECK(has_error_key(result, "scheme"));
}

TEST_CASE("scheme must begin with alpha character", "[invalid][scheme]") {
	auto result = URL::can_parse("1http://example.com");
	CHECK(result.has_error());
	CHECK(has_error_key(result, "scheme"));
}

// -----------------------------------------------------------------------------
// Unsupported scheme
// -----------------------------------------------------------------------------

TEST_CASE("unsupported scheme - ftp", "[invalid][scheme]") {
	auto result = URL::can_parse("ftp://example.com/file.txt");
	CHECK(result.has_error());
	CHECK(has_error_key(result, "scheme"));
}

// -----------------------------------------------------------------------------
// Control characters
// -----------------------------------------------------------------------------

TEST_CASE("control character in URL", "[invalid][control_chars]") {
	auto result = URL::can_parse("https://example.com/\x01path");
	CHECK(result.has_error());
	CHECK(has_error_key(result, "invalid_character"));
}

TEST_CASE("newline in URL", "[invalid][control_chars]") {
	auto result = URL::can_parse("https://example.com/path\nmore");
	CHECK(result.has_error());
	CHECK(has_error_key(result, "invalid_character"));
}

TEST_CASE("tab in URL", "[invalid][control_chars]") {
	auto result = URL::can_parse("https://example.com/path\tmore");
	CHECK(result.has_error());
	CHECK(has_error_key(result, "invalid_character"));
}
