/*A simple unix toolkit project
* @Author Liting Zhang
* @Date 2/20/2021
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
#include <signal.h>

typedef struct
{
    int size;
    char **items;
} tokenlist;

typedef struct
{
    int size;
    tokenlist **items;
} cmdlist;

//parser, reused from COP4610 OS Project 1

tokenlist *get_tokens(char *input);
tokenlist *new_tokenlist(void);
void add_token(tokenlist *tokens, char *item);
void free_tokens(tokenlist *tokens);

//I/O redirction
void check_io(tokenlist *tokens, int *input_idx, int *output_idx);
tokenlist *get_argvs(tokenlist *tokens);
void out_redirect(tokenlist *tokens);

//myexit
int myexit(char *input, char *command, tokenlist *tokens);

//mycd
void mycd(tokenlist *tokens);
//mytime
void mytime(tokenlist *tokens, char *home_dir);
static struct tms st_cpu;
static struct tms en_cpu;
//for elapsed time
struct timeval start_t, end_t;

void end_clock();

//mymtimes
void mymtimes(tokenlist *tokens);
long int get_mtime(char *path);
void mtime(char *path, int count[], time_t curr_time);
void print_t(time_t);

//run external command
int exe(char *cmd, tokenlist *tokens);
char *path_search(char *command, char *home_dir);

//pasrsing for pipe
cmdlist *new_cmdlist(void);
cmdlist *get_cmd(tokenlist *tokens);
void free_cmds(cmdlist *cmds);
void exe_pipe(cmdlist *cmds, char *home_dir);

//timeout
//only works for external command

pid_t pid;
void mytimeout(int secs, tokenlist *argvs);
static void timer_handler(int signo)
{
    //  printf("Time out\n");
    kill(pid, SIGALRM);
    exit(0);
}

//variables
static int input_idx = -1;
static int output_idx = -1;

int main()
{
    char home_dir[128];
    int bytes = readlink("/proc/self/cwd", home_dir, sizeof(home_dir) - 1);
    if (bytes >= 0)
        home_dir[bytes] = '\0';
    //printf("%s%d\n", home_dir, strlen(home_dir));

    while (1)
    {
        //prompt();
        printf("%s@%s:%s$ ", getenv("USER"), getenv("HOSTNAME"), getenv("PWD"));
        size_t input_size = 80;
        char *input = (char *)malloc(80 * sizeof(char));
        size_t tmp;
        tmp = getline(&input, &input_size, stdin);
        // printf("%s\n", input);
        if (tmp == -1)
        {
            exit(0); //handle EOF/CTRL-D
        }
        if (strncmp(input, "\n", 1) == 0)
        {
            continue; //handle newline
        }

        input[strlen(input) - 1] = '\0';
        //get tokens
        tokenlist *tokens = get_tokens(input);
        //get command list
        cmdlist *cmds = new_cmdlist();
        cmds = get_cmd(tokens);
        // for (size_t i = 0; i < cmds->size; i++)
        // {
        //     for (size_t j = 0; j < cmds->items[i]->size; j++)
        //     {
        //         printf("%s%d\n", cmds->items[i]->items[j], strlen(cmds->items[i]->items[j]));

        //     }

        // }

        //get the command
        char *command = (char *)malloc(strlen(tokens->items[0]) + 1);
        strcpy(command, tokens->items[0]);
        if (strlen(command) == 0)
        {
            free(command);
            free_tokens(tokens);
            continue; //handle newline and CTRL-D
        }

        //pipe
        if (cmds->size > 1)
        {
            exe_pipe(cmds, home_dir);
            free_cmds(cmds);
            free_tokens(tokens);
            continue;
        }

        //single command & I/O redirection
        input_idx = -1;
        output_idx = -1;
        check_io(tokens, &input_idx, &output_idx);

        if (strcmp(command, "myexit") == 0)

        {
            myexit(input, command, tokens);
        }
        else if (strcmp(command, "mycd") == 0)
        {
            // printf("current dir is %s\n", getenv("PWD"));
            mycd(tokens);
            // printf("dir after change is %s\n", getenv("PWD"));
        }

        else if (strcmp(command, "mypwd") == 0)
        {
            if (output_idx != -1)
            {
                int saved_stdout = dup(1);
                out_redirect(tokens);
                printf("%s\n", getenv("PWD"));
                dup2(saved_stdout, 1);
                close(saved_stdout);
            }
            else
            {
                printf("%s\n", getenv("PWD"));
            }
        }
        else if (strcmp(command, "mytime") == 0)
        {
            if (output_idx != -1)
            {
                int saved_stdout = dup(1);
                out_redirect(tokens);

                tokenlist *argvs = get_argvs(tokens);
                mytime(argvs, home_dir);
                dup2(saved_stdout, 1);
                close(saved_stdout);
                free_tokens(argvs);
            }
            else
            {
                mytime(tokens, home_dir);
            }
        }
        else if (strcmp(command, "mytimeout") == 0)
        {
            tokenlist *argvs = new_tokenlist();
            int secs = atoi(tokens->items[1]);
            //path search
            char *cmd = path_search(tokens->items[2], home_dir);
            if (cmd != NULL)
            {
                add_token(argvs, cmd);
                for (size_t i = 3; i < tokens->size; i++)
                {
                    add_token(argvs, tokens->items[i]);
                }
                pid_t temp;
                if ((temp = fork()) == 0)
                {
                    mytimeout(secs, argvs);
                }
                else
                {
                    waitpid(temp, NULL, 0);
                }
            }
            else
            {
                printf("%s: Command not found\n", tokens->items[2]);
            }
            free_tokens(argvs);
        }
        else
        {
            char *exe_cmd = path_search(command, home_dir);

            //tokenlist *argvs = get_argvs(tokens);
            if (exe_cmd != NULL)
            {
 //               printf("%s\n", tokens->items[0]);
                // printf("%s %d\n", exe_cmd, strlen(exe_cmd));
                exe(exe_cmd, tokens);
            }
            free(exe_cmd);
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
/*I/O redirection check
* All I/O redirection require space between "<"" and input, ">" and output 
*/
void check_io(tokenlist *tokens, int *input_idx, int *output_idx)
{
    for (size_t i = 0; i < tokens->size; i++)
    {
        if (strcmp(tokens->items[i], "<") == 0)
        {
            *input_idx = i + 1;
        }
        if (strcmp(tokens->items[i], ">") == 0)
        {
            *output_idx = i + 1;
        }
    }
}
int myexit(char *input, char *command, tokenlist *tokens)
{
    free(input);
    free(command);
    free_tokens(tokens);
    exit(0);
}

void mycd(tokenlist *tokens)
{
    //cd to home
    if (tokens->size == 1)
    {
        char *home = getenv("HOME");
        int flag = chdir(home);
        char *cwd = getcwd(NULL, 0);
        if (flag != 0)
        {
            perror(tokens->items[1]);
        }
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

void mytime(tokenlist *tokens, char *home_dir)
{
    //create new tokenlist without the first tokens
    tokenlist *toks = new_tokenlist();

    //toks->size = tokens->size - 1;
    for (size_t i = 0; i < tokens->size - 1; i++)
    {
        add_token(toks, tokens->items[i + 1]);
    }

    //if built in function
    if (strcmp(toks->items[0], "myexit") == 0)
    {
        exit(0);
        //will terminate the program without output
    }
    else if (strcmp(toks->items[0], "mycd") == 0)
    {
        gettimeofday(&start_t, NULL);
        times(&st_cpu);
        mycd(toks);
        end_clock();
    }
    else if (strcmp(toks->items[0], "mypwd") == 0)
    {
        gettimeofday(&start_t, NULL);
        times(&st_cpu);
        printf("%s\n", getenv("PWD"));
        end_clock();
    }
    else if (strcmp(toks->items[0], "mytimeout") == 0)
    {
        gettimeofday(&start_t, NULL);
        times(&st_cpu);
        tokenlist *argvs = new_tokenlist();
        int secs = atoi(toks->items[1]);

        //path search
        char *cmd = path_search(toks->items[2], home_dir);

        if (cmd != NULL)
        {
            add_token(argvs, cmd);
            for (size_t i = 3; i < toks->size; i++)
            {
                add_token(argvs, toks->items[i]);
            }
            pid_t temp;
            if ((temp = fork()) == 0)
            {
                mytimeout(secs, argvs);
            }
            else
            {
                waitpid(temp, NULL, 0);
            }
        }
        else
        {
            printf("%s: Command not found\n", toks->items[2]);
        }
        free_tokens(argvs);
        end_clock();
    }
    else
    {
        gettimeofday(&start_t, NULL);
        times(&st_cpu);

        char *exe_cmd = path_search(toks->items[0], home_dir);

        //tokenlist *argvs = get_argvs(tokens);
        if (exe_cmd != NULL)
        {
            exe(exe_cmd, toks);
        }
        else
        {
            printf("%s: Command not found\n", toks->items[0]);
        }
        free(exe_cmd);
        end_clock();
    }
    free_tokens(toks);
}

void end_clock()
{
    times(&en_cpu);
    gettimeofday(&end_t, NULL);
    long clk_tck = sysconf(_SC_CLK_TCK);

    double sec = ((en_cpu.tms_utime + en_cpu.tms_cutime) - (st_cpu.tms_utime + st_cpu.tms_cutime));
    sec = (sec / (double)clk_tck);
    printf("\nusr cpu time:\t%.3fs\n", sec);

    sec = ((en_cpu.tms_stime + en_cpu.tms_cstime) - (st_cpu.tms_stime + st_cpu.tms_cstime));
    sec = (sec / (double)clk_tck);
    printf("sys cpu time:\t%.3fs\n", sec);

    double elapsed = end_t.tv_sec - start_t.tv_sec;
    elapsed = elapsed + (end_t.tv_usec - start_t.tv_usec) * 1e-6;
    printf("elapsed time:\t%.3fs\n", elapsed);
}

void mymtimes(tokenlist *tokens)
{
    time_t curr_time;
    time(&curr_time);

    // printf("current time: %s%ld\n", ctime(&curr_time), curr_time);
    int count[24];
    for (size_t i = 0; i < 24; i++)
    {
        count[i] = 0;
    }

    if (tokens->size == 1)
    {
        mtime(".", count, curr_time);
    }
    else
    {
        mtime(tokens->items[1], count, curr_time);
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
        if (strncmp(dp->d_name, ".", 1) == 0)
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

void out_redirect(tokenlist *tokens)
{
    int output_fd = open(tokens->items[output_idx], O_WRONLY | O_CREAT | O_TRUNC, 0666);
    close(1);
    dup(output_fd);
    close(output_fd);
}
tokenlist *get_argvs(tokenlist *tokens)
{
    tokenlist *argvs = new_tokenlist();

    int j = 0;
    for (size_t i = 0; i < tokens->size; i++)
    {
        if (i == input_idx - 1 || i == output_idx - 1 || i == input_idx || i == output_idx)
        {
            continue;
        }
        else
        {
            j++;
            add_token(argvs, tokens->items[i]);
        }
    }

    return argvs;
}

//run external command
int exe(char *cmd, tokenlist *tokens)
{
    //create argument list
    tokenlist *argvs = get_argvs(tokens);
    pid_t pid = fork();
    if (pid == 0)
    {
        //both input redirection and output redirect
        if ((input_idx != -1 && output_idx != -1))
        {

            int input_fd = open(tokens->items[input_idx], O_RDONLY, 0);
            if (input_fd < 0)
            {
                fprintf(stderr, "Fail to open %s for reading\n", tokens->items[input_idx]);

                return (EXIT_FAILURE);
            }
            close(0); //close for stdin
            dup(input_fd);
            close(input_fd);

            int output_fd = open(tokens->items[output_idx], O_WRONLY | O_CREAT | O_TRUNC, 0666);
            close(1);
            dup(output_fd);
            close(output_fd);

            execv(cmd, argvs->items);
        }
        else if (input_idx != -1)
        {
            //printf("in\n");
            int input_fd = open(tokens->items[input_idx], O_RDONLY);
            if (input_fd < 0)
            {
                fprintf(stderr, "Fail to open %s for reading\n", tokens->items[input_idx]);
                return (EXIT_FAILURE);
            }
            tokenlist *argvs = get_argvs(tokens);
            close(0); //close for stdin
            dup2(input_fd, 0);
            close(input_fd);

            //get argument list

            execv(cmd, argvs->items);
            free_tokens(argvs);
        }
        else if (output_idx != -1)
        {
            //printf("out\n");
            int output_fd = open(tokens->items[output_idx], O_WRONLY | O_CREAT | O_TRUNC, 0666);
            //create argument list
            //printf("size %d\n", argvs->size);
            close(1);
            dup(output_fd);
            close(output_fd);
            execv(cmd, argvs->items);
        }
        else
        {
            int flag = execv(cmd, tokens->items);
            printf("%d\n", flag);
        }
    }
    else
    {
        waitpid(pid, NULL, 0);
    }
    free_tokens(argvs);
    return 0;
}

char *path_search(char *command, char *home_dir)
{
    char *builtin_exe = (char *)malloc(strlen(home_dir) + strlen(command) + 2);
    strcpy(builtin_exe, home_dir);
    strcat(builtin_exe, "/");
    strcat(builtin_exe, command);
    if (access(builtin_exe, X_OK) != -1)
    {
        return builtin_exe;
    }

    char *paths = getenv("PATH");
    char *paths_cp = (char *)malloc(strlen(paths) + 1);
    strcpy(paths_cp, paths);
    //   printf("%s\n", paths_cp);
    char *path = strtok(paths_cp, ":");

    while (path != NULL)
    {
        char *temp = (char *)malloc(strlen(path) + strlen(command) + 2);
        strcpy(temp, path);
        strcat(temp, "/");
        strcat(temp, command);
        temp[strlen(temp)] = '\0';

        if (access(temp, X_OK) != -1)
        {
            return temp;
        }
        free(temp);
        path = strtok(NULL, ":");
    }
    free(paths_cp);
    //free(paths);
    return NULL;
}

cmdlist *new_cmdlist(void)
{
    cmdlist *cmds = (cmdlist *)malloc(sizeof(cmdlist));
    cmds->size = 0;
    cmds->items = (tokenlist **)malloc(sizeof(tokenlist *));
    cmds->items[0] = NULL; /* make NULL terminated */
    return cmds;
}

cmdlist *get_cmd(tokenlist *tokens)
{

    cmdlist *cmds = new_cmdlist();

    int k = cmds->size; //0
    //cmds->size = 1;
    cmds->items = (tokenlist **)realloc(cmds->items, (k + 2) * sizeof(tokenlist *));
    //cmds->items[k] = (tokenlist *)malloc(sizeof(tokenlist *));
    cmds->items[k] = new_tokenlist();
    cmds->items[k + 1] = NULL;
    for (size_t i = 0; i < tokens->size; i++)
    {

        if (strcmp(tokens->items[i], "|") == 0)
        {

            k += 1;

            cmds->items = (tokenlist **)realloc(cmds->items, (k + 2) * sizeof(tokenlist *));
            //cmds->items[k] = (tokenlist *)malloc(sizeof(tokenlist *));
            cmds->items[k] = new_tokenlist();
            cmds->items[k + 1] = NULL;
            cmds->size += 1;

            //j = 0;
            continue;
        }
        else
        {
            add_token(cmds->items[k], tokens->items[i]);
        }
    }
    cmds->size += 1;
    return cmds;
}

void free_cmds(cmdlist *cmds)
{
    for (size_t i = 0; i < cmds->size; i++)
    {
        free_tokens(cmds->items[i]);
    }
    free(cmds);
}

void exe_pipe(cmdlist *cmds, char *home_dir)
{
    int i;
    pid_t pid = fork();
    if (pid == 0)
    {
        for (i = 0; i < cmds->size - 1; i++)
        {
            //    printf("i is %d\n", i);
            int pd[2];
            pipe(pd);
            pid_t pid1 = fork();
            //     printf("pid1 is %d\n", pid1);
            if (pid1 == 0)
            {

                if (strcmp(cmds->items[i]->items[0], "myexit") == 0)
                {
                    exit(0);
                }
                else if (strcmp(cmds->items[i]->items[0], "mycd") == 0)
                {
                    mycd(cmds->items[i]);
                }
                else if (strcmp(cmds->items[i]->items[0], "mypwd") == 0)
                {
                    dup2(pd[1], 1);
                    printf("%s\n", getenv("PWD"));
                }
                else if (strcmp(cmds->items[i]->items[0], "mytime") == 0)
                {
                    printf("mytime\n");
                    dup2(pd[1], 1);
                    mytime(cmds->items[i], home_dir);
                }
                else
                {
                    dup2(pd[1], 1);
                    char *exe_cmd = path_search(cmds->items[i]->items[0], home_dir);
                    if (exe_cmd != NULL)
                    {
                        execv(exe_cmd, cmds->items[i]->items);
                        perror("exec");
                    }
                    free(exe_cmd);
                }
            }
            else
            {
                waitpid(pid1, NULL, 0);
            }
            dup2(pd[0], 0); //map output from previous child to input
            close(pd[1]);   //close input
            printf("close input");
        }
        char *exe_cmd = path_search(cmds->items[i]->items[0], home_dir);
        if (exe_cmd != NULL)
        {

            execv(exe_cmd, cmds->items[i]->items);
        }
        free(exe_cmd);
    }
    else
    {
        waitpid(pid, NULL, 0);
    }
}

void mytimeout(int secs, tokenlist *argvs)
{

    struct sigaction t_out;
    t_out.sa_handler = timer_handler;
    sigemptyset(&t_out.sa_mask);
    t_out.sa_flags = 0;
    output_idx = -1;
    check_io(argvs, &input_idx, &output_idx);
    if ((pid = fork()) == 0)
    {
        if (output_idx != -1)
        {
            //printf("out index is %d\n", output_idx);
            int saved_stdout = dup(1);
            out_redirect(argvs);
            tokenlist *new_argvs = get_argvs(argvs);
            execv(argvs->items[0], new_argvs->items);
            dup2(saved_stdout, 1);
            close(saved_stdout);
        }
        else
        {
            execv(argvs->items[0], argvs->items);
        }
    }
    else
    {
        t_out.sa_handler = timer_handler;
        sigaction(SIGALRM, &t_out, 0);

        alarm(secs);
        for (;;)
        {
            // printf("inside for\n");
            //printf("pid %d\t%d", pid, waitpid(pid, NULL, WNOHANG));
            if (pid == (waitpid(pid, NULL, WNOHANG)))
            {
                // printf("finished\n");
                exit(0);
            }
        }
    }
    exit(0);
}
