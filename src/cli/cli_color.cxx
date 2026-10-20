#include "cli_color.hxx"
#include "../util/Stream.hxx"
#include "../util/print.hxx"
namespace swartalf {
CliColorMode colored_output = CliColorMode::uncolored;
Decorator red(std::string_view text) {
	return Decorator {"\033[31m", text, "\033[0m"};
}

Decorator::operator std::string() const {
	return fmt::format("{}{}{}", start, content, end);
}

Decorator blue(std::string_view text) {
	return Decorator {"\033[34m", text, "\033[0m"};
}

Decorator green(std::string_view text) {
	return Decorator {"\033[32m", text, "\033[0m"};
}
Decorator italic(std::string_view text) {
	return Decorator {"\033[3m", text, "\033[23m"};
}
Decorator bold_blue(std::string_view text) {
	return Decorator {"\033[1;34m", text, "\033[0m"};
}

Decorator bold_green(std::string_view text) {
	return Decorator {"\033[1;32m", text, "\033[0m"};
}

Decorator bold_red(std::string_view text) {
	return Decorator {"\033[1;31m", text, "\033[0m"};
}
Decorator yellow(std::string_view text) {
	return Decorator {"\033[33m", text, "\033[0m"};
}
Decorator bold_yellow(std::string_view text) {
	return Decorator {"\033[1;33m", text, "\033[0m"};
}
Decorator cyan(std::string_view text) {
	return Decorator {"\033[36m", text, "\033[0m"};
}
Decorator magenta(std::string_view text) {
	return Decorator {"\033[35m", text, "\033[0m"};
}
Decorator bold_cyan(std::string_view text) {
	return Decorator {"\033[1;36m", text, "\033[0m"};
}
Decorator bold_magenta(std::string_view text) {
	return Decorator {"\033[1;35m", text, "\033[0m"};
}
Decorator bold(std::string_view text) {
	return Decorator {"\033[1m", text, "\033[0m"};
}

inline swartalf::Stream& operator<<(swartalf::Stream& os, const Decorator& d) {
	os << d.start << d.content << d.end;
	return os;
}

std::string colorize(std::string_view message, std::string_view path, std::string_view severity) {
	swartalf::Stream oss;
	if (not path.empty()) {
		auto dpath = bold(path);
		oss << dpath.start << dpath.content << ": " << dpath.end;
	}
	if (not severity.empty()) {
		if (severity == "error") {
			auto dseverity = bold_red(severity);
			oss << dseverity.start << dseverity.content << ": " << dseverity.end;
		} else if (severity == "warning") {
			auto dseverity = bold_yellow(severity);
			oss << dseverity.start << dseverity.content << ": " << dseverity.end;
		} else {
			oss << severity << ": ";
		}
	}
	oss << message;
	return oss.str();
}
void colored_print(FILE* stream, std::string_view message, std::string_view path, std::string_view severity) {
	print(stream, "{}", colorize(message, path, severity));
}
void colored_println(FILE* stream, std::string_view message, std::string_view path, std::string_view severity) {
	println(stream, "{}", colorize(message, path, severity));
}

std::string uncolorize(std::string_view message, std::string_view path, std::string_view severity) {
	swartalf::Stream oss;
	if (not path.empty()) {
		oss << path << ": ";
	}
	if (not severity.empty()) {
		oss << severity << ": ";
	}
	oss << message;
	return oss.str();
}
void uncolored_print(FILE* stream, std::string_view message, std::string_view path, std::string_view severity) {
	print(stream, "{}", uncolorize(message, path, severity));
}
void uncolored_println(FILE* stream, std::string_view message, std::string_view path, std::string_view severity) {
	println(stream, "{}", uncolorize(message, path, severity));
}

}