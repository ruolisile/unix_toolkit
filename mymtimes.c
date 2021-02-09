#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/times.h>
#include <time.h>


//mymtimes
long int get_mtime(char *path);
void mtime(char *path, int count[], time_t curr_time);
void print_t(time_t);

int main(int argv, char *argvs[])
{
    time_t curr_time;
    time(&curr_time);

    // printf("current time: %s%ld\n", ctime(&curr_time), curr_time);
    int count[24];
    for (size_t i = 0; i < 24; i++)
    {
        count[i] = 0;
    }

    if (argv == 1)
    {
        mtime(".", count, curr_time);
    }
    else
    {
        mtime(argvs[1], count, curr_time);
    }

    for (size_t i = 1; i <= 24; i++)
    {
        time_t hour = curr_time - 3600 * i;
        print_t(hour);
        printf(": %d\n", count[i - 1]);
    }
}

void mtime(char *path, int count[], time_t curr_time)
{
    char *path_cp;
    if (path[strlen(path) - 1] != '/')
    {
        path_cp = malloc(strlen(path) + 1 + 1);
        strcpy(path_cp, path);
        strcat(path_cp, "/");
    }
    else
    {
        path_cp = malloc(strlen(path) + 1);
        strcpy(path_cp, path);
    }

    struct dirent *dp;
    DIR *dir = opendir(path_cp);
    if (!dir)
    {
        perror(path_cp);
        return;
    }
    while ((dp = readdir(dir)) != NULL)
    {
        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
        {
            continue;
        }
        if (dp->d_type == DT_DIR)
        {
            char new_dir[128];
            //printf("last char is %c\n", dir[strlen(dir) - 1]);
            if (path_cp[strlen(path_cp) - 1] == '/')
            {
                //  printf("with /\n");
                strcpy(new_dir, path_cp);
                strcat(new_dir, dp->d_name);
            }

            mtime(new_dir, count, curr_time);
        }
        else
        {
            //get file modification time
            //char *f_path = malloc(strlen(path_cp) + strlen(dp->d_name) + 1);
            char f_path[512];

            strcpy(f_path, path_cp);
            strcat(f_path, dp->d_name);
            struct stat attr;
            stat(f_path, &attr);

            int index = (curr_time - attr.st_mtime) / 3600;

            if (index < 24)
            {
                count[index]++;
            }

            // free(f_path);
        }
    }
    free(path_cp);
}

void print_t(time_t hour)
{
    char s[512];
    struct tm *p = localtime(&hour);
    strftime(s, 512, "%a %b %d %T %Y", p);
    printf("%s", s);
}