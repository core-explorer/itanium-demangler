#pragma once
#include <string>
#include <string_view>
namespace swartalf {
enum class CliColorMode {
	uncolored,
	colors16,
	colors256,
	colors_rgb
};
extern CliColorMode colored_output;
namespace color {
constexpr inline std::string_view c(const std::string_view& text, bool colored) {
	return colored ? text : std::string_view {};
}

inline std::string_view c(const std::string_view& text) {
	return unsigned(colored_output) ? text : std::string_view {};
}
}

struct Decorator {
	std::string_view start;
	std::string_view content;
	std::string_view end;

	operator std::string() const;
};
Decorator bold(std::string_view text);
Decorator red(std::string_view text);
Decorator yellow(std::string_view text);
Decorator cyan(std::string_view text);
Decorator blue(std::string_view text);
Decorator green(std::string_view text);
Decorator magenta(std::string_view text);
Decorator bold_red(std::string_view text);
Decorator bold_yellow(std::string_view text);
Decorator bold_cyan(std::string_view text);
Decorator bold_blue(std::string_view text);
Decorator bold_green(std::string_view text);
Decorator bold_magenta(std::string_view text);
Decorator italic(std::string_view text);
std::string colorize(std::string_view message, std::string_view path, std::string_view severity);
void colored_print(FILE* stream, std::string_view message, std::string_view path, std::string_view severity);
void colored_println(FILE* stream, std::string_view message, std::string_view path, std::string_view severity);

std::string uncolorize(std::string_view message, std::string_view path, std::string_view severity);
void uncolored_print(FILE* stream, std::string_view message, std::string_view path, std::string_view severity);
void uncolored_println(FILE* stream, std::string_view message, std::string_view path, std::string_view severity);
}