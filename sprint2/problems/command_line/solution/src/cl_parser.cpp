#include "cl_parser.h"

#include <iostream>
#include <sstream>

using namespace std::literals;

namespace cl_parser {

namespace po = boost::program_options;

[[nodiscard]] std::optional<Args> ParseComandLine(int argc, const char* const argv[]) {
    po::options_description desc{"Allowed options:"s};

    Args args;
    desc.add_options()
        ("help,h", "produce help message")
        ("tick-period,t", po::value(&args.tick_period)->value_name("milliseconds"s), "set tick period")
        ("config-file,c", po::value(&args.config_file_path)->value_name("file"s), "set config file path")
        ("www-root,w", po::value(&args.static_root)->value_name("dir"s), "set static files root")
        ("randomize-spawn-points", po::bool_switch(&args.random_spawn_point), "spawn dogs at random positions");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.contains("help")) {
        std::cout << desc;
        return std::nullopt;
    }

    if (!vm.contains("config-file") && !vm.contains("www-root")) {
        std::stringstream ss;
        ss << "Basic usage: game_server\n"s
           << "             --tick-period <tick-period in ms> (optional)\n"s
           << "             --config-file <game-config-json>\n"s
           << "             --www-root <static-files-dir>"s
           << "             --randomize-spawn-points (optional)\n"s;
        throw std::runtime_error(ss.str());
    }

    if (vm.contains("tick-period") && args.tick_period < 0) {
        throw std::runtime_error("Tick-period must be positive number in ms"s);
    }

    return args;
}

} // namespace cl_parser