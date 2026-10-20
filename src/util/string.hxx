#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
namespace swartalf {

constexpr bool is_lower_ascii(char c) {
	return 'a' <= c and c <= 'z';
}

constexpr char convert_to_upper_ascii(char c) {
	return c + ('A' - 'a');
}

constexpr bool is_upper_ascii(char c) {
	return 'A' <= c and c <= 'Z';
}

constexpr char convert_to_lower_ascii(char c) {
	return c + ('a' - 'A');
}

inline void modify_to_upper(std::string& result) {
	for (auto& c : result) {
		if (is_lower_ascii(c)) {
			c = convert_to_upper_ascii(c);
		}
	}
}

inline void modify_to_lower(std::string& result) {
	for (auto& c : result) {
		if (is_upper_ascii(c)) {
			c = convert_to_lower_ascii(c);
		}
	}
}

std::optional<std::pair<int64_t, size_t>> parse_int(std::string_view str);
std::optional<std::pair<uint64_t, size_t>> parse_uint(std::string_view str);
std::optional<std::pair<uint64_t, size_t>> parse_uint_base16(std::string_view str);

inline std::optional<std::pair<std::string_view, std::string_view>> split(std::string_view input, char c) {
	auto pos = input.find(c);
	if (pos != 0 and pos != std::string_view::npos) {
		return std::make_pair(input.substr(0, pos), input.substr(pos + 1));
	}
	return std::nullopt;
}

}