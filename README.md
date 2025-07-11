_NOTE:_ This package can be installed through `forge` [ https://github.com/malloc-nbytes/forge ].

_NOTE_: _This language is currently being rewritten to be a bytecode interpreter. See branch `rewrite` for updates._

_NOTE_: _This language is still in the infant stage and it is _NOT_ recommended to use in any real or professional capacity._

_NOTE_: As of now, the only supported platform is Linux, and no guarantees for MacOS.

_NOTE_: (If contributing) do not modify the `README.html` as it is auto generated from `README.org` by https://orgmode.org/ (Emacs Org Mode).

![EARL logo](./misc/earl-logo-resized.jpg)

# Introduction

EARL (Evaluate And Run Language) is a gradually typed programming language that introduces higher level programming concepts to BASH scripting,
and designed to replace BASH scripts.
It is most similar to Python and Rust but is gradually typed.

It has (and plans to have):
- [X] Object Oriented Programming
- [X] fully interactive REPL
- [X] standard library
- [X] syntax highlighting for common editors (VSCode, Emacs, Vim, NeoVim)
- [X] extensive documentation
- [-] package manager (WIP) (see https://github.com/malloc-nbytes/earlmgr)
- [-] the ability to be translated to Python (WIP) (see the =--to-py= flag in =earl --help=)

# Quick Examples

This examples performs =ls -lah= and takes the permissions and the filename
and prints them.

Here is the BASH version:

```
files=$(ls -lah)

echo "$files" | while read -r line; do
    # Skip the header
    if [[ "$line" =~ ^total ]]; then
        continue
    fi

    permissions=$(echo "$line" | awk '{print $1}')
    filename=$(echo "$line" | awk '{print $9}')

    echo "Permissions: $permissions - File: $filename"
done
```

And here is the EARL version:
```
$"ls -lah" |> let files;

foreach f in files.split("\n")[1:] {
    let parts = f.split(" ");
    let permissions, filename = (parts[0], parts.back());
    println(f"Permissions: {permissions} - File: {filename}");
}
```

This is a script to list processes matching the given input
and asks which one(s) to kill:

```
module Main

import "std/script.rl"; as scr

fn handle_input(num, candidates) {
    if num == 'a' || num == "all" {
        let pids = candidates.map(|info| {return info[0];});
        for i in 0 to len(pids) {
            println("killing: ", candidates[i]);
            $"kill " + str(pids[i]);
        }
    }
    else if num == 'q' || num == "quit" {
        println("quitting...");
        exit(0);
    }
    else {
        let idx = int(num);
        if idx < 0 || idx >= len(candidates) {
            panic(f"index {idx} is out of range of length ", len(candidates));
        }
        println("killing: ", candidates[idx]);
        $"kill " + str(candidates[idx][0]);
    }
}

if len(argv()) < 2 {
    panic("Usage: ", argv()[0], " <proc>");
}

let max_display_chars, program_name = (
    50,        # Truncate output because it can get long
    argv()[1], # User supplied CLI arg
);

# Get all PIDs and process names
let candidates = scr::get_pid(program_name)
    .rev()[1:] # Cut off this earl proc
    .rev();    # Undo rev

if len(candidates) == 0 {
    println("no processes found");
    exit(0);
}

println(
    "Choose a number or list of numbers, `a` for all, or `q` to quit"
);

for i in 0 to len(candidates) {
    let pid, cmd = (
        candidates[i][0],
        candidates[i][1],
    );
    if len(cmd) > max_display_chars {
        println(f"  {i}: ({pid}, ", cmd.substr(0, max_display_chars), "...)");
    }
    else {
        println(f"  {i}: ({pid}, {cmd})");
    }
}

# Handle user input
input("[a(ll), q(uit), ", 0, '-', len(candidates)-1, "]: ")
    .split(" ")                   # could have multiple inputs
    .filter(|s|{return s != "";}) # remove empty entries
    .foreach(|k|{handle_input(k, candidates);});
```

For the manual on how to use the language, see https://malloc-nbytes.github.io/EARL-web/

# Compiling

Requirements:
1. `c++17`
2. `cmake` (at least v3.25.1)
3. `doxygen` (optional) (for c++ source code documentation [good for doing development on the interpreter])

Users compiling for the first time should do

```
    cd EARL
    mkdir build
    cd build
    cmake -S ../ -B .
```

_Note_: You can also supply a `prefix` option to changed the default installation location (`/usr/local`) by using `-DINSTALL_PREFIX`<prefix>`.
_However, there must be an `include/` and `bin/` directory inside of the prefix that is supplied_.

This will create the Makefile. Use `make <opt>` where `<opt>` is one of,

```
- make -> builds the project
- make clean -> cleans the project
- make test -> build the project and runs tests (stdlib _must- be installed)
- make docs -> generate the c++ source code documentation Doxygen
```

# Installation
Once the configuration step in Compiling is done, use the following to install EARL as well as the stdlib.

```
  cd build
  make
  sudo make install
  make test
```

To uninstall, simply do `sudo make uninstall`.

# Syntax Highlighting

Syntax highlighting for Emacs, Vim, and VSCode and can be installed here https://github.com/malloc-nbytes/EARL-language-support

# earlmgr

`earlmgr` is a script similar to Python's =pip= and can be downloaded here https://github.com/malloc-nbytes/earlmgr.

_Note_: `earlmgr` is a WIP. USE AT YOUR OWN RISK!

# Contributing

## Style Guide

### Functions

- Functions should be formatted as such:

```
<return type>
<name>(<params>) {
  body...
}
```

- All indents should be 4 spaces and do not use tabs.
- Macros should be in CAPS. The only exception is for when you want to treat a macro as a function.
- Always put a comment after =#endif= with what it is ending.

### Structs

```
struct <NameOfStruct> {
  ...
}
```

- All struct names must use pascal case =<NameOfStruct>=.

### Misc

- All `if`, `for`, `while` etc. should have brackets (the left bracket should be on the same line as the statement).
  While not required, if a conditional/loop only has one statement, you can ommit the brackets. Just make sure it
  is on a different line.
- `+`, `-`, `*`, `/`, etc. does not need whitespace, but `` should.
- Lines have a soft limit of 85 characters.
- `typedef`'d types should have `_t` appended to it.
  Example:

```
typedef int my_type_t;
```

- All variable, function, and macro names should be =snake_case=.
- All file names have a hyphen ('-') to separate words.
- _Remove unnecessary trailing whitespace_.
- _Disable auto-formatters_ (unless it conforms with the C default style of BSD).

# Documentation

## EARL Language Reference

https://malloc-nbytes.github.io/EARL-web/

## Autogenerated Development Docs

EARL uses https://doxygen.nl/ (Doxygen) to auto generate documentation for the source code of EARL.

All header files should have doxygen comments explaining what is necessary. Do not put
any in `.cpp` files (unless there is no related header for it).

Please read the https://www.doxygen.nl/manual/ (Doxygen documentation) (or at least what is relevant) before documenting the code. At the very least,
refer to other files and follow the current way Doxygen comments are written.

If new directories are created and the files need documentation,
edit `EARL/src/Doxyfile` under the `Configuration options related to the input files`,
the `INPUT` switch by adding the directory(s) and file(s) that are needed.

To see the documentation, run `make docs` or `make all` and open `EARL/docs/html/index.html` in a browser.

# Changelog

## [0.9.6]
### Added
- =try= / =catch= blocks.

## [0.9.6]
### Added
- Intrinsic =REPL_input()=

## [0.9.5]
### Added
- Intrinsics =persist()=, =persist_lookup()=, and =persist_del()=.
- =format()= intrinsic.

### Fixed
- Fstrs can now identify builtin identifiers i.e., =__FILE__=, =__OS__= etc.

## [0.9.4]
### Fixed
- Segementation fault when using a =foreach= loop over an empty string.

## [0.8.9]

### Added
- New portable flag to bake the stdlib into the interpeter

## [0.8.8]

### Added
- =flush= intrinsic function.

## [0.8.5] - 2024-12-01

### Added
- New containers in Stdlib

### Changed
- Reorganized imports.

## [0.8.4] - 2024-16-11

### Added
- =with= statements.

## [0.8.3] - 2024-12-11

### Added
- New flag =disable-implicit-returns=.
- New flag =--oneshot=.

### Changed
- You can now implicitly return.

## [0.8.1] - 2024-06-11

### Added
- =unwrap_or= member intrinsic for =option= datatype.

## [0.7.9] - 2024-01-11

### Added
- New intrinsic function =copy=.
- Added release script.

## [0.7.7] - 2024-31-10

### Added
- New Colors module.
- New =case= expression.

### Fixed
- Not being able to escape certain sequences in characters.

## [0.7.7] - 2024-29-10

### Added
- =use= and =exec= statements. This allows to import external shell scripts.
- =-e= flag to fail when bash fails.

## [0.7.5] - 2024-27-10

### Added
- OS and MODULE builtin identifiers.
- =--batch=, =-b= flags for running multiple scripts.

## [0.7.4] - 2024-26-10

### Added
- Event listeners.
- Themes in REPL.
- Configuration file.

## [0.7.2] - 2024-26-10

### Added
- CLI flag -I and --include.
- CLI flag -i and --import.

## [0.7.0] - 2024-25-10

### Added
- New REPL command sequence `auto`.

### Changed
- REPL autocomplete menu now uses a prefix trie.

## [0.6.9] - 2024-22-10

### Added
- Added multiline bash commands

## [0.6.8] - 2024-14-10

### Added
- REPL keywords autocomplete.
- REPL parens count to know when exited suspended eval-state.

## [0.6.7] - 2024-11-10

### Added
- REPL syntax highlighting.
- REPL emacs keybinds.

## [0.6.5] - 2024-9-10

### Added
- Bash literal piping.

## [0.6.5] - 2024-8-10

### Added
- =@experimental= flag.

## [0.6.4] - 2024-7-10

### Added
- set_flag, unset_flag, new debug flags.
- Updated stdlib-docs-gen.rl to accomodate new doccoments

## [0.6.3] - 2024-5-10

### Added
- Doc Comments to variables, functions, enums, and classes.

## [0.6.1] - 2024-4-10

### Added
- Bash commands

## [0.6.0] - 2024-2-10

### Changed
- [MAJOR] Imports now use expression rather than string literals

## [0.5.9] - 2024-30-9

### Added
- =--to-py= option to convert EARL to Python

## [0.5.8] - 2024-27-9

### Added
- Added cd() to System module
- You can now add aliases to imports.
- New intrinsic =env=.
- New module =Script=.

## [0.5.6] - 2024-23-9

### Added
- [MAJOR] Time datatype

### Fixed
- Segfaulting when running EARL with -c and --watch and a parser error occurs.

## [0.5.5] - 2024-20-9

### Added
- floor and ceil functions to math module

## [0.5.5] - 2024-15-9

### Added
- Various optimizations

## [0.5.4] - 2024-13-9

### Added
- [MAJOR] Option of explicit typing.

## [0.5.1] - 2024-10-9

### Added
- [MAJOR] Tuple destructuring in foreach loops.
- Foreach enumerate over dictionaries.

## [0.5.0] - 2024-8-9

### Added
- More functions to the Str stdlib module.
- New unix_cmd function in the System stdlib module.

## [0.4.9] - 2024-7-9

### Fixed
- Function with `return;` would not actually return from the function.
- -=, /=, and %= giving incorrect values.

### Added
- Assert module in the stdlib.

## [0.4.8] - 2024-6-9

### Added
- Builtin Identifiers __FILE__ and __FUNC__.

## [0.4.8] - 2024-5-9

### Added
- [MAJOR] New REPL functionality.

## [0.4.7] - 2024-3-9

### Added
- New repl command :reset that will clear all declared identifiers.

## [0.4.6] - 2024-2-9

### Added
- Levenshtein distance algorithm on function names and misspelled interpreter flags.
- You can now see functions and variables in scope in the repl.
- Intrinsic function 'warn'

## [0.4.4] - 2024-1-9

### Added
- += operations on strs and lists are now optimized.
- StdLib docs generator.
- Levenshtein distance algorithm for interpreter flags.

## [0.4.3] - 2024-31-8

### Fixed
- spec_mutate() for str did not copy over the allocated characters.

## [0.4.2] - 2024-30-8

### Added
- You can now mutate classes.
- Added better error message with malformed classes.

### Fixed
- Various segfault bugs.

## [0.4.1] - 2024-27-8

### Added
- @const attribute.

## [0.4.0] - 2024-26-8

### Added
- Variable and function caching.
- List map() member intrinsic

## [0.3.9] - 2024-25-8

### Added
- contains() member intrinsic for lists, tuples, and strs.

### Fixed
- You can now use the 'this' keyword in closures inside of classes.

## [0.3.8] - 2024-24-8

### Added
- Power operator =##=.

### Fixed
- Fixed segfaulting on parser errors when hot reloading.
- Fixed the len() unreachable case happening.

### Changed
- All EARL values now have an appropriate to_cxxstring.
- Greatly simplified __intrinsic_print.
- Better closure printing.
- Intrinsics now report better error messages.

## [0.3.6] - 2024-23-8

### Added
- Fstrings.

### Fixed
- Fixed the bug where foreach loops would always take the value that they are iterating over as a reference.

## [0.3.5] - 2024-22-8

### Added
- Bitwise operators.

## [0.3.4] - 2024-21-8

### Fixed
- Using .append() on a list no longer always does an unnecessary copy.
- Updated class deep copy to now actually copy over methods.
- Fixed segfault with incorrect function/closure paremeters are provided in evaluate\_function\_parameters\_wrefs.
- Fixed segfault because function\_exists and function\_get for function context not being the same.

## [0.3.3] - 2024-20-8

### Added
- New flag to watch files for changes and perform hot reloading.
- String optimization
- Implemented the =continue= keyword.

### Fixed
- List/str/dictionary/tuple indexing no longer returns the value as a reference by default.
- Strings can now have delimiters again.
- Fixed the bug where dictionaries would end parsing expressions.

### Changed
- Dictionaries now return none if the value does not exist, and some(V) if it does exist.

## [0.3.1] - 2024-19-8

### Added
- Some recursion optimizations

### Fixed
- Calling a class method that has the same name as a member intrinsic no longer fails.

### Changed
- std/set.rl now uses an AVL tree instead of a regulary binary search tree.

## [0.3.0] - 2024-18-8

### Added
- MAJOR Added new Dictionary type.
- Added new TypeKW type.

### Changed
- Unit types now return false in boolean expressions.

## [0.2.6] - 2024-17-8

### Added
- Added new Char module to stdlib
- Added tuple destructuring to allow for multiple variables to be declared in a single =let= statement.

### Fixed
- Classes allowing duplicate member variables.
- Precedence changes to ranges and slices.
- MAJOR Fixed nested for loop bug where the second enumerator would point to the first if set to it.

## [0.2.4] - 2024-14-8

### Added
- Added list slicing
- [MAJOR] Added much better error reporting during runtime (interpreter.cpp).

### Fixed
- Better error messages for the expect_keyword function in parser.
- The minimum cmake version in the README is now up to date with what it actually it.

## [0.2.2] - 2024-13-8

### Added
- Ranges can now be inclusive using the === symbol.

### Fixes
- Fixed the bug when removing or editing a line of code in the REPL that had a bracket.

### Changed
- MAJOR New build system CMake
- Adjusted minimum CMake version to support Debian

## [0.2.1] - 2024-12-8

### Added
- New name_and_ext function in the OS module.

### Fixes
- Performing str += str caused a segfault because of =char= conversion.
- Fixed having excess semicolons segfaulting. They now throw an error.
- MAJOR Fixed array null access that was causing segfaults on MacOS.
- Fixed allowing the creation of duplicate enums of the same name.
- Fixed segfault with invalid getter operation. It now throws an error.

## [0.2.0] - 2024-11-8

### Added
- Added Intrinsic casting functions.
- MAJOR Added a REPL

### Fixed
- Fixed file handler mode type to except multiple types
- The appropriate unit functions now return a unit type instead of null.

## [0.1.3] - 2024-10-8

### Added
- MAJOR New Tuple type

## [0.1.2] - 2024-09-8

### Changes
- MAJOR =foreach= loops now take an expression (list/range) as the argument.
- MAJOR in =foreach= loops, the enumerator can now have attributes.
- MAJOR =<list>.rev()= does not reverse the list in-place anymore. It now produces a new list with the elements reversed.
- MAJOR changed syntax for the regular =for= loops.

### Added
- Syntax sugar for lists in the form of a range ie [0..10] (range is from 0-9) or ['a'..'z'].
- Member variables and methods in classes now adhere to the @pub and private system

## [0.1.1] - 2024-08-8

### Changes
- Renamed keyword =mod= -> =module=

### Added
- Enums
- Float type
- Unary Expressions
- Better output when printing enumerations
- Import depth
- Enum type security
- Better error messages for type mismatchs

### Fixed

- MINOR Fixed segfault bug when creating a float without a left number ie =.34=.
- MAJOR Fixed integer+float bug
