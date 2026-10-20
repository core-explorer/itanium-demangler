#include "string.hxx"
#include <charconv>

namespace swartalf {
std::optional<std::pair<int64_t, size_t>> parse_int(std::string_view str) {
	int64_t value = 0;
	auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
	if (ec == std::errc()) {
		return std::make_pair(value, ptr - str.data());
	}
	return std::nullopt;
}

std::optional<std::pair<uint64_t, size_t>> parse_uint(std::string_view str) {
	uint64_t value = 0;
	auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
	if (ec == std::errc()) {
		return std::make_pair(value, ptr - str.data());
	}
	return std::nullopt;
}

std::optional<std::pair<uint64_t, size_t>> parse_uint_base16(std::string_view str) {
	uint64_t value = 0;
	auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value, 16);
	if (ec == std::errc()) {
		return std::make_pair(value, ptr - str.data());
	}
	return std::nullopt;
}

}