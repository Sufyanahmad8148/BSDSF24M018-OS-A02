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
