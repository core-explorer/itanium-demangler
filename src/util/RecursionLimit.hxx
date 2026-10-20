#pragma once
#include <compare>
#include <cstddef>
namespace swartalf {
struct RecursionLimit {
	using value_type = size_t;
	value_type val;

	constexpr explicit RecursionLimit(value_type v) : val(v) {}

	constexpr RecursionLimit& operator--() {
		val--;
		return *this;
	}

	constexpr friend bool operator==(const RecursionLimit& l, value_type v) { return l.val == v; }
	constexpr friend std::strong_ordering operator<=>(const RecursionLimit& l, value_type v) { return l.val <=> v; }
};
}