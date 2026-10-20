#pragma once
#include "../util/Stream.hxx"
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
namespace swartalf {
namespace html {
struct InstantTag {
	std::string name;
	InstantTag() = default;

	InstantTag(const std::string& n) :
	    name(n) {}
	InstantTag operator()(const std::string& key, const std::string& value) const {
		InstantTag copy = *this;
		copy.attributes.push_back({key, value});
		return copy;
	}

	std::vector<std::pair<std::string, std::string>> attributes;
};

template <typename T>
struct ContentTag;
struct HtmlTag {
	std::string name;
	HtmlTag() = default;

	HtmlTag(const std::string& n) :
	    name(n) {}

	std::vector<std::pair<std::string, std::string>> attributes;

	template <typename T>
	ContentTag<T> operator()(const T& content) const;
	ContentTag<std::string> operator()(const char* content) const;

	HtmlTag operator()(const std::string& key, const std::string& value) const {
		HtmlTag copy = *this;
		copy.attributes.push_back({key, value});
		return copy;
	}
	[[nodiscard]] std::string open() const {
		std::stringstream stream;
		stream << "<" << name;
		for (const auto& [key, value] : attributes) {
			stream << " " << key << "=\"" << value << "\"";
		}
		stream << ">";
		return stream.str();
	}
	[[nodiscard]] std::string close() const {
		return "</" + name + ">";
	}
	auto operator+() const {
		return open();
	}
	auto operator-() const {
		return close();
	}
};

template <typename T>
struct ContentTag {
	ContentTag() = default;

	ContentTag(const HtmlTag& t, const T& c) :
	    tag(t), content(c) {}

	HtmlTag tag;
	T content;

	operator std::string() const;
};

template <typename T>
ContentTag<T> HtmlTag::operator()(const T& content) const {
	return ContentTag<T> {*this, content};
}

inline ContentTag<std::string> HtmlTag::operator()(const char* content) const {
	return ContentTag<std::string> {*this, content};
}

inline std::ostream& operator<<(std::ostream& stream, const InstantTag& tag) {
	stream << "<" << tag.name;
	for (const auto& [key, value] : tag.attributes) {
		stream << " " << key << "=\"" << value << "\"";
	}
	stream << "/>";
	return stream;
}

template <typename T>
ContentTag<T>::operator std::string() const {
	std::ostringstream os;
	os << *this;
	return os.str();
}

template <typename T>
std::string operator+(const std::string& lhs, const ContentTag<T>& rhs) {
	return lhs + static_cast<std::string>(rhs);
}

template <typename T>
std::string operator+(const ContentTag<T>& lhs, const std::string& rhs) {
	return static_cast<std::string>(lhs) + rhs;
}

inline std::ostream& operator<<(std::ostream& stream, const HtmlTag& tag) {
	stream << "<" << tag.name;
	for (const auto& [key, value] : tag.attributes) {
		stream << " " << key << "=\"" << value << "\"";
	}
	stream << ">";
	return stream;
}

template <typename T>
inline std::ostream& operator<<(std::ostream& stream, const ContentTag<T>& tag) {
	stream << "<" << tag.tag.name;
	for (const auto& [key, value] : tag.tag.attributes) {
		stream << " " << key << "=\"" << value << "\"";
	}
	stream << ">" << tag.content << "</" << tag.tag.name << ">";
	return stream;
}

inline swartalf::Stream& operator<<(swartalf::Stream& stream, const HtmlTag& tag) {
	stream << "<" << tag.name;
	for (const auto& [key, value] : tag.attributes) {
		stream << " " << key << "=\"" << value << "\"";
	}
	stream << ">";
	return stream;
}

template <typename T>
inline swartalf::Stream& operator<<(swartalf::Stream& stream, const ContentTag<T>& tag) {
	stream << "<" << tag.tag.name;
	for (const auto& [key, value] : tag.tag.attributes) {
		stream << " " << key << "=\"" << value << "\"";
	}
	stream << ">" << tag.content << "</" << tag.tag.name << ">";
	return stream;
}

}

}

inline namespace tags {
namespace html = swartalf::html;
using HtmlTag = html::HtmlTag;
inline constexpr std::string_view br {"<br/>\n"};
inline constexpr std::string_view nbsp {"&nbsp;"};

inline const std::string id = "id";
inline const std::string clazz = "class";
inline const std::string style = "style";
inline const std::string href = "href";
inline const std::string title = "title";
inline const std::string src = "src";
inline const std::string sorttable_customkey = "sorttable_customkey";

inline const HtmlTag div {"div"};
inline const HtmlTag div_ {"div"};
inline const HtmlTag span {"span"};
inline const HtmlTag h1 {"h1"};
inline const HtmlTag h2 {"h2"};
inline const HtmlTag h3 {"h3"};
inline const HtmlTag h4 {"h4"};
inline const HtmlTag a {"a"};
inline const HtmlTag pre {"pre"};
inline const HtmlTag code {"code"};
inline const HtmlTag script {"script"};

inline const HtmlTag a_data = a(clazz, "data");
inline const HtmlTag a_code = a(clazz, "code");
inline const HtmlTag span_addr = span(clazz, "addr");
inline const HtmlTag span_data = span(clazz, "data");
inline const HtmlTag span_code = span(clazz, "code");
inline const HtmlTag div_memory = div(clazz, "memory");
inline const HtmlTag div_elfsegments = div(clazz, "elfsegments");
inline const HtmlTag div_elfsections = div(clazz, "elfsections");

inline const HtmlTag div_segment = div(clazz, "segment");
inline const HtmlTag div_struct = div(clazz, "struct");
inline const HtmlTag div_alloc = div(clazz, "alloc");
inline const HtmlTag div_alloc_freed = div(clazz, "alloc_freed");
inline const HtmlTag span_perm = span(clazz, "perm");
inline const HtmlTag span_null = span(clazz, "null");
inline const HtmlTag span_string = span(clazz, "string");
inline const HtmlTag span_symbol = span(clazz, "symbol");
inline const HtmlTag code_keyword = code(clazz, "keyword");
inline const HtmlTag code_type_keyword = code(clazz, "type_keyword");
inline const HtmlTag code_const_keyword = code(clazz, "const_keyword");
inline const HtmlTag code_ppkeyword = code(clazz, "ppkeyword");
inline const HtmlTag span_brace = span(clazz, "brace");
inline const HtmlTag span_noise = span(clazz, "noise");
inline const HtmlTag span_idiosyncratic = span(clazz, "idiosyncratic");

inline const std::array<HtmlTag, 4> span_braces = {
    span(clazz, "braces4"),
    span(clazz, "braces1"),
    span(clazz, "braces2"),
    span(clazz, "braces3")};

inline const HtmlTag table {"table"};
inline const HtmlTag table_sortable = table(clazz, "sortable");
inline const HtmlTag thead {"thead"};
inline const HtmlTag tbody {"tbody"};
inline const HtmlTag tr {"tr"};
inline const HtmlTag th {"th"};
inline const HtmlTag td {"td"};

inline const HtmlTag td_num = td(clazz, "num");
inline const HtmlTag td_addr = td(clazz, "addr");
inline const HtmlTag td_desc = td(clazz, "desc");
inline const HtmlTag td_right = td(clazz, "cright");
inline const HtmlTag td_left = td(clazz, "cleft");

inline auto brace(std::string_view txt) {
	return span_brace(txt);
}

inline auto braces(std::string_view txt, unsigned level) {
	return span_braces[level % 4](txt);
}

inline auto keyword(std::string_view txt) {
	return code_keyword(txt);
}
inline auto type_keyword(std::string_view txt) {
	return code_type_keyword(txt);
}
inline auto const_keyword(std::string_view txt) {
	return code_const_keyword(txt);
}
}
