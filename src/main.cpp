/** @file */

// MIT License

// Copyright (c) 2023 malloc-nbytes

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <filesystem>
#include <iostream>
#include <vector>
#include <iostream>
#include <unordered_map>
#include <chrono>

#include "err.hpp"
#include "parser.hpp"
#include "ast.hpp"
#include "common.hpp"
#include "utils.hpp"
#include "lexer.hpp"
#include "interpreter.hpp"
#include "repl.hpp"
#include "config.h"
#include "hot-reload.hpp"
#include "earl-to-py.hpp"
#include "hidden-file.hpp"
#include "mem-file.hpp"


namespace config {
    namespace prelude {
        namespace watch {
            std::vector<std::string> files = {};
            size_t run_count = 1;
        };
        namespace one_shot {
            std::string code = "";
        };
        namespace time {
            std::chrono::time_point<std::chrono::high_resolution_clock> start;
            std::chrono::time_point<std::chrono::high_resolution_clock> end;
        };
        namespace include {
            std::vector<std::string> dirs = {};
        };
        namespace import {
            std::vector<std::string> dirs = {};
        };
        namespace topy {
            std::string formatter = "";
            std::string output = "";
        };
        std::vector<std::string> scripts = {};
    };

    namespace runtime {
        std::vector<std::string> argv = {};
        uint32_t flags = 0x00;
        std::unordered_map<std::string, std::string> persistent_mem;
    };

    namespace repl {
        namespace welcome {
            std::string msg = "";
        };
        namespace theme {
            std::vector<std::string> available = COMMON_EARL_REPL_THEME_ASCPL;
            std::string selected = COMMON_EARL_REPL_THEME_DEFAULT;
        };
    };
};

static std::vector<std::string> one_shot_cli_import_dirs_copy = {};

#ifdef _WIN32
#include <windows.h>
#include <tchar.h>

std::string
get_os_info() {
    OSVERSIONINFO osvi;
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&osvi);
    return "Windows " + osvi.dwMajorVersion + "."
        + osvi.dwMinorVersion + "(Build " + osvi.dwBuildNumber + ")";
}

#elif __linux__
#include <sys/utsname.h>

std::string
get_os_info() {
    struct utsname buffer;
    uname(&buffer);
    std::string sysname(buffer.sysname);
    std::string version(buffer.release);
    return sysname + " " + version;
}

#elif __APPLE__
#include <sys/utsname.h>

std::string
get_os_info() {
    struct utsname buffer;
    uname(&buffer);
    std::string sysname(buffer.sysname);
    std::string release(buffer.release);
    return sysname + " " + release;
}

#else
std::string
get_os_info() {
    return "Unknown";
}
#endif

static std::string
stdlib_installed(void) {
    std::filesystem::path path = PREFIX "/include/EARL/std";
    if (std::filesystem::exists(path) && std::filesystem::is_directory(path))
        return "YES";
    return "NO";
}

static std::string
earlmgr_installed(void) {
    std::filesystem::path path = PREFIX "/bin/earlmgr";
    if (std::filesystem::exists(path))
        return "YES";
    return "NO";
}

static void
usage(void) {
    std::cerr << "(MIT License) Copyright (c) 2023 malloc-nbytes" << std::endl << std::endl;

#ifdef PORTABLE
    std::cerr << "EARL <portable> v";
#else
    std::cerr << "EARL v";
#endif

    std::cerr << VERSION << ", " << "(compiler) " << COMPILER_INFO << ", (platform) " << get_os_info() << std::endl << std::endl;

    std::cerr << "StdLib installed: " << stdlib_installed() << std::endl;
    std::cerr << "earlmgr installed: " << earlmgr_installed() << std::endl;

    std::cerr << "Builtin StdLib: ";
#ifdef PORTABLE
    std::cerr << "YES" << std::endl << std::endl;
#else
    std::cerr << "NO" << std::endl << std::endl;
#endif

    std::cerr << "The GitHub repository is located at: https://github.com/malloc-nbytes/EARL/" << std::endl << std::endl;

    std::cerr << "Bugs can be reported at <zdhdev@yahoo.com>" << std::endl;
    std::cerr << "or https://github.com/malloc-nbytes/EARL/issues" << std::endl << std::endl;

    std::cerr << "Documentation can be found at: https://malloc-nbytes.github.io/EARL-web/" << std::endl << std::endl;

    std::cerr << "USAGE: earl [script (max 1 file)] [options...] -- [passed_args...]" << std::endl;
    std::cerr << "|  or: provide no script to launch the REPL" << std::endl << std::endl;
    std::cerr << "OPTIONS:" << std::endl;
    std::cerr << "    Query Flags" << std::endl;
    std::cerr << "        -h, --help . . . . . . . . . . . . . . Print this help message" << std::endl;
    std::cerr << "        -v, --version  . . . . . . . . . . . . Print version information" << std::endl;
    std::cerr << "            --install-prefix . . . . . . . . . Print the installation prefix" << std::endl;
    std::cerr << "            --is-portable  . . . . . . . . . . Show whether EARL portable is installed" << std::endl;
    std::cerr << "    Prelude Config" << std::endl;
    std::cerr << "        -c, --check  . . . . . . . . . . . . . Only parse the file given" << std::endl;
    std::cerr << "        -I, --include <DIR>  . . . . . . . . . Add an include directory" << std::endl;
    std::cerr << "        -i, --import <filepath>  . . . . . . . Import a file from the CLI" << std::endl;
    std::cerr << "        -w, --watch [files...] . . . . . . . . Watch files for changes and hot reload on save" << std::endl;
    std::cerr << "        -b, --batch [files...] . . . . . . . . Run multiple scripts in batch" << std::endl;
    std::cerr << "        -O  --oneshot \"<code>\" . . . . . . . . Evaluate code in the CLI and print the result (if non-unit type)" << std::endl;
    std::cerr << "            --time . . . . . . . . . . . . . . Time execution" << std::endl;
    std::cerr << "            --clear-mem  . . . . . . . . . . . Clear the persistent memory file" << std::endl;
    std::cerr << "            --without-stdlib . . . . . . . . . Do not use standard library" << std::endl;
    std::cerr << "    Runtime Config" << std::endl;
    std::cerr << "        -S, --suppress-warnings  . . . . . . . Suppress all warnings" << std::endl;
    std::cerr << "        -e, --error-on-bash-fail . . . . . . . Stop the program on a failed BASH command" << std::endl;
    std::cerr << "        -x, --print-bash-execution . . . . . . Print all inlined BASH" << std::endl;
    std::cerr << "        -V, --verbose  . . . . . . . . . . . . Enable verbose mode" << std::endl;
    std::cerr << "            --disable-implicit-returns . . . . All returns must explicitly use the keyword `return` (does not effect REPL)" << std::endl;
    std::cerr << "            --show-funs  . . . . . . . . . . . Print every function call evaluated" << std::endl;
    std::cerr << "            --show-lets  . . . . . . . . . . . Print all variable instantiations" << std::endl;
    std::cerr << "            --show-muts  . . . . . . . . . . . Print all value mutations" << std::endl;
    std::cerr << "            --no-sanitize-pipes  . . . . . . . Do not sanitize BASH pipes" << std::endl;
    std::cerr << "    REPL Config" << std::endl;
    std::cerr << "            --repl-nocolor . . . . . . . . . . Do not use color in the REPL" << std::endl;
    std::cerr << "            --repl-welcome . . . . . . . . . . Display a welcome message in the REPL" << std::endl;
    std::cerr << "            --repl-theme <theme>|list  . . . . Use a color theme in the REPL (`default` if this option is not used)" << std::endl;
    std::cerr << "                where" << std::endl;
    std::cerr << "                    list = list all available themes" << std::endl;
    std::cerr << "    Misc. Options" << std::endl;
    std::cerr << "            --create-default-config  . . . . . Create a default configuration file" << std::endl;
    std::cerr << "            --to-py output=O [formatter=F] . . Convert an EARL file to Python (experimental)" << std::endl;
    std::cerr << "                where" << std::endl;
    std::cerr << "                    O = stdout|<file>" << std::endl;
    std::cerr << "                    F = <program>" << std::endl;

    std::cerr << std::endl;

    std::exit(0);
}

static void
version() {
    std::cout << "EARL " << VERSION << std::endl;
    exit(0);
}

static void
gather_watch_files(std::vector<std::string> &args) {
    while (args.size() > 0) {
        if (!std::filesystem::exists(std::filesystem::path(args.at(0))))
            break;
        config::prelude::watch::files.push_back(std::string(args.at(0)));
        args.erase(args.begin());
    }
}

static std::string
try_guess_wrong_arg(std::string &arg) {
    std::vector<std::string> possible = COMMON_EARL2ARG_ASCPL;
    int min = 1e9;
    int min_idx = 0;

    for (size_t i = 0; i < possible.size(); ++i) {
        int score = levenshtein_distance(arg, possible[i]);
        if (score < min) {
            min = score;
            min_idx = i;
        }
    }

    return possible.at(min_idx);
}

static void
handle_to_py_flag(std::vector<std::string> &args) {
    std::cout << "warning: flag `--" << COMMON_EARL2ARG_TOPY << "` is experimental and may not work correctly" << std::endl;
    config::runtime::flags |= __TOPY;
    while (args.size() != 0 && args[0][0] != '-') {
        const std::string &option = args.at(0);
        std::string left = "", right = "";
        size_t pos = option.find('=');
        if (pos != std::string::npos) {
            left = option.substr(0, pos);
            right = option.substr(pos+1);
            if (left == "formatter")
                config::prelude::topy::formatter = right;
            else if (left == "output")
                config::prelude::topy::output = right;
            else {
                std::cerr << "error: invalid option `" << left <<  "` for `--" << COMMON_EARL2ARG_TOPY << "`";
                std::exit(EXIT_FAILURE);
            }
            args.erase(args.begin());
        }
        else {
            std::cerr << "error: missing `=` in `--" << COMMON_EARL2ARG_TOPY << "` option" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }
}

static void
install_prefix(void) {
    std::cout << "[EARL install-prefix] EARL and StdLib installed at " << PREFIX << std::endl;
    std::exit(0);
}

static void
show_is_portable(void) {
#ifdef PORTABLE
    std::cout << "YES" << std::endl;
#else
    std::cout << "NO" << std::endl;
#endif
    std::exit(0);
}

static void
handle_clear_mem_file(void) {
    clear_mem_file();
    std::exit(0);
}

static void
add_import_file(std::vector<std::string> &args) {
    if (args.size() > 0) {
        one_shot_cli_import_dirs_copy.push_back(args.at(0));
        config::prelude::import::dirs.push_back(args.at(0));
        args.erase(args.begin());
    }
    else {
        std::cerr << "error: missing import file for flag `--" COMMON_EARL2ARG_IMPORT "`" << std::endl;
        std::exit(1);
    }
}

static void
assert_repl_theme_valid(void) {
    bool found = false;
    for (auto t : COMMON_EARL_REPL_THEME_ASCPL) {
        if (config::repl::theme::selected == t) {
            found = true;
            break;
        }
    }
    if (!found) {
        std::cerr << "invalid theme: `" << config::repl::theme::selected << "`" << std::endl;
        std::cerr << "see `--" << COMMON_EARL2ARG_REPL_THEME << " list` to view all themes" << std::endl;
        std::exit(1);
    }
}

static void
get_repl_theme(std::vector<std::string> &args) {
    if (args.size() > 0) {
        if (args[0] == "list") {
            std::cerr << "REPL Themes:" << std::endl;
            for (auto t : COMMON_EARL_REPL_THEME_ASCPL) {
                std::cerr << "| " << t;
                if (config::repl::theme::selected == t)
                    std::cout << "        [selected]";
                std::cout << std::endl;
            }
            std::exit(0);
        }

        config::repl::theme::selected = args.at(0);
        args.erase(args.begin());
        // Theme checking is done in main().
    }
    else {
        std::cerr << "error: missing theme for flag `--" COMMON_EARL2ARG_REPL_THEME "`" << std::endl;
        std::exit(1);
    }
}

static void
add_include_file(std::vector<std::string> &args) {
    if (args.size() > 0) {
        config::prelude::include::dirs.push_back(args.at(0));
        args.erase(args.begin());
    }
    else {
        std::cerr << "error: missing include directory for flag `--" COMMON_EARL2ARG_INCLUDE "`" << std::endl;
        std::exit(1);
    }
}

static void
get_batch_scripts(std::vector<std::string> &args) {
    while (args.size() > 0) {
        if (!std::filesystem::exists(std::filesystem::path(args.at(0))))
            break;
        config::prelude::scripts.push_back(std::string(args.at(0)));
        args.erase(args.begin());
    }
}

static void
handle_one_shot(std::vector<std::string> &args) {
    config::runtime::argv.push_back("EARL-one-shot");
    if (args.size() == 0) {
        std::cerr << "flag `--" COMMON_EARL2ARG_ONE_SHOT "` requires a line of code" << std::endl;
        std::exit(1);
    }
    config::prelude::one_shot::code = args.at(0);
    args.erase(args.begin());
}

static void
handle_repl_welcome(std::vector<std::string> &args) {
    if (args.size() == 0) {
        std::cerr << "flag `--" COMMON_EARL2ARG_REPL_WELCOME "` requires a message" << std::endl;
        std::exit(1);
    }
    config::repl::welcome::msg = args.at(0);
    args.erase(args.begin());
}

static void
handle_time(void) {
    config::runtime::flags |= __TIME;
    config::prelude::time::start = std::chrono::high_resolution_clock::now();
}

static void
parse_2hypharg(std::string arg, std::vector<std::string> &args) {
    if (arg == COMMON_EARL2ARG_WITHOUT_STDLIB)
        config::runtime::flags |= __WITHOUT_STDLIB;
    else if (arg == COMMON_EARL2ARG_HELP)
        usage();
    else if (arg == COMMON_EARL2ARG_VERSION)
        version();
    else if (arg == COMMON_EARL2ARG_REPL_NOCOLOR)
        config::runtime::flags |= __REPL_NOCOLOR;
    else if (arg == COMMON_EARL2ARG_WATCH) {
        gather_watch_files(args);
        config::runtime::flags |= __WATCH;
    }
    else if (arg == COMMON_EARL2ARG_SHOWFUNS)
        config::runtime::flags |= __SHOWFUNS;
    else if (arg == COMMON_EARL2ARG_CHECK)
        config::runtime::flags |= __CHECK;
    else if (arg == COMMON_EARL2ARG_TOPY)
        handle_to_py_flag(args);
    else if (arg == COMMON_EARL2ARG_VERBOSE)
        config::runtime::flags |= __VERBOSE;
    else if (arg == COMMON_EARL2ARG_INSTALL_PREFIX)
        install_prefix();
    else if (arg == COMMON_EARL2ARG_SHOWBASH)
        config::runtime::flags |= __SHOWBASH;
    else if (arg == COMMON_EARL2ARG_SHOWLETS)
        config::runtime::flags |= __SHOWLETS;
    else if (arg == COMMON_EARL2ARG_SHOWMUTS)
        config::runtime::flags |= __SHOWMUTS;
    else if (arg == COMMON_EARL2ARG_NO_SANITIZE_PIPES)
        config::runtime::flags |= __NO_SANITIZE_PIPES;
    else if (arg == COMMON_EARL2ARG_INCLUDE)
        add_include_file(args);
    else if (arg == COMMON_EARL2ARG_IMPORT)
        add_import_file(args);
    else if (arg == COMMON_EARL2ARG_REPL_THEME)
        get_repl_theme(args);
    else if (arg == COMMON_EARL2ARG_CREATE_DEF_CONF)
        create_default_config_file();
    else if (arg == COMMON_EARL2ARG_BATCH)
        get_batch_scripts(args);
    else if (arg == COMMON_EARL2ARG_ERROR_ON_BASH_FAIL)
        config::runtime::flags |= __ERROR_ON_BASH_FAIL;
    else if (arg == COMMON_EARL2ARG_SUPPRESS_WARNINGS)
        config::runtime::flags |= __SUPPRESS_WARNINGS;
    else if (arg == COMMON_EARL2ARG_DISABLE_IMPLICIT_RETURNS)
        config::runtime::flags |= __DISABLE_IMPLICIT_RETURNS;
    else if (arg == COMMON_EARL2ARG_ONE_SHOT) {
        config::runtime::flags |= __ONE_SHOT;
        handle_one_shot(args);
    }
    else if (arg == COMMON_EARL2ARG_PORTABLE)
        show_is_portable();
    else if (arg == COMMON_EARL2ARG_REPL_WELCOME)
        handle_repl_welcome(args);
    else if (arg == COMMON_EARL2ARG_TIME)
        handle_time();
    else if (arg == COMMON_EARL2ARG_CLEAR_MEM_FILE)
        handle_clear_mem_file();
    else {
        std::cerr << "error: Unrecognised argument: " << arg << std::endl;
        std::cerr << "Did you mean: " << try_guess_wrong_arg(arg) << "?" << std::endl;
        exit(1);
    }
}

static void
parse_1hypharg(std::string arg, std::vector<std::string> &args) {
    for (size_t i = 0; i < arg.size(); ++i) {
        switch (arg[i]) {
        case COMMON_EARL1ARG_HELP: {
            usage();
        } break;
        case COMMON_EARL1ARG_VERSION: {
            version();
        } break;
        case COMMON_EARL1ARG_CHECK: {
            config::runtime::flags |= __CHECK;
        } break;
        case COMMON_EARL1ARG_WATCH: {
            gather_watch_files(args);
            config::runtime::flags |= __WATCH;
        } break;
        case COMMON_EARL1ARG_INCLUDE: {
            add_include_file(args);
        } break;
        case COMMON_EARL1ARG_IMPORT: {
            add_import_file(args);
        } break;
        case COMMON_EARL1ARG_BATCH: {
            get_batch_scripts(args);
        } break;
        case COMMON_EARL1ARG_VERBOSE: {
            config::runtime::flags |= __VERBOSE;
        } break;
        case COMMON_EARL1ARG_SHOW_BASH: {
            config::runtime::flags |= __SHOWBASH;
        } break;
        case COMMON_EARL1ARG_ERROR_ON_BASH_FAIL: {
            config::runtime::flags |= __ERROR_ON_BASH_FAIL;
        } break;
        case COMMON_EARL1ARG_SUPPRESS_WARNINGS: {
            config::runtime::flags |= __SUPPRESS_WARNINGS;
        } break;
        case COMMON_EARL1ARG_ONE_SHOT: {
            config::runtime::flags |= __ONE_SHOT;
            handle_one_shot(args);
        } break;
        default: {
            ERR_WARGS(Err::Type::Fatal, "unrecognised argument `%c`", arg[i]);
        } break;
        }
    }
}

static void
parsearg(std::string line, std::vector<std::string> &args) {
    if (line.size() > 1 && line[0] == '-' && line[1] == '-') {
        args.erase(args.begin());
        parse_2hypharg(line.substr(2), args);
    }
    else if (line[0] == '-') {
        args.erase(args.begin());
        parse_1hypharg(line.substr(1), args);
    }
    else {
        UNIMPLEMENTED("parsearg: last case");
    }
}

static void
parse_earl_argv(std::vector<std::string> &args) {
    args.erase(args.begin());
    for (auto &s : args)
        config::runtime::argv.push_back(s);
}

static void
handlecli(int argc, char **argv) {
    std::string filepath = "";
    std::vector<std::string> args = {};
    while (*argv) {
        args.push_back(std::string(*argv));
        ++argv;
    }

    int i = 0;
    while (args.size() > 0) {
        const std::string &entry = args.at(i);
        if (entry == "--") {
            parse_earl_argv(args);
            break;
        }
        if (entry[0] == '-') {
            parsearg(entry, args);
        }
        else {
            if (filepath != "")
                ERR(Err::Type::Fatal, "too many input files provided");
            filepath = entry;
            config::runtime::argv.push_back(filepath);
            args.erase(args.begin());
            config::prelude::scripts.push_back(filepath);
        }
    }
}

static void
to_py(std::string &filepath,
      std::vector<std::string> keywords,
      std::vector<std::string> types,
      std::string comment) {
    std::unique_ptr<Lexer> lexer = nullptr;
    std::unique_ptr<Program> program = nullptr;

    if (config::prelude::topy::output == "") {
        std::cerr << "[EARL] error: missing output filepath for flag `--" << COMMON_EARL2ARG_TOPY << "`" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    try {
        std::string src_code = read_file(filepath.c_str(), config::prelude::include::dirs);
        lexer = lex_file(src_code, filepath, keywords, types, comment);
    } catch (const LexerException &e) {
        std::cerr << "Lexer error: " << e.what() << std::endl;
    }
    try {
        program = Parser::parse_program(*lexer.get(), filepath);
    } catch (const ParserException &e) {
        std::cerr << "Parser error: " << e.what() << std::endl;
    }

    auto pysrc = earl_to_py(std::move(program));

    if (config::prelude::topy::output == "stdout")
        std::cout << pysrc << std::endl;
    else {
        std::ofstream pysrc_outfile(config::prelude::topy::output);
        if (!pysrc_outfile) {
            std::cerr << "[EARL] error: opening file: " << config::prelude::topy::output << std::endl;
            std::exit(EXIT_FAILURE);
        }
        pysrc_outfile << pysrc;
        pysrc_outfile.close();
    }

    if (config::prelude::topy::formatter != "") {
        std::string cmd = config::prelude::topy::formatter+" "+config::prelude::topy::output;
        int exit_code = system(cmd.c_str());
        if (exit_code != 0) {
            std::cerr << "[EARL] error: formatting failed with code " << exit_code << std::endl;
            std::exit(1);
        }
    }
}

static void
do_one_shot(std::string &src,
            std::vector<std::string> keywords,
            std::vector<std::string> types,
            std::string comment) {

    if (one_shot_cli_import_dirs_copy.size() != config::prelude::import::dirs.size())
        config::prelude::import::dirs = one_shot_cli_import_dirs_copy;

    std::unique_ptr<Lexer> lexer = nullptr;
    std::unique_ptr<Program> program = nullptr;

    const std::string filepath = "EARL-one-shot";

    try {
        lexer = lex_file(src, filepath, keywords, types, comment);
    } catch (const LexerException &e) {
        std::cerr << "Lexer error: " << e.what() << std::endl;
    }
    try {
        program = Parser::parse_program(*lexer.get(), filepath);
    } catch (const ParserException &e) {
        std::cerr << "Parser error: " << e.what() << std::endl;
    }
    try {
        (void)Interpreter::interpret(std::move(program), std::move(lexer));
    } catch (const InterpreterException &e) {
        std::cerr << "Interpreter error: " << e.what() << std::endl;
    }
}

int
main(int argc, char **argv) {
    init_mem_file();
    config::runtime::persistent_mem = parse_mem_file();

    ++argv; --argc;

    std::vector<std::string> keywords = COMMON_EARLKW_ASCPL;
    std::vector<std::string> types = {};
    std::string comment = "#";

    assert_repl_theme_valid();
    handle_hidden_file();
    handlecli(argc, argv);

    if ((config::runtime::flags & __WATCH) != 0) {
        if (config::prelude::watch::files.size() == 0) {
            std::cerr << "Cannot use flag `" << COMMON_EARL2ARG_WATCH << "` with no watch files\n";
            std::exit(1);
        }
        hot_reload::register_watch_files(config::prelude::watch::files);
        std::cout << "[EARL] Now watching files and will hot reload on file save" << std::endl;
    }

    bool locked = true;
    if (config::prelude::scripts.size() > 0 || (config::runtime::flags & __ONE_SHOT) != 0) {
        do {
            // No need to check for __WATCH cause this statement
            // will not happen unless we are looping, which is
            // already determined by __WATCH.
            if (!locked)
                hot_reload::watch();
            else
                locked = false;

            if ((config::runtime::flags & __ONE_SHOT) != 0)
                do_one_shot(config::prelude::one_shot::code, keywords, types, comment);
            else {
                for (auto &filepath : config::prelude::scripts) {
                    if ((config::runtime::flags & __TOPY) != 0)
                        to_py(filepath, keywords, types, comment);

                    else if (filepath != "") {
                        if ((config::runtime::flags & __WATCH) != 0)
                            std::cout << "=== " << config::prelude::watch::run_count++ << " ======================" << std::endl;

                        std::unique_ptr<Lexer> lexer = nullptr;
                        std::unique_ptr<Program> program = nullptr;
                        try {
                            std::string src_code = read_file(filepath.c_str(), config::prelude::include::dirs);
                            lexer = lex_file(src_code, filepath, keywords, types, comment);
                        } catch (const LexerException &e) {
                            std::cerr << "Lexer error: " << e.what() << std::endl;
                            if ((config::runtime::flags & __WATCH) == 0)
                                return 1;
                            continue;
                        }
                        try {
                            program = Parser::parse_program(*lexer.get(), filepath);
                        } catch (const ParserException &e) {
                            std::cerr << "Parser error: " << e.what() << std::endl;
                            if ((config::runtime::flags & __WATCH) == 0)
                                return 1;
                            continue;
                        }
                        try {
                            (void)Interpreter::interpret(std::move(program), std::move(lexer));
                        } catch (const InterpreterException &e) {
                            std::cerr << "Interpreter error: " << e.what() << std::endl;
                            if ((config::runtime::flags & __WATCH) == 0)
                                return 1;
                        }
                    }
                }
            }
        } while ((config::runtime::flags & __WATCH) != 0);
    }
    else {
        assert_repl_theme_valid();
        config::runtime::flags |= __REPL;
        config::runtime::argv.push_back("EARL-REPLv" VERSION);
        if (config::repl::welcome::msg != "")
            std::cout << config::repl::welcome::msg << '\n';
        else
            std::cout << "EARL REPL v" << VERSION << '\n';
        std::cout << "Use `:help` for help and `:q` or C-c to quit" << std::endl;
        repl::run(config::prelude::include::dirs);
    }

    if ((config::runtime::flags & __TIME) != 0) {
        config::prelude::time::end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = config::prelude::time::end - config::prelude::time::start;
        std::cout << "[EARL time] Execution time: " << elapsed.count() << " seconds" << std::endl;
    }

    return 0;
}
