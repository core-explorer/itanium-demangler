#include "cli_color256.hxx"
#include "cli_color.hxx"
#include "../util/format.hxx"
#include "../util/print.hxx"
#include <array>
namespace swartalf {

std::array<std::string_view, 5> dither {
    " ",
    "░",
    "▒",
    "▓",
    "█"};

std::string block_dither256(Color256 c) {
	unsigned best_i = 0;
	unsigned best_j = 0;
	unsigned best_k = 1;
	const auto& lut = color256;
	unsigned best_dist = 4 * distl2sq(c, lut[0]);

	for (unsigned i = 16; i < lut.size(); i++) {
		for (unsigned j = i; j < lut.size(); j++) {
			for (unsigned k = 0; k < 4; k++) {
				auto blended = fblend(lut[i], lut[j], float(k) * 0.2f);
				unsigned disti = distl2sq(lut[i], c);
				unsigned distj = distl2sq(lut[j], c);
				unsigned distb = distl2sq(blended, c);
				auto dist = disti + distj + 2 * distb;

				if (dist < best_dist) {

					best_i = i;
					best_j = j;
					best_k = k;
					best_dist = dist;
				}
			}
		}
	}

	if (best_k == 0) {
		return fmt::format("\033[38;5;{}m{}", best_j, dither.back());
	} else {
		return fmt::format("\033[38;5;{};48;5;{}m{}", best_i, best_j, dither[best_k]);
	}
}

std::string foreground_color16(Color256 current) {
	std::string color_sequence;

	unsigned fgIdx = bestMatchl1(current, std::span(color256.data(), 16));

	if (fgIdx >= 8) {
		color_sequence = fmt::format("\033[{}m", 90 + (fgIdx - 8));
	} else {
		color_sequence = fmt::format("\033[{}m", 30 + fgIdx);
	}

	return color_sequence;
}

std::string foreground_color256(Color256 current) {
	unsigned fgIdx = bestMatchl1(current, std::span(color256).subspan(16)) + 16;

	std::string color_sequence = fmt::format("\033[38;5;{}m", fgIdx);

	return color_sequence;
}

std::string foreground_color_rgb(Color256 current) {
	std::string color_sequence = fmt::format("\033[38;2;{};{};{}m", current.r, current.g, current.b);

	return color_sequence;
}

std::string background_color_rgb(Color256 current) {
	std::string color_sequence = fmt::format("\033[48;2;{};{};{}m", current.r, current.g, current.b);

	return color_sequence;
}

std::string fgbgcolor_rgb(Color256 fg, Color256 bg) {
	std::string color_sequence = fmt::format("\033[38;2;{};{};{};48;2;{};{};{}m", fg.r, fg.g, fg.b, bg.r, bg.g, bg.b);
	return color_sequence;
}

std::string foreground_color(Color256 current) {
	switch (colored_output) {
	case CliColorMode::uncolored:
		return {};
	case CliColorMode::colors16:
		return foreground_color16(current);

	case CliColorMode::colors256:
		return foreground_color256(current);
	case CliColorMode::colors_rgb:
		return foreground_color_rgb(current);
	}
	return {};
}

std::string format_element(const TextElement& te0, const ColorConfig& config) {
	bool only16 = false;
	bool only256 = false;

	switch (colored_output) {
	case CliColorMode::uncolored:
		return te0.text;
		break;
	case CliColorMode::colors16:
		only16 = true;
		break;
	case CliColorMode::colors256:
		only256 = true;
		break;
	case CliColorMode::colors_rgb:
		break;
	}

	int cs = only256 ? 2 : 1;
	if (only16) {
		cs = 0;
	}

	uint8_t factor = 16 + 16 * cs;
	Color256 outer_ov;
	switch (config.outer_overlay) {
	case OuterOverlay::none:
		outer_ov = "#000000"_rgb;
		break;
	case OuterOverlay::code:
		outer_ov = "#0000CD"_rgb;
		break;
	case OuterOverlay::rodata:
		outer_ov = "#CDCDCD"_rgb;
		break;
	case OuterOverlay::cie:
		outer_ov = "#C0D0CD"_rgb;
		break;
	case OuterOverlay::fde:
		outer_ov = "#D0C0CD"_rgb;
		break;
	default:
		outer_ov = "#000000"_rgb;
		break;
	}
	if (outer_ov != "#000000"_rgb) {
		if (config.outer_order == OuterOrder::even) {
			outer_ov.r += 16;
			outer_ov.b -= 16;
		} else {
			outer_ov.r -= 16;
			outer_ov.b += 16;
		}
	}

	Color256 inner_ov;
	switch (config.inner_overlay) {
	case InnerOverlay::none:
		inner_ov = "#000000"_rgb;
		break;
	case InnerOverlay::struct_:
		inner_ov = "#CDCDCD"_rgb;
		break;
	case InnerOverlay::function:
		inner_ov = "#5555CD"_rgb;
		break;
	default:
		inner_ov = "#000000"_rgb;
		break;
	}
	if (inner_ov != "#000000"_rgb) {
		if (config.inner_order == InnerOrder::even) {
			inner_ov.r += 16 * cs;
			inner_ov.g += 8 * cs;
			inner_ov.b -= 16 * cs;
		} else {
			inner_ov.r -= 16 * cs;
			inner_ov.g -= 8 * cs;
			inner_ov.b += 16 * cs;
		}
	}
	Color256 bg;
	Color256 fg = color256[7];
	bool bold = false;
	bool italic = false;

	switch (te0.element) {
	case Element::offset:
		fg = color256[6];
		bold = true;
		break;
	case Element::address:
		fg = color256[33];
		bold = true;
		break;
	case Element::label:
		italic = true;
		break;
	case Element::symbol:
		fg = color256[11];
		bold = true;
		break;
	case Element::expression:
		fg = color256[11];
		italic = true;
		break;
	case Element::enum_:
		fg = color256[15];

		break;
	case Element::mnemonic:
		bold = true;
		break;
	default:
		fg = color256[7];
		break;
	}

	if (outer_ov != "#000000"_rgb) {
		bg = blend(bg, outer_ov, factor);
		fg = blend(fg, outer_ov, factor);
	}

	if (inner_ov != "#000000"_rgb) {
		bg = blend(bg, inner_ov, factor);
		fg = blend(fg, inner_ov, factor);
	}

	std::string result;
	if (bold and not only16) {
		result += "\033[1m";
	}
	if (italic and not only16) {
		result += "\033[3m";
	}

	if (only16) {
		unsigned fgIdx = bestMatchl1(fg, std::span(color256.data(), 16));
		unsigned bgIdx = bestMatchl1(bg, std::span(color256.data(), 8));
		if (fgIdx >= 8) {
			if (bgIdx == 0) {
				result += fmt::format("\033[{}m", 90 + (fgIdx - 8));

			} else {
				result += fmt::format("\033[{};{}m", 90 + (fgIdx - 8), 40 + bgIdx);
			}
		} else {
			if (bgIdx == 0) {
				result += fmt::format("\033[{}m", 30 + fgIdx);
			} else {
				result += fmt::format("\033[{};{}m", 30 + fgIdx, 40 + bgIdx);
			}
		}
	} else if (only256) {
		unsigned fgIdx = bestMatchl1(fg, color256);
		unsigned bgIdx = bestMatchl1(bg, color256);

		if (bgIdx == 0) {
			result += fmt::format("\033[38;5;{}m", fgIdx);
		} else {
			result += fmt::format("\033[38;5;{};48;5;{}m", fgIdx, bgIdx);
		}
	} else {
		if (bg == "#000000"_rgb) {
			result += fmt::format("\033[38;2;{};{};{}m", fg.r, fg.g, fg.b);
		} else {
			result += fmt::format("\033[38;2;{};{};{};48;2;{};{};{}m", fg.r, fg.g, fg.b, bg.r, bg.g, bg.b);
		}
	}

	result += te0.text;
	for (size_t i = te0.text.size(); i < te0.pad_width; i++) {
		result += " ";
	}
	result += "\033[0m";
	return result;
}

void align(TextBlock& block) {
	std::vector<size_t> widths;
	for (auto& line : block) {
		for (size_t i = 0; i < line.elements.size(); i++) {
			if (i == widths.size()) {
				if (not line.elements[i].stretch) {
					widths.push_back(line.elements[i].text.size());
				} else {
					widths.push_back(0);
				}
			} else {
				if (not line.elements[i].stretch) {
					widths[i] = std::max(widths[i], line.elements[i].text.size());
				}
			}
		}
	}
	for (auto& line : block) {
		for (size_t i = 0; i < line.elements.size(); i++) {
			line.elements[i].pad_width = widths[i];
		}
		for (size_t j = line.elements.size(); j < widths.size(); j++) {
			line.elements.back().pad_width += widths[j];
		}
	}
}

void print(const TextBlock& block, ColorConfig config) {
	for (const auto& line : block) {
		for (const auto& elt : line.elements) {
			fmt::print("{}", format_element(elt, config));
		}

		fmt::print("\n");
	}
}

void print(const TextLine& line, ColorConfig config) {
	for (const auto& elt : line.elements) {
		fmt::print("{}", format_element(elt, config));
	}

	fmt::print("\n");
}

}