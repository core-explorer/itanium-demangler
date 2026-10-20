#include "FiltArgs.hxx"
namespace swartalf::filt {

constexpr std::string_view o(OptionName v) {
	return to_string_view(v);
}

}
namespace swartalf {

using namespace swartalf::filt;
using enum OptionName;
using enum OptionValueType;
using enum OptionMultiplicity;

ArgParser makeFiltParser() {
	ArgParser parser;

	parser.argument("name", multiple, ArgumentHandling::disabled);
	parser.option(o(help), "", "show this usage information", ArgumentHandling::disabled);
	parser.option(o(version), "", "show version", ArgumentHandling::disabled);

	parser.option(o(color), "", "use colored output, arg must be one of always|never|auto, defaults to auto", required);
	parser.option(o(remangle), "-r", "demangle and remangle again");
	parser.option(o(grammar), "-g", "dump grammar");
	parser.option(o(partial), "-p", "allow partial parsing");
	parser.option(o(light_mode), "-L", "change color scheme to light mode");
	parser.option(o(short_), "-s", "use a short representation");
	parser.option(o(subs), "-S", "preserve substitutions in output");
	parser.option(o(templ), "-T", "preserve template parameters in output");
	parser.option(o(pack), "-P", "preserve packs in output");
	parser.option(o(return_), "-R", "do not show return types in output");
	parser.option(o(expression), "-X", "do not show expressions");
	parser.option(o(trailing), "-t", "show trailing return types in output");
	parser.option(o(function_signature), "-F", "do not show function signatures in output");
	parser.option(o(category), "-o", "category (default is mangled_name)", required);
	parser.option(o(dialect), "-a", "demangling dialect, gnu or llvm", required);
	parser.option(o(debug), "-d", "show verbose ouput");
	parser.option(o(length), "-l", "maximum length for demangled name", required);
	parser.option(o(function_params), "", "maximum number of function parameters to display", required);
	parser.option(o(template_params), "", "maximum number of template parameters to display", required);
	parser.option(o(check), "-c", "compare against demangler from linked C++ runtime library");
	parser.option(o(terminal_width), "-W", "limit output to integer multiple of terminal width", required);

	return parser;
}
}