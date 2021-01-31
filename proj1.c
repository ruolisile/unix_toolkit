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

typedef struct
{
    int size;
    char **items;
} tokenlist;

typedef struct
{
    int size;
    char **items;
} cmd_list;

//parser, reused from COP4610 OS Project 1
char *get_input(void);
tokenlist *get_tokens(char *input);

tokenlist *new_tokenlist(void);
void add_token(tokenlist *tokens, char *item);
void free_tokens(tokenlist *tokens);

//
void prompt();
void mycd(tokenlist *tokens);
void mytree(char *dir, int *level, int curr_level);

int main()
{
    while (1)
    {
        //prompt();
        printf("$ ");
        char *input = get_input();

        if (input == NULL)
        {
            return 0; //exit with ctrl-D
        }
        else if (strlen(input) == 0) //handle newline
        {
            continue;
        }

        tokenlist *tokens = get_tokens(input);
        char *command = (char *)malloc(strlen(tokens->items[0]) + 1);
        strcpy(command, tokens->items[0]);

        if (strcmp(command, "myexit") == 0)
        {
            free(input);
            free(command);
            free_tokens(tokens);
            return 0;
        }
        if (strcmp(command, "mycd") == 0)
        {
            // printf("current dir is %s\n", getenv("PWD"));
            mycd(tokens);
            // printf("dir after change is %s\n", getenv("PWD"));
        }

        if (strcmp(command, "mypwd") == 0)
        {
            printf("%s\n", getenv("PWD"));
        }

        if (strcmp(command, "mytree") == 0)
        {
            if (tokens->size == 1)
            {
                int level = 0;
                mytree(".", &level, level);
            }
            else if (tokens->size == 2)
            {
                int level = 0;
                mytree(tokens->items[1], &level, level);
            }
        }

        free(input);
        free(command);
        free_tokens(tokens);
    } // end of while loop

    return 0;
}

tokenlist *new_tokenlist(void)
{
    tokenlist *tokens = (tokenlist *)malloc(sizeof(tokenlist));
    tokens->size = 0;
    tokens->items = (char **)malloc(sizeof(char *));
    tokens->items[0] = NULL; /* make NULL terminated */
    return tokens;
}

void add_token(tokenlist *tokens, char *item)
{
    int i = tokens->size;
    tokens->items = (char **)realloc(tokens->items, (i + 2) * sizeof(char *));
    tokens->items[i] = (char *)malloc(strlen(item) + 1);
    tokens->items[i + 1] = NULL;
    strcpy(tokens->items[i], item);
    tokens->size += 1;
}

char *get_input(void)
{
    char *buffer = NULL;
    int bufsize = 0;

    char line[5];
    while (fgets(line, 5, stdin) != NULL)
    {
        // printf("line is %s\n", line);
        int addby = 0;
        char *newln = strchr(line, '\n');
        //printf("newln is %s\n", newln);
        if (newln != NULL)
        {
            //  printf("newln is not null\n");
            addby = newln - line;
            // printf("addby is %d\n", addby);
        }
        else
            addby = 5 - 1;

        buffer = (char *)realloc(buffer, bufsize + addby);
        memcpy(&buffer[bufsize], line, addby);
        // printf("buffer is %s\n", buffer);
        bufsize += addby;
        // printf("bufsize is %d\n", bufsize);

        if (newln != NULL)
            break;
    }
    //modified for ctrl-D
    if (bufsize != 0)
    {
        buffer = (char *)realloc(buffer, bufsize + 1);
        buffer[bufsize] = 0;
    }

    //printf("buffer is %s\n", buffer);
    return buffer;
}

tokenlist *get_tokens(char *input)
{
    char *buf = (char *)malloc(strlen(input) + 1);
    strcpy(buf, input);

    tokenlist *tokens = new_tokenlist();

    char *tok = strtok(buf, " ");
    while (tok != NULL)
    {
        add_token(tokens, tok);
        tok = strtok(NULL, " ");
    }

    free(buf);
    return tokens;
}

void free_tokens(tokenlist *tokens)
{
    for (int i = 0; i < tokens->size; i++)
        free(tokens->items[i]);

    free(tokens);
}

void prompt()
{
    //prompt
    char usr[] = "USER"; //get user name
    char *usrptr = usr;
    char machine[] = "HOSTNAME"; //get machine name
    char *machineptr = machine;
    char pwd[] = "PWD"; //get word dir
    char *pwdptr = pwd;

    printf("%s@%s: %s $ ", getenv(usrptr), getenv(machineptr), getenv(pwdptr));
}

void mycd(tokenlist *tokens)
{
    //cd to home
    if (tokens->size == 1)
    {
        //get home dir
        char *home = getenv("HOME");
        int flag = chdir(home);
        char *cwd = getcwd(NULL, 0);
        setenv("PWD", cwd, 1);
        free(cwd);
    }
    //cd to a directory
    //after cd to home, cd doesn't work
    if (tokens->size == 2)
    {
        int flag = chdir(tokens->items[1]);
        char *cwd = getcwd(NULL, 0);
        if (flag != 0)
        {
            perror(tokens->items[1]);
        }
        setenv("PWD", cwd, 1);
        free(cwd);
    }
}

void mytree(char *dir, int *level, int curr_level)
{
    struct dirent **namelist;
    int i, n;
    n = scandir(dir, &namelist, 0, alphasort);
    if (n < 0)
    {
        printf("mytree: No such file or directory\n");
        return;
    }
    else
    {
        if (*level == 0)
        {
            printf("%s\n", dir);
        }
        for (i = 0; i < n; i++)
        {
            if (strcmp(namelist[i]->d_name, "..") == 0 | strcmp(namelist[i]->d_name, ".") == 0)
            {
                continue;
            }
            if (namelist[i]->d_type == DT_DIR)
            {
                //  printf("level is %d\n", *level);
                printf("|");
                for (size_t i = 0; i <= *level; i++)
                {
                    printf("----");
                }

                printf("%s\n", namelist[i]->d_name);
                *level += 1;
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
                mytree(new_dir, level, *level);
            }
            else
            {
                //  printf("current level is %d\n", curr_level);
                printf("|");
                for (size_t i = 0; i <= curr_level; i++)
                {
                    printf("----");
                }
                printf("%s\n", namelist[i]->d_name);
            }
            free(namelist[i]);
            //decrease level by 1 when all files in a dir is listed
            if (i == n - 1)
            {
                *level -= 1;
            }
        }
    }
    free(namelist);
}