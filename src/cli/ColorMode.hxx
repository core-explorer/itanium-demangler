#pragma once
#include <optional>
#include <string_view>
namespace swartalf {
enum class ColorMode {
	automatic,
	never,
	always,
	always16,
	always256,
};

constexpr std::string_view to_string_view(ColorMode mode) {
	switch (mode) {
	case ColorMode::automatic:
		return "auto";
	case ColorMode::never:
		return "never";
	case ColorMode::always:
		return "always";
	case ColorMode::always16:
		return "always16";
	case ColorMode::always256:
		return "always256";
	}
}

constexpr std::optional<ColorMode> to_ColorMode(std::string_view mode) {
	if (mode == "auto") {
		return ColorMode::automatic;
	} else if (mode == "never") {
		return ColorMode::never;
	} else if (mode == "always") {
		return ColorMode::always;
	} else if (mode == "always16") {
		return ColorMode::always16;
	} else if (mode == "always256") {
		return ColorMode::always256;
	} else {
		return std::nullopt;
	}
}
void init_color_mode();
void set_color_mode(ColorMode mode);

}