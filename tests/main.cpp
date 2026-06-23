#include <catch2/catch_test_macros.hpp>
#include <slim/common/http/url.h>
#include <slim/common/http/error_codes.h>

using slim::common::http::ErrorStatus;
using slim::common::http::URL;
using slim::common::http::hints_map;
using slim::common::http::UrlParseException;

// =============================================================================
// Shared helpers
// =============================================================================

namespace {
    std::pair<int, int> get_coordinates(const hints_map& hints, const std::string& key) {
        auto it = hints.find(key);
        REQUIRE(it != hints.end());
        return it->second;
    }
}

// =============================================================================
// file:// — can_parse
// =============================================================================

TEST_CASE("file:// - can_parse", "[file][can_parse]") {

    SECTION("missing or invalid path") {
        SECTION("no path") {
            hints_map hints;
            auto e = URL::can_parse("file://", hints);
            REQUIRE(e == ErrorStatus::UrlFilePathMissing);
        }
        SECTION("path is only root slash") {
            hints_map hints;
            auto e = URL::can_parse("file:///", hints);
            REQUIRE(e == ErrorStatus::UrlFilePathTrailingSlash);
        }
        SECTION("path ends with slash (directory, no filename)") {
            hints_map hints;
            auto e = URL::can_parse("file:///foo/", hints);
            REQUIRE(e == ErrorStatus::UrlFilePathTrailingSlash);
        }
    }

    SECTION("'?' absorbed into path, never treated as search") {
        SECTION("query string on root path") {
            hints_map hints;
            REQUIRE(URL::can_parse("file:///?", hints) == ErrorStatus::OK);
        }
        SECTION("query string on valid path") {
            hints_map hints;
            REQUIRE(URL::can_parse("file:///foo/bar?query=1", hints) == ErrorStatus::OK);
        }
    }

    SECTION("'#' absorbed into path, never treated as fragment") {
        SECTION("hash as sole filename character") {
            hints_map hints;
            REQUIRE(URL::can_parse("file:///#", hints) == ErrorStatus::OK);
        }
        SECTION("hash in filename") {
            hints_map hints;
            REQUIRE(URL::can_parse("file:///foo/bar#baz", hints) == ErrorStatus::OK);
        }
        SECTION("hash in directory segment") {
            hints_map hints;
            REQUIRE(URL::can_parse("file:///foo#dir/bar", hints) == ErrorStatus::OK);
        }
    }

    SECTION("valid paths") {
        SECTION("simple two-segment path") {
            hints_map hints;
            REQUIRE(URL::can_parse("file:///foo/bar", hints) == ErrorStatus::OK);
        }
        SECTION("filename with extension") {
            hints_map hints;
            REQUIRE(URL::can_parse("file:///foo/bar.txt", hints) == ErrorStatus::OK);
        }
        SECTION("hyphens and underscores") {
            hints_map hints;
            REQUIRE(URL::can_parse("file:///foo/bar-baz_qux.txt", hints) == ErrorStatus::OK);
        }
        SECTION("percent-encoded space") {
            hints_map hints;
            REQUIRE(URL::can_parse("file:///foo/bar%20baz", hints) == ErrorStatus::OK);
        }
        SECTION("parentheses and brackets") {
            hints_map hints;
            REQUIRE(URL::can_parse("file:///foo/my(file)[1].txt", hints) == ErrorStatus::OK);
        }
        SECTION("wildcard in filename") {
            hints_map hints;
            REQUIRE(URL::can_parse("file:///*", hints) == ErrorStatus::OK);
        }
        SECTION("wildcard in directory segment") {
            hints_map hints;
            REQUIRE(URL::can_parse("file:///foo*/bar", hints) == ErrorStatus::OK);
        }
        SECTION("angle brackets") {
            hints_map hints;
            REQUIRE(URL::can_parse("file:///foo/<bar>", hints) == ErrorStatus::OK);
        }
        SECTION("literal space") {
            hints_map hints;
            REQUIRE(URL::can_parse("file:///foo/bar baz", hints) == ErrorStatus::OK);
        }
    }

    SECTION("coordinates") {
        SECTION("'?' absorbed — path coords span full remainder") {
            hints_map hints;
            auto e = URL::can_parse("file:///foo/bar?query=1", hints);
            REQUIRE(e == ErrorStatus::OK);
            auto coords = get_coordinates(hints, "path");
            REQUIRE(coords.first == 7);
            REQUIRE(coords.second == 22);
        }
    }
}

// =============================================================================
// http:// — can_parse
// =============================================================================

TEST_CASE("http:// - can_parse", "[http][can_parse]") {

    SECTION("valid URLs") {

        SECTION("host only") {
            hints_map hints;
            auto e = URL::can_parse("http://example.com", hints);
            REQUIRE(e == ErrorStatus::OK);
            auto scheme   = get_coordinates(hints, "scheme");   REQUIRE(scheme.first   == 0);  REQUIRE(scheme.second   == 3);
            auto host     = get_coordinates(hints, "host");     REQUIRE(host.first     == 7);  REQUIRE(host.second     == 17);
            auto port     = get_coordinates(hints, "port");     REQUIRE(port.first     == -1); REQUIRE(port.second     == -1);
            auto path     = get_coordinates(hints, "path");     REQUIRE(path.first     == -1); REQUIRE(path.second     == -1);
            auto search   = get_coordinates(hints, "search");   REQUIRE(search.first   == -1); REQUIRE(search.second   == -1);
            auto fragment = get_coordinates(hints, "fragment"); REQUIRE(fragment.first == -1); REQUIRE(fragment.second == -1);
        }

        SECTION("host with / as path") {
            hints_map hints;
            auto e = URL::can_parse("http://example.com/", hints);
            REQUIRE(e == ErrorStatus::OK);
            auto scheme   = get_coordinates(hints, "scheme");   REQUIRE(scheme.first   == 0);  REQUIRE(scheme.second   == 3);
            auto host     = get_coordinates(hints, "host");     REQUIRE(host.first     == 7);  REQUIRE(host.second     == 17);
            auto port     = get_coordinates(hints, "port");     REQUIRE(port.first     == -1); REQUIRE(port.second     == -1);
            auto path     = get_coordinates(hints, "path");     REQUIRE(path.first     == 18); REQUIRE(path.second     == 18);
            auto search   = get_coordinates(hints, "search");   REQUIRE(search.first   == -1); REQUIRE(search.second   == -1);
            auto fragment = get_coordinates(hints, "fragment"); REQUIRE(fragment.first == -1); REQUIRE(fragment.second == -1);
        }

        SECTION("host with path") {
            hints_map hints;
            auto e = URL::can_parse("http://example.com/path", hints);
            REQUIRE(e == ErrorStatus::OK);
            auto scheme   = get_coordinates(hints, "scheme");   REQUIRE(scheme.first   == 0);  REQUIRE(scheme.second   == 3);
            auto host     = get_coordinates(hints, "host");     REQUIRE(host.first     == 7);  REQUIRE(host.second     == 17);
            auto port     = get_coordinates(hints, "port");     REQUIRE(port.first     == -1); REQUIRE(port.second     == -1);
            auto path     = get_coordinates(hints, "path");     REQUIRE(path.first     == 18); REQUIRE(path.second     == 22);
            auto search   = get_coordinates(hints, "search");   REQUIRE(search.first   == -1); REQUIRE(search.second   == -1);
            auto fragment = get_coordinates(hints, "fragment"); REQUIRE(fragment.first == -1); REQUIRE(fragment.second == -1);
        }

        SECTION("host with path and query string") {
            hints_map hints;
            auto e = URL::can_parse("http://example.com/path?q=hello", hints);
            REQUIRE(e == ErrorStatus::OK);
            auto scheme   = get_coordinates(hints, "scheme");   REQUIRE(scheme.first   == 0);  REQUIRE(scheme.second   == 3);
            auto host     = get_coordinates(hints, "host");     REQUIRE(host.first     == 7);  REQUIRE(host.second     == 17);
            auto port     = get_coordinates(hints, "port");     REQUIRE(port.first     == -1); REQUIRE(port.second     == -1);
            auto path     = get_coordinates(hints, "path");     REQUIRE(path.first     == 18); REQUIRE(path.second     == 22);
            auto search   = get_coordinates(hints, "search");   REQUIRE(search.first   == 24); REQUIRE(search.second   == 30);
            auto fragment = get_coordinates(hints, "fragment"); REQUIRE(fragment.first == -1); REQUIRE(fragment.second == -1);
        }

        SECTION("host with path, query string, and fragment") {
            hints_map hints;
            auto e = URL::can_parse("http://example.com/path?q=hello#section", hints);
            REQUIRE(e == ErrorStatus::OK);
            auto scheme   = get_coordinates(hints, "scheme");   REQUIRE(scheme.first   == 0);  REQUIRE(scheme.second   == 3);
            auto host     = get_coordinates(hints, "host");     REQUIRE(host.first     == 7);  REQUIRE(host.second     == 17);
            auto port     = get_coordinates(hints, "port");     REQUIRE(port.first     == -1); REQUIRE(port.second     == -1);
            auto path     = get_coordinates(hints, "path");     REQUIRE(path.first     == 18); REQUIRE(path.second     == 22);
            auto search   = get_coordinates(hints, "search");   REQUIRE(search.first   == 24); REQUIRE(search.second   == 30);
            auto fragment = get_coordinates(hints, "fragment"); REQUIRE(fragment.first == 32); REQUIRE(fragment.second == 38);
        }

        SECTION("host with explicit port and path") {
            hints_map hints;
            auto e = URL::can_parse("http://example.com:8080/path", hints);
            REQUIRE(e == ErrorStatus::OK);
            auto scheme   = get_coordinates(hints, "scheme");   REQUIRE(scheme.first   == 0);  REQUIRE(scheme.second   == 3);
            auto host     = get_coordinates(hints, "host");     REQUIRE(host.first     == 7);  REQUIRE(host.second     == 17);
            auto port     = get_coordinates(hints, "port");     REQUIRE(port.first     == 19); REQUIRE(port.second     == 22);
            auto path     = get_coordinates(hints, "path");     REQUIRE(path.first     == 23); REQUIRE(path.second     == 27);
            auto search   = get_coordinates(hints, "search");   REQUIRE(search.first   == -1); REQUIRE(search.second   == -1);
            auto fragment = get_coordinates(hints, "fragment"); REQUIRE(fragment.first == -1); REQUIRE(fragment.second == -1);
        }

        SECTION("host with explicit port, path and search") {
            hints_map hints;
            auto e = URL::can_parse("http://example.com:8080/path?q=hello", hints);
            REQUIRE(e == ErrorStatus::OK);
            auto scheme   = get_coordinates(hints, "scheme");   REQUIRE(scheme.first   == 0);  REQUIRE(scheme.second   == 3);
            auto host     = get_coordinates(hints, "host");     REQUIRE(host.first     == 7);  REQUIRE(host.second     == 17);
            auto port     = get_coordinates(hints, "port");     REQUIRE(port.first     == 19); REQUIRE(port.second     == 22);
            auto path     = get_coordinates(hints, "path");     REQUIRE(path.first     == 23); REQUIRE(path.second     == 27);
            auto search   = get_coordinates(hints, "search");   REQUIRE(search.first   == 29); REQUIRE(search.second   == 35);
            auto fragment = get_coordinates(hints, "fragment"); REQUIRE(fragment.first == -1); REQUIRE(fragment.second == -1);
        }

        SECTION("host with explicit port, path, search and fragment") {
            hints_map hints;
            auto e = URL::can_parse("http://example.com:8080/path?q=hello#section", hints);
            REQUIRE(e == ErrorStatus::OK);
            auto scheme   = get_coordinates(hints, "scheme");   REQUIRE(scheme.first   == 0);  REQUIRE(scheme.second   == 3);
            auto host     = get_coordinates(hints, "host");     REQUIRE(host.first     == 7);  REQUIRE(host.second     == 17);
            auto port     = get_coordinates(hints, "port");     REQUIRE(port.first     == 19); REQUIRE(port.second     == 22);
            auto path     = get_coordinates(hints, "path");     REQUIRE(path.first     == 23); REQUIRE(path.second     == 27);
            auto search   = get_coordinates(hints, "search");   REQUIRE(search.first   == 29); REQUIRE(search.second   == 35);
            auto fragment = get_coordinates(hints, "fragment"); REQUIRE(fragment.first == 37); REQUIRE(fragment.second == 43);
        }

        SECTION("username and password") {
            SECTION("username and password with host only") {
                // http://user:pass@example.com
                // scheme: 0-3, username: 7-10, password: 12-15, host: 17-27
                hints_map hints;
                auto e = URL::can_parse("http://user:pass@example.com", hints);
                REQUIRE(e == ErrorStatus::OK);
                auto scheme   = get_coordinates(hints, "scheme");   REQUIRE(scheme.first   == 0);  REQUIRE(scheme.second   == 3);
                auto username = get_coordinates(hints, "username"); REQUIRE(username.first == 7);  REQUIRE(username.second == 10);
                auto password = get_coordinates(hints, "password"); REQUIRE(password.first == 12); REQUIRE(password.second == 15);
                auto host     = get_coordinates(hints, "host");     REQUIRE(host.first     == 17); REQUIRE(host.second     == 27);
                auto port     = get_coordinates(hints, "port");     REQUIRE(port.first     == -1); REQUIRE(port.second     == -1);
                auto path     = get_coordinates(hints, "path");     REQUIRE(path.first     == -1); REQUIRE(path.second     == -1);
            }

            SECTION("username and password with host and path") {
                // http://user:pass@example.com/path
                // scheme: 0-3, username: 7-10, password: 12-15, host: 17-27, path: 28-32
                hints_map hints;
                auto e = URL::can_parse("http://user:pass@example.com/path", hints);
                REQUIRE(e == ErrorStatus::OK);
                auto username = get_coordinates(hints, "username"); REQUIRE(username.first == 7);  REQUIRE(username.second == 10);
                auto password = get_coordinates(hints, "password"); REQUIRE(password.first == 12); REQUIRE(password.second == 15);
                auto host     = get_coordinates(hints, "host");     REQUIRE(host.first     == 17); REQUIRE(host.second     == 27);
                auto path     = get_coordinates(hints, "path");     REQUIRE(path.first     == 28); REQUIRE(path.second     == 32);
            }

            SECTION("username and password with host, port and path") {
                // http://user:pass@example.com:8080/path
                // scheme: 0-3, username: 7-10, password: 12-15, host: 17-27, port: 29-32, path: 33-37
                hints_map hints;
                auto e = URL::can_parse("http://user:pass@example.com:8080/path", hints);
                REQUIRE(e == ErrorStatus::OK);
                auto username = get_coordinates(hints, "username"); REQUIRE(username.first == 7);  REQUIRE(username.second == 10);
                auto password = get_coordinates(hints, "password"); REQUIRE(password.first == 12); REQUIRE(password.second == 15);
                auto host     = get_coordinates(hints, "host");     REQUIRE(host.first     == 17); REQUIRE(host.second     == 27);
                auto port     = get_coordinates(hints, "port");     REQUIRE(port.first     == 29); REQUIRE(port.second     == 32);
                auto path     = get_coordinates(hints, "path");     REQUIRE(path.first     == 33); REQUIRE(path.second     == 37);
            }

            SECTION("username only (no password)") {
                // http://user@example.com
                // scheme: 0-3, username: 7-10, password: -1--1, host: 12-22
                hints_map hints;
                auto e = URL::can_parse("http://user@example.com", hints);
                REQUIRE(e == ErrorStatus::OK);
                auto username = get_coordinates(hints, "username"); REQUIRE(username.first == 7);  REQUIRE(username.second == 10);
                auto password = get_coordinates(hints, "password"); REQUIRE(password.first == -1); REQUIRE(password.second == -1);
                auto host     = get_coordinates(hints, "host");     REQUIRE(host.first     == 12); REQUIRE(host.second     == 22);
            }

            SECTION("username only with path") {
                // http://user@example.com/path
                // scheme: 0-3, username: 7-10, password: -1--1, host: 12-22, path: 23-27
                hints_map hints;
                auto e = URL::can_parse("http://user@example.com/path", hints);
                REQUIRE(e == ErrorStatus::OK);
                auto username = get_coordinates(hints, "username"); REQUIRE(username.first == 7);  REQUIRE(username.second == 10);
                auto password = get_coordinates(hints, "password"); REQUIRE(password.first == -1); REQUIRE(password.second == -1);
                auto host     = get_coordinates(hints, "host");     REQUIRE(host.first     == 12); REQUIRE(host.second     == 22);
                auto path     = get_coordinates(hints, "path");     REQUIRE(path.first     == 23); REQUIRE(path.second     == 27);
            }

            SECTION("empty username with password") {
                // http://:pass@example.com
                // scheme: 0-3, username: 7-6 (empty), password: 8-11, host: 13-23
                hints_map hints;
                auto e = URL::can_parse("http://:pass@example.com", hints);
                REQUIRE(e == ErrorStatus::OK);
                auto username = get_coordinates(hints, "username"); REQUIRE(username.first == 7);  REQUIRE(username.second == 6);
                auto password = get_coordinates(hints, "password"); REQUIRE(password.first == 8);  REQUIRE(password.second == 11);
                auto host     = get_coordinates(hints, "host");     REQUIRE(host.first     == 13); REQUIRE(host.second     == 23);
            }

            SECTION("empty username and empty password") {
                // http://@example.com
                // scheme: 0-3, username: 7-6 (empty), password: -1--1, host: 8-18
                hints_map hints;
                auto e = URL::can_parse("http://@example.com", hints);
                REQUIRE(e == ErrorStatus::OK);
                auto username = get_coordinates(hints, "username"); REQUIRE(username.first == 7);  REQUIRE(username.second == 6);
                auto password = get_coordinates(hints, "password"); REQUIRE(password.first == -1); REQUIRE(password.second == -1);
                auto host     = get_coordinates(hints, "host");     REQUIRE(host.first     == 8);  REQUIRE(host.second     == 18);
            }

            SECTION("@ with no host returns UrlHostMissing") {
                hints_map hints;
                REQUIRE(URL::can_parse("http://user@", hints) == ErrorStatus::UrlHostMissing);
            }

            SECTION("@ with no host or userinfo returns UrlHostMissing") {
                hints_map hints;
                REQUIRE(URL::can_parse("http://@", hints) == ErrorStatus::UrlHostMissing);
            }
        }
    }

    SECTION("invalid URLs") {
        SECTION("no host (bare http://)") {
            hints_map hints;
            REQUIRE(URL::can_parse("http://", hints) == ErrorStatus::UrlHostMissing);
        }
        SECTION("scheme starting with digit") {
            hints_map hints;
            auto e = URL::can_parse("1http://example.com", hints);
            REQUIRE(e == ErrorStatus::UrlSchemeInvalidCharacter);
        }
        SECTION("space in URL") {
            hints_map hints;
            auto e = URL::can_parse("http://example.com/path with spaces", hints);
            REQUIRE(e == ErrorStatus::UrlBodyInvalidCharacter);
        }
        SECTION("empty scheme") {
            hints_map hints;
            auto e = URL::can_parse("://example.com", hints);
            REQUIRE(e == ErrorStatus::UrlSchemeUnsupported);
        }
        SECTION("colon directly after authority delimiter, before digits") {
            hints_map hints;
            auto e = URL::can_parse("http://:abc/path", hints);
            REQUIRE(e == ErrorStatus::UrlHostInvalidStart);
        }
    }

    SECTION("regression") {
        SECTION("root path slash is valid") {
            hints_map hints;
            auto e = URL::can_parse("http://example.com/", hints);
            REQUIRE(e == ErrorStatus::OK);
            auto scheme = get_coordinates(hints, "scheme"); REQUIRE(scheme.first == 0);  REQUIRE(scheme.second == 3);
            auto host   = get_coordinates(hints, "host");   REQUIRE(host.first   == 7);  REQUIRE(host.second   == 17);
            auto path   = get_coordinates(hints, "path");   REQUIRE(path.first   == 18); REQUIRE(path.second   == 18);
        }
        SECTION("host with port and no path") {
            hints_map hints;
            auto e = URL::can_parse("http://example.com:8080", hints);
            REQUIRE(e == ErrorStatus::OK);
            auto scheme = get_coordinates(hints, "scheme"); REQUIRE(scheme.first == 0);  REQUIRE(scheme.second == 3);
            auto host   = get_coordinates(hints, "host");   REQUIRE(host.first   == 7);  REQUIRE(host.second   == 17);
            auto port   = get_coordinates(hints, "port");   REQUIRE(port.first   == 19); REQUIRE(port.second   == 22);
            auto path   = get_coordinates(hints, "path");   REQUIRE(path.first   == -1); REQUIRE(path.second   == -1);
        }
        SECTION("port followed directly by query string") {
            hints_map hints;
            auto e = URL::can_parse("http://example.com:8080?q=1", hints);
            REQUIRE(e == ErrorStatus::OK);
            auto scheme = get_coordinates(hints, "scheme"); REQUIRE(scheme.first == 0);  REQUIRE(scheme.second == 3);
            auto host   = get_coordinates(hints, "host");   REQUIRE(host.first   == 7);  REQUIRE(host.second   == 17);
            auto port   = get_coordinates(hints, "port");   REQUIRE(port.first   == 19); REQUIRE(port.second   == 22);
            auto path   = get_coordinates(hints, "path");   REQUIRE(path.first   == -1); REQUIRE(path.second   == -1);
            auto search = get_coordinates(hints, "search"); REQUIRE(search.first == 24); REQUIRE(search.second == 26);
        }
        SECTION("port followed directly by fragment") {
            hints_map hints;
            auto e = URL::can_parse("http://example.com:8080#section", hints);
            REQUIRE(e == ErrorStatus::OK);
            auto scheme   = get_coordinates(hints, "scheme");   REQUIRE(scheme.first   == 0);  REQUIRE(scheme.second   == 3);
            auto host     = get_coordinates(hints, "host");     REQUIRE(host.first     == 7);  REQUIRE(host.second     == 17);
            auto port     = get_coordinates(hints, "port");     REQUIRE(port.first     == 19); REQUIRE(port.second     == 22);
            auto path     = get_coordinates(hints, "path");     REQUIRE(path.first     == -1); REQUIRE(path.second     == -1);
            auto search   = get_coordinates(hints, "search");   REQUIRE(search.first   == -1); REQUIRE(search.second   == -1);
            auto fragment = get_coordinates(hints, "fragment"); REQUIRE(fragment.first == 24); REQUIRE(fragment.second == 30);
        }
        SECTION("port followed by bare fragment '#'") {
            hints_map hints;
            auto e = URL::can_parse("http://example.com:8080#", hints);
            REQUIRE(e == ErrorStatus::OK);
            auto scheme   = get_coordinates(hints, "scheme");   REQUIRE(scheme.first   == 0);  REQUIRE(scheme.second   == 3);
            auto host     = get_coordinates(hints, "host");     REQUIRE(host.first     == 7);  REQUIRE(host.second     == 17);
            auto port     = get_coordinates(hints, "port");     REQUIRE(port.first     == 19); REQUIRE(port.second     == 22);
            auto path     = get_coordinates(hints, "path");     REQUIRE(path.first     == -1); REQUIRE(path.second     == -1);
            auto search   = get_coordinates(hints, "search");   REQUIRE(search.first   == -1); REQUIRE(search.second   == -1);
            auto fragment = get_coordinates(hints, "fragment"); REQUIRE(fragment.first == -1); REQUIRE(fragment.second == -1);
        }
        SECTION("bare fragment '#' with no text") {
            hints_map hints;
            auto e = URL::can_parse("http://example.com#", hints);
            REQUIRE(e == ErrorStatus::OK);
            auto scheme   = get_coordinates(hints, "scheme");   REQUIRE(scheme.first   == 0);  REQUIRE(scheme.second   == 3);
            auto host     = get_coordinates(hints, "host");     REQUIRE(host.first     == 7);  REQUIRE(host.second     == 17);
            auto port     = get_coordinates(hints, "port");     REQUIRE(port.first     == -1); REQUIRE(port.second     == -1);
            auto path     = get_coordinates(hints, "path");     REQUIRE(path.first     == -1); REQUIRE(path.second     == -1);
            auto search   = get_coordinates(hints, "search");   REQUIRE(search.first   == -1); REQUIRE(search.second   == -1);
            auto fragment = get_coordinates(hints, "fragment"); REQUIRE(fragment.first == -1); REQUIRE(fragment.second == -1);
        }
        SECTION("fragment '#' with section text") {
            hints_map hints;
            auto e = URL::can_parse("http://example.com#section", hints);
            REQUIRE(e == ErrorStatus::OK);
            auto scheme   = get_coordinates(hints, "scheme");   REQUIRE(scheme.first   == 0);  REQUIRE(scheme.second   == 3);
            auto host     = get_coordinates(hints, "host");     REQUIRE(host.first     == 7);  REQUIRE(host.second     == 17);
            auto port     = get_coordinates(hints, "port");     REQUIRE(port.first     == -1); REQUIRE(port.second     == -1);
            auto path     = get_coordinates(hints, "path");     REQUIRE(path.first     == -1); REQUIRE(path.second     == -1);
            auto search   = get_coordinates(hints, "search");   REQUIRE(search.first   == -1); REQUIRE(search.second   == -1);
            auto fragment = get_coordinates(hints, "fragment"); REQUIRE(fragment.first == 19); REQUIRE(fragment.second == 25);
        }
    }
}

// =============================================================================
// https:// — can_parse
// =============================================================================

TEST_CASE("https:// - can_parse", "[https][can_parse]") {

    SECTION("valid URLs") {
        SECTION("host only") {
            hints_map hints;
            REQUIRE(URL::can_parse("https://example.com", hints) == ErrorStatus::OK);
        }
        SECTION("host with path") {
            hints_map hints;
            REQUIRE(URL::can_parse("https://example.com/path", hints) == ErrorStatus::OK);
        }
        SECTION("host with path and query string") {
            hints_map hints;
            REQUIRE(URL::can_parse("https://example.com/path?q=hello", hints) == ErrorStatus::OK);
        }
        SECTION("host with path, query, and fragment") {
            hints_map hints;
            REQUIRE(URL::can_parse("https://example.com/path?q=hello#section", hints) == ErrorStatus::OK);
        }
        SECTION("host with explicit port") {
            hints_map hints;
            REQUIRE(URL::can_parse("https://example.com:8443/path", hints) == ErrorStatus::OK);
        }
        SECTION("fragment only (no path or query)") {
            hints_map hints;
            REQUIRE(URL::can_parse("https://example.com#section", hints) == ErrorStatus::OK);
        }
        SECTION("username and password") {
            hints_map hints;
            REQUIRE(URL::can_parse("https://user:pass@example.com/path", hints) == ErrorStatus::OK);
        }
        SECTION("username only") {
            hints_map hints;
            REQUIRE(URL::can_parse("https://user@example.com/path", hints) == ErrorStatus::OK);
        }
    }

    SECTION("invalid URLs") {
        SECTION("no host (bare https://)") {
            hints_map hints;
            REQUIRE(URL::can_parse("https://", hints) == ErrorStatus::UrlHostMissing);
        }
        SECTION("space in URL") {
            hints_map hints;
            auto e = URL::can_parse("https://example.com/path with spaces", hints);
            REQUIRE(e == ErrorStatus::UrlBodyInvalidCharacter);
        }
        SECTION("@ with no host") {
            hints_map hints;
            REQUIRE(URL::can_parse("https://user@", hints) == ErrorStatus::UrlHostMissing);
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
            CHECK(url.username().empty());
            CHECK(url.password().empty());
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
            CHECK(url.username().empty());
            CHECK(url.password().empty());
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

        SECTION("host with path and query string") {
            auto url = URL("http://example.com/path?q=hello");
            CHECK(url.protocol() == "http");
            CHECK(url.pathname() == "/path");
        }

        SECTION("host with path, query string, fragment") {
            auto url = URL("http://example.com/path?q=hello#section");
            CHECK(url.protocol() == "http");
            CHECK(url.pathname() == "/path");
            CHECK(url.hash()     == "section");
        }

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

        SECTION("host with explicit port, path and search") {
            auto url = URL("http://example.com:8080/path?q=hello");
            CHECK(url.protocol() == "http");
            CHECK(url.pathname() == "/path");
        }

        SECTION("host with explicit port, path, search and fragment") {
            auto url = URL("http://example.com:8080/path?q=hello#section");
            CHECK(url.protocol() == "http");
            CHECK(url.pathname() == "/path");
            CHECK(url.hash()     == "section");
        }

        SECTION("username and password") {
            SECTION("username and password with host only") {
                auto url = URL("http://user:pass@example.com");
                CHECK(url.protocol() == "http");
                CHECK(url.username() == "user");
                CHECK(url.password() == "pass");
                CHECK(url.hostname() == "example.com");
                CHECK(url.host()     == "example.com");
                CHECK(url.origin()   == "http://example.com");
                CHECK(url.port().empty());
                CHECK(url.pathname().empty());
            }

            SECTION("username and password with host and path") {
                auto url = URL("http://user:pass@example.com/path");
                CHECK(url.username() == "user");
                CHECK(url.password() == "pass");
                CHECK(url.hostname() == "example.com");
                CHECK(url.pathname() == "/path");
            }

            SECTION("username and password with host, port and path") {
                auto url = URL("http://user:pass@example.com:8080/path");
                CHECK(url.username() == "user");
                CHECK(url.password() == "pass");
                CHECK(url.hostname() == "example.com");
                CHECK(url.host()     == "example.com:8080");
                CHECK(url.origin()   == "http://example.com:8080");
                CHECK(url.port()     == "8080");
                CHECK(url.pathname() == "/path");
            }

            SECTION("username only (no password)") {
                auto url = URL("http://user@example.com");
                CHECK(url.username() == "user");
                CHECK(url.password().empty());
                CHECK(url.hostname() == "example.com");
            }

            SECTION("username only with path") {
                auto url = URL("http://user@example.com/path");
                CHECK(url.username() == "user");
                CHECK(url.password().empty());
                CHECK(url.hostname() == "example.com");
                CHECK(url.pathname() == "/path");
            }

            SECTION("empty username with password") {
                auto url = URL("http://:pass@example.com");
                CHECK(url.username().empty());
                CHECK(url.password() == "pass");
                CHECK(url.hostname() == "example.com");
            }

            SECTION("empty username and empty password") {
                auto url = URL("http://@example.com");
                CHECK(url.username().empty());
                CHECK(url.password().empty());
                CHECK(url.hostname() == "example.com");
            }
        }
    }

    SECTION("invalid URLs throw UrlParseException") {
        SECTION("no host (bare http://)") {
            REQUIRE_THROWS_AS(URL("http://"), UrlParseException);
        }
        SECTION("scheme starting with digit") {
            REQUIRE_THROWS_AS(URL("1http://example.com"), UrlParseException);
        }
        SECTION("space in URL") {
            REQUIRE_THROWS_AS(URL("http://example.com/path with spaces"), UrlParseException);
        }
        SECTION("empty scheme") {
            REQUIRE_THROWS_AS(URL("://example.com"), UrlParseException);
        }
        SECTION("invalid host followed by digits-only port") {
            REQUIRE_THROWS_AS(URL("http://:abc/path"), UrlParseException);
        }
        SECTION("@ with no host") {
            REQUIRE_THROWS_AS(URL("http://user@"), UrlParseException);
        }
        SECTION("@ with no host or userinfo") {
            REQUIRE_THROWS_AS(URL("http://@"), UrlParseException);
        }
    }

    SECTION("regression") {
        SECTION("root path slash is valid") {
            auto url = URL("http://example.com/");
            CHECK(url.pathname() == "/");
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
            CHECK(url.port() == "8080");
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
