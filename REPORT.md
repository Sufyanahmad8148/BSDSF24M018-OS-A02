# Report

## Feature-2: Long Listing Format (v1.1.0)

**Q1. What is the difference between stat() and lstat()? When is lstat() more appropriate in ls?**

Both system calls fill a `struct stat` with information about a file. The difference is how they treat a symbolic link. `stat()` follows the link and returns information about the file the link points to. `lstat()` does not follow it and returns information about the link itself. In `ls -l`, `lstat()` is the right choice, because a symbolic link must be shown as a link (type `l`) with its own permissions and size. With `stat()`, a link would look like the target file, and a broken link would cause an error because its target does not exist.

**Q2. How can bitwise operators and macros extract file type and permissions from st_mode?**

`st_mode` is an integer in which different bits mean different things. The upper bits store the file type and the lower bits store the permissions. To test a bit we use the bitwise AND operator `&` with a predefined mask. For example, `mode & S_IRUSR` is non-zero only if the owner read bit is set, so `(mode & S_IRUSR) ? 'r' : '-'` gives the `r` or `-` character. The same idea works for `S_IWUSR`, `S_IXUSR`, `S_IRGRP`, `S_IROTH` and so on. For the file type, we can use `mode & S_IFMT` to keep only the type bits and compare the result with `S_IFDIR` (directory) or `S_IFREG` (regular file). The macros `S_ISDIR(mode)` and `S_ISLNK(mode)` do exactly this. The special bits `S_ISUID`, `S_ISGID` and `S_ISVTX` are tested the same way and change the execute character to `s`, `S`, `t` or `T`.
