#pragma once
#include "ascii.hxx"
#include "../itanium/Decoration.hxx"
#include "../web/HtmlFormat.hxx"
#include "../web/Hyperlink.hxx"
#include "../web/PageConfig.hxx"

namespace swartalf {

enum class NameResolution : uint8_t {
	local,
	relative,
	global
};

inline std::string_view getUntemplatedName(std::string_view fullname) {
	auto pos = fullname.find('<');

	while (pos != std::string_view::npos and pos >= 8 and fullname.substr(pos - 8, 10) == "operator<<") {
		pos = fullname.find('<', pos + 2);
	}
	while (pos != std::string_view::npos and pos >= 8 and fullname.substr(pos - 8, 9) == "operator<") {
		pos = fullname.find('<', pos + 1);
	}
	if (pos == std::string_view::npos) {
		return fullname;
	}
	return fullname.substr(0, pos);
}

inline swartalf::Unsanitized<std::string_view> getUntemplatedName(swartalf::Unsanitized<std::string_view> fullname) {
	return swartalf::Unsanitized(getUntemplatedName(fullname.get_value()));
}

struct XmlDecorator {
	using value_type = std::string;
	static std::string decorate(Decoration deco, std::string_view tn, unsigned level = 0);
};
struct XmlEscaper {
	static std::string escape(std::string_view tn) {
		return escape_xml(tn);
	}
	static std::string escape(swartalf::Unsanitized<std::string_view> tn) {
		return escape_xml(tn);
	}
};
struct XmlLinker {
	std::string_view prefix;
	PageConfig cfg;

	bool can_link() const {
		return true;
	}

	XmlLinker() = default;
	XmlLinker(std::string_view p, const PageConfig& c) :
	    prefix(p), cfg(c) {}
	XmlLinker(const PageConfig& c, std::string_view p) :
	    prefix(p), cfg(c) {}

	void add_replacement(const std::string&, const std::string&) {
	}

	void drop_replacement(const std::string&) {
	}
};

struct CuXmlLinker {
	bool can_link() const {
		return true;
	}
	std::string_view prefix;
	PageConfig cfg;

	CuXmlLinker(std::string_view p, const PageConfig& c) :
	    prefix(p), cfg(c) {}

	void add_replacement(const std::string&, const std::string&) {
	}

	void drop_replacement(const std::string&) {
	}
};

struct AsciiDecorator {
	using value_type = std::string;
	static constexpr value_type decorate(Decoration, std::string_view tn, unsigned = 0) {
		return std::string(tn);
	}
};

struct AsciiEscaper {
	static std::string escape(std::string_view tn) {
		return swartalf::escape_ascii(tn);
	}
	static std::string escape(Unsanitized<std::string_view> tn) {
		return swartalf::escape_ascii(tn);
	}
};

template <typename StringType>
struct NopLinker {
	using value_type = StringType;
	template <typename T, typename... Args>
	static value_type link_to(const T&, value_type&& tn, Args&&...) {
		return tn;
	}

	static constexpr bool can_link() {
		return false;
	}

	template <typename T, typename... Args>
	static value_type link_to(const T&, const value_type& tn, Args&&...) {
		return tn;
	}

	template <typename T, typename C, typename... Args>
	static std::string link_to(const T&, const C& tn, Args&&...) {
		return fmt::format("{}", tn);
	}

	void add_replacement(const std::string&, const std::string&) {
	}

	void drop_replacement(const std::string&) {
	}
};

using AsciiLinker = NopLinker<std::string>;

template <typename D, typename E, typename L>
struct Formatter {
	using Decorator = D;
	using Escaper = E;
	using Linker = L;

	Decorator decorator;
	Escaper escaper;
	Linker linker;
	using value_type = typename Decorator::value_type;

	template <typename... Args>
	Formatter(Args&&... args) :
	    linker(std::forward<Args>(args)...) {}

	template <typename... Args>
	auto link_to(Args&&... args) const {
		return linker.link_to(std::forward<Args>(args)...);
	}

	bool can_link() const {
		return linker.can_link();
	}

	template <typename... Args>
	auto escape(Args&&... args) const {
		return escaper.escape(std::forward<Args>(args)...);
	}

	template <typename... Args>
	auto decorate(Args&&... args) const {
		return decorator.decorate(std::forward<Args>(args)...);
	}
	template <typename... Args>
	auto decorate(Args&&... args) {
		return decorator.decorate(std::forward<Args>(args)...);
	}

	template <typename... Args>
	auto add_replacement(Args&&... args) {
		return linker.add_replacement(std::forward<Args>(args)...);
	}

	template <typename... Args>
	auto drop_replacement(Args&&... args) {
		return linker.drop_replacement(std::forward<Args>(args)...);
	}
};

using XmlFormatter = Formatter<XmlDecorator, XmlEscaper, XmlLinker>;
using CuXmlFormatter = Formatter<XmlDecorator, XmlEscaper, CuXmlLinker>;

using AsciiFormatter = Formatter<AsciiDecorator, AsciiEscaper, AsciiLinker>;

}
