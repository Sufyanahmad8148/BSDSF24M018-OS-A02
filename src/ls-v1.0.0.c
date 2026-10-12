/*
* Programming Assignment 02: lsv1.4.0
* This is the source file of version 1.4.0
* Adds: -l long listing (v1.1.0), column display (v1.2.0),
*       -x horizontal (v1.3.0), alphabetical sort (v1.4.0)
* Usage:
*       $ lsv1.4.0
*       $ lsv1.4.0 -l
*       $ lsv1.4.0 -x
*       $ lsv1.4.0 /home /etc
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

extern int errno;

/* the three display modes */
enum { MODE_DEFAULT, MODE_LONG, MODE_HORIZONTAL };

void do_ls(const char *dir, int mode);
void do_ls_long(const char *dir, char **names, int count);
void display_columns(char **names, int count, int maxlen);
void display_horizontal(char **names, int count, int maxlen);
int compare_names(const void *a, const void *b);
int get_terminal_width(void);
void get_permissions(mode_t mode, char *str);
const char *user_name(uid_t uid, char *buf);
const char *group_name(gid_t gid, char *buf);

int main(int argc, char *argv[])
{
    int opt;
    int mode = MODE_DEFAULT;

    while ((opt = getopt(argc, argv, "lx")) != -1)
    {
        switch (opt)
        {
        case 'l':
            mode = MODE_LONG;
            break;
        case 'x':
            mode = MODE_HORIZONTAL;
            break;
        default:
            fprintf(stderr, "Usage: %s [-l | -x] [directory...]\n", argv[0]);
            exit(1);
        }
    }

    if (optind == argc)
    {
        do_ls(".", mode);
    }
    else
    {
        for (int i = optind; i < argc; i++)
        {
            printf("Directory listing of %s : \n", argv[i]);
            do_ls(argv[i], mode);
            puts("");
        }
    }
    return 0;
}

/* Comparison function for qsort: compares two filenames */
int compare_names(const void *a, const void *b)
{
    const char *s1 = *(char **)a;   /* cast to char ** then take the string */
    const char *s2 = *(char **)b;
    return strcmp(s1, s2);
}

/* Returns the terminal width, or 80 if it cannot be found */
int get_terminal_width(void)
{
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1 || w.ws_col == 0)
        return 80;
    return w.ws_col;
}

/* Prints names in columns, "down then across" */
void display_columns(char **names, int count, int maxlen)
{
    if (count == 0)
        return;

    int width = get_terminal_width();
    int col_width = maxlen + 2;          /* longest name + 2 spaces */
    int cols = width / col_width;        /* how many columns fit */
    if (cols < 1)
        cols = 1;
    int rows = (count + cols - 1) / cols; /* total rows, rounded up */

    for (int r = 0; r < rows; r++)
    {
        for (int c = 0; c < cols; c++)
        {
            int idx = r + c * rows;      /* item that belongs here */
            if (idx >= count)
                break;
            printf("%-*s", col_width, names[idx]);
        }
        printf("\n");
    }
}

/* Prints names left to right, wrapping when the line is full */
void display_horizontal(char **names, int count, int maxlen)
{
    if (count == 0)
        return;

    int width = get_terminal_width();
    int col_width = maxlen + 2;
    int pos = 0;                         /* current position on the line */

    for (int i = 0; i < count; i++)
    {
        if (pos + col_width > width && pos > 0)
        {
            printf("\n");                /* line is full, start a new one */
            pos = 0;
        }
        printf("%-*s", col_width, names[i]);
        pos += col_width;
    }
    printf("\n");
}

void do_ls(const char *dir, int mode)
{
    struct dirent *entry;
    DIR *dp = opendir(dir);
    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory : %s\n", dir);
        return;
    }

    char **names = NULL;   /* dynamic array of strings */
    int count = 0;         /* how many names stored */
    int capacity = 0;      /* how many names the array can hold */
    int maxlen = 0;        /* longest name length */

    errno = 0;
    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;

        if (count == capacity)
        {
            capacity = (capacity == 0) ? 16 : capacity * 2;
            names = realloc(names, capacity * sizeof(char *));
            if (names == NULL)
            {
                perror("realloc failed");
                exit(1);
            }
        }

        names[count] = strdup(entry->d_name);
        if (names[count] == NULL)
        {
            perror("strdup failed");
            exit(1);
        }

        int len = strlen(names[count]);
        if (len > maxlen)
            maxlen = len;
        count++;
    }

    if (errno != 0)
    {
        perror("readdir failed");
    }

    closedir(dp);

    /* sort the names after reading and before any display function */
    if (count > 1)
        qsort(names, count, sizeof(char *), compare_names);

    /* check the display mode and call the right function */
    if (mode == MODE_LONG)
        do_ls_long(dir, names, count);
    else if (mode == MODE_HORIZONTAL)
        display_horizontal(names, count, maxlen);
    else
        display_columns(names, count, maxlen);

    /* free the memory */
    for (int i = 0; i < count; i++)
        free(names[i]);
    free(names);
}

/* Converts st_mode into a string like "drwxr-xr-x" */
void get_permissions(mode_t mode, char *str)
{
    /* character 0: file type */
    if (S_ISDIR(mode))        str[0] = 'd';
    else if (S_ISLNK(mode))   str[0] = 'l';
    else if (S_ISCHR(mode))   str[0] = 'c';
    else if (S_ISBLK(mode))   str[0] = 'b';
    else if (S_ISFIFO(mode))  str[0] = 'p';
    else if (S_ISSOCK(mode))  str[0] = 's';
    else                      str[0] = '-';

    /* characters 1-3: owner */
    str[1] = (mode & S_IRUSR) ? 'r' : '-';
    str[2] = (mode & S_IWUSR) ? 'w' : '-';
    str[3] = (mode & S_IXUSR) ? 'x' : '-';

    /* characters 4-6: group */
    str[4] = (mode & S_IRGRP) ? 'r' : '-';
    str[5] = (mode & S_IWGRP) ? 'w' : '-';
    str[6] = (mode & S_IXGRP) ? 'x' : '-';

    /* characters 7-9: others */
    str[7] = (mode & S_IROTH) ? 'r' : '-';
    str[8] = (mode & S_IWOTH) ? 'w' : '-';
    str[9] = (mode & S_IXOTH) ? 'x' : '-';

    /* special bits */
    if (mode & S_ISUID)
        str[3] = (mode & S_IXUSR) ? 's' : 'S';
    if (mode & S_ISGID)
        str[6] = (mode & S_IXGRP) ? 's' : 'S';
    if (mode & S_ISVTX)
        str[9] = (mode & S_IXOTH) ? 't' : 'T';

    str[10] = '\0';
}

/* Returns the user name, or the number if no name exists */
const char *user_name(uid_t uid, char *buf)
{
    struct passwd *pw = getpwuid(uid);
    if (pw != NULL)
        return pw->pw_name;
    sprintf(buf, "%u", (unsigned int)uid);
    return buf;
}

/* Returns the group name, or the number if no name exists */
const char *group_name(gid_t gid, char *buf)
{
    struct group *gr = getgrgid(gid);
    if (gr != NULL)
        return gr->gr_name;
    sprintf(buf, "%u", (unsigned int)gid);
    return buf;
}

/* Long listing: works on the array of names */
void do_ls_long(const char *dir, char **names, int count)
{
    struct stat st;
    char path[1024];
    char perms[11];
    char ubuf[32], gbuf[32], tmp[32];
    long total = 0;
    int w_link = 0, w_user = 0, w_group = 0, w_size = 0;
    int len;

    /* LOOP 1: find the widest value in each column and the total */
    for (int i = 0; i < count; i++)
    {
        snprintf(path, sizeof(path), "%s/%s", dir, names[i]);
        if (lstat(path, &st) == -1)
        {
            perror(path);
            continue;
        }
        total += st.st_blocks;

        len = snprintf(tmp, sizeof(tmp), "%ld", (long)st.st_nlink);
        if (len > w_link) w_link = len;

        len = strlen(user_name(st.st_uid, ubuf));
        if (len > w_user) w_user = len;

        len = strlen(group_name(st.st_gid, gbuf));
        if (len > w_group) w_group = len;

        len = snprintf(tmp, sizeof(tmp), "%ld", (long)st.st_size);
        if (len > w_size) w_size = len;
    }

    printf("total %ld\n", total / 2);

    /* LOOP 2: print every line */
    for (int i = 0; i < count; i++)
    {
        snprintf(path, sizeof(path), "%s/%s", dir, names[i]);
        if (lstat(path, &st) == -1)
            continue;

        get_permissions(st.st_mode, perms);

        printf("%s %*ld %-*s %-*s %*ld %.12s %s\n",
               perms,
               w_link, (long)st.st_nlink,
               w_user, user_name(st.st_uid, ubuf),
               w_group, group_name(st.st_gid, gbuf),
               w_size, (long)st.st_size,
               ctime(&st.st_mtime) + 4,
               names[i]);
    }
}
