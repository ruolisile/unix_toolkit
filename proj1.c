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
char *get_input(void);
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

//mytree
void mytree(char *dir, char *prefix);
char *prefix_cat(const char *prefix, const char *ext);

//mytime
void mytime(tokenlist *tokens, char *home_dir);
static clock_t st_time;
static clock_t en_time;
static struct tms st_cpu;
static struct tms en_cpu;

static int input_idx = -1;
static int output_idx = -1;

//void real_time(struct timeval start_t, struct timeval end_t, int *total_min, double *total_sec);
//void sys_time(struct timeval start_t, struct timeval end_t, int *total_min, double *total_sec);
void end_clock();

//mymtimes
void mymtimes(tokenlist *tokens);
long int get_mtime(char *path);
void mtime(char *path, int count[], time_t curr_time);
void print_t(time_t);

//run external command
int exe(char *cmd, tokenlist *tokens);
char *path_search(char *command, char *home_dir);

//  static char *home_path; //get the dir where this program is located
// home_path = getcwd(NULL, 0);
cmdlist *new_cmdlist(void);
cmdlist *get_cmd(tokenlist *tokens);
void free_cmds(cmdlist *cmds);
void exe_pipe(cmdlist *cmds, char *home_dir);

//timeout
//only works for external command
void mytimeout(int secs, tokenlist *argvs);

pid_t pid;
static void timer_handler(int signo)
{
    printf("Time out\n");
     kill(pid, SIGALRM);
    exit(0);
}


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
        char *input = get_input();
        // printf("%s\n", input);
        if (input == NULL)
        {
            return 0; //exit with ctrl-D
        }
        else if (strlen(input) == 0) //handle newline
        {
            continue;
        }

        //printf("%s: %d\n", input, strlen(input));
        tokenlist *tokens = get_tokens(input);

        cmdlist *cmds = new_cmdlist();
        cmds = get_cmd(tokens);

        char *command = (char *)malloc(strlen(tokens->items[0]) + 1);
        strcpy(command, tokens->items[0]);
        if (strlen(command) == 0)
        {
            free(command);
            free_tokens(tokens);
            continue;
        }

        //pipe without I/O redirection
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
                mytime(tokens, home_dir);
                dup2(saved_stdout, 1);
                close(saved_stdout);
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
                if((temp = fork()) == 0)
                {
                    mytimeout(secs, argvs);
                }
                else
                {
                    waitpid(temp, NULL, 0);
                }
                
            }
            free_tokens(argvs);
        }
        else
        {
            char *exe_cmd = path_search(command, home_dir);

            //tokenlist *argvs = get_argvs(tokens);
            if (exe_cmd != NULL)
            {
                printf("%s\n", tokens->items[0]);
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

//tree doesn't support input redirection
// only inplement output redirection
void mytree(char *dir, char *prefix)
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
        

            if (strncmp(namelist[i]->d_name, ".", 1) == 0)
            {
                printf("hi");
                break;
                continue;
            }
            if (namelist[i]->d_type == DT_DIR)
            {

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
                mytree(new_dir, prefix_ext);
                free(prefix_ext);
            }
            else
            {
                //  printf("current level is %d\n", curr_level);
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

void mytime(tokenlist *tokens, char *home_dir)
{
    //create new tokenlist without the first tokens
    tokenlist *toks = new_tokenlist();

    //toks->size = tokens->size - 1;
    for (size_t i = 0; i < tokens->size - 1; i++)
    {
        //  toks->items[i] = tokens->items[i + 1];
        add_token(toks, tokens->items[i + 1]);
        //   printf("%s%d\n", toks->items[i], strlen(toks->items[i]));
    }

    struct timeval start_t, end_t;

    //if built in function
    if (strcmp(toks->items[0], "myexit") == 0)
    {
        exit(0);
        //will terminate the program without output
    }
    else if (strcmp(toks->items[0], "mycd") == 0)
    {
        gettimeofday(&start_t, NULL);

        st_time = times(&st_cpu);
        mycd(toks);
        end_clock();
        // printf("Real Time: %jd, User Time %jd, System Time %jd\n",
        // (intmax_t)(en_time - st_time),
        // (intmax_t)(en_cpu.tms_utime - st_cpu.tms_utime),
        // (intmax_t)(en_cpu.tms_stime - st_cpu.tms_stime));

        gettimeofday(&end_t, NULL);
        //real time
        printf("%ld\t%ld\n", start_t.tv_sec, end_t.tv_sec);
        printf("%ld\t%ld\n", start_t.tv_usec, end_t.tv_usec);
        // real_time(start_t, end_t, &total_min, &total_sec);

        // total_sec += total_msec;
        // printf("real\t\t%dm%.3fs\n", total_min, total_sec);
        // //user time

        // //sys CPU time
        // sys_time(start_t, end_t, &total_min, &total_sec);
        // printf("sys\t\t%dm%.3fs\n", total_min, total_sec);
    }
    else if (strcmp(toks->items[0], "mypwd") == 0)
    {
        printf("%s\n", getenv("PWD"));
    }

    else
    {
        gettimeofday(&start_t, NULL);

        st_time = times(&st_cpu);

        char *exe_cmd = path_search(toks->items[0], home_dir);

        //tokenlist *argvs = get_argvs(tokens);
        if (exe_cmd != NULL)
        {
            printf("%s\n", toks->items[0]);
            //  printf("%s %d\n", exe_cmd, strlen(exe_cmd));
            exe(exe_cmd, toks);
        }
        free(exe_cmd);
        end_clock();

        gettimeofday(&end_t, NULL);
        //real time
        printf("%ld\t%ld\n", start_t.tv_sec, end_t.tv_sec);
        printf("%ld\t%ld\n", start_t.tv_usec, end_t.tv_usec);
    }
    free_tokens(toks);
}

// void real_time(struct timeval start_t, struct timeval end_t, int *total_min, double *total_sec)
// {
//     *total_min = (int)(end_t.tv_sec - start_t.tv_sec) / 60;
//     *total_sec = (double)((end_t.tv_sec - start_t.tv_sec) % 60);
//     double total_msec = (end_t.tv_usec - start_t.tv_usec) / (double)1000000;
//     *total_sec += total_msec;
// }

// void sys_time(struct timeval start_t, struct timeval end_t, int *total_min, double *total_sec)
// {
//         *total_min = (int) ((end_t.tv_sec - start_t.tv_sec) / clk_tck) / 60;
//         *total_sec =  (end_t.tv_sec - start_t.tv_sec) % clk_tck;
//         double total_msec;
//         total_msec = ((end_t.tv_usec - start_t.tv_usec) /(double)1000000);
//         total_msec = total_msec / (double)clk_tck;
//         *total_sec += total_msec;
// }

void end_clock()
{
    long clk_tck = sysconf(_SC_CLK_TCK);
    en_time = times(&en_cpu);
    //printf("%d\n", _SC_CLK_TCK);
    //printf("en_time %ld\n", en_time);
    //printf("st_time %ld\n", st_time);
    //printf("%f\n", (en_time - st_time) / (double)clk_tck);
    int min = ((en_time - st_time) / (double)clk_tck) / 60;
    double sec = (en_time - st_time) / (double)clk_tck - min;
    //printf("%f\n", (en_time - st_time) / (double)clk_tck - min);
    printf("real\t\t%dm%.3fs\n", min, sec);

    //printf("%ld\n", (en_cpu.tms_utime + en_cpu.tms_cutime));
    //printf("%ld\n", (st_cpu.tms_utime + st_cpu.tms_cutime));
    min = ((en_cpu.tms_utime + en_cpu.tms_cutime) - (st_cpu.tms_utime + st_cpu.tms_cutime));
    min = (min / (double)clk_tck) / 60;
    sec = ((en_cpu.tms_utime + en_cpu.tms_cutime) - (st_cpu.tms_utime + st_cpu.tms_cutime)) / (double)clk_tck - min;
    printf("usr\t\t%dm%.3fs\n", min, sec);

    //printf("%ld\n", (en_cpu.tms_stime + en_cpu.tms_cstime));
    //printf("%ld\n", (st_cpu.tms_stime + st_cpu.tms_cstime));
    min = ((en_cpu.tms_stime + en_cpu.tms_cstime) - (st_cpu.tms_stime + st_cpu.tms_cstime));
    min = (min / (double)clk_tck) / 60;
    sec = ((en_cpu.tms_stime + en_cpu.tms_cstime) -
           (st_cpu.tms_stime + st_cpu.tms_cstime)) /
              (double)clk_tck -
          min;
    printf("sys\t\t%dm%.3fs\n", min, sec);
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
            //create argument list
            tokenlist *argvs = get_argvs(tokens);
            execv(cmd, argvs->items);
            free_tokens(argvs);
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

            execvp(cmd, argvs->items);
            free_tokens(argvs);
        }
        else if (output_idx != -1)
        {
            //printf("out\n");
            int output_fd = open(tokens->items[output_idx], O_WRONLY | O_CREAT | O_TRUNC, 0666);
            //create argument list
            tokenlist *argvs = get_argvs(tokens);
            //printf("size %d\n", argvs->size);
            close(1);
            dup(output_fd);
            close(output_fd);
            execv(cmd, argvs->items);

            free_tokens(argvs);
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
    return 0;
}

char *path_search(char *command, char *home_dir)
{
    char *builtin_exe = (char *)malloc(strlen(home_dir) + strlen(command) + 1);
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

        if (access(temp, X_OK) != -1)
        {
            return temp;
        }
        free(temp);
        path = strtok(NULL, ":");
    }
    free(paths_cp);
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
    //int j = 0; //index for tokenlist

    int k = cmds->size;
    cmds->items = (tokenlist **)realloc(cmds->items, (k + 2) * sizeof(tokenlist *));
    cmds->items[k] = (tokenlist *)malloc(sizeof(tokenlist *));
    for (size_t i = 0; i < tokens->size; i++)
    {
        if (strcmp(tokens->items[i], "|") == 0)
        {
            k += 1;
            cmds->items = (tokenlist **)realloc(cmds->items, (k + 2) * sizeof(tokenlist *));
            cmds->items[k] = (tokenlist *)malloc(sizeof(tokenlist *));
            cmds->size += 1;
            //printf("new command\n");

            //j = 0;
            continue;
        }
        else
        {
            add_token(cmds->items[k], tokens->items[i]);
            //    printf("k %d\n", k);
            //     for (size_t l = 0; l < cmds->items[k]->size; l++)
            //     {
            //         printf("%s\t", cmds->items[k]->items[l]);
            //     }

            //     printf("added token\n");
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
}

void exe_pipe(cmdlist *cmds, char *home_dir)
{
    int i;
    pid_t pid = fork();
    if (pid == 0)
    {
        for (i = 0; i < cmds->size - 1; i++)
        {
            int pd[2];
            pipe(pd);
            pid = fork();
            if (pid == 0)
            {
                dup2(pd[1], 1);
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
                    printf("%s\n", getenv("PWD"));
                }
                else if (strcmp(cmds->items[i]->items[0], "mytime") == 0)
                {
                    mytime(cmds->items[i], home_dir);
                }
                else
                {
                    char *exe_cmd = path_search(cmds->items[i]->items[0], home_dir);
                    if (exe_cmd != NULL)
                    {
                        execv(exe_cmd, cmds->items[i]->items);
                        perror("exec");
                    }
                }
            }
            else
            {
                waitpid(pid, NULL, 0);
            }
            dup2(pd[0], 0); //map output from previous child to input
            close(pd[1]);   //close input
        }
        char *exe_cmd = path_search(cmds->items[i]->items[0], home_dir);
        if (exe_cmd != NULL)
        {

            execv(exe_cmd, cmds->items[i]->items);
        }
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

    
    if ((pid = fork()) == 0)
    {
        execv(argvs->items[0], argvs->items);
    }
    else
    {
        t_out.sa_handler = timer_handler;
        sigaction(SIGALRM, &t_out, 0);

        alarm(secs);
        int temp = 0;
        for(;;)
        {
           // printf("inside for\n");
         //printf("pid %d\t%d", pid, waitpid(pid, NULL, WNOHANG));
            if(pid == (waitpid(pid, NULL, WNOHANG)))
            {
                printf("finished\n");
                exit(0);
            }
        }
    }
    exit(0);
}