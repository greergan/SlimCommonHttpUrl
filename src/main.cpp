#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>

#include <slim/common/http/error_codes.h>
#include <slim/common/http/url.h>
#include <slim/common/utilities.h>

namespace slim::common::http {
namespace {
    enum struct ParseState : uint8_t { SCHEME, HOST, PORT, PATH, SEARCH, SEARCH_PARAMS, FRAGMENT, INVALID };

    std::unordered_set<std::string_view> valid_schemes = {"file", "http", "https", "ws", "wss"};

    ErrorStatus can_parse_impl(std::string_view s, hints_map& hints) noexcept{
        size_t string_length = s.size();

        if (string_length == 0) return ErrorStatus::UrlStringEmpty;

        ParseState state = ParseState::SCHEME;

        std::pair<int, int> scheme_coords{0, -1};
        std::pair<int, int> username_coords{-1, -1};
        std::pair<int, int> password_coords{-1, -1};
        std::pair<int, int> host_coords{-1, -1};
        std::pair<int, int> port_coords{-1, -1};
        std::pair<int, int> path_coords{-1, -1};
        std::pair<int, int> search_coords{-1, -1};
        std::pair<int, int> fragment_coords{-1, -1};

        bool is_file_scheme = false;
        ErrorStatus error = ErrorStatus::OK;
        size_t string_position = 0;

        for (; string_position < string_length; ++string_position) {
            if (state == ParseState::INVALID) break;

            const unsigned char character = static_cast<unsigned char>(s[string_position]);

            if (std::iscntrl(character)) {
                error = ErrorStatus::UrlInvalidControlCharacter;
                state = ParseState::INVALID;
                continue;
            }

            if (character == ' ') {
                if (state != ParseState::PATH || (state == ParseState::PATH && !is_file_scheme)) {
                    error = (state == ParseState::SCHEME) ? ErrorStatus::UrlSchemeInvalidCharacter : ErrorStatus::UrlBodyInvalidCharacter;
                    state = ParseState::INVALID;
                    continue;
                }
            }

            switch (state) {
                case ParseState::SCHEME: {
                    if (character == ':') {
                        if (string_position + 2 < string_length && s[string_position + 1] == '/' && s[string_position + 2] == '/') {
                            scheme_coords.second = static_cast<int>(string_position) - 1;
                            string_position += 2;
                            std::string_view scheme =
                                s.substr(static_cast<size_t>(scheme_coords.first), static_cast<size_t>(scheme_coords.second - scheme_coords.first + 1));
                            bool scheme_valid = std::any_of(valid_schemes.begin(), valid_schemes.end(),
                                [&](std::string_view candidate) { return slim::common::utilities::iequals(scheme, candidate); });
                            if (!scheme_valid) {
                                error = ErrorStatus::UrlSchemeUnsupported;
                                state = ParseState::INVALID;
                            }
                            else {
                                is_file_scheme = slim::common::utilities::iequals(scheme, "file");
                                if (is_file_scheme) {
                                    if (string_position + 1 < string_length) {
                                        state = ParseState::PATH;
                                        path_coords.first = static_cast<int>(string_position) + 1;
                                    }
                                }
                                else {
                                    if (string_position + 1 < string_length) {
                                        state = ParseState::HOST;
                                        host_coords.first = static_cast<int>(string_position) + 1;
                                    }
                                }
                            }
                        }
                        else {
                            error = ErrorStatus::UrlSchemeDelimiterMissing;
                            state = ParseState::INVALID;
                        }
                    }
                    else if (!slim::common::utilities::is_alpha(static_cast<char>(character))) {
                        error = ErrorStatus::UrlSchemeInvalidCharacter;
                        state = ParseState::INVALID;
                    }
                    break;
                }
                case ParseState::HOST: {
                    // one-time lookahead for '@' when we first enter HOST
                    if (host_coords.first == static_cast<int>(string_position)) {
                        for (size_t i = string_position; i < string_length; ++i) {
                            const char c = s[i];
                            if (c == '/' || c == '?' || c == '#') break;
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
                                    // no ':', whole thing is username
                                    username_coords.second = static_cast<int>(i) - 1;
                                }
                                host_coords.first = static_cast<int>(i) + 1;
                                string_position = static_cast<size_t>(host_coords.first);
                                break;
                            }
                        }
                    }
                    if (host_coords.first == static_cast<int>(string_position) && !slim::common::utilities::is_alnum(static_cast<char>(s[string_position]))) {
                        error = ErrorStatus::UrlHostInvalidStart;
                        state = ParseState::INVALID;
                    }
                    else if (character == '/') {
                        host_coords.second = static_cast<int>(string_position) - 1;
                        path_coords.first = static_cast<int>(string_position);
                        state = ParseState::PATH;
                    }
                    else if (character == '?') {
                        host_coords.second = static_cast<int>(string_position) - 1;
                        if (string_position + 1 < string_length) search_coords.first = static_cast<int>(string_position) + 1;
                        state = ParseState::SEARCH;
                    }
                    else if (character == '#') {
                        host_coords.second = static_cast<int>(string_position) - 1;
                        if (string_position + 1 < string_length) fragment_coords.first = static_cast<int>(string_position) + 1;
                        state = ParseState::FRAGMENT;
                    }
                    else if (character == ':') {
                        host_coords.second = static_cast<int>(string_position) - 1;
                        if (string_position + 1 < string_length) port_coords.first = static_cast<int>(string_position) + 1;
                        state = ParseState::PORT;
                    }
                    else if (!slim::common::utilities::is_alnum(static_cast<char>(character))) {
                        if (character != '-' && character != '.') {
                            error = ErrorStatus::UrlHostInvalidCharacter;
                            state = ParseState::INVALID;
                        }
                    }
                    break;
                }
                case ParseState::PATH: {
                    if (!is_file_scheme) {
                        if (character == '?') {
                            path_coords.second = static_cast<int>(string_position) - 1;
                            if (string_position + 1 < string_length) search_coords.first = static_cast<int>(string_position) + 1;
                            state = ParseState::SEARCH;
                        }
                        else if (character == '#' && !is_file_scheme) {
                            path_coords.second = static_cast<int>(string_position) - 1;
                            if (string_position + 1 < string_length) fragment_coords.first = static_cast<int>(string_position) + 1;
                            state = ParseState::FRAGMENT;
                        }
                    }
                    break;
                }
                case ParseState::PORT: {
                    if (character == '/') {
                        port_coords.second = static_cast<int>(string_position) - 1;
                        path_coords.first = static_cast<int>(string_position);
                        state = ParseState::PATH;
                    }
                    else if (character == '?') {
                        port_coords.second = static_cast<int>(string_position) - 1;
                        if (string_position + 1 < string_length) search_coords.first = static_cast<int>(string_position) + 1;
                        state = ParseState::SEARCH;
                    }
                    else if (character == '#') {
                        port_coords.second = static_cast<int>(string_position) - 1;
                        if (string_position + 1 < string_length) fragment_coords.first = static_cast<int>(string_position) + 1;
                        state = ParseState::FRAGMENT;
                    }
                    else if (!slim::common::utilities::is_digit(static_cast<char>(character))) {
                        error = ErrorStatus::UrlPortInvalidCharacter;
                        state = ParseState::INVALID;
                    }
                    break;
                }
                case ParseState::SEARCH: {
                    if (character == '#') {
                        search_coords.second = static_cast<int>(string_position) - 1;
                        if (string_position + 1 < string_length) fragment_coords.first = static_cast<int>(string_position) + 1;
                        state = ParseState::FRAGMENT;
                    }
                    break;
                }
                case ParseState::FRAGMENT: {
                    break;
                }
                case ParseState::INVALID:
                default:
                    break;
            }
        }

        int last = static_cast<int>(string_position) - 1;

        if (state == ParseState::SCHEME && scheme_coords.first != -1 && scheme_coords.second == -1) scheme_coords.second = last;
        else if (state == ParseState::HOST && host_coords.first != -1 && host_coords.second == -1) host_coords.second = last;
        else if (state == ParseState::PORT && port_coords.first != -1 && port_coords.second == -1) port_coords.second = last;
        else if (state == ParseState::PATH && path_coords.first != -1 && path_coords.second == -1) path_coords.second = last;
        else if (state == ParseState::SEARCH && search_coords.first != -1 && search_coords.second == -1) search_coords.second = last;
        else if (state == ParseState::FRAGMENT && fragment_coords.first != -1 && fragment_coords.second == -1) fragment_coords.second = last;

        if (error == ErrorStatus::OK && state != ParseState::INVALID) {
            if (is_file_scheme) {
                if (path_coords.first == -1 || path_coords.first > path_coords.second) error = ErrorStatus::UrlFilePathMissing;
                else if (s[static_cast<size_t>(path_coords.second)] == '/') error = ErrorStatus::UrlFilePathTrailingSlash;
            }
            else if (!is_file_scheme) {
                if (host_coords.first == -1 || host_coords.first > host_coords.second)
                    error = ErrorStatus::UrlHostMissing;
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

URL::URL() {}

URL::URL(std::string_view s) {
    href_ = std::string(s);
    hints_map hints;
    ErrorStatus error = can_parse_impl(s, hints);
    if (error != ErrorStatus::OK) throw UrlParseException(error);
    parse(hints);
}

URL::URL(std::string_view s, const hints_map& hints) {
    href_ = std::string(s);
    parse(hints);
}

ErrorStatus URL::can_parse(std::string_view s, hints_map& hints) noexcept {
    return can_parse_impl(s, hints);
}

void URL::parse(const hints_map& hints) noexcept {
    if (!hints.empty()) {
        auto extract = [&](const std::string& key) -> std::string {
            auto it = hints.find(key);
            if (it == hints.end()) return {};
            const auto& coords = it->second;
            if (coords.first > -1 && coords.second >= coords.first && static_cast<size_t>(coords.second) <= href_.length()) {
                int count = coords.second - coords.first + 1;
                return href_.substr(static_cast<size_t>(coords.first), static_cast<size_t>(count));
            }
            return {};
        };

        protocol_   = extract("scheme");
        username_   = extract("username");
        password_   = extract("password");
        hostname_   = extract("host");
        port_       = extract("port");
        pathname_   = extract("path");
        search_     = extract("search");
        hash_       = extract("fragment");

        if (!hostname_.empty()) {
            if (!port_.empty()) host_ = hostname_ + ":" + port_;
            else host_ = hostname_;
        }

        origin_ = protocol_ + "://" + hostname_;
        if (!port_.empty()) {
            bool add_port = false;

            if (slim::common::utilities::iequals(protocol_, "http") || slim::common::utilities::iequals(protocol_, "ws")) {
                if (port_ != "80") add_port = true;
            }
            else if (slim::common::utilities::iequals(protocol_, "https") || slim::common::utilities::iequals(protocol_, "wss")) {
                if (port_ != "443") add_port = true;
            }

            if (add_port) origin_ += ":" + port_;
        }
    }
}

} // namespace slim::common::http
