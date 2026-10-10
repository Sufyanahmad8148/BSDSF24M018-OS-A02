/*
* Programming Assignment 02: lsv1.1.0
* This is the source file of version 1.1.0 (adds -l long listing)
* Usage:
*       $ lsv1.1.0
*       $ lsv1.1.0 -l
*       $ lsv1.1.0 -l /home /etc
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

extern int errno;

void do_ls(const char *dir, int long_flag);
void do_ls_long(const char *dir);
void get_permissions(mode_t mode, char *str);
const char *user_name(uid_t uid, char *buf);
const char *group_name(gid_t gid, char *buf);

int main(int argc, char *argv[])
{
    int opt;
    int long_flag = 0;

    while ((opt = getopt(argc, argv, "l")) != -1)
    {
        switch (opt)
        {
        case 'l':
            long_flag = 1;
            break;
        default:
            fprintf(stderr, "Usage: %s [-l] [directory...]\n", argv[0]);
            exit(1);
        }
    }

    if (optind == argc)
    {
        do_ls(".", long_flag);
    }
    else
    {
        for (int i = optind; i < argc; i++)
        {
            printf("Directory listing of %s : \n", argv[i]);
            do_ls(argv[i], long_flag);
            puts("");
        }
    }
    return 0;
}

void do_ls(const char *dir, int long_flag)
{
    if (long_flag)
    {
        do_ls_long(dir);
        return;
    }

    struct dirent *entry;
    DIR *dp = opendir(dir);
    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory : %s\n", dir);
        return;
    }
    errno = 0;
    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;
        printf("%s\n", entry->d_name);
    }

    if (errno != 0)
    {
        perror("readdir failed");
    }

    closedir(dp);
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

void do_ls_long(const char *dir)
{
    struct dirent *entry;
    struct stat st;
    char path[1024];
    char perms[11];
    char ubuf[32], gbuf[32], tmp[32];
    long total = 0;
    int w_link = 0, w_user = 0, w_group = 0, w_size = 0;
    int len;

    DIR *dp = opendir(dir);
    if (dp == NULL)
    {
        fprintf(stderr, "Cannot open directory : %s\n", dir);
        return;
    }

    /* PASS 1: find the widest value in each column and the total */
    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;
        snprintf(path, sizeof(path), "%s/%s", dir, entry->d_name);
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

    /* PASS 2: go back to the start and print every line */
    rewinddir(dp);
    while ((entry = readdir(dp)) != NULL)
    {
        if (entry->d_name[0] == '.')
            continue;
        snprintf(path, sizeof(path), "%s/%s", dir, entry->d_name);
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
               entry->d_name);
    }

    closedir(dp);
}
