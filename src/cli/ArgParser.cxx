#include "ArgParser.hxx"
#include "cli_color.hxx"
#include "../util/Stream.hxx"
#include "../util/format.hxx"
#include "../util/print.hxx"
namespace swartalf {

std::string ArgParser::get_usage() const {
	const char bold[] = "\033[1m";
	const char reset[] = "\033[0m";
	const char underline[] = "\033[4m";
	bool b = unsigned(swartalf::colored_output);
	auto c = [&b](std::string_view text) {
		return b ? text : std::string_view {};
	};
	Stream os;

	if (not expectedArgs.empty() and expectedArgs.front().skip == ArgumentSkip::skipArg) {
		os << fmt::format("Usage: {} {} [options]", programName, expectedArgs[0].name);
	} else {
		os << fmt::format("Usage: {} [options]", programName);
	}

	for (const auto& arg : expectedArgs) {
		if (arg.skip == ArgumentSkip::skipArg) {
			continue;
		}
		os << fmt::format(" {}{}{}", c(underline), arg.name, c(reset));
	}
	for (const auto& opt : expectedOptions) {
		if (opt.required == OptionRequirement::mandatory and opt.argument == OptionValueType::required) {
			os << fmt::format(" --{}={}{}{}", opt.long_name, c(underline), "arg", c(reset));
		}
	}
	os << '\n';
	os << "Options:" << '\n';

	for (const auto& eopt : expectedOptions) {
		if (eopt.short_name.empty()) {
			os << fmt::format("{}--{}{}", c(bold), eopt.long_name, c(reset));

		} else {
			os << fmt::format("{}{}{}, {}--{}{}", c(bold), eopt.short_name, c(reset), c(bold), eopt.long_name, c(reset));
		}
		if (eopt.argument == OptionValueType::required) {
			os << fmt::format("={}{}{}", c(underline), "arg", c(reset));
		}
		os << '\n';
		os << fmt::format("    {}\n", eopt.description);
	}
	return os.str();
}

void ArgParser::print_usage(FILE* stream) const {
	auto usage = get_usage();
	print(stream, "{}", usage);
}

std::expected<ParsingResult, ParsingError> ArgParser::parse(int argc, char** argv) {
	ParsingResult result;
	std::vector<ParsedArg>& parsedArgs = result.args;
	if (programName.empty()) {
		programName = argv[0];
	}
	bool end_of_options = false;
	bool need_args = true;
	for (int i = 1; i < argc; i++) {
		bool isOption = false;
		if (end_of_options) {
			parsedArgs.emplace_back(argv[i]);
			continue;
		}
		std::string_view arg = argv[i];
		if (arg == std::string_view("--")) {
			end_of_options = true;
			continue;
		}
		for (auto& opt : expectedOptions) {
			if (arg.size() < 2 or arg[0] != '-') {
				continue;
			}
			if ((arg.substr(0) == opt.short_name) || (arg[1] == '-' and arg.substr(2) == opt.long_name)) {
				ParsedOption parsedOpt;
				parsedOpt.long_name = opt.long_name;
				if (opt.argument == OptionValueType::required) {
					if (i + 1 >= argc) {
						return std::unexpected(ParsingError {"Option requires a value"});
					}
					parsedOpt.value = argv[i + 1];
					i++;
				}
				result.options.push_back(parsedOpt);
				isOption = true;
				if (opt.handling == ArgumentHandling::disabled) {
					need_args = false;
				}
				break;
			}
			if (auto pos = arg.find('='); pos != std::string_view::npos) {
				std::string_view optname = arg.substr(0, pos);
				std::string_view optval = arg.substr(pos + 1);
				if (optname.substr(1) == opt.short_name || (optname[1] == '-' and optname.substr(2) == opt.long_name)) {
					if (opt.argument == OptionValueType::none) {
						return std::unexpected(ParsingError {"Option does not accept a value"});
					}
					ParsedOption parsedOpt;
					parsedOpt.long_name = opt.long_name;
					parsedOpt.value = optval;
					result.options.push_back(parsedOpt);
					isOption = true;
					if (opt.handling == ArgumentHandling::disabled) {
						need_args = false;
					}
					break;
				}
			}
		}
		if (isOption) {
			continue;
		} else {
			if (arg.size() >= 2 and arg[0] == '-') {
				if (not allowUnknownOptions) {
					return std::unexpected(ParsingError {"Unknown option '" + std::string(arg) + "'"});
				}
			}

			parsedArgs.emplace_back(arg);
		}
	}
	for (auto& ea : expectedArgs) {
		if (ea.handling == ArgumentHandling::disabled) {
			need_args = false;
		}
	}
	if (need_args and parsedArgs.size() < expectedArgs.size()) {
		return std::unexpected(ParsingError {"Wrong number of arguments"});
	}
	if (need_args and parsedArgs.size() > expectedArgs.size() and not expectedArgs.empty() and expectedArgs.back().multiple == ArgumentMultiplicity::single) {
		return std::unexpected(ParsingError {"Wrong number of arguments"});
	}
	for (auto& opt : expectedOptions) {
		size_t optcount = 0;
		for (auto& parsedOpt : result.options) {
			if (parsedOpt.long_name == opt.long_name) {
				optcount++;
			}
		}
		if (optcount > 1 and opt.multiple == OptionMultiplicity::single) {
			return std::unexpected(ParsingError {"Option is not allowed to be specified multiple times"});
		}
		if (need_args and optcount == 0 and opt.required == OptionRequirement::mandatory) {
			auto msg = fmt::format("Required option '{}' not specified", opt.long_name);
			return std::unexpected(ParsingError {msg});
		}
	}
	if (not expectedArgs.empty() and expectedArgs.front().skip == ArgumentSkip::skipArg) {
		parsedArgs.erase(parsedArgs.begin());
	}
	return result;
}

}