#pragma once

#include "../util/Unsanitized.hxx"
#include "../util/format.hxx"
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
namespace swartalf {

inline std::string url_encode(std::string_view value) {
	std::string escaped;
	escaped.reserve(value.size());
	for (uint8_t c : value) {
		if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
			escaped += char(c);
		} else {
			escaped += fmt::format("%{:02x}", unsigned(c));
		}
	}

	return escaped;
}

constexpr unsigned get_hex_val(char c) {
	if (c >= 'a') {
		return c - 'a' + 10;
	} else if (c >= 'A') {
		return c - 'A' + 10;
	} else {
		return c - '0';
	}
}

inline swartalf::Unsanitized<std::string> url_decode(std::string_view value) {
	std::string unescaped;
	unescaped.reserve(value.size());
	for (decltype(value.begin()) it = value.begin(); it != value.end(); ++it) {
		if (*it == '%') {
			if (std::distance(it, value.end()) < 3) {
				break;
			}

			unsigned hex_val = get_hex_val(it[1]) * 16u + get_hex_val(it[2]);
			auto c = static_cast<char>(hex_val);
			unescaped.push_back(c);
			it += 2;
		} else {
			unescaped.push_back(*it);
		}
	}
	return Unsanitized(std::move(unescaped));
}

inline std::string url_encode(swartalf::Unsanitized<std::string_view> value) {
	return url_encode(value.get_value());
}

inline swartalf::Unsanitized<std::string> url_decode(swartalf::Unsanitized<std::string_view> value) {
	return url_decode(value.get_value());
}

struct HtmlTagNone {
	static constexpr std::string_view tag() {
		return "";
	}
};

struct HtmlTagTd {
	static constexpr std::string_view tag() {
		return "td";
	}
};

struct HtmlTagDd {
	static constexpr std::string_view tag() {
		return "dd";
	}
};

struct HtmlTagDt {
	static constexpr std::string_view tag() {
		return "dt";
	}
};

struct HtmlTagSpan {
	static constexpr std::string_view tag() {
		return "span";
	}
};

constexpr bool needs_escape(std::string_view sv) {
	for (auto c : sv) {
		if (c == '<' or c == '>' or c == '&' or c == '"') {
			return true;
		}
	}
	return false;
}

inline std::string escape_xml(std::string_view sv) {
	std::string result;
	result.reserve(sv.size());
	for (size_t i = 0; i < sv.size(); i++) {
		uint8_t c = sv[i];

		if ((c < 32 and c != '\n') or c == 127) {
			continue;
		}
		switch (c) {
		case '<':
			result += "&lt;";
			break;
		case '>':
			result += "&gt;";
			break;
		case '&':
			result += "&amp;";
			break;
		case '\n':
			result += "<br/>\n";
			break;
		case '"':
			result += "&quot;";
			break;
		default:
			result += c;
		}
	}
	constexpr std::array<char8_t, 4> lre = {u8"\u202A"}; // Left-to-Right Embedding
	constexpr std::array<char8_t, 4> rle = {u8"\u202B"}; // Right-to-Left Embedding
	constexpr std::array<char8_t, 4> pdf = {u8"\u202C"}; // Pop Directional Formatting
	constexpr std::array<char8_t, 4> lro = {u8"\u202D"}; // Left-to-Right Override
	constexpr std::array<char8_t, 4> rlo = {u8"\u202E"}; // Right-to-Left Override
	constexpr std::array<char8_t, 4> lri = {u8"\u2066"}; // Left-to-Right isolate
	constexpr std::array<char8_t, 4> rli = {u8"\u2067"}; // Right-to-Left isolate
	constexpr std::array<char8_t, 4> fsi = {u8"\u2068"}; // First Strong isolate
	constexpr std::array<char8_t, 4> pdi = {u8"\u2069"}; // Pop Directional isolate

	constexpr std::array<std::array<char8_t, 4>, 9> control = {lre, rle, pdf, lro, rlo, lri, rli, fsi, pdi};
	for (const auto& sec : control) {
		auto bidi = std::string_view(reinterpret_cast<const char*>(sec.data()), sec.size() - 1);
		auto pos = result.find(bidi);
		while (pos != std::string::npos) {
			result.replace(pos, sec.size(), "");
			pos = result.find(bidi);
		}
	}
	return result;
}

inline std::string escape_xml(swartalf::Unsanitized<std::string_view> sv) {
	return escape_xml(sv.get_value());
}

inline std::string escape_xml(const swartalf::Unsanitized<std::string>& sv) {
	return escape_xml(sv.get_value());
}

struct Html {
	static std::string escape(swartalf::Unsanitized<std::string_view> sv) {
		return escape_xml(sv.get_value());
	}

	static std::string escape(const swartalf::Unsanitized<std::string>& sv) {
		return escape_xml(sv.get_value());
	}
};

namespace html {
template <typename... Args>
inline std::string format(fmt::format_string<decltype(wrap(std::declval<Args>()))...> fmt, Args&&... args) {
	return fmt::format(fmt, wrap(std::forward<Args>(args))...);
}
}

struct HtmlPrinter {
	template <typename... Args>
	static std::string format(fmt::format_string<decltype(wrap(std::declval<Args>()))...> fmt, Args&&... args) {
		return fmt::format(fmt, wrap(std::forward<Args>(args))...);
	}
};

}
