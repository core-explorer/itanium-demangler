#pragma once
#include "../util/format.hxx"
#include <string>
#include <string_view>

namespace swartalf {
enum class Navigation {
	hyperlink,
	javascript
};

enum class HyperlinkType {
	root_relative,
	absolute,
	relative
};

struct HyperlinkConfig {
	std::string absolute_host;
	std::string root_prefix;
	HyperlinkType hyper_type = HyperlinkType::root_relative;
};

std::string rewrite_url(std::string_view target, std::string_view current_url, const HyperlinkConfig& hyperconfig);

template <typename Content>
[[nodiscard]] std::string hyperlink(std::string_view url, Content&& content, Navigation nav) {
	if (nav == Navigation::javascript) {
		return fmt::format("<a href=\"#\" onclick=\"set_url('{}');return false;\">{}</a>", url, content);
	}
	return fmt::format("<a href=\"{}\">{}</a>", url, content);
}

template <typename Content>
[[nodiscard]] std::string hyperlink(std::string_view url, Content&& content, Navigation nav, std::string_view current_url, const HyperlinkConfig& hyperconfig) {
	if (nav == Navigation::javascript) {
		return fmt::format("<a href=\"#\" onclick=\"set_url('{}');return false;\">{}</a>", url, content);
	}
	auto rewritten_url = rewrite_url(url, current_url, hyperconfig);
	return fmt::format("<a href=\"{}\">{}</a>", rewritten_url, content);
}

}
