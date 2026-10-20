#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
namespace swartalf {

constexpr bool isDigitBase36(char c) {
	return (c >= '0' and c <= '9') or (c >= 'A' and c <= 'Z');
}

constexpr unsigned base36Value(char c) {
	if (c >= '0' and c <= '9') {
		return c - '0';
	} else if (c >= 'A' and c <= 'Z') {
		return 10 + (c - 'A');
	} else {
		return ~0u;
	}
}

inline std::optional<std::pair<uint64_t, size_t>> parseNumberBase36(std::string_view mangled_name) {
	if (mangled_name.empty() or not isDigitBase36(mangled_name[0])) {
		return std::nullopt;
	}
	uint64_t value = 0;
	size_t idx = 0;
	while (idx < mangled_name.size() and isDigitBase36(mangled_name[idx])) {
		value = value * 36 + base36Value(mangled_name[idx]);
		idx++;
	}
	return std::make_pair(value, idx + 1);
}

std::string encodeNumberBase36(uint64_t value);
std::string encodeItaniumIndexBase36(uint64_t value);

inline std::optional<std::pair<uint64_t, size_t>> parseItaniumIndexBase36(std::string_view mangled_name) {
	if (mangled_name.empty()) {
		return std::nullopt;
	}
	if (mangled_name[0] == '_') {
		return std::make_pair(0, 1);
	}
	auto value_opt = parseNumberBase36(mangled_name);
	if (not value_opt) {
		return std::nullopt;
	}
	auto [value, len] = *value_opt;
	auto idx = len - 1;
	if (idx >= mangled_name.size() or mangled_name[idx] != '_') {
		return std::nullopt;
	}

	return std::make_pair(value + 1, len);
}

}