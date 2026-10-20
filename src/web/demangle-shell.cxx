#include "HtmlFormat.hxx"
#include "../itanium/FiltAnalyzer.hxx"
#include "../itanium/FormatDemangleLimit.hxx"
#include "../itanium/ItaniumDemangler.hxx"
#include "../itanium/ParseName.hxx"
#include "../util/Stream.hxx"
#include "../util/string.hxx"
#include "../util/version.hxx"
#include <atomic>
#include <forward_list>
#include <list>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#ifdef __EMSCRIPTEN__
#include <emscripten/html5.h>
#include <emscripten.h>
#endif

using namespace swartalf;

struct State {
	std::atomic<size_t> done = 0;
	Stream os;

	std::string autonav;

	bool darkMode = false;
	FiltAnalyzerConfig config;
};

std::unique_ptr<State> ustate;

namespace swartalf {
using namespace swartalf::itanium;

void demangleText(std::string_view mangled) {
	std::string demangled;

	static ItaniumDemangler demangler;
	auto cat = itanium::ProductionType::any_mangled_name;
	size_t indent = 0;
	RecursionLimit max_recursion {1000};
	auto& rules = demangler.rules;
	bool must_consume = true;
	auto parsed = parse_as(mangled, cat, max_recursion, indent, rules, must_consume);
	SkipSpec skip;
	DemanglingDialect dv;
	auto& config = ustate->config;

	if (parsed) {
		SubstitutionTable su = build_substitution_table(*parsed);
		auto subs = view(su);
		NestedTemplateParams nt;
		auto num_params = get_max_num_function_and_template_parameters(*parsed, subs, nt, max_recursion);
		dv.max_function_parameters = std::min(dv.max_function_parameters, num_params.first);
		dv.max_template_parameters = std::min(dv.max_template_parameters, num_params.second);

		std::expected<std::string, ParseError> eraw;
		if (config.length < 100000) {
			auto elraw = format_demangled_ascii_limit(config.length, *parsed, subs, nt, dv, skip, max_recursion);
			if (elraw) {
				eraw = elraw->first;
				dv = elraw->second;
			} else {
				eraw = std::unexpected(elraw.error());
			}
		} else {
			eraw = format_demangled_ascii(*parsed, subs, nt, dv, skip, max_recursion);
		}
		if (eraw) {
			demangled = escape_xml(*eraw);
		}

		std::expected<std::string, ParseError> epretty;
		auto light_mode = not ustate->darkMode;
		if (light_mode) {
			epretty = format_demangled_html_light(*parsed, subs, nt, dv, skip, max_recursion);

		} else {
			epretty = format_demangled_html_dark(*parsed, subs, nt, dv, skip, max_recursion);
		}
		if (epretty) {
			demangled = *epretty;
		}
	}

	auto& escaped = demangled;
	EM_ASM_({ document.getElementById('demangled').innerHTML = UTF8ToString($0, $1); }, escaped.data(), escaped.size());
}

void setDarkMode(bool value) {
	ustate->darkMode = value;
}

void setMaxLength(int nrows, int ncols) {
	ustate->config.length = nrows * ncols;
}

}

#ifdef __EMSCRIPTEN__

extern "C" {
EMSCRIPTEN_KEEPALIVE
void setDarkMode(int val) {
	swartalf::setDarkMode(val);
}
}

extern "C" {
EMSCRIPTEN_KEEPALIVE
void demangleText(const char* name1, int nlength1) {
	swartalf::demangleText(std::string_view(name1, nlength1));
}
}

extern "C" {
EMSCRIPTEN_KEEPALIVE
void setMaxLength(int nrows, int ncols) {
	swartalf::setMaxLength(nrows, ncols);
}
}
#endif

void main_loop(void* arg) {
	(void)arg;
}

void init_loop(void*) {
#ifdef __EMSCRIPTEN__
	emscripten_cancel_main_loop();

	EM_ASM_({
		Module.demangleText = Module.cwrap('demangleText', null, [ 'number', 'number' ]);
		Module.setMaxLength = Module.cwrap('setMaxLength', null, [ 'number', 'number' ]);
		Module.setDarkMode = Module.cwrap('setDarkMode', null, ['number']);
		is_emscripten_initialized = 1;
		set_darkmode(darkmode);
		if (pending_demangle) {
			document.getElementById('mangled').value = pending_demangle;
			demangle_text(pending_demangle);
			pending_demangle = null;
		}
	});
	auto version = get_version_string();
	if (not version.empty()) {
		EM_ASM_({ document.getElementById('version').innerHTML = UTF8ToString($0, $1);document.getElementById('version').style.visibility = ""; }, version.data(), version.size());
	}
	auto source = get_source_string();
	if (not source.empty()) {
		EM_ASM_({ document.getElementById('source-link').href = UTF8ToString($0, $1);document.getElementById('source-span').style.visibility = ""; }, source.data(), source.size());
	}
	emscripten_set_main_loop_arg(main_loop, ustate.get(), 0, true);
#endif
}

int main(int, char**) {
	ustate = std::make_unique<State>();
#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop_arg(init_loop, nullptr, 0, false);
#else
	init_loop(nullptr);
#endif

#ifdef __EMSCRIPTEN__
#else

	float max_fps = 60;
	auto frame_time = std::chrono::nanoseconds(long(int(1.0e9f / max_fps)));
	while (not ustate->done) {
		auto tbegin = std::chrono::steady_clock::now();
		main_loop(ustate.get());
		auto tend = std::chrono::steady_clock::now();
		auto dur = tend - tbegin;
		if (frame_time > dur) {
			std::this_thread::sleep_for(frame_time - dur);
		}
	}
#endif
	return 0;
}
