#include <format>
#include <set>
#include <slim/common/http/url.h>
#include <slim/common/log.h>
#include <slim/SlimValue.hpp>

int main() {
	using namespace slim::common;
	std::set all_urls = {
		"",
		"a",
		"a:",
		"a:/",
		"a://",
		"file://"
	};
    log::trace(log::Message{__func__, "begins", __FILE__, __LINE__});
	
	for(auto& url : all_urls) {
		auto parsable = http::URL::can_parse(url);
		log::debug(log::Message{__func__, std::format("can_parse({}) => {}", url, parsable.to_string()), __FILE__, __LINE__});
		if(parsable.has_error()) {
			log::error(log::Message{__func__, std::format("can_parse({}) => {}", url, parsable.get_error().message()), __FILE__, __LINE__});
		}

		if(parsable.has_map("errors")) {
 			auto error_map = parsable.get_map("errors").get();
			if(error_map.size() > 0) {
				for(auto& [key, value] : error_map) {
					log::error(log::Message{__func__, std::format("can_parse({}) => {} : {}", url, key, value.to_string()), __FILE__, __LINE__});
				}
			}
		}
	}

	log::trace(log::Message{__func__, "ends", __FILE__, __LINE__});
	return 0;
}