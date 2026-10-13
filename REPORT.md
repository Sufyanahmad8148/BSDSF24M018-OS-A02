# Report

## Feature-2: Long Listing Format (v1.1.0)

**Q1. What is the difference between stat() and lstat()? When is lstat() more appropriate in ls?**

Both system calls fill a `struct stat` with information about a file. The difference is how they treat a symbolic link. `stat()` follows the link and returns information about the file the link points to. `lstat()` does not follow it and returns information about the link itself. In `ls -l`, `lstat()` is the right choice, because a symbolic link must be shown as a link (type `l`) with its own permissions and size. With `stat()`, a link would look like the target file, and a broken link would cause an error because its target does not exist.

**Q2. How can bitwise operators and macros extract file type and permissions from st_mode?**

`st_mode` is an integer in which different bits mean different things. The upper bits store the file type and the lower bits store the permissions. To test a bit we use the bitwise AND operator `&` with a predefined mask. For example, `mode & S_IRUSR` is non-zero only if the owner read bit is set, so `(mode & S_IRUSR) ? 'r' : '-'` gives the `r` or `-` character. The same idea works for `S_IWUSR`, `S_IXUSR`, `S_IRGRP`, `S_IROTH` and so on. For the file type, we can use `mode & S_IFMT` to keep only the type bits and compare the result with `S_IFDIR` (directory) or `S_IFREG` (regular file). The macros `S_ISDIR(mode)` and `S_ISLNK(mode)` do exactly this. The special bits `S_ISUID`, `S_ISGID` and `S_ISVTX` are tested the same way and change the execute character to `s`, `S`, `t` or `T`.

## Feature-3: Column Display, Down Then Across (v1.2.0)

**Q1. Explain the general logic for printing items in a "down then across" format. Why is a single loop insufficient?**

First we read all the filenames into an array and find the longest one. The column width is the longest name plus 2 spaces for spacing. The number of columns is the terminal width divided by the column width, and the number of rows is the total number of files divided by the number of columns, rounded up. Then we print row by row. For row `r` and column `c`, the item to print is at index `r + c * rows`. This way the first column holds items 0 to rows-1, the second column holds the next block, and so on, so names run downwards first and then continue in the next column. A single loop that prints the array from start to finish would print the names in one line or one column, because it moves through the array one item after another. In this layout, neighbouring items on the same row are `rows` positions apart in the array, so we need two loops (rows and columns) and an index calculation to jump between them. We must also know the total count and longest name before printing, so a single loop that prints as it reads would not work.

**Q2. What is the purpose of ioctl in this context? What are the limitations of a fixed 80-column width?**

`ioctl` with the `TIOCGWINSZ` request asks the terminal driver for the current window size and fills a `struct winsize`. We read `ws_col` from it to know how many characters fit on one line, so the number of columns can be calculated for the actual window. If we used only a fixed width of 80, the output would not adapt. In a wide terminal, much of the screen would be unused and there would be more rows than needed. In a terminal narrower than 80 columns, the lines would be longer than the window and the terminal would wrap them, which breaks the column alignment and makes the output hard to read. It would also ignore the user resizing the window.

## Feature-4: Horizontal Column Display, -x (v1.3.0)

**Q1. Compare the complexity of "down then across" and "across" printing. Which needs more pre-calculation, and why?**

The "across" (horizontal) logic is simpler. It needs only the terminal width and the column width (longest name plus 2). It then loops once over the array, prints each name, adds the column width to a position counter, and starts a new line when the next name would not fit. It never needs to know the number of rows or columns in advance. The "down then across" (vertical) logic needs more pre-calculation. Before printing anything it must work out the number of columns (terminal width divided by column width) and then the number of rows (total files divided by columns, rounded up). It also needs two nested loops and the index formula `r + c * rows` to pick the right name for each position. So the vertical layout requires more pre-calculation, because the position of each name depends on the total number of rows, which is only known after all names have been counted.

**Q2. Describe the strategy used to manage the display modes. How does the program decide which function to call?**

I used an `enum` with three values: `MODE_DEFAULT`, `MODE_LONG` and `MODE_HORIZONTAL`. In `main`, the `getopt` loop sets a variable `mode` when it finds `-l` or `-x`. If both are given, the last one wins. `main` passes `mode` to `do_ls`. `do_ls` first reads all the filenames into a dynamic array, and then checks `mode`: if it is `MODE_LONG` it calls `do_ls_long`, if it is `MODE_HORIZONTAL` it calls `display_horizontal`, and otherwise it calls `display_columns`, the default "down then across" display. Because the reading code is shared and only the printing function changes, each new mode needs only one new function and one new `if` branch.

## Feature-5: Alphabetical Sort (v1.4.0)

**Q1. Why is it necessary to read all directory entries into memory before sorting? What are the drawbacks for directories with millions of files?**

`readdir()` returns entries one at a time in whatever order the file system stores them, which is not alphabetical. To sort, the program must be able to compare any entry with any other, and it cannot know which name should come first until it has seen all of them. The last entry read could belong at the very top. So all names must be stored in memory first, sorted, and only then displayed. The drawbacks for a directory with millions of files are: it uses a lot of memory (every name is copied with `strdup` plus a pointer in the array); nothing can be shown until all entries are read and sorted, so the user waits with no output; sorting millions of strings takes extra CPU time; and if memory runs out, `realloc` or `strdup` can fail and the program would stop.

**Q2. Explain the purpose and signature of the comparison function required by qsort(). How does it work, and why must it take const void * arguments?**

`qsort()` is a general function that can sort an array of any type, but it does not know how to compare the elements. The comparison function tells it how. Its signature is `int compare(const void *a, const void *b)`. `qsort()` calls it with pointers to two elements of the array and expects an integer back: a negative number if the first element should come before the second, zero if they are equal, and a positive number if the first should come after. In my program the array holds `char *` (strings), so `a` and `b` are pointers to `char *`, that is `char **`. The function casts them with `*(char **)a` to get the strings and returns `strcmp(s1, s2)`, which gives exactly the negative, zero or positive result that `qsort()` needs. The arguments must be `const void *` because `qsort()` is written once for all types, and `void *` is the generic pointer type that can point to anything. `const` promises that the comparison will not modify the elements being compared.

## Feature-6: Colorized Output (v1.5.0)

**Q1. How do ANSI escape codes work to produce color in a standard Linux terminal? Show the sequence for green text.**

ANSI escape codes are special sequences of characters that the terminal does not print but interprets as commands. Each sequence starts with the escape character (`\033`, ASCII 27), followed by `[`, then one or more numbers separated by semicolons, and ends with the letter `m`. The numbers select a style or color: for example 0 means reset, 7 means reverse video, 31 is red, 32 is green and 34 is blue. After a color sequence is printed, everything that follows is shown in that color until another sequence changes it, so we must print the reset code `\033[0m` after the text. The sequence for printing text in green is: `printf("\033[0;32m%s\033[0m", name);` where `\033[0;32m` switches the color to green, `name` is the text printed in green, and `\033[0m` resets the terminal back to the default color.

**Q2. To color an executable file, which bits in st_mode do you check to see if it is executable by the owner, group, or others?**

The execute permission is stored in three bits of `st_mode`: `S_IXUSR` (0100) for the owner, `S_IXGRP` (0010) for the group, and `S_IXOTH` (0001) for others. We test them with the bitwise AND operator. For example, `st_mode & S_IXUSR` is non-zero if the owner can execute the file. To check all three at once, we combine the masks: `st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)` is non-zero if anyone has execute permission, and then the file is printed in green. Directories also have execute bits (they mean "can enter the directory"), so the directory test with `S_ISDIR` must come before the executable test, otherwise directories would be colored green.

## Feature-7: Recursive Listing, -R (v1.6.0)

**Q1. In a recursive function, what is a "base case"? What is the base case in your recursive ls?**

A base case is the condition under which a recursive function stops calling itself and simply returns. Without one, the function would call itself forever. In my recursive `ls`, the recursion happens only when an entry is a directory, so the base case is a directory that has no sub-directories (or cannot be opened, or is empty). In that case the loop that looks for sub-directories finds nothing to call `do_ls` on, the function finishes and returns to the caller. Names that start with `.` (including `.` and `..`) are skipped when reading, so the function never calls itself on the current or parent directory, which would loop forever. Because `lstat()` is used, a symbolic link to a directory is not followed, which also prevents endless loops. Since every directory tree is finite, the recursion always ends.

**Q2. Why must a full path be constructed before the recursive call? What would happen if do_ls("subdir") was called from do_ls("parent_dir")?**

The entry names returned by `readdir()` are only names, not paths, for example `subdir`. They are relative to the directory being read. If the program called `do_ls("subdir")`, `opendir("subdir")` would look for `subdir` inside the directory the program was started from (the current working directory), not inside `parent_dir`. Unless a folder with that name happens to exist there, `opendir` would fail with "Cannot open directory", and the recursion would stop. If a folder with the same name did exist in the starting directory, the program would wrongly list that one instead. The same problem affects `lstat()`, which also needs a real path to find the file. Building the full path `parent_dir/subdir` with `snprintf` gives a path that is correct from the starting directory, so `opendir` and `lstat` find the right folder at any depth.
