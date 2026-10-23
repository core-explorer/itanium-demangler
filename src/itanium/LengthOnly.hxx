#pragma once
#include "Decoration.hxx"
#include <string>
#include <string_view>
namespace swartalf::itanium {
struct LengthOnly {
	unsigned length = 0;
	char first = '\0';
	char last = '\0';

	constexpr const char& back() const {
		return last;
	}

	constexpr LengthOnly() = default;
	constexpr LengthOnly(std::string_view s) : length(s.size()), first(s.empty() ? '\0' : s[0]), last(s.empty() ? '0' : s.back()) {}

	constexpr LengthOnly& operator=(std::string_view s) {
		*this = LengthOnly(s);
		return *this;
	}

	constexpr LengthOnly& operator+=(std::string_view s) {
		if (not s.empty()) {
			length += s.size();
			last = s.back();
		}
		return *this;
	}

	constexpr LengthOnly& operator+=(char c) {
		length += 1;
		last = c;

		return *this;
	}

	constexpr LengthOnly& operator+=(const LengthOnly& s) {
		if (not s.empty()) {
			length += s.size();
			last = s.back();
		}
		return *this;
	}

	constexpr size_t size() const {
		return length;
	}
	constexpr bool empty() const {
		return length == 0;
	}

	constexpr bool ends_with(char c) const {
		return last == c;
	}
	constexpr bool starts_with(char c) const {
		return first == c;
	}

	friend LengthOnly operator+(const LengthOnly& t, const LengthOnly& s) {
		auto a = t;
		a += s;
		return a;
	}
	friend LengthOnly operator+(std::string_view t, const LengthOnly& s) {
		LengthOnly a;
		a += t;
		a += s;
		return a;
	}
	friend LengthOnly operator+(const LengthOnly& t, std::string_view s) {
		auto a = t;
		a += s;
		return a;
	}

	void clear() {
		length = 0;
		first = '\0';
		last = '\0';
	}
};

struct LengthOnlyDecorator {
	using value_type = LengthOnly;

	constexpr LengthOnly decorate(Decoration, std::string_view tn, unsigned = 0) const {
		return LengthOnly(tn);
	}

	constexpr LengthOnly decorate(Decoration, const LengthOnly& ts, unsigned = 0) const {
		return ts;
	}
};

struct LengthOnlyEscaper {
	using value_type = LengthOnly;

	constexpr LengthOnly escape(std::string_view tn) const {
		return LengthOnly(tn);
	}
};

}