#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <string_view>
#include <utility>

#include <slim/common/http/error_codes.h>
#include <slim/common/http/url.h>
#include <slim/common/utilities.h>

namespace slim::common::http {

namespace {

using slim::common::utilities::is_alpha;
using slim::common::utilities::is_alnum;
using slim::common::utilities::is_digit;
using slim::common::utilities::iequals;

enum struct ParseState : uint8_t { SCHEME, HOST, PORT, PATH, SEARCH, FRAGMENT, INVALID };

constexpr std::array<std::string_view, 5> valid_schemes = {"file", "http", "https", "ws", "wss"};

ErrorStatus can_parse_impl(std::string_view s, hints_map& hints, UrlParseMode mode = UrlParseMode::FULL) noexcept {
    const size_t string_length = s.size();

    if (string_length == 0) {
        return ErrorStatus::UrlStringEmpty;
    }

    ParseState state = ParseState::SCHEME;

    std::pair<int, int> scheme_coords   = {0,  -1};
    std::pair<int, int> username_coords = {-1, -1};
    std::pair<int, int> password_coords = {-1, -1};
    std::pair<int, int> host_coords     = {-1, -1};
    std::pair<int, int> port_coords     = {-1, -1};
    std::pair<int, int> path_coords     = {-1, -1};
    std::pair<int, int> search_coords   = {-1, -1};
    std::pair<int, int> fragment_coords = {-1, -1};

    bool        is_file_scheme  = false;
    ErrorStatus error           = ErrorStatus::OK;
    size_t      string_position = 0;

    if (mode == UrlParseMode::PATH) {
        state             = ParseState::PATH;
        path_coords.first = 0;
        scheme_coords     = {-1, -1};
    }

    for (; string_position < string_length; ++string_position) {
        if (state == ParseState::INVALID) {
            break;
        }

        const unsigned char character = static_cast<unsigned char>(s[string_position]);

        if (std::iscntrl(character)) {
            error = ErrorStatus::UrlInvalidControlCharacter;
            state = ParseState::INVALID;
            continue;
        }

        if (character == ' ') {
            if (state != ParseState::PATH || !is_file_scheme) {
                error = (state == ParseState::SCHEME) ? ErrorStatus::UrlSchemeInvalidCharacter
                                                      : ErrorStatus::UrlBodyInvalidCharacter;
                state = ParseState::INVALID;
                continue;
            }
        }

        switch (state) {
        case ParseState::SCHEME: {
            if (character == ':') {
                if (string_position + 2 < string_length && s[string_position + 1] == '/' && s[string_position + 2] == '/') {
                    scheme_coords.second = static_cast<int>(string_position) - 1;
                    string_position     += 2;
                    std::string_view scheme = s.substr(static_cast<size_t>(scheme_coords.first),
                                                       static_cast<size_t>(scheme_coords.second - scheme_coords.first + 1));
                    bool scheme_valid = std::any_of(valid_schemes.begin(), valid_schemes.end(),
                                                    [&](std::string_view candidate) {
                                                        return iequals(scheme, candidate);
                                                    });
                    if (!scheme_valid) {
                        error = ErrorStatus::UrlSchemeUnsupported;
                        state = ParseState::INVALID;
                    } else {
                        is_file_scheme = iequals(scheme, "file");
                        if (string_position + 1 < string_length) {
                            if (is_file_scheme) {
                                state             = ParseState::PATH;
                                path_coords.first = static_cast<int>(string_position) + 1;
                            } else {
                                state             = ParseState::HOST;
                                host_coords.first = static_cast<int>(string_position) + 1;
                            }
                        }
                    }
                } else {
                    error = ErrorStatus::UrlSchemeDelimiterMissing;
                    state = ParseState::INVALID;
                }
            } else if (!is_alpha(static_cast<char>(character))) {
                error = ErrorStatus::UrlSchemeInvalidCharacter;
                state = ParseState::INVALID;
            }
            break;
        }
        case ParseState::HOST: {
            if (host_coords.first == static_cast<int>(string_position)) {
                for (size_t i = string_position; i < string_length; ++i) {
                    const char c = s[i];
                    if (c == '/' || c == '?' || c == '#') {
                        break;
                    }
                    if (c == '@') {
                        username_coords.first = static_cast<int>(string_position);
                        for (size_t j = string_position; j < i; ++j) {
                            if (s[j] == ':') {
                                username_coords.second = static_cast<int>(j) - 1;
                                password_coords.first  = static_cast<int>(j) + 1;
                                password_coords.second = static_cast<int>(i) - 1;
                                break;
                            }
                        }
                        if (password_coords.first == -1) {
                            username_coords.second = static_cast<int>(i) - 1;
                        }
                        host_coords.first = static_cast<int>(i) + 1;
                        string_position   = static_cast<size_t>(host_coords.first);
                        break;
                    }
                }
                if (string_position >= string_length) {
                    error = ErrorStatus::UrlHostMissing;
                    state = ParseState::INVALID;
                    break;
                }
            }
            const unsigned char current = static_cast<unsigned char>(s[string_position]);
            if (host_coords.first == static_cast<int>(string_position) &&
                !is_alnum(static_cast<char>(current))) {
                error = ErrorStatus::UrlHostInvalidStart;
                state = ParseState::INVALID;
            } else if (current == '/') {
                host_coords.second = static_cast<int>(string_position) - 1;
                path_coords.first  = static_cast<int>(string_position);
                state              = ParseState::PATH;
            } else if (current == '?') {
                host_coords.second = static_cast<int>(string_position) - 1;
                if (string_position + 1 < string_length) {
                    search_coords.first = static_cast<int>(string_position) + 1;
                }
                state = ParseState::SEARCH;
            } else if (current == '#') {
                host_coords.second = static_cast<int>(string_position) - 1;
                if (string_position + 1 < string_length) {
                    fragment_coords.first = static_cast<int>(string_position) + 1;
                }
                state = ParseState::FRAGMENT;
            } else if (current == ':') {
                host_coords.second = static_cast<int>(string_position) - 1;
                if (string_position + 1 < string_length) {
                    port_coords.first = static_cast<int>(string_position) + 1;
                }
                state = ParseState::PORT;
            } else if (!is_alnum(static_cast<char>(current)) && current != '-' && current != '.') {
                error = ErrorStatus::UrlHostInvalidCharacter;
                state = ParseState::INVALID;
            }
            break;
        }
        case ParseState::PATH: {
            if (!is_file_scheme) {
                if (character == '?') {
                    path_coords.second = static_cast<int>(string_position) - 1;
                    if (string_position + 1 < string_length) {
                        search_coords.first = static_cast<int>(string_position) + 1;
                    }
                    state = ParseState::SEARCH;
                } else if (character == '#') {
                    path_coords.second = static_cast<int>(string_position) - 1;
                    if (string_position + 1 < string_length) {
                        fragment_coords.first = static_cast<int>(string_position) + 1;
                    }
                    state = ParseState::FRAGMENT;
                }
            }
            break;
        }
        case ParseState::PORT: {
            if (character == '/' || character == '?' || character == '#') {
                if (static_cast<int>(string_position) == port_coords.first) {
                    port_coords = {-1, -1};
                } else {
                    port_coords.second = static_cast<int>(string_position) - 1;
                }
                if (character == '/') {
                    path_coords.first = static_cast<int>(string_position);
                    state             = ParseState::PATH;
                } else if (character == '?') {
                    if (string_position + 1 < string_length) {
                        search_coords.first = static_cast<int>(string_position) + 1;
                    }
                    state = ParseState::SEARCH;
                } else if (character == '#') {
                    if (string_position + 1 < string_length) {
                        fragment_coords.first = static_cast<int>(string_position) + 1;
                    }
                    state = ParseState::FRAGMENT;
                }
            } else if (!is_digit(static_cast<char>(character))) {
                error = ErrorStatus::UrlPortInvalidCharacter;
                state = ParseState::INVALID;
            }
            break;
        }
        case ParseState::SEARCH: {
            if (character == '#') {
                search_coords.second = static_cast<int>(string_position) - 1;
                if (string_position + 1 < string_length) {
                    fragment_coords.first = static_cast<int>(string_position) + 1;
                }
                state = ParseState::FRAGMENT;
            }
            break;
        }
        case ParseState::FRAGMENT:
        case ParseState::INVALID:
        default:
            break;
        }
    }

    const int last = static_cast<int>(string_position) - 1;

    if (state == ParseState::SCHEME && scheme_coords.first != -1 && scheme_coords.second == -1) {
        scheme_coords.second = last;
    } else if (state == ParseState::HOST && host_coords.first != -1 && host_coords.second == -1) {
        host_coords.second = last;
    } else if (state == ParseState::PORT && port_coords.first != -1 && port_coords.second == -1) {
        if (port_coords.first > last) {
            port_coords = {-1, -1};
        } else {
            port_coords.second = last;
        }
    } else if (state == ParseState::PATH && path_coords.first != -1 && path_coords.second == -1) {
        path_coords.second = last;
    } else if (state == ParseState::SEARCH && search_coords.first != -1 && search_coords.second == -1) {
        search_coords.second = last;
    } else if (state == ParseState::FRAGMENT && fragment_coords.first != -1 && fragment_coords.second == -1) {
        fragment_coords.second = last;
    }

    if (error == ErrorStatus::OK && state != ParseState::INVALID) {
        if (mode == UrlParseMode::FULL) {
            if (is_file_scheme) {
                if (path_coords.first == -1 || path_coords.first > path_coords.second) {
                    error = ErrorStatus::UrlFilePathMissing;
                } else if (s[static_cast<size_t>(path_coords.second)] == '/') {
                    error = ErrorStatus::UrlFilePathTrailingSlash;
                }
            } else {
                if (host_coords.first == -1 || host_coords.first > host_coords.second) {
                    error = ErrorStatus::UrlHostMissing;
                }
            }
        }
    }

    hints["scheme"]   = scheme_coords;
    hints["username"] = username_coords;
    hints["password"] = password_coords;
    hints["host"]     = host_coords;
    hints["port"]     = port_coords;
    hints["path"]     = path_coords;
    hints["search"]   = search_coords;
    hints["fragment"] = fragment_coords;

    return error;
}

} // namespace

URL::URL(std::string_view s) {
    hints_map   hints;
    ErrorStatus error = can_parse_impl(s, hints);
    if (error != ErrorStatus::OK) {
        throw UrlParseException(error);
    }
    try {
        href_ = std::string(s);
    } catch (const std::bad_alloc&) {
        throw UrlParseException(ErrorStatus::BadAllocation);
    }
    if (auto e = parse(hints); e != ErrorStatus::OK) {
        throw UrlParseException(e);
    }
}

URL::URL(std::string_view s, const hints_map& hints) {
    try {
        href_ = std::string(s);
    } catch (const std::bad_alloc&) {
        throw UrlParseException(ErrorStatus::BadAllocation);
    }
    if (auto e = parse(hints); e != ErrorStatus::OK) {
        throw UrlParseException(e);
    }
}

URL::URL(std::string_view s, UrlParseMode mode) {
    hints_map   hints;
    ErrorStatus error = can_parse_impl(s, hints, mode);
    if (error != ErrorStatus::OK) {
        throw UrlParseException(error);
    }
    try {
        href_ = std::string(s);
    } catch (const std::bad_alloc&) {
        throw UrlParseException(ErrorStatus::BadAllocation);
    }
    if (auto e = parse(hints); e != ErrorStatus::OK) {
        throw UrlParseException(e);
    }
}

ErrorStatus URL::can_parse(std::string_view s, hints_map& hints) noexcept {
    return can_parse_impl(s, hints);
}

ErrorStatus URL::parse(const hints_map& hints) noexcept {
    using slim::common::utilities::iequals;

    if (hints.empty()) {
        return ErrorStatus::OK;
    }
    auto extract = [&](const std::string& key) -> std::string {
        auto it = hints.find(key);
        if (it == hints.end()) {
            return {};
        }
        const auto& [first, second] = it->second;
        if (first > -1 && second >= first && static_cast<size_t>(second) <= href_.length()) {
            return href_.substr(static_cast<size_t>(first), static_cast<size_t>(second - first + 1));
        }
        return {};
    };

    try {
        protocol_ = extract("scheme");
        username_ = extract("username");
        password_ = extract("password");
        hostname_ = extract("host");
        port_     = extract("port");
        pathname_ = extract("path");
        search_   = extract("search");
        hash_     = extract("fragment");

        if (!hostname_.empty()) {
            host_ = port_.empty() ? hostname_ : hostname_ + ":" + port_;
        }

        origin_ = protocol_ + "://" + hostname_;
        if (!port_.empty()) {
            bool add_port = false;
            if (iequals(protocol_, "http") || iequals(protocol_, "ws")) {
                add_port = (port_ != "80");
            } else if (iequals(protocol_, "https") || iequals(protocol_, "wss")) {
                add_port = (port_ != "443");
            }
            if (add_port) {
                origin_ += ":" + port_;
            }
        }

        search_params_ = std::make_shared<UrlSearchParams>(search_);
    } catch (const std::bad_alloc&) {
        return ErrorStatus::BadAllocation;
    } catch (const SearchParamParseException& e) {
        return e.error();
    }

    return ErrorStatus::OK;
}

} // namespace slim::common::http
