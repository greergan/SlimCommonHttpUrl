#include <catch2/catch_test_macros.hpp>
#include <slim/common/http/url.h>
#include <slim/SlimValue.hpp>

using slim::common::http::URL;

// =============================================================================
// Shared helpers
// =============================================================================

namespace {
	bool has_error_key(slim::SlimValue& result, const std::string& key) {
		if(!result.has_map("errors")) return false;
		for(auto& [k, v] : result.get_map("errors").get())
			if(k == key) return true;
		return false;
	}

	slim::slim_coordinates get_coordinates(slim::SlimValue& result, const std::string& key) {
		CHECK(result.has_map("hints"));
		if(!result.has_map("hints")) return {-1, -1};
		auto hints_map = result.get_map("hints");
		CHECK(hints_map.has(key));
		if(hints_map.has(key)) {
			auto value_maybe = hints_map.get(key).try_coordinates();
			CHECK(value_maybe.has_value());
			if(value_maybe.has_value())
				return value_maybe.value();
		}
		return {-1, -1};
	}
}

// =============================================================================
// file:// — can_parse
// =============================================================================

TEST_CASE("file:// - can_parse", "[file][can_parse]") {

	SECTION("missing or invalid path") {
		SECTION("no path") {
			auto result = URL::can_parse("file://");
			CHECK(result.has_error());
			CHECK(has_error_key(result, "path"));
		}
		SECTION("path is only root slash") {
			auto result = URL::can_parse("file:///");
			CHECK(result.has_error());
			CHECK(has_error_key(result, "path"));
		}
		SECTION("path ends with slash (directory, no filename)") {
			auto result = URL::can_parse("file:///foo/");
			CHECK(result.has_error());
			CHECK(has_error_key(result, "path"));
		}
	}

	SECTION("'?' absorbed into path, never treated as search") {
		SECTION("query string on root path")  { CHECK_FALSE(URL::can_parse("file:///?").has_error()); }
		SECTION("query string on valid path") { CHECK_FALSE(URL::can_parse("file:///foo/bar?query=1").has_error()); }
	}

	SECTION("'#' absorbed into path, never treated as fragment") {
		SECTION("hash as sole filename character") { CHECK_FALSE(URL::can_parse("file:///#").has_error()); }
		SECTION("hash in filename")               { CHECK_FALSE(URL::can_parse("file:///foo/bar#baz").has_error()); }
		SECTION("hash in directory segment")      { CHECK_FALSE(URL::can_parse("file:///foo#dir/bar").has_error()); }
	}

	SECTION("valid paths") {
		SECTION("simple two-segment path")       { CHECK_FALSE(URL::can_parse("file:///foo/bar").has_error()); }
		SECTION("filename with extension")       { CHECK_FALSE(URL::can_parse("file:///foo/bar.txt").has_error()); }
		SECTION("hyphens and underscores")       { CHECK_FALSE(URL::can_parse("file:///foo/bar-baz_qux.txt").has_error()); }
		SECTION("percent-encoded space")         { CHECK_FALSE(URL::can_parse("file:///foo/bar%20baz").has_error()); }
		SECTION("parentheses and brackets")      { CHECK_FALSE(URL::can_parse("file:///foo/my(file)[1].txt").has_error()); }
		SECTION("wildcard in filename")          { CHECK_FALSE(URL::can_parse("file:///*").has_error()); }
		SECTION("wildcard in directory segment") { CHECK_FALSE(URL::can_parse("file:///foo*/bar").has_error()); }
		SECTION("angle brackets")                { CHECK_FALSE(URL::can_parse("file:///foo/<bar>").has_error()); }
		SECTION("literal space")                 { CHECK_FALSE(URL::can_parse("file:///foo/bar baz").has_error()); }
	}

	SECTION("coordinates") {
		SECTION("'?' absorbed — path coords span full remainder") {
			auto result = URL::can_parse("file:///foo/bar?query=1");
			CHECK_FALSE(result.has_error());
			auto coords = get_coordinates(result, "path");
			CHECK(coords.first  == 7);
			CHECK(coords.second == 22);
		}
	}
}

// =============================================================================
// http:// — can_parse
// =============================================================================

TEST_CASE("http:// - can_parse", "[http][can_parse]") {

	SECTION("valid URLs") {

		SECTION("host only") {
			auto result = URL::can_parse("http://example.com");
			CHECK_FALSE(result.has_error());
			auto scheme   = get_coordinates(result, "scheme");   CHECK(scheme.first   == 0);  CHECK(scheme.second   == 3);
			auto host     = get_coordinates(result, "host");     CHECK(host.first     == 7);  CHECK(host.second     == 17);
			auto port     = get_coordinates(result, "port");     CHECK(port.first     == -1); CHECK(port.second     == -1);
			auto path     = get_coordinates(result, "path");     CHECK(path.first     == -1); CHECK(path.second     == -1);
			auto search   = get_coordinates(result, "search");   CHECK(search.first   == -1); CHECK(search.second   == -1);
			auto fragment = get_coordinates(result, "fragment"); CHECK(fragment.first == -1); CHECK(fragment.second == -1);
		}

		SECTION("host with / as path") {
			auto result = URL::can_parse("http://example.com/");
			CHECK_FALSE(result.has_error());
			auto scheme   = get_coordinates(result, "scheme");   CHECK(scheme.first   == 0);  CHECK(scheme.second   == 3);
			auto host     = get_coordinates(result, "host");     CHECK(host.first     == 7);  CHECK(host.second     == 17);
			auto port     = get_coordinates(result, "port");     CHECK(port.first     == -1); CHECK(port.second     == -1);
			auto path     = get_coordinates(result, "path");     CHECK(path.first     == 18); CHECK(path.second     == 18);
			auto search   = get_coordinates(result, "search");   CHECK(search.first   == -1); CHECK(search.second   == -1);
			auto fragment = get_coordinates(result, "fragment"); CHECK(fragment.first == -1); CHECK(fragment.second == -1);
		}

		SECTION("host with path") {
			auto result = URL::can_parse("http://example.com/path");
			CHECK_FALSE(result.has_error());
			auto scheme   = get_coordinates(result, "scheme");   CHECK(scheme.first   == 0);  CHECK(scheme.second   == 3);
			auto host     = get_coordinates(result, "host");     CHECK(host.first     == 7);  CHECK(host.second     == 17);
			auto port     = get_coordinates(result, "port");     CHECK(port.first     == -1); CHECK(port.second     == -1);
			auto path     = get_coordinates(result, "path");     CHECK(path.first     == 18); CHECK(path.second     == 22);
			auto search   = get_coordinates(result, "search");   CHECK(search.first   == -1); CHECK(search.second   == -1);
			auto fragment = get_coordinates(result, "fragment"); CHECK(fragment.first == -1); CHECK(fragment.second == -1);
		}

		SECTION("host with path and query string") {
			auto result = URL::can_parse("http://example.com/path?q=hello");
			CHECK_FALSE(result.has_error());
			auto scheme   = get_coordinates(result, "scheme");   CHECK(scheme.first   == 0);  CHECK(scheme.second   == 3);
			auto host     = get_coordinates(result, "host");     CHECK(host.first     == 7);  CHECK(host.second     == 17);
			auto port     = get_coordinates(result, "port");     CHECK(port.first     == -1); CHECK(port.second     == -1);
			auto path     = get_coordinates(result, "path");     CHECK(path.first     == 18); CHECK(path.second     == 22);
			auto search   = get_coordinates(result, "search");   CHECK(search.first   == 24); CHECK(search.second   == 30);
			auto fragment = get_coordinates(result, "fragment"); CHECK(fragment.first == -1); CHECK(fragment.second == -1);
		}

		SECTION("host with path, query string, and fragment") {
			auto result = URL::can_parse("http://example.com/path?q=hello#section");
			CHECK_FALSE(result.has_error());
			auto scheme   = get_coordinates(result, "scheme");   CHECK(scheme.first   == 0);  CHECK(scheme.second   == 3);
			auto host     = get_coordinates(result, "host");     CHECK(host.first     == 7);  CHECK(host.second     == 17);
			auto port     = get_coordinates(result, "port");     CHECK(port.first     == -1); CHECK(port.second     == -1);
			auto path     = get_coordinates(result, "path");     CHECK(path.first     == 18); CHECK(path.second     == 22);
			auto search   = get_coordinates(result, "search");   CHECK(search.first   == 24); CHECK(search.second   == 30);
			auto fragment = get_coordinates(result, "fragment"); CHECK(fragment.first == 32); CHECK(fragment.second == 38);
		}

		SECTION("host with explicit port and path") {
			auto result = URL::can_parse("http://example.com:8080/path");
			CHECK_FALSE(result.has_error());
			auto scheme   = get_coordinates(result, "scheme");   CHECK(scheme.first   == 0);  CHECK(scheme.second   == 3);
			auto host     = get_coordinates(result, "host");     CHECK(host.first     == 7);  CHECK(host.second     == 17);
			auto port     = get_coordinates(result, "port");     CHECK(port.first     == 19); CHECK(port.second     == 22);
			auto path     = get_coordinates(result, "path");     CHECK(path.first     == 23); CHECK(path.second     == 27);
			auto search   = get_coordinates(result, "search");   CHECK(search.first   == -1); CHECK(search.second   == -1);
			auto fragment = get_coordinates(result, "fragment"); CHECK(fragment.first == -1); CHECK(fragment.second == -1);
		}

		SECTION("host with explicit port, path and search") {
			auto result = URL::can_parse("http://example.com:8080/path?q=hello");
			CHECK_FALSE(result.has_error());
			auto scheme   = get_coordinates(result, "scheme");   CHECK(scheme.first   == 0);  CHECK(scheme.second   == 3);
			auto host     = get_coordinates(result, "host");     CHECK(host.first     == 7);  CHECK(host.second     == 17);
			auto port     = get_coordinates(result, "port");     CHECK(port.first     == 19); CHECK(port.second     == 22);
			auto path     = get_coordinates(result, "path");     CHECK(path.first     == 23); CHECK(path.second     == 27);
			auto search   = get_coordinates(result, "search");   CHECK(search.first   == 29); CHECK(search.second   == 35);
			auto fragment = get_coordinates(result, "fragment"); CHECK(fragment.first == -1); CHECK(fragment.second == -1);
		}

		SECTION("host with explicit port, path, search and fragment") {
			auto result = URL::can_parse("http://example.com:8080/path?q=hello#section");
			CHECK_FALSE(result.has_error());
			auto scheme   = get_coordinates(result, "scheme");   CHECK(scheme.first   == 0);  CHECK(scheme.second   == 3);
			auto host     = get_coordinates(result, "host");     CHECK(host.first     == 7);  CHECK(host.second     == 17);
			auto port     = get_coordinates(result, "port");     CHECK(port.first     == 19); CHECK(port.second     == 22);
			auto path     = get_coordinates(result, "path");     CHECK(path.first     == 23); CHECK(path.second     == 27);
			auto search   = get_coordinates(result, "search");   CHECK(search.first   == 29); CHECK(search.second   == 35);
			auto fragment = get_coordinates(result, "fragment"); CHECK(fragment.first == 37); CHECK(fragment.second == 43);
		}
	}

	SECTION("invalid URLs") {
		SECTION("no host (bare http://)") {
			CHECK(URL::can_parse("http://").has_error());
		}
		SECTION("scheme starting with digit") {
			auto result = URL::can_parse("1http://example.com");
			CHECK(result.has_error());
			CHECK(has_error_key(result, "scheme"));
		}
		SECTION("space in URL") {
			auto result = URL::can_parse("http://example.com/path with spaces");
			CHECK(result.has_error());
			CHECK(has_error_key(result, "body"));
		}
		SECTION("empty scheme") {
			auto result = URL::can_parse("://example.com");
			CHECK(result.has_error());
			CHECK(has_error_key(result, "scheme"));
		}
		SECTION("only first error field recorded when host is invalid") {
			auto result = URL::can_parse("http://:abc/path");
			CHECK(result.has_error());
			CHECK(has_error_key(result, "host"));
			CHECK_FALSE(has_error_key(result, "port"));
		}
	}

	SECTION("regression") {
		SECTION("root path slash is valid") {
			auto result = URL::can_parse("http://example.com/");
			CHECK_FALSE(result.has_error());
			auto scheme = get_coordinates(result, "scheme"); CHECK(scheme.first == 0);  CHECK(scheme.second == 3);
			auto host   = get_coordinates(result, "host");   CHECK(host.first   == 7);  CHECK(host.second   == 17);
			auto path   = get_coordinates(result, "path");   CHECK(path.first   == 18); CHECK(path.second   == 18);
		}
		SECTION("host with port and no path") {
			auto result = URL::can_parse("http://example.com:8080");
			CHECK_FALSE(result.has_error());
			auto scheme = get_coordinates(result, "scheme"); CHECK(scheme.first == 0);  CHECK(scheme.second == 3);
			auto host   = get_coordinates(result, "host");   CHECK(host.first   == 7);  CHECK(host.second   == 17);
			auto port   = get_coordinates(result, "port");   CHECK(port.first   == 19); CHECK(port.second   == 22);
			auto path   = get_coordinates(result, "path");   CHECK(path.first   == -1); CHECK(path.second   == -1);
		}
		SECTION("port followed directly by query string") {
			auto result = URL::can_parse("http://example.com:8080?q=1");
			CHECK_FALSE(result.has_error());
			auto scheme = get_coordinates(result, "scheme"); CHECK(scheme.first  == 0);  CHECK(scheme.second  == 3);
			auto host   = get_coordinates(result, "host");   CHECK(host.first    == 7);  CHECK(host.second    == 17);
			auto port   = get_coordinates(result, "port");   CHECK(port.first    == 19); CHECK(port.second    == 22);
			auto path   = get_coordinates(result, "path");   CHECK(path.first    == -1); CHECK(path.second    == -1);
			auto search = get_coordinates(result, "search"); CHECK(search.first  == 24); CHECK(search.second  == 26);
		}
		SECTION("port followed directly by fragment") {
			auto result = URL::can_parse("http://example.com:8080#section");
			CHECK_FALSE(result.has_error());
			auto scheme   = get_coordinates(result, "scheme");   CHECK(scheme.first   == 0);  CHECK(scheme.second   == 3);
			auto host     = get_coordinates(result, "host");     CHECK(host.first     == 7);  CHECK(host.second     == 17);
			auto port     = get_coordinates(result, "port");     CHECK(port.first     == 19); CHECK(port.second     == 22);
			auto path     = get_coordinates(result, "path");     CHECK(path.first     == -1); CHECK(path.second     == -1);
			auto search   = get_coordinates(result, "search");   CHECK(search.first   == -1); CHECK(search.second   == -1);
			auto fragment = get_coordinates(result, "fragment"); CHECK(fragment.first == 24); CHECK(fragment.second == 30);
		}
		SECTION("port followed by bare fragment '#'") {
			auto result = URL::can_parse("http://example.com:8080#");
			CHECK_FALSE(result.has_error());
			auto scheme   = get_coordinates(result, "scheme");   CHECK(scheme.first   == 0);  CHECK(scheme.second   == 3);
			auto host     = get_coordinates(result, "host");     CHECK(host.first     == 7);  CHECK(host.second     == 17);
			auto port     = get_coordinates(result, "port");     CHECK(port.first     == 19); CHECK(port.second     == 22);
			auto path     = get_coordinates(result, "path");     CHECK(path.first     == -1); CHECK(path.second     == -1);
			auto search   = get_coordinates(result, "search");   CHECK(search.first   == -1); CHECK(search.second   == -1);
			auto fragment = get_coordinates(result, "fragment"); CHECK(fragment.first == -1); CHECK(fragment.second == -1);
		}
		SECTION("bare fragment '#' with no text") {
			auto result = URL::can_parse("http://example.com#");
			CHECK_FALSE(result.has_error());
			auto scheme   = get_coordinates(result, "scheme");   CHECK(scheme.first   == 0);  CHECK(scheme.second   == 3);
			auto host     = get_coordinates(result, "host");     CHECK(host.first     == 7);  CHECK(host.second     == 17);
			auto port     = get_coordinates(result, "port");     CHECK(port.first     == -1); CHECK(port.second     == -1);
			auto path     = get_coordinates(result, "path");     CHECK(path.first     == -1); CHECK(path.second     == -1);
			auto search   = get_coordinates(result, "search");   CHECK(search.first   == -1); CHECK(search.second   == -1);
			auto fragment = get_coordinates(result, "fragment"); CHECK(fragment.first == -1); CHECK(fragment.second == -1);
		}
		SECTION("fragment '#' with section text") {
			auto result = URL::can_parse("http://example.com#section");
			CHECK_FALSE(result.has_error());
			auto scheme   = get_coordinates(result, "scheme");   CHECK(scheme.first   == 0);  CHECK(scheme.second   == 3);
			auto host     = get_coordinates(result, "host");     CHECK(host.first     == 7);  CHECK(host.second     == 17);
			auto port     = get_coordinates(result, "port");     CHECK(port.first     == -1); CHECK(port.second     == -1);
			auto path     = get_coordinates(result, "path");     CHECK(path.first     == -1); CHECK(path.second     == -1);
			auto search   = get_coordinates(result, "search");   CHECK(search.first   == -1); CHECK(search.second   == -1);
			auto fragment = get_coordinates(result, "fragment"); CHECK(fragment.first == 19); CHECK(fragment.second == 25);
		}
	}
}

// =============================================================================
// https:// — can_parse
// =============================================================================

TEST_CASE("https:// - can_parse", "[https][can_parse]") {

	SECTION("valid URLs") {
		SECTION("host only")                           { CHECK_FALSE(URL::can_parse("https://example.com").has_error()); }
		SECTION("host with path")                      { CHECK_FALSE(URL::can_parse("https://example.com/path").has_error()); }
		SECTION("host with path and query string")     { CHECK_FALSE(URL::can_parse("https://example.com/path?q=hello").has_error()); }
		SECTION("host with path, query, and fragment") { CHECK_FALSE(URL::can_parse("https://example.com/path?q=hello#section").has_error()); }
		SECTION("host with explicit port")             { CHECK_FALSE(URL::can_parse("https://example.com:8443/path").has_error()); }
		SECTION("fragment only (no path or query)")    { CHECK_FALSE(URL::can_parse("https://example.com#section").has_error()); }
	}

	SECTION("invalid URLs") {
		SECTION("no host (bare https://)") {
			CHECK(URL::can_parse("https://").has_error());
		}
		SECTION("space in URL") {
			auto result = URL::can_parse("https://example.com/path with spaces");
			CHECK(result.has_error());
			CHECK(has_error_key(result, "body"));
		}
	}
}

// =============================================================================
// http:// — URL construction
// =============================================================================

TEST_CASE("http:// - URL construction", "[http][construct]") {

	SECTION("valid URLs") {

		SECTION("host only") {
			auto url = URL("http://example.com");
			CHECK(url.protocol() == "http");
			CHECK(url.host()     == "example.com");
			CHECK(url.hostname() == "example.com");
			CHECK(url.origin()   == "http://example.com");
			CHECK(url.port().empty());
			CHECK(url.pathname().empty());
			CHECK(url.search().empty());
			CHECK(url.hash().empty());
		}

		SECTION("host with / as path") {
			auto url = URL("http://example.com/");
			CHECK(url.protocol() == "http");
			CHECK(url.host()     == "example.com");
			CHECK(url.hostname() == "example.com");
			CHECK(url.origin()   == "http://example.com");
			CHECK(url.port().empty());
			CHECK(url.pathname() == "/");
			CHECK(url.search().empty());
			CHECK(url.hash().empty());
		}

		SECTION("host with path") {
			auto url = URL("http://example.com/path");
			CHECK(url.protocol() == "http");
			CHECK(url.host()     == "example.com");
			CHECK(url.hostname() == "example.com");
			CHECK(url.origin()   == "http://example.com");
			CHECK(url.port().empty());
			CHECK(url.pathname() == "/path");
			CHECK(url.search().empty());
			CHECK(url.hash().empty());
		}

		SECTION("host with path and query string")         { auto url = URL("http://example.com/path?q=hello"); }
		SECTION("host with path, query string, fragment")  { auto url = URL("http://example.com/path?q=hello#section"); }

		SECTION("host with explicit port and / as path") {
			auto url = URL("http://example.com:8080/");
			CHECK(url.protocol() == "http");
			CHECK(url.host()     == "example.com:8080");
			CHECK(url.hostname() == "example.com");
			CHECK(url.origin()   == "http://example.com:8080");
			CHECK(url.port()     == "8080");
			CHECK(url.pathname() == "/");
			CHECK(url.search().empty());
			CHECK(url.hash().empty());
		}

		SECTION("host with explicit port, / as path, and fragment") {
			auto url = URL("http://example.com:8080/#abc");
			CHECK(url.protocol() == "http");
			CHECK(url.host()     == "example.com:8080");
			CHECK(url.hostname() == "example.com");
			CHECK(url.origin()   == "http://example.com:8080");
			CHECK(url.port()     == "8080");
			CHECK(url.pathname() == "/");
			CHECK(url.search().empty());
			CHECK(url.hash()     == "abc");
		}

		SECTION("host with explicit port and path") {
			auto url = URL("http://example.com:8080/path");
			CHECK(url.protocol() == "http");
			CHECK(url.host()     == "example.com:8080");
			CHECK(url.hostname() == "example.com");
			CHECK(url.origin()   == "http://example.com:8080");
			CHECK(url.port()     == "8080");
			CHECK(url.pathname() == "/path");
			CHECK(url.search().empty());
			CHECK(url.hash().empty());
		}

		SECTION("host with explicit port, path and fragment") {
			auto url = URL("http://example.com:8080/path#section");
			CHECK(url.protocol() == "http");
			CHECK(url.host()     == "example.com:8080");
			CHECK(url.hostname() == "example.com");
			CHECK(url.origin()   == "http://example.com:8080");
			CHECK(url.port()     == "8080");
			CHECK(url.pathname() == "/path");
			CHECK(url.search().empty());
			CHECK(url.hash()     == "section");
		}

		SECTION("host with explicit port, path and search")            { auto url = URL("http://example.com:8080/path?q=hello"); }
		SECTION("host with explicit port, path, search and fragment")  { auto url = URL("http://example.com:8080/path?q=hello#section"); }
	}

	SECTION("invalid URLs") {
		SECTION("no host (bare http://)")      { auto url = URL("http://"); }
		SECTION("scheme starting with digit")  { auto url = URL("1http://example.com"); }
		SECTION("space in URL")                { auto url = URL("http://example.com/path with spaces"); }
		SECTION("empty scheme")                { auto url = URL("://example.com"); }
		SECTION("invalid host")                { auto url = URL("http://:abc/path"); }
	}

	SECTION("regression") {
		SECTION("root path slash is valid") {
			auto url = URL("http://example.com/");
		}

		SECTION("host with port 80 and no path") {
			auto url = URL("http://example.com:80");
			CHECK(url.protocol() == "http");
			CHECK(url.host()     == "example.com:80");
			CHECK(url.hostname() == "example.com");
			CHECK(url.origin()   == "http://example.com");
			CHECK(url.port()     == "80");
			CHECK(url.pathname().empty());
			CHECK(url.search().empty());
			CHECK(url.hash().empty());
		}

		SECTION("port followed directly by query string") {
			auto url = URL("http://example.com:8080?q=1");
		}

		SECTION("port followed directly by bare fragment") {
			auto url = URL("http://example.com:8080#");
			CHECK(url.protocol() == "http");
			CHECK(url.host()     == "example.com:8080");
			CHECK(url.hostname() == "example.com");
			CHECK(url.origin()   == "http://example.com:8080");
			CHECK(url.port()     == "8080");
			CHECK(url.pathname().empty());
			CHECK(url.search().empty());
			CHECK(url.hash().empty());
		}

		SECTION("bare fragment '#' with no text") {
			auto url = URL("http://example.com#");
			CHECK(url.protocol() == "http");
			CHECK(url.host()     == "example.com");
			CHECK(url.hostname() == "example.com");
			CHECK(url.origin()   == "http://example.com");
			CHECK(url.port().empty());
			CHECK(url.pathname().empty());
			CHECK(url.search().empty());
			CHECK(url.hash().empty());
		}

		SECTION("fragment '#' with section text") {
			auto url = URL("http://example.com#section");
			CHECK(url.protocol() == "http");
			CHECK(url.host()     == "example.com");
			CHECK(url.hostname() == "example.com");
			CHECK(url.origin()   == "http://example.com");
			CHECK(url.port().empty());
			CHECK(url.pathname().empty());
			CHECK(url.search().empty());
			CHECK(url.hash()     == "section");
		}

		SECTION("host with port and fragment with section text") {
			auto url = URL("http://example.com:8090#section");
			CHECK(url.protocol() == "http");
			CHECK(url.host()     == "example.com:8090");
			CHECK(url.hostname() == "example.com");
			CHECK(url.origin()   == "http://example.com:8090");
			CHECK(url.port()     == "8090");
			CHECK(url.pathname().empty());
			CHECK(url.search().empty());
			CHECK(url.hash()     == "section");
		}
	}
}
