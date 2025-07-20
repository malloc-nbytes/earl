# MIT License

# Copyright (c) 2023 malloc-nbytes

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

module System

### Function
#-- Name: ls
#-- Parameter: path: str
#-- Returns: list
#-- Description:
#--   List all items at the path `path` and return a list of
#--   all elements it contains.
@pub fn ls(path) {
    return __internal_ls__(path);
}
### End

### Function
#-- Name: isdir
#-- Parameter: path: str
#-- Returns: bool
#-- Description:
#--   Return `true` if `path` is a directory or `false` if otherwise.
@pub fn isdir(path: str): bool {
    return __internal_isdir__(path);
}
### End

# ### Function
# #-- Name: cd
# #-- Parameter: path: str
# #-- Returns: unit
# #-- Description:
# #--   Change the current working directory to `path`.
# @pub fn cd(path: str): unit {
#     __internal_cd__(path);
# }
# ### End

### Function
#-- Name: mkdir
#-- Parameter: name: str
#-- Returns: unit
#-- Description:
#--   Create a new directory in the `cwd` called `name`.
@pub fn mkdir(name) {
    __internal_mkdir__(name);
}
### End

### Function
#-- Name: fullpath_mkdir
#-- Parameter: fullpath: str
#-- Returns: unit
#-- Description:
#--   Creates a full path directory as well as a file
#--   i.e., ./dir1/dir2/dir3/file.txt. The last entry
#--   in the path will be used as the file to be created.
#--   It is functionally equivalent to: `mkdir -p <path> && touch <path>/file.txt`.
@pub fn fullpath_mkdir(fullpath) {
    if len(fullpath) == 0 {
        panic("fullpath_mkdir: the path specified cannot be empty");
    }

    let parts = fullpath.split("/");

    let curpath = "";
    for i in 0 to len(parts)-1 {
        curpath = curpath + parts[i];
        mkdir(curpath);
        curpath = curpath + '/';
    }
    curpath = curpath + parts.back();

    let f = open(curpath, "w");
    f.write("");
    f.close();
}
### End

### Function
#-- Name: name_and_ext
#-- Parameter: path: str
#-- Returns: tuple<option<str>, option<str>>
#-- Description:
#--   Returns a tuple of filename and extension. If either the name or extension
#--   cannot be found, the respective one will be set to `none`.
@pub fn name_and_ext(path) {
    if (len(path) == 0) {
        return (none, none);
    }

    let ext = "";
    let name = "";
    let period = none;

    for i in len(path)-1 to 0 {
        if (path[i] == '.') {
            period = some(i);
            break;
        }
        ext += path[i];
    }

    ext = ext.rev();

    if (!period) {
        return (some(ext), none);
    }

    name = path.substr(0, period.unwrap());

    if (len(name) == 0) {
        return (none, some(ext));
    }

    return (some(name), some(ext));
}
### End

### Function
#-- Name: move
#-- Parameter: path_from: str
#-- Parameter: path_to: str
#-- Returns: unit
#-- Description:
#--   Move the file `path_from` to `path_to`.
@pub fn move(path_from, path_to) {
    __internal_move__(path_from, path_to);
}
### End

### Function
#-- Name: cmd
#-- Parameter: cmd: str
#-- Returns: int
#-- Description:
#--   Run the bash command `cmd`. Returns the exit code.
@pub fn cmd(cmd: str) {
    return __internal_unix_system__(cmd);
}
### End

### Function
#-- Name: cmd_wcheck
#-- Parameter: cmd: str
#-- Returns: unit
#-- Description:
#--   Run the bash command `cmd` and checks the exit code.
#--   Will print a warning if the exit code is not zero.
@pub fn cmd_wcheck(cmd: str) {
    let status = cmd(cmd);
    if status != 0 {
        warn(f"command `{cmd}` failed with exit code {status}");
    }
}
### End

### Function
#-- Name: cmd_onfail
#-- Parameter: cmd: str
#-- Parameter: onfail: closure
#-- Returns: unit
#-- Description:
#--   Run the bash command `cmd` and checks the exit code.
#--   Will run `onfail` if the exit code is not 0.
@pub fn cmd_onfail(cmd: str, onfail: closure): unit {
    let status = cmd(cmd);
    if status != 0 {
        onfail();
    }
}
### End

### Function
#-- Name: cmdstr
#-- Parameter: cmd: str
#-- Returns: str
#-- Description:
#--   Run the bash command `cmd` and return the
#--   output as a `str`.
@pub fn cmdstr(cmd: str): str {
    return __internal_unix_system_woutput__(cmd)[1];
}
### End

### Function
#-- Name: cmdstr_wexitcode
#-- Parameter: cmd: str
#-- Returns: tuple<int, str>
#-- Description:
#--   Run the bash command `cmd` and return the
#--   output as a tuple of `(exit_code, output)`.
@pub fn cmdstr_wexitcode(cmd: str): tuple {
    return __internal_unix_system_woutput__(cmd);
}
### End

### Function
#-- Name: get_all_files_by_ext
#-- Parameter: dir: @const @ref str
#-- Parameter: ext: @const @ref str
#-- Returns: list<str>
#-- Description:
#--   Get all files in the directory `dir` that have the
#--   file extension `ext`.
#-- Example:
#--   let cppfiles = get_all_files_by_ext(".", "cpp");
@pub fn get_all_files_by_ext(@const @ref dir: str, @const @ref ext: str): list {
    let matches = [];

    with files = ls(dir) in
    foreach f in files {
        with parts = name_and_ext(f) in
        if (parts[0] && parts[1] && parts[1].unwrap() == ext) {
            matches.append(f);
        }
    }

    return matches;
}
### End

### Function
#-- Name: cmd_on_files
#-- Parameter: fp: @const @ref str
#-- Parameter: cmd: @const @ref str
#-- Returns: unit
#-- Description:
#--   Perform `cmd` on all files in dir `fp`, or if `fp` is
#--   a single file, perform `cmd` on it.
@pub fn cmd_on_files(@const @ref fp: str, @const @ref cmd: str): unit {
    if (isdir(fp)) {
        let files = ls(fp).filter(|f| { return !isdir(f); });
        foreach f in files {
            $format(cmd, ' ', f);
        }
    } else {
        $format(cmd, ' ', fp);
    }
}
### End
