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

#include <cstdlib>
#include <sstream>
#include <algorithm>
#include <cassert>
#include <vector>
#include <iostream>
#include <memory>
#include <unistd.h>
#include <termios.h>

#include "config.h"
#include "repl.hpp"
#include "repled.hpp"
#include "parser.hpp"
#include "interpreter.hpp"
#include "intrinsics.hpp"
#include "err.hpp"
#include "token.hpp"
#include "ast.hpp"
#include "ctx.hpp"
#include "common.hpp"
#include "earl.hpp"
#include "lexer.hpp"

#define EARL_REPL_HISTORY_FILENAME ".earl_history"
#define REPL_HISTORY_MAX_FILESZ 1024 * 1024

#define QUIT ":q"
#define CLEAR ":c"
#define SKIP ":skip"
#define HELP ":help"
#define RM_ENTRY ":rm"
#define EDIT_ENTRY ":e"
#define LIST_ENTRIES ":show"
#define IMPORT ":i"
#define DISCARD ":d"
#define EE ":q!"
#define VARS ":vars"
#define FUNCS ":funcs"
#define RESET ":reset"
#define AUTO ":auto"
#define HISTORY ":hist"

#define CMD_OPTION_ASCPL {QUIT, CLEAR, SKIP, HELP, RM_ENTRY, EDIT_ENTRY, LIST_ENTRIES, IMPORT, DISCARD, VARS, FUNCS, RESET, AUTO, HISTORY}

static std::string REPL_HIST = "";

static size_t LINENO = 0;
static std::vector<std::string> HIST = {};

static repled::SS SS;

static void
try_clear_repl_history() {
    const char* home_dir = std::getenv("HOME");
    if (home_dir == nullptr) {
        std::cerr << "Unable to get home directory path. Will not clear REPL history." << std::endl;
        return;
    }

    std::string fp = std::string(home_dir) + "/" + EARL_REPL_HISTORY_FILENAME;

    std::ifstream file(fp, std::ios::binary);
    if (!file) {
        std::cerr << "Unable to open file for reading. Will not clear REPL history." << std::endl;
        return;
    }

    file.seekg(0, std::ios::end);
    std::streampos filesz = file.tellg();
    file.close();

    if (filesz >= REPL_HISTORY_MAX_FILESZ) {
        std::ofstream file(fp, std::ios::trunc | std::ios::binary);
        if (!file) {
            std::cerr << "Unable to open file for clearing. Will not clear REPL history." << std::endl;
            return;
        }
        file.close();
        std::cout << "REPL history file cleared." << std::endl;
    }
}

static void
save_repl_history() {
    const char *home_dir = std::getenv("HOME");
    if (home_dir == nullptr)
        std::cerr << "Unable to get home directory path. Will not save REPL history." << std::endl;
    std::string fp = std::string(home_dir) + "/" EARL_REPL_HISTORY_FILENAME;
    std::ofstream of(fp, std::ios::app);
    if (!of)
        std::cerr << "Unable to open file for writing. Will not save REPL history." << std::endl;
    of << REPL_HIST;
    of.close();
}

static void
read_repl_history() {
    const char *home_dir = std::getenv("HOME");
    if (home_dir == nullptr)
        return;

    std::string fp = std::string(home_dir) + "/" EARL_REPL_HISTORY_FILENAME;
    std::ifstream ifs(fp, std::ios::app);
    if (!ifs)
        return;

    std::string line = "";
    while (std::getline(ifs, line)) {
        if (line != "") {
            HIST.push_back(line);
        }
    }
}

static void
yellow(void) {
    if ((config::runtime::flags & __REPL_NOCOLOR) == 0)
        std::cout << YELLOW;
}

static void
blue(void) {
    if ((config::runtime::flags & __REPL_NOCOLOR) == 0)
        std::cout << BLUE;
}

static void
green(void) {
    if ((config::runtime::flags & __REPL_NOCOLOR) == 0)
        std::cout << GREEN;
}

static void
gray(void) {
    if ((config::runtime::flags & __REPL_NOCOLOR) == 0)
        std::cout << GRAY;
}

static void
red(void) {
    if ((config::runtime::flags & __REPL_NOCOLOR) == 0)
        std::cout << RED;
}

static void
noc(void) {
    if ((config::runtime::flags & __REPL_NOCOLOR) == 0)
        std::cout << NOC;
}

static void
log(std::string msg, const char *(*color)(void) = nullptr) {
    repled::clearln(0, true);
    noc();
    if (color)
        std::cout << color();
    std::cout << msg;
    noc();
}

static std::vector<std::string>
split_on_space(std::string &line) {
    std::vector<std::string> res = {};
    std::stringstream ss(line);
    std::string word;
    while (ss >> word)
        res.push_back(word);
    return res;
}

static std::vector<std::string>
split_on_newline(std::string &line) {
    std::vector<std::string> res = {};
    std::string buf = "";
    for (auto &c : line) {
        if (c == '\n') {
            res.push_back(buf);
            buf = "";
        }
        else
            buf += c;
    }
    return res;
}

static void
help(void) {
    log("Command Sequences:\n");
    log("| " HELP " -> show this message\n");
    log("| " QUIT " -> quit the session\n");
    log("| " VARS " -> show current variables in scope\n");
    log("| " FUNCS " -> show current functions in scope\n");
    log("| " CLEAR " -> clear the screen\n");
    log("| " IMPORT " [files...] -> import local EARL files\n");
    log("| " RM_ENTRY " [lineno...] -> remove entries (previous if blank)\n");
    log("| " EDIT_ENTRY " [lineno...] -> edit previous entries (previous if blank)\n");
    log("| " LIST_ENTRIES " -> list all entries in the current session\n");
    log("| " DISCARD " -> discard the current session\n");
    log("| " RESET " -> reset all identifiers\n");
    log("| " AUTO " -> show all completions\n");
    log("| " HISTORY " -> show REPL history\n");
    log("Controls: (C = [CONTROL], M = [META] (ALT))\n");
    log("| Navigation:\n");
    log("| | [UP] -> previous line\n");
    log("| | [DOWN] -> next line\n");
    log("| | [LEFT] -> previous char\n");
    log("| | [RIGHT] -> next char\n");
    log("| | C-f -> next char\n");
    log("| | C-b -> previous char\n");
    log("| | M-f -> next word\n");
    log("| | M-b -> previous word\n");
    log("| | C-p -> previous line\n");
    log("| | C-n -> next line\n");
    log("| | C-a -> jump to beginning of line\n");
    log("| | C-e -> jump to end of line\n");
    log("| Editing:\n");
    log("| | [ENTER] -> submit\n");
    log("| | [TAB] -> insert 2 spaces at cursor\n");
    log("| | C-j -> submit\n");
    log("| | C-d -> delete char under cursor\n");
    log("| | M-d -> delete word from cursor\n");
    log("| Other:\n");
    log("| | C-l -> clear screen\n");
    log("| | C-c -> quit REPL\n");
}

static void
import_file(std::vector<std::string> &args, std::vector<std::string> &lines, std::vector<std::string> &include_dirs) {
    if (args.size() == 0) {
        repled::clearln(0, true);
        log("No files supplied\n");
        return;
    }
    for (auto &f : args) {
        const char *src_c = read_file(f.c_str(), include_dirs);
        std::string src = std::string(src_c);
        auto src_lines = split_on_newline(src);
        LINENO += src_lines.size();
        std::for_each(src_lines.begin(), src_lines.end(), [&](auto &l){lines.push_back(l);});
        repled::clearln(0, true);
        log("Imported " + f + "\n", repled::status_ok);
        noc();
    }
}

static std::string
get_special_input(repled::RawInput &ri, repled::SS &ss) {
    auto line = repled::getln(ri, ">>> ", HIST, true, ss);
    std::cout << std::endl;
    return line;
}

static void
analyze_new_line(std::string &line) {
    for (auto it = line.rbegin(); it != line.rend(); ++it) {
        switch (*it) {
        case '{': {
            SS.braces++;
        } break;
        case '}': {
            SS.braces--;
        } break;
        case '[':{
            SS.brackets++;
        } break;
        case ']': {
            SS.brackets--;
        } break;
        case '(': {
            SS.parens++;
        } break;
        case ')': {
            SS.parens--;
        } break;
        default: break;
        }
    }
}

static void
manage_removed_or_edited_line(std::string &line) {
    for (auto it = line.rbegin(); it != line.rend(); ++it) {
        switch (*it) {
        case '{': SS.braces--; break;
        case '}': SS.braces++; break;
        case '[': SS.brackets--; break;
        case ']': SS.brackets++; break;
        case '(': SS.parens--; break;
        case ')': SS.parens++; break;
        }
    }
}

static void
rm_entries(std::vector<std::string> &args, std::vector<std::string> &lines) {
    if (lines.size() == 0) {
        repled::clearln(0, true);
        log("No previous entry exists\n", repled::msg_color);
        return;
    }

    std::vector<std::string> hist = {};

    if (args.size() == 0) {
        hist.push_back(lines.back());
        manage_removed_or_edited_line(lines.back());
        lines.pop_back();
    }
    else {
        int i = 0;
        for (auto &arg : args) {
            int lnum = i == 0 ? std::stoi(arg) : std::stoi(arg)-i;
            if (lnum < 0 || lnum >= (int)lines.size()) {
                repled::clearln(0, true);
                log("Line number is out of range for session\n", repled::msg_color);
                return;
            }
            hist.push_back(lines[lnum]);
            manage_removed_or_edited_line(lines[lnum]);
            lines.erase(lines.begin()+lnum);
            ++i;
        }
    }

    repled::clearln(0, true);
    log("Removed:\n");
    for (auto &h : hist) {
        repled::clearln(0, true);
        // red();
        std::cout << repled::error_color();
        std::cout << h;
        noc();
        std::cout << std::endl;
        --LINENO;
    }
}

static void
edit_entry(repled::RawInput &ri, std::vector<std::string> &args, std::vector<std::string> &lines) {
    log("`" SKIP "` to skip current selection or `" QUIT "` to cancel the session editing\n", repled::msg_color);
    if (args.size() > 0) {
        for (auto arg : args) {
            int lnum = std::stoi(arg);
            if (lnum < 0 || lnum >= (int)lines.size()) {
                repled::clearln(0, true);
                log("Line number is out of range for session\n", repled::msg_color);
                return;
            }
            repled::clearln(0, true);
            log("Editing [", repled::msg_color);
            std::cout << repled::main_color();
            // yellow();
            std::cout << lines.at(lnum);
            std::cout << repled::msg_color();
            // gray();
            std::cout << "]" << std::endl;
            noc();
            std::string newline = get_special_input(ri, SS);
            if (newline == QUIT) {
                repled::clearln(0, true);
                log("Quitting session editor\n", repled::msg_color);
                return;
            }
            if (newline == SKIP) {
                repled::clearln(0, true);
                log("Skipping current selection\n", repled::msg_color);
            }
            else if (newline == "") {
                manage_removed_or_edited_line(lines.at(lnum));
                lines.erase(lines.begin()+lnum);
            }
            else {
                manage_removed_or_edited_line(lines.at(lnum));
                lines.at(lnum) = newline;
                analyze_new_line(lines.at(lnum));
            }
        }
    }
    else {
        if (lines.size() == 0) {
            log("No lines to edit\n", repled::msg_color);
            return;
        }
        repled::clearln(0, true);
        log("Editing [", repled::msg_color);
        // yellow();
        std::cout << repled::main_color();
        std::cout << lines.back();
        // gray();
        std::cout << repled::msg_color();
        std::cout << "]" << std::endl;
        noc();
        std::string newline = get_special_input(ri, SS);
        manage_removed_or_edited_line(lines.at(lines.size()-1));
        if (newline == "")
            lines.erase(lines.end()-1);
        else {
            lines.at(lines.size()-1) = newline;
            analyze_new_line(lines.at(lines.size()-1));
        }
    }
}

static void
clearscrn(void) {
    if (system("clear") != 0)
        log("warning: failed to clearscrn\n", repled::msg_color);
}

static void
ls_entries(std::vector<std::string> &lines) {
    if (lines.size() == 0) {
        repled::clearln(0, true);
        log("Nothing appropriate\n", repled::msg_color);
        return;
    }
    for (size_t i = 0; i < lines.size(); ++i) {
        repled::clearln(0, true);
        // green();
        std::cout << repled::status_ok();
        std::cout << i << ": ";
        std::cout << lines[i];
        noc();
        std::cout << std::endl;
    }
}

static void
bash(std::string &line) {
    std::string cmd = line.substr(1, line.size());
    if (system(cmd.c_str()) != 0)
        log("warning: failed to ls\n", repled::error_color);
}

static void
discard(repled::RawInput &ri, std::vector<std::string> &lines) {
    auto to_lower = [](std::string &str) {
        for (char& c : str)
            c = std::tolower(static_cast<unsigned char>(c));
    };

    std::string line = "";
    while (true) {
        // red();
        std::cout << repled::error_color();
        std::string line = repled::getln(ri, "Discard the current session? [Y/n]: ", HIST, true, SS);
        noc();
        to_lower(line);
        if (line == "" || line == "y" || line == "yes") {
            SS.braces = SS.parens = SS.brackets = 0;
            lines.clear();
            break;
        }
        else if (line == "n" || line == "no") {
            std::cout << std::endl;
            return;
        }
        else
            log("invalid input\n");
    }

    LINENO = 0;
    std::cout << std::endl;
}

static void
ee(void) {
    log("Hey! This is not vim! D:\n", repled::error_color);
    log("Did you mean " QUIT "\n", repled::msg_color);
}

static void
show_vars(std::shared_ptr<Ctx> &ctx) {
    auto vars = ctx->get_available_variable_names();

    if (vars.size() == 0)
        log("Nothing appropriate\n", repled::msg_color);

    repled::clearln(0, true);
    for (auto &v : vars) {
        repled::clearln(0, true);
        std::cout << v << std::endl;
    }
}

static void
show_funcs(std::shared_ptr<Ctx> &ctx) {
    auto funcs = ctx->get_available_function_names();

    if (funcs.size() == 0)
        log("Nothing appropriate\n", repled::msg_color);

    for (auto &v : funcs) {
        repled::clearln(0, true);
        std::cout << v << std::endl;
    }
}

static void
reset(std::shared_ptr<Ctx> &ctx) {
    std::vector<std::string> vars = ctx->get_available_variable_names();
    std::vector<std::string> funcs = ctx->get_available_function_names();

    log("Removing: [", repled::msg_color);

    for (int i = 0; i < vars.size(); ++i) {
        // red();
        std::cout << repled::error_color();
        std::cout << vars[i];
        noc();
        if (i != vars.size()-1) {
            // gray();
            std::cout << repled::msg_color();
            std::cout << ", ";
            noc();
        }
    }

    if (funcs.size() > 0 && vars.size() > 0) {
        // gray();
        std::cout << repled::msg_color();
        std::cout << ", ";
        noc();
    }

    for (int i = 0; i < funcs.size(); ++i) {
        // red();
        std::cout << repled::error_color();
        std::cout << funcs[i];
        noc();
        if (i != funcs.size()-1) {
            //gray();
            std::cout << repled::msg_color();
            std::cout << ", ";
            noc();
        }
    }

    // gray();
    std::cout << repled::msg_color();
    std::cout << "]" << std::endl;
    noc();

    ctx = std::make_shared<WorldCtx>();
}

static void
show_hist(void)
{
    for (size_t i = 0; i < HIST.size(); ++i) {
        std::cout << HIST.at(i) << std::endl;
    }
}

static void
handle_repl_arg(repled::RawInput &ri, std::string &line, std::vector<std::string> &lines, std::vector<std::string> &include_dirs, std::shared_ptr<Ctx> &ctx) {
    std::vector<std::string> lst = split_on_space(line);
    std::vector<std::string> args(lst.begin()+1, lst.end());

    if (lst[0] == RM_ENTRY)
        rm_entries(args, lines);
    else if (lst[0] == EDIT_ENTRY)
        edit_entry(ri, args, lines);
    else if (lst[0] == LIST_ENTRIES)
        ls_entries(lines);
    else if (lst[0] == IMPORT)
        import_file(args, lines, include_dirs);
    else if (lst[0] == CLEAR)
        clearscrn();
    else if (lst[0] == QUIT) {
        std::cout << "\033[1E";
        repled::clearln(50);
        std::cout << "\033[A";
        ri.~RawInput();
        exit(0);
    }
    else if (lst[0] == DISCARD)
        discard(ri, lines);
    else if (lst[0] == EE)
        ee();
    else if (lst[0] == VARS)
        show_vars(ctx);
    else if (lst[0] == FUNCS)
        show_funcs(ctx);
    else if (lst[0] == RESET)
        reset(ctx);
    else if (lst[0] == HELP)
        help();
    else if (lst[0] == AUTO)
        repled::show_prefix_trie();
    else if (lst[0] == HISTORY)
        show_hist();
    else
        log("unknown command sequence `" + lst[0] + "`\n", repled::msg_color);
}

std::shared_ptr<Ctx>
repl::run(std::vector<std::string> &include_dirs, std::string *oneshot_output, std::string *oneshot_prompt) {
    try_clear_repl_history();

    repled::init(CMD_OPTION_ASCPL);

    repled::RawInput ri;

    std::vector<std::string> keywords = COMMON_EARLKW_ASCPL;
    std::vector<std::string> types    = {};
    std::string comment               = COMMON_EARL_COMMENT;

    std::shared_ptr<Ctx> ctx = std::make_shared<WorldCtx>();
    read_repl_history();

    while (true) {
        bool ready = false;
        std::vector<std::string> lines = {};
        while (true) {
            bool closed_braces = !SS.braces && !SS.brackets && !SS.parens;

            std::string line = "";
            if (oneshot_prompt) {
                line = repled::getln(ri, *oneshot_prompt, HIST, !closed_braces, SS);
            } else {
                line = repled::getln(ri, std::to_string(LINENO)+": ", HIST, !closed_braces, SS);
            }
            analyze_new_line(line);

            if (oneshot_output != nullptr) {
                REPL_HIST += line + "\n";
                save_repl_history();
                *oneshot_output = line;
                std::cout << "\033[1E";
                repled::clearln(50);
                std::cout << "\033[A";
                ri.~RawInput();
                return nullptr;
            }

            if (line == "" && closed_braces) {
                break;
            }
            else if (line[0] == ':') {
                repled::clearln(line.size());
                handle_repl_arg(ri, line, lines, include_dirs, ctx);
            }
            else {
                if (line != "") {
                    lines.push_back(line);
                    HIST.push_back(line);
                    ready = true;
                }
                ++LINENO;
                std::cout << std::endl;
                std::cout << "\033[K";
            }
        }

        if (lines.size() != 0)
            repled::clearln(lines.back().size());
        else {
            std::cout << std::endl;
        }
        repled::clearln(0, true);

        std::string combined = "";

        std::for_each(lines.begin(), lines.end(), [&](auto &s) {combined += s+"\n";});
        REPL_HIST += combined;
        lines.clear();
        LINENO = 0;

        std::unique_ptr<Program> program = nullptr;
        std::unique_ptr<Lexer> lexer = nullptr;

        try {
            lexer = lex_file(combined, "", keywords, types, comment);
        }
        catch (const LexerException &e) {
            std::cerr << "Parser error: " << e.what() << std::endl;
            continue;
        }
        try {
            program = Parser::parse_program(*lexer.get(), "EARL-Builtin-REPLv" VERSION);
        } catch (const ParserException &e) {
            std::cerr << "Parser error: " << e.what() << std::endl;
            continue;
        }

        WorldCtx *wctx = dynamic_cast<WorldCtx*>(ctx.get());
        wctx->add_repl_lexer(std::move(lexer));
        wctx->add_repl_program(std::move(program));

        for (size_t i = 0; i < wctx->stmts_len(); ++i) {
            Stmt *stmt = wctx->stmt_at(i);
            if (!stmt->m_evald) {
                try {
                    auto val = Interpreter::eval_stmt(stmt, ctx);
                    if (val) {
                        repled::clearln(0, true);

                        // Clear next line
                        std::cout << "\033[1E";
                        repled::clearln(0);
                        std::cout << "\033[A";

                        std::vector<std::shared_ptr<earl::value::Obj>> params = {val};
                        std::cout << repled::stdout_color();
                        (void)Intrinsics::intrinsic_print(params, ctx, nullptr);
                        std::cout << " -> ";
                        std::cout << repled::stdout_type_color();
                        std::cout << earl::value::type_to_str(val->type())
                                  << std::string(std::string("[Enter to Evaluate]").size(), ' ')
                                  << std::endl;
                        noc();

                        int cursor_row = repled::get_cursor_row();
                        int terminal_height = repled::get_terminal_height();
                        if (cursor_row >= terminal_height) {
                            std::cout << std::endl;
                            std::cout << "\033[A" << std::flush; // move cursor up one line
                        }
                    }
                }
                catch (InterpreterException &e) {
                    std::cerr << "Interpreter error: " << e.what() << std::endl;
                }
            }
        }

        save_repl_history();
    }

    return nullptr;
}

std::string
repl::oneshot(std::string &prompt) {
    std::vector<std::string> include_dirs = {};
    std::string res = "";
    run(include_dirs, &res, &prompt);
    return res;
}
