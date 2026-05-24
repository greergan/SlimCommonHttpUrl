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
// Missing or invalid path
// -----------------------------------------------------------------------------

TEST_CASE("file:// - no path", "[file][path]") {
	auto result = URL::can_parse("file://");
	CHECK(result.has_error());
	CHECK(has_error_key(result, "path"));
}

TEST_CASE("file:// - path is only root slash", "[file][path]") {
	auto result = URL::can_parse("file:///");
	CHECK(result.has_error());
	CHECK(has_error_key(result, "path"));
}

TEST_CASE("file:// - path ends with slash (directory, no filename)", "[file][path]") {
	auto result = URL::can_parse("file:///foo/");
	CHECK(result.has_error());
	CHECK(has_error_key(result, "path"));
}

// -----------------------------------------------------------------------------
// '?' is a valid Linux filename character — absorbed into path, never search
// -----------------------------------------------------------------------------

TEST_CASE("file:// - query string on root path", "[file][search]") {
	// path is '/' so both a path error and a search error are raised
	auto result = URL::can_parse("file:///?");
	CHECK_FALSE(result.has_error());
}

TEST_CASE("file:// - query string on valid path", "[file][search]") {
	auto result = URL::can_parse("file:///foo/bar?query=1");
	CHECK_FALSE(result.has_error());
}

// -----------------------------------------------------------------------------
// '#' is a valid Linux filename character — absorbed into path, never fragment
// -----------------------------------------------------------------------------

TEST_CASE("file:// - hash as sole filename character", "[file][hash]") {
	auto result = URL::can_parse("file:///#");
	CHECK_FALSE(result.has_error());
}

TEST_CASE("file:// - hash in filename", "[file][hash]") {
	auto result = URL::can_parse("file:///foo/bar#baz");
	CHECK_FALSE(result.has_error());
}

TEST_CASE("file:// - hash in directory segment", "[file][hash]") {
	auto result = URL::can_parse("file:///foo#dir/bar");
	CHECK_FALSE(result.has_error());
}

// -----------------------------------------------------------------------------
// Valid paths
// -----------------------------------------------------------------------------

TEST_CASE("file:// - simple two-segment path", "[file][valid]") {
	auto result = URL::can_parse("file:///foo/bar");
	CHECK_FALSE(result.has_error());
}

TEST_CASE("file:// - filename with extension", "[file][valid]") {
	auto result = URL::can_parse("file:///foo/bar.txt");
	CHECK_FALSE(result.has_error());
}

TEST_CASE("file:// - filename with hyphens and underscores", "[file][valid]") {
	auto result = URL::can_parse("file:///foo/bar-baz_qux.txt");
	CHECK_FALSE(result.has_error());
}

TEST_CASE("file:// - percent-encoded space in path", "[file][valid]") {
	auto result = URL::can_parse("file:///foo/bar%20baz");
	CHECK_FALSE(result.has_error());
}

TEST_CASE("file:// - parentheses and brackets in filename", "[file][valid]") {
	auto result = URL::can_parse("file:///foo/my(file)[1].txt");
	CHECK_FALSE(result.has_error());
}

TEST_CASE("file:// - wildcard in filename", "[file][glob]") {
	auto result = URL::can_parse("file:///*");
	CHECK_FALSE(result.has_error());
}

TEST_CASE("file:// - wildcard in directory segment", "[file][glob]") {
	auto result = URL::can_parse("file:///foo*/bar");
	CHECK_FALSE(result.has_error());
}

TEST_CASE("file:// - angle brackets in path", "[file][ltgt]") {
	auto result = URL::can_parse("file:///foo/<bar>");
	CHECK_FALSE(result.has_error());
}

TEST_CASE("file:// - literal space in path ", "[file][space]") {
	auto result = URL::can_parse("file:///foo/bar baz");
	CHECK_FALSE(result.has_error());
}
