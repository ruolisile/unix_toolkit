/*A simple unix toolkit project
* @Author Liting Zhang
* @Date 1/30/2021
*/
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


void mytree(char *dir, char *prefix, long *num_dir, long *num_file);
char *prefix_cat(const char *prefix, const char *ext);

    static long num_dir = 1;
    static long num_file = 0;
int main(int argv, char *argvs[])
{
    if (argv == 1)
    {
        printf(".\n");
        mytree(".", "|----", &num_dir, &num_file);
    }
    else
    {
        printf("%s\n", argvs[1]);
        mytree(argvs[1], "|----", &num_dir, &num_file);
    }
    printf("\n\n%ld directories, %ld files\n", num_dir, num_file);
    exit(0);
}

//tree doesn't support input redirection
// only inplement output redirection
void mytree(char *dir, char *prefix, long *num_dir, long *num_file)
{
    struct dirent **namelist;
    int i, n;
    char *prefix_ext;
    n = scandir(dir, &namelist, 0, alphasort);
    if (n < 0)
    {
        printf("mytree: No such file or directory\n");
        return;
    }
    else
    {
        for (i = 0; i < n; i++)
        {

            if (strncmp(namelist[i]->d_name, ".", 1) == 0 )
            {
                continue;
            }
            if (namelist[i]->d_type == DT_DIR)
            {
		*num_dir += 1;
                printf("%s%s\n", prefix, namelist[i]->d_name);

                char new_dir[128];
                //printf("last char is %c\n", dir[strlen(dir) - 1]);
                if (dir[strlen(dir) - 1] == '/')
                {
                    //  printf("with /\n");
                    strcpy(new_dir, dir);
                    strcat(new_dir, namelist[i]->d_name);
                }
                else
                {
                    // printf("without /\n");
                    strcpy(new_dir, dir);
                    // printf("new dir is %s\n", new_dir);
                    strcat(new_dir, "/");
                    //printf("new dir is %s\n", new_dir);
                    strcat(new_dir, namelist[i]->d_name);
                }
                prefix_ext = "|----";

                prefix_ext = prefix_cat(prefix, prefix_ext);
                mytree(new_dir, prefix_ext,  num_dir, num_file);
                free(prefix_ext);
            }
            else
            {
                //  printf("current level is %d\n", curr_level);
                *num_file += 1;
                printf("%s%s\n", prefix, namelist[i]->d_name);
            }
            free(namelist[i]);
        }
    }
    free(namelist);
}

char *prefix_cat(const char *prefix, const char *ext)
{
    char *res = malloc(strlen(prefix) + strlen(ext) + 1);
    strcpy(res, prefix);
    strcat(res, ext);
    return res;
}
