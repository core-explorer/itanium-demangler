#pragma once
#include "Unsanitized.hxx"
#include <string>
#include <string_view>
#include <vector>
namespace swartalf {

inline void split(std::vector<std::string_view>& parts, std::string_view str, char delim) {
	auto pos = str.find(delim);
	while (pos != std::string_view::npos) {
		if (not str.empty()) {
			parts.push_back(str.substr(0, pos));
		}
		str.remove_prefix(pos + 1);
		pos = str.find(delim);
	}
	if (not str.empty()) {
		parts.push_back(str);
	}
}

inline void split(std::vector<std::string>& parts, std::string_view str, char delim) {
	auto pos = str.find(delim);
	while (pos != std::string_view::npos) {
		if (not str.empty()) {
			parts.emplace_back(str.substr(0, pos));
		}
		str.remove_prefix(pos + 1);
		pos = str.find(delim);
	}
	if (not str.empty()) {
		parts.emplace_back(str);
	}
}

inline void split(std::vector<std::string>& parts, std::string_view str, std::string_view delims) {
	auto pos = str.find_first_of(delims);
	while (pos != std::string_view::npos) {
		if (not str.empty()) {
			parts.emplace_back(str.substr(0, pos));
		}
		str.remove_prefix(pos + 1);
		pos = str.find_first_of(delims);
	}
	if (not str.empty()) {
		parts.emplace_back(str);
	}
}

inline void split(std::vector<std::string_view>& parts, std::string_view str, std::string_view delims) {
	auto pos = str.find_first_of(delims);
	while (pos != std::string_view::npos) {
		if (not str.empty()) {
			parts.emplace_back(str.substr(0, pos));
		}
		str.remove_prefix(pos + 1);
		pos = str.find_first_of(delims);
	}
	if (not str.empty()) {
		parts.emplace_back(str);
	}
}

inline void split_with_delims(std::vector<std::string_view>& parts, std::string_view str, std::string_view delims) {
	auto pos = str.find_first_of(delims);
	while (pos != std::string_view::npos) {
		if (not str.empty()) {
			parts.emplace_back(str.substr(0, pos));
		}
		parts.emplace_back(str.substr(pos, 1));
		str.remove_prefix(pos + 1);
		pos = str.find_first_of(delims);
	}
	if (not str.empty()) {
		parts.emplace_back(str);
	}
}

inline void split(std::vector<Unsanitized<std::string_view>>& parts, Unsanitized<std::string_view> str, char delim) {
	auto pos = str.find(delim);
	while (pos != std::string_view::npos) {
		if (not str.empty()) {
			parts.emplace_back(str.substr(0, pos));
		}
		str.remove_prefix(pos + 1);
		pos = str.find(delim);
	}
	if (not str.empty()) {
		parts.emplace_back(str);
	}
}

inline void split(std::vector<Unsanitized<std::string>>& parts, Unsanitized<std::string_view> str, char delim) {
	auto pos = str.find(delim);
	while (pos != std::string_view::npos) {
		if (not str.empty()) {
			parts.emplace_back(std::string(str.substr(0, pos).get_value()));
		}
		str.remove_prefix(pos + 1);
		pos = str.find(delim);
	}
	if (not str.empty()) {
		parts.emplace_back(std::string(str.get_value()));
	}
}

inline void split(std::vector<Unsanitized<std::string>>& parts, Unsanitized<std::string_view> str, std::string_view delims) {
	auto pos = str.find_first_of(delims);
	while (pos != std::string_view::npos) {
		if (not str.empty()) {
			parts.emplace_back(std::string(str.substr(0, pos).get_value()));
		}
		str.remove_prefix(pos + 1);
		pos = str.find_first_of(delims);
	}
	if (not str.empty()) {
		parts.emplace_back(std::string(str.get_value()));
	}
}

}
