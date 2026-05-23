#include <filesystem>
#include <optional>
#include <unordered_set>
#include <string_view>
#include <slim/common/http/url.h>
#include <slim/common/utilities.h>
#include <slim/SlimValue.hpp>

namespace slim::common::http::url {
	enum struct ParseState : uint8_t {
		SCHEME,
		HOST,
		PORT,
		PATH,
		SEARCH,
		SEARCH_PARAMS,
		FRAGMENT,
		INVALID
	};
	std::unordered_set<std::string_view> valid_schemes = {"file","http","https","ws","wss"};
}

slim::SlimValue slim::common::http::url::can_parse(std::string_view _string) {
	log::trace(log::Message(__func__, "begins", __FILE__, __LINE__));
	log::debug(log::Message(__func__, std::format("std::string => \"{}\" => size_t => {}", _string, _string.size()), __FILE__, __LINE__));
 
	slim::SlimValue return_value = false;
	size_t string_length = _string.size();
	auto& error_map = return_value.get_map("errors");
	auto& hints_map = return_value.get_map("hints");
 
	if(string_length == 0) {
		return_value = false;
		error_map.set("url", "string is empty");
	}
	else {
		ParseState state = ParseState::SCHEME;
 
		slim::slim_coordinates scheme_coords        {-1, -1};
		slim::slim_coordinates host_coords          {-1, -1};
		slim::slim_coordinates port_coords          {-1, -1};
		slim::slim_coordinates path_coords          {-1, -1};
		slim::slim_coordinates search_coords        {-1, -1};
		slim::slim_coordinates search_params_coords {-1, -1};
		slim::slim_coordinates fragment_coords      {-1, -1};
 
		scheme_coords.first = 0;
		bool is_file_scheme = false;
 		size_t string_position = 0;
		for(; string_position < string_length; ++string_position) {
			if(state == ParseState::INVALID) break;
 
			const unsigned char character = static_cast<unsigned char>(_string[string_position]);
 
			if(std::iscntrl(character)) {
				error_map.set("invalid_character", control_character_to_string_view(character));
				state = ParseState::INVALID;
				continue;
			}

			if(character == ' ') {
				if(state != ParseState::PATH || (state == ParseState::PATH && !is_file_scheme)) {
					return_value = false;
					if(state == ParseState::SCHEME) {
						error_map.set("scheme", std::format("invalid character in scheme => {}", "space"));
					}
					else {
						error_map.set("body", std::format("invalid character in URL body => {}", "space"));
					}
					state = ParseState::INVALID;
					continue;
				}
			}

			switch(state) {
				case ParseState::SCHEME: {
					if(character == ':') {
						if(string_position + 2 < string_length && _string[string_position + 1] == '/' && _string[string_position + 2] == '/') {
							scheme_coords.second = static_cast<int>(string_position) - 1;
							string_position += 2;
							if(string_position + 1 < string_length) {
								host_coords.first = static_cast<int>(string_position) + 1;
							}

							std::string_view scheme = _string.substr(static_cast<size_t>(scheme_coords.first),
								static_cast<size_t>(scheme_coords.second - scheme_coords.first + 1));
							std::string scheme_lower{scheme};
							std::transform(scheme_lower.begin(), scheme_lower.end(), scheme_lower.begin(), [](unsigned char c){ return std::tolower(c); });
							if(!valid_schemes.contains(scheme_lower)) {
								return_value = false;
								error_map.set("scheme", std::format("unsupported scheme => \"{}\"", scheme));
    							state = ParseState::INVALID;
							}
							else {
								return_value = true;
								is_file_scheme = (scheme_lower == "file");
								state = ParseState::HOST;
							}
						}
						else {
							return_value = false;
							error_map.set("scheme", "scheme delimiter not found => \"://\"");
							state = ParseState::INVALID;
						}
					}
					else if(!std::isalpha(static_cast<unsigned char>(character))) {
						return_value = false;
						error_map.set("scheme", std::format("invalid character in scheme => {}", character));
						state = ParseState::INVALID;
					}
					break;
				}
				case ParseState::HOST: {
					if(host_coords.first == static_cast<int>(string_position) && !std::isalnum(character)) {
						return_value = false;
						error_map.set("host", std::format("host must begin with alphanumeric data not {}", _string[string_position]));
						state = ParseState::INVALID;
					}
					else if(character == '/') {
						host_coords.second = static_cast<int>(string_position) - 1;
						path_coords.first = static_cast<int>(string_position);
						state = ParseState::PATH;
					}
					else if(character == '?') {
						host_coords.second = static_cast<int>(string_position) - 1;
						search_coords.first = static_cast<int>(string_position) + 1;
						state = ParseState::SEARCH;
					}
					else if(character == '#') {
						host_coords.second = static_cast<int>(string_position) - 1;
						fragment_coords.first = static_cast<int>(string_position) + 1;
						state = ParseState::FRAGMENT;
					}
					else if(character == ':') {
						host_coords.second = static_cast<int>(string_position) - 1;
						port_coords.first = static_cast<int>(string_position) + 1;
						state = ParseState::PORT;
					}
					else if(!std::isalnum(character)) {
						if(character != '-' && character != '.') {
							return_value = false;
							error_map.set("host", std::format("invalid character in host => {}", _string[string_position]));
							state = ParseState::INVALID;
						}
					}
					break;
				}
				case ParseState::PATH: {
					if(character == '?') {
						path_coords.second  = static_cast<int>(string_position) - 1;
						search_coords.first = static_cast<int>(string_position) + 1;
						state = ParseState::SEARCH;
					}
					else if(character == '#' && !is_file_scheme) {
						path_coords.second    = static_cast<int>(string_position) - 1;
						fragment_coords.first = static_cast<int>(string_position) + 1;
						state = ParseState::FRAGMENT;
					}
					break;
				}
				case ParseState::PORT: {
					if(character == '/') {
						port_coords.second = static_cast<int>(string_position) - 1;
						path_coords.first = static_cast<int>(string_position);
						state = ParseState::PATH;
					}
					else if(!std::isdigit(character)) {
						return_value = false;
						error_map.set("port", std::format("invalid character in port => {}", _string[string_position]));
						state = ParseState::INVALID;
					}
					break;
				}
				case ParseState::SEARCH: {
					if(character == '#') {
						search_coords.second  = static_cast<int>(string_position) - 1;
						fragment_coords.first = static_cast<int>(string_position) + 1;
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
 
		if(state == ParseState::SCHEME) {
			return_value = false;
			error_map.set("scheme", "scheme delimiter not found => \"://\"");
		}
		else if(state == ParseState::HOST && host_coords.first != -1 && host_coords.second == -1) {
			host_coords.second = last;
		}
		else if(state == ParseState::PORT && port_coords.first != -1 && port_coords.second == -1) {
			port_coords.second = last;
		}
		else if(state == ParseState::PATH && path_coords.first != -1 && path_coords.second == -1) {
			path_coords.second = last;
		}
		else if(state == ParseState::SEARCH && search_coords.first != -1 && search_coords.second == -1) {
			search_coords.second = last;
		}
		else if(state == ParseState::FRAGMENT && fragment_coords.first != -1 && fragment_coords.second == -1) {
			fragment_coords.second = last;
		}
 
		if(is_file_scheme && state != ParseState::INVALID) {
			if(path_coords.first == -1 || path_coords.first > path_coords.second) {
				return_value = false;
				error_map.set("path", "file:// URL must contain a path (e.g. file:///etc/hosts)");
			}
			else if(_string[static_cast<size_t>(path_coords.second)] == '/') {
				return_value = false;
				error_map.set("path", "file:// URL path must end with a filename, not '/'");
			}
		}
		else if(!is_file_scheme && state != ParseState::INVALID) {
			if(host_coords.first == -1) {
				return_value = false;
				error_map.set("host", "URL must contain valid host[:port] (e.g. https://www.google.com)");
			}
		}

		hints_map.set("scheme",    scheme_coords);
		hints_map.set("host",      host_coords);
		hints_map.set("port",      port_coords);
		hints_map.set("path",      path_coords);
		hints_map.set("search",    search_coords);
		hints_map.set("fragment",  fragment_coords);
	}
 
	if(error_map.size() > 0) {
		return_value.set_error("URL is unparsable");
	}
 
	log::debug(log::Message(__func__, return_value.get_error().message_or("successful"), __FILE__, __LINE__));
	log::trace(log::Message(__func__, "ends", __FILE__, __LINE__));
	return return_value;
}

std::string_view slim::common::http::url::control_character_to_string_view(const unsigned char _character) {
	std::string label;

	switch(_character) {
		case '\0': label = "Null"; break;
		case '\x01': label = "Start of Heading"; break;
		case '\x02': label = "Start of Text"; break;
		case '\x03': label = "End of Text"; break;
		case '\x04': label = "End of Transmission"; break;
		case '\x05': label = "Enquiry"; break;
		case '\x06': label = "Acknowledge"; break;
		case '\a': label = "Bell"; break;
		case '\b': label = "Backspace"; break;
		case '\t': label = "Horizontal Tab"; break;
		case '\n': label = "Line Feed"; break;
		case '\v': label = "Vertical Tab"; break;
		case '\f': label = "Form Feed"; break;
		case '\r': label = "Carriage Return"; break;
		case '\x0E': label = "Shift Out"; break;
		case '\x0F': label = "Shift In"; break;
		case '\x10': label = "Data Link Escape"; break;
		case '\x11': label = "Device Control 1"; break;
		case '\x12': label = "Device Control 2"; break;
		case '\x13': label = "Device Control 3"; break;
		case '\x14': label = "Device Control 4"; break;
		case '\x15': label = "Negative Acknowledge"; break;
		case '\x16': label = "Synchronous Idle"; break;
		case '\x17': label = "End of Transmission Block"; break;
		case '\x18': label = "Cancel"; break;
		case '\x19': label = "End of Medium"; break;
		case '\x1A': label = "Substitute"; break;
		case '\x1B': label = "Escape"; break;
		case '\x1C': label = "File Separator"; break;
		case '\x1D': label = "Group Separator"; break;
		case '\x1E': label = "Record Separator"; break;
		case '\x1F': label = "Unit Separator"; break;
		case '\x7F': label = "Delete"; break;
		default: label = "Unknown Control Character"; break;
	}

	return label;
}

slim::common::http::URL::URL() {}
slim::common::http::URL::URL(std::string_view _string) {
	log::trace(log::Message(__func__, "begins", __FILE__,__LINE__));
	//parse(_string);
	log::trace(log::Message(__func__, "ends", __FILE__,__LINE__));
}

slim::SlimValue slim::common::http::URL::can_parse(std::string_view _string) {
	log::trace(log::Message(__func__, "begins", __FILE__,__LINE__));

	auto result = slim::common::http::url::can_parse(_string);

	log::debug(log::Message(__func__, std::format("{} => returns => {} ", _string, result.to_string()), __FILE__,__LINE__));
	log::trace(log::Message(__func__, "ends", __FILE__,__LINE__));
	return result;
}
bool slim::common::http::URL::is_valid() const {
	log::trace(log::Message(__func__, "begins", __FILE__,__LINE__));
//	log::debug(log::Message(__func__, "return =>" + utilities::to_string(__is_valid), __FILE__,__LINE__));
	log::trace(log::Message(__func__, "begins", __FILE__,__LINE__));
    return __is_valid;
}
/* void slim::common::http::URL::parse(const std::string& _string) {
	log::trace(log::Message(__func__, "begins", __FILE__,__LINE__));

    std::smatch match;
	if(std::regex_match(_string, match, url_pattern)) {
		__protocol = match[1];
		__hostname = match[2];
		__port     = match[3];
		__pathname = match[4];
		__search   = match[5];
		__hash     = match[6];

		if(__protocol == "") {
			__protocol = "https";
		}

		if(__port == "") {
			if(__protocol == "http" || __protocol == "ws") {
				__port = "80";
			}
			else if(__protocol == "https" || __protocol == "wss") {
				__port = "443";
			}
			else if(__protocol == "ftp") {
				__port = "21";
			}
		}

		if(__hostname != "") {
			__host = __port != "" ? std::format("{}:{}", __hostname.value(), __port.value()) : __hostname;
		}

		if(__pathname == "") {
			__pathname = "/";
		}

		if(__protocol == "file" && !__pathname.value().starts_with("/")) {
			__pathname = (std::filesystem::absolute(__pathname.value()).string());
			log::debug(log::Message(__func__,"set request absolute file path => " + __pathname.value(),__FILE__,__LINE__));
		}
		else if(__protocol.value().starts_with("http") && __pathname.value().length() > 1) {
			slim::common::utilities::replace_all(__pathname.value(), "..", "");
			slim::common::utilities::replace_all(__pathname.value(), "/.", "/");
			slim::common::utilities::replace_all(__pathname.value(), "./", "/");
			slim::common::utilities::replace_all(__pathname.value(), "//", "/");
		}

		// must run after required bits have been set
		if(__protocol != "" && __host.has_value() && __host != "" && __pathname != "") {
			__href = std::format("{}://{}{}", __protocol.value(), __host.value(), __pathname.value());
		}
		__url = _string;
		__is_valid = true;
		log::debug(log::Message(__func__, "URL parsed successfully", __FILE__,__LINE__));
    }
	else {
		__is_valid = false;
		log::debug(log::Message(__func__, "URL was not parsed successfully => " + _string, __FILE__,__LINE__));
	}
	log::trace(log::Message(__func__, "ends", __FILE__,__LINE__));
} */

std::optional<std::string> slim::common::http::URL::hash() const {
	log::trace(log::Message(__func__, "begins", __FILE__,__LINE__));
	log::debug(log::Message(__func__, std::format("return => {}", __hash.value_or("not set")), __FILE__,__LINE__));
	log::trace(log::Message(__func__, "ends", __FILE__,__LINE__));
	return __hash;
}
std::optional<std::string> slim::common::http::URL::host() const {
	log::trace(log::Message(__func__, "begins", __FILE__,__LINE__));
	log::debug(log::Message(__func__, std::format("return => host => {}", __host.value_or("is not set")), __FILE__,__LINE__));
	log::trace(log::Message(__func__, "ends", __FILE__,__LINE__));
	return __host;
}
std::optional<std::string> slim::common::http::URL::hostname() const {
	log::trace(log::Message(__func__, "begins", __FILE__,__LINE__));
	log::debug(log::Message(__func__, std::format("return => {}", __hostname.value_or("not set")), __FILE__,__LINE__));
	log::trace(log::Message(__func__, "ends", __FILE__,__LINE__));
	return __hostname;
}
std::optional<std::string> slim::common::http::URL::href() const {
	log::trace(log::Message(__func__, "begins", __FILE__,__LINE__));
	log::debug(log::Message(__func__, std::format("return => {}", __host.value_or("is not set")), __FILE__,__LINE__));
	log::trace(log::Message(__func__, "ends", __FILE__,__LINE__));
	return __href;
}
std::optional<std::string> slim::common::http::URL::origin() const {
	log::trace(log::Message(__func__, "begins", __FILE__,__LINE__));
	log::debug(log::Message(__func__, std::format("return => {}", __origin.value_or("not set")), __FILE__,__LINE__));
	log::trace(log::Message(__func__, "ends", __FILE__,__LINE__));
	return __origin;
}
std::optional<std::string> slim::common::http::URL::password() const {
	log::trace(log::Message(__func__, "begins", __FILE__,__LINE__));
	log::debug(log::Message(__func__, std::format("return => {}", __password.value_or("not set")), __FILE__,__LINE__));
	log::trace(log::Message(__func__, "ends", __FILE__,__LINE__));
	return __password;
}
std::optional<std::string> slim::common::http::URL::pathname() const {
	log::trace(log::Message(__func__, "begins", __FILE__,__LINE__));
	log::debug(log::Message(__func__, std::format("return => {}", __pathname.value_or("not set")), __FILE__,__LINE__));
	log::trace(log::Message(__func__, "ends", __FILE__,__LINE__));
	return __pathname;
}
std::optional<std::string> slim::common::http::URL::port() const {
	log::trace(log::Message(__func__, "begins", __FILE__,__LINE__));
	log::debug(log::Message(__func__, std::format("return => {}", __port.value_or("not set")), __FILE__,__LINE__));
	log::trace(log::Message(__func__, "ends", __FILE__,__LINE__));
	return __port;
}
std::string slim::common::http::URL::protocol() const {
	log::trace(log::Message(__func__, "begins", __FILE__,__LINE__));
	log::debug(log::Message(__func__, std::format("return => {}", __protocol), __FILE__,__LINE__));
	log::trace(log::Message(__func__, "ends", __FILE__,__LINE__));
	return __protocol;
}
std::optional<std::string> slim::common::http::URL::search() const {
	log::trace(log::Message(__func__, "begins", __FILE__,__LINE__));
	log::error(log::Message(__func__, "full parsing not implemented =>", __FILE__,__LINE__));
	log::debug(log::Message(__func__, "full parsing not implemented =>", __FILE__,__LINE__));
	log::debug(log::Message(__func__, std::format("return => {}", __search.value_or("not set")), __FILE__,__LINE__));
	log::trace(log::Message(__func__, "ends", __FILE__,__LINE__));
	return __search;
}
std::optional<std::string> slim::common::http::URL::searchParams() const {
	log::trace(log::Message(__func__, "begins", __FILE__,__LINE__));
	log::error(log::Message(__func__, "not implemented =>", __FILE__,__LINE__));
	log::debug(log::Message(__func__, "not implemented =>", __FILE__,__LINE__));
	log::trace(log::Message(__func__, "ends", __FILE__,__LINE__));
	return "";
}
std::optional<std::string> slim::common::http::URL::username() const {
	log::trace(log::Message(__func__, "begins", __FILE__,__LINE__));
	log::debug(log::Message(__func__, std::format("return => {}", __username.value_or("not set")), __FILE__,__LINE__));
	log::trace(log::Message(__func__, "ends", __FILE__,__LINE__));
	return __username;
}

std::optional<std::string> slim::common::http::URL::url() const {
	log::trace(log::Message(__func__, "begins", __FILE__,__LINE__));
	log::debug(log::Message(__func__, std::format("return => {}", __url.value_or("not set")), __FILE__,__LINE__));
	log::trace(log::Message(__func__, "ends", __FILE__,__LINE__));
	return __url;
}
