#include "ColorMode.hxx"
#include "cli_color.hxx"
#include <unistd.h>
#include <cstdlib>
#include <string>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

namespace swartalf {

bool is_a_tty(int fd) {
#if defined(__EMSCRIPTEN__) && defined(CDX_EMSCRIPTEN_ENV_NODE)
	switch (fd) {
	case 0:
		return EM_ASM_INT({
			return !!process.stdin.isTTY;
		});
	case 1:
		return EM_ASM_INT({
			return !!process.stdout.isTTY;
		});
	case 2:
		return EM_ASM_INT({
			return !!process.stderr.isTTY;
		});
	default:
		return false;
	}
#else
	return isatty(fd);
#endif
}

bool is_no_color_active() {
	int active = 0;
#ifdef __EMSCRIPTEN__
#if defined(CDX_EMSCRIPTEN_ENV_NODE)
	active = EM_ASM_INT({
		return process.env.NO_COLOR ? 1 : 0;
	});

#endif
#else

	const char* no_color_var = getenv("NO_COLOR");
	if (no_color_var != nullptr && no_color_var[0] != '\0') {
		active = 1;
	}
#endif
	return active;
}

void set_color_mode(ColorMode color_mode) {
	if (color_mode == ColorMode::always) {
		colored_output = CliColorMode::colors_rgb;
	} else if (color_mode == ColorMode::never) {
		colored_output = CliColorMode::uncolored;
	} else if (color_mode == ColorMode::always16) {
		colored_output = CliColorMode::colors16;
	} else if (color_mode == ColorMode::always256) {
		colored_output = CliColorMode::colors256;
	} else {
		bool color = not is_no_color_active();

		bool colored_output_enable = color and is_a_tty(fileno(stdout)) and is_a_tty(fileno(stderr));
		if (colored_output_enable) {
			colored_output = CliColorMode::colors_rgb;
		} else {
			colored_output = CliColorMode::uncolored;
		}
	}
}

void init_color_mode() {
	set_color_mode(ColorMode::automatic);
}

}