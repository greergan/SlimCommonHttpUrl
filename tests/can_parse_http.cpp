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
 
	slim::slim_coordinates get_coordinates(slim::SlimValue& result, const std::string& key) {
		CHECK(result.has_map("hints"));
		if(!result.has_map("hints")) return {-1,-1};
		auto hints_map = result.get_map("hints");
		CHECK(hints_map.has(key));
		if(hints_map.has(key)) {
			auto value_maybe = hints_map.get(key).try_coordinates();
			CHECK(value_maybe.has_value());
			if(value_maybe.has_value()) {
				return value_maybe.value();
			}
		}
		return {-1,-1};
	}
}

// -----------------------------------------------------------------------------
// Valid http:// URLs
// -----------------------------------------------------------------------------

TEST_CASE("http:// - host only", "[http][valid]") {
	auto result = URL::can_parse("http://example.com");
	CHECK_FALSE(result.has_error());
	auto scheme_coords = get_coordinates(result, "scheme");
	CHECK(scheme_coords.first == 0);
	CHECK(scheme_coords.second == 3);
 	auto host_coords = get_coordinates(result, "host");
	CHECK(host_coords.first == 7);
	CHECK(host_coords.second == 17);
 	auto port_coords = get_coordinates(result, "port");
	CHECK(port_coords.first == -1);
	CHECK(port_coords.second == -1);
 	auto path_coords = get_coordinates(result, "path");
	CHECK(path_coords.first == -1);
	CHECK(path_coords.second == -1);
 	auto search_coords = get_coordinates(result, "search");
	CHECK(search_coords.first == -1);
	CHECK(search_coords.second == -1);
 	auto fragment_coords = get_coordinates(result, "fragment");
	CHECK(fragment_coords.first == -1);
	CHECK(fragment_coords.second == -1);
}

TEST_CASE("http:// - host with path", "[http][valid]") {
	auto result = URL::can_parse("http://example.com/path");
	CHECK_FALSE(result.has_error());
	auto scheme_coords = get_coordinates(result, "scheme");
	CHECK(scheme_coords.first == 0);
	CHECK(scheme_coords.second == 3);
 	auto host_coords = get_coordinates(result, "host");
	CHECK(host_coords.first == 7);
	CHECK(host_coords.second == 17);
 	auto port_coords = get_coordinates(result, "port");
	CHECK(port_coords.first == -1);
	CHECK(port_coords.second == -1);
 	auto path_coords = get_coordinates(result, "path");
	CHECK(path_coords.first == 18);
	CHECK(path_coords.second == 22);
 	auto search_coords = get_coordinates(result, "search");
	CHECK(search_coords.first == -1);
	CHECK(search_coords.second == -1);
 	auto fragment_coords = get_coordinates(result, "fragment");
	CHECK(fragment_coords.first == -1);
	CHECK(fragment_coords.second == -1);
}

TEST_CASE("http:// - host with path and query string", "[http][valid]") {
	auto result = URL::can_parse("http://example.com/path?q=hello");
	CHECK_FALSE(result.has_error());
	auto scheme_coords = get_coordinates(result, "scheme");
	CHECK(scheme_coords.first == 0);
	CHECK(scheme_coords.second == 3);
 	auto host_coords = get_coordinates(result, "host");
	CHECK(host_coords.first == 7);
	CHECK(host_coords.second == 17);
 	auto port_coords = get_coordinates(result, "port");
	CHECK(port_coords.first == -1);
	CHECK(port_coords.second == -1);
 	auto path_coords = get_coordinates(result, "path");
	CHECK(path_coords.first == 18);
	CHECK(path_coords.second == 22);
 	auto search_coords = get_coordinates(result, "search");
	CHECK(search_coords.first == 24);
	CHECK(search_coords.second == 30);
 	auto fragment_coords = get_coordinates(result, "fragment");
	CHECK(fragment_coords.first == -1);
	CHECK(fragment_coords.second == -1);
}

TEST_CASE("http:// - host with path, query string, and fragment", "[http][valid]") {
	auto result = URL::can_parse("http://example.com/path?q=hello#section");
	CHECK_FALSE(result.has_error());
	auto scheme_coords = get_coordinates(result, "scheme");
	CHECK(scheme_coords.first == 0);
	CHECK(scheme_coords.second == 3);
 	auto host_coords = get_coordinates(result, "host");
	CHECK(host_coords.first == 7);
	CHECK(host_coords.second == 17);
 	auto port_coords = get_coordinates(result, "port");
	CHECK(port_coords.first == -1);
	CHECK(port_coords.second == -1);
 	auto path_coords = get_coordinates(result, "path");
	CHECK(path_coords.first == 18);
	CHECK(path_coords.second == 22);
 	auto search_coords = get_coordinates(result, "search");
	CHECK(search_coords.first == 24);
	CHECK(search_coords.second == 30);
 	auto fragment_coords = get_coordinates(result, "fragment");
	CHECK(fragment_coords.first == 32);
	CHECK(fragment_coords.second == 38);
}

TEST_CASE("http:// - host with explicit port and path", "[http][valid]") {
	auto result = URL::can_parse("http://example.com:8080/path");
	CHECK_FALSE(result.has_error());
	auto scheme_coords = get_coordinates(result, "scheme");
	CHECK(scheme_coords.first == 0);
	CHECK(scheme_coords.second == 3);
 	auto host_coords = get_coordinates(result, "host");
	CHECK(host_coords.first == 7);
	CHECK(host_coords.second == 17);
 	auto port_coords = get_coordinates(result, "port");
	CHECK(port_coords.first == 19);
	CHECK(port_coords.second == 22);
 	auto path_coords = get_coordinates(result, "path");
	CHECK(path_coords.first == 23);
	CHECK(path_coords.second == 27);
 	auto search_coords = get_coordinates(result, "search");
	CHECK(search_coords.first == -1);
	CHECK(search_coords.second == -1);
 	auto fragment_coords = get_coordinates(result, "fragment");
	CHECK(fragment_coords.first == -1);
	CHECK(fragment_coords.second == -1);
}

TEST_CASE("http:// - host with explicit port, path and search", "[http][valid]") {
	auto result = URL::can_parse("http://example.com:8080/path?q=hello");
	CHECK_FALSE(result.has_error());
	auto scheme_coords = get_coordinates(result, "scheme");
	CHECK(scheme_coords.first == 0);
	CHECK(scheme_coords.second == 3);
 	auto host_coords = get_coordinates(result, "host");
	CHECK(host_coords.first == 7);
	CHECK(host_coords.second == 17);
 	auto port_coords = get_coordinates(result, "port");
	CHECK(port_coords.first == 19);
	CHECK(port_coords.second == 22);
 	auto path_coords = get_coordinates(result, "path");
	CHECK(path_coords.first == 23);
	CHECK(path_coords.second == 27);
 	auto search_coords = get_coordinates(result, "search");
	CHECK(search_coords.first == 29);
	CHECK(search_coords.second == 35);
 	auto fragment_coords = get_coordinates(result, "fragment");
	CHECK(fragment_coords.first == -1);
	CHECK(fragment_coords.second == -1);
}

TEST_CASE("http:// - host with explicit port, path, search and fragment", "[http][valid]") {
	auto result = URL::can_parse("http://example.com:8080/path?q=hello#section");
	CHECK_FALSE(result.has_error());
	auto scheme_coords = get_coordinates(result, "scheme");
	CHECK(scheme_coords.first == 0);
	CHECK(scheme_coords.second == 3);
 	auto host_coords = get_coordinates(result, "host");
	CHECK(host_coords.first == 7);
	CHECK(host_coords.second == 17);
 	auto port_coords = get_coordinates(result, "port");
	CHECK(port_coords.first == 19);
	CHECK(port_coords.second == 22);
 	auto path_coords = get_coordinates(result, "path");
	CHECK(path_coords.first == 23);
	CHECK(path_coords.second == 27);
 	auto search_coords = get_coordinates(result, "search");
	CHECK(search_coords.first == 29);
	CHECK(search_coords.second == 35);
 	auto fragment_coords = get_coordinates(result, "fragment");
	CHECK(fragment_coords.first == 37);
	CHECK(fragment_coords.second == 43);
}

// -----------------------------------------------------------------------------
// Invalid http:// URLs
// -----------------------------------------------------------------------------

TEST_CASE("http:// - no host (bare http://)", "[http][invalid]") {
	auto result = URL::can_parse("http://");
	CHECK(result.has_error());
}

TEST_CASE("http - scheme starting with digit is rejected", "[http][scheme][regression]") {
	auto result = URL::can_parse("1http://example.com");
	CHECK(result.has_error());
	CHECK(has_error_key(result, "scheme"));
}

TEST_CASE("http:// - space in URL", "[http][invalid]") {
	auto result = URL::can_parse("http://example.com/path with spaces");
	CHECK(result.has_error());
	CHECK(has_error_key(result, "body"));
}

TEST_CASE("http - empty scheme is an error", "[http][scheme]") {
	auto result = URL::can_parse("://example.com");
	CHECK(result.has_error());
	CHECK(has_error_key(result, "scheme"));
}

TEST_CASE("http:// - only first error field is recorded when host is invalid", "[http][errors]") {
	auto result = URL::can_parse("http://:abc/path");
	CHECK(result.has_error());
	CHECK(has_error_key(result, "host"));
	// Port was never reached, so "port" error key should NOT exist.
	CHECK_FALSE(has_error_key(result, "port"));
}

TEST_CASE("http:// - root path slash is valid and path coord is set", "[http][coords][regression]") {
	auto result = URL::can_parse("http://example.com/");
	CHECK_FALSE(result.has_error());
	auto scheme_coords = get_coordinates(result, "scheme");
	CHECK(scheme_coords.first == 0);
	CHECK(scheme_coords.second == 3);
 	auto host_coords = get_coordinates(result, "host");
	CHECK(host_coords.first == 7);
	CHECK(host_coords.second == 17);
 	auto path_coords = get_coordinates(result, "path");
	CHECK(path_coords.first == 18);
	CHECK(path_coords.second == 18);
}

TEST_CASE("http:// - host with port and no path is valid", "[http][coords][regression]") {
	auto result = URL::can_parse("http://example.com:8080");
	CHECK_FALSE(result.has_error());
	auto scheme_coords = get_coordinates(result, "scheme");
	CHECK(scheme_coords.first == 0);
	CHECK(scheme_coords.second == 3);
 	auto host_coords = get_coordinates(result, "host");
	CHECK(host_coords.first == 7);
	CHECK(host_coords.second == 17);
 	auto port_coords = get_coordinates(result, "port");
	CHECK(port_coords.first == 19);
	CHECK(port_coords.second == 22);
 	auto path_coords = get_coordinates(result, "path");
	CHECK(path_coords.first == -1);
	CHECK(path_coords.second == -1);
}

TEST_CASE("http:// - port followed directly by query string is valid", "[http][coords][regression]") {
	auto result = URL::can_parse("http://example.com:8080?q=1");
	CHECK_FALSE(result.has_error());
	auto scheme_coords = get_coordinates(result, "scheme");
	CHECK(scheme_coords.first == 0);
	CHECK(scheme_coords.second == 3);
 	auto host_coords = get_coordinates(result, "host");
	CHECK(host_coords.first == 7);
	CHECK(host_coords.second == 17);
 	auto port_coords = get_coordinates(result, "port");
	CHECK(port_coords.first == 19);
	CHECK(port_coords.second == 22);
 	auto path_coords = get_coordinates(result, "path");
	CHECK(path_coords.first == -1);
	CHECK(path_coords.second == -1);
 	auto search_coords = get_coordinates(result, "search");
	CHECK(search_coords.first == 24);
	CHECK(search_coords.second == 26);
}

TEST_CASE("http:// - port followed directly by fragment is valid", "[http][coords][regression]") {
	auto result = URL::can_parse("http://example.com:8080#section");
	CHECK_FALSE(result.has_error());
	auto scheme_coords = get_coordinates(result, "scheme");
	CHECK(scheme_coords.first == 0);
	CHECK(scheme_coords.second == 3);
 	auto host_coords = get_coordinates(result, "host");
	CHECK(host_coords.first == 7);
	CHECK(host_coords.second == 17);
 	auto port_coords = get_coordinates(result, "port");
	CHECK(port_coords.first == 19);
	CHECK(port_coords.second == 22);
 	auto path_coords = get_coordinates(result, "path");
	CHECK(path_coords.first == -1);
	CHECK(path_coords.second == -1);
 	auto search_coords = get_coordinates(result, "search");
	CHECK(search_coords.first == -1);
	CHECK(search_coords.second == -1);
 	auto fragment_coords = get_coordinates(result, "fragment");
	CHECK(fragment_coords.first == 24);
	CHECK(fragment_coords.second == 30);
}

TEST_CASE("http:// - port followed directly by bare fragment is valid", "[http][coords][regression]") {
	auto result = URL::can_parse("http://example.com:8080#");
	CHECK_FALSE(result.has_error());
	auto scheme_coords = get_coordinates(result, "scheme");
	CHECK(scheme_coords.first == 0);
	CHECK(scheme_coords.second == 3);
 	auto host_coords = get_coordinates(result, "host");
	CHECK(host_coords.first == 7);
	CHECK(host_coords.second == 17);
 	auto port_coords = get_coordinates(result, "port");
	CHECK(port_coords.first == 19);
	CHECK(port_coords.second == 22);
 	auto path_coords = get_coordinates(result, "path");
	CHECK(path_coords.first == -1);
	CHECK(path_coords.second == -1);
 	auto search_coords = get_coordinates(result, "search");
	CHECK(search_coords.first == -1);
	CHECK(search_coords.second == -1);
 	auto fragment_coords = get_coordinates(result, "fragment");
	CHECK(fragment_coords.first == -1);
	CHECK(fragment_coords.second == -1);
}

TEST_CASE("http:// - bare fragment '#' with no fragment text is valid", "[http][coords][regression]") {
	auto result = URL::can_parse("http://example.com#");
	CHECK_FALSE(result.has_error());
	auto scheme_coords = get_coordinates(result, "scheme");
	CHECK(scheme_coords.first == 0);
	CHECK(scheme_coords.second == 3);
 	auto host_coords = get_coordinates(result, "host");
	CHECK(host_coords.first == 7);
	CHECK(host_coords.second == 17);
 	auto port_coords = get_coordinates(result, "port");
	CHECK(port_coords.first == -1);
	CHECK(port_coords.second == -1);
 	auto path_coords = get_coordinates(result, "path");
	CHECK(path_coords.first == -1);
	CHECK(path_coords.second == -1);
 	auto search_coords = get_coordinates(result, "search");
	CHECK(search_coords.first == -1);
	CHECK(search_coords.second == -1);
 	auto fragment_coords = get_coordinates(result, "fragment");
	CHECK(fragment_coords.first == -1);
	CHECK(fragment_coords.second == -1);
}

TEST_CASE("http:// - fragment '#' with section is valid", "[http][coords][regression]") {
	auto result = URL::can_parse("http://example.com#section");
	CHECK_FALSE(result.has_error());
	auto scheme_coords = get_coordinates(result, "scheme");
	CHECK(scheme_coords.first == 0);
	CHECK(scheme_coords.second == 3);
 	auto host_coords = get_coordinates(result, "host");
	CHECK(host_coords.first == 7);
	CHECK(host_coords.second == 17);
 	auto port_coords = get_coordinates(result, "port");
	CHECK(port_coords.first == -1);
	CHECK(port_coords.second == -1);
 	auto path_coords = get_coordinates(result, "path");
	CHECK(path_coords.first == -1);
	CHECK(path_coords.second == -1);
 	auto search_coords = get_coordinates(result, "search");
	CHECK(search_coords.first == -1);
	CHECK(search_coords.second == -1);
 	auto fragment_coords = get_coordinates(result, "fragment");
	CHECK(fragment_coords.first == 19);
	CHECK(fragment_coords.second == 25);
}