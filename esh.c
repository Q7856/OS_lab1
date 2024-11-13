#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

#define ERROR_INFO "An error has occurred\n"
#define MULTI_CMDS_SIZE 20
#define SINGLE_CMD_SIZE 100
#define ARGS_SIZE 64
#define MAX_PATH 100
#define PATH_INIT "/bin"
#define MAX_BG


typedef struct BG_DETAIL{
    int pid;
    char *cmd;
}BG;

char *paths[MAX_PATH];
int paths_length;
BG bg_recod[MAX_BG];
int bg_place;

//
// You should use the following functions to print information
// Do not modify these functions
//
void print_error_info() {
    printf(ERROR_INFO);
    fflush(stdout);
}

void print_path_info(int index, char *path) {
    printf("%d\t %s\n", index, path);
    fflush(stdout);
}

void print_bg_info(int index, int pid, char *cmd) {
    printf("%d\t %s\t %s\n", index, pid, cmd);
    fflush(stdout);
}

void print_current_bg(int pid, char *cmd) {
    printf("Process %d %s: running in background\n", pid, cmd);
    fflush(stdout);
}

void init_path(){
    paths[0] = PATH_INIT;
    paths_length = 0;
}

void init_bg(){
    bg_place = 0;
}

char *readline(){
    char *line = NULL;
    size_t bufszie;
    
    ssize_t num = getline(&line,&bufszie,stdin);
    if(num == -1){
        print_error_info();
        exit(0);
    }
    if(line[num - 1] == '\n'){
        line[num - 1] = '\0';
    }
    for(int i = 0; i < num; i++){
        if(line[i] == ';' || line[i] == '&'){
            int j = i + 1;
            while(j < num && isspace(line[j])){
                j++;
            }
            if(j < num && (line[j] == ';' || line[j] =='&')){//check,cannot with continue ;; && ;&
                print_error_info();
                return NULL;
            }
        }
    }
    return line;
}

char ** split_multi_cmds(char *line,int *num){
    *num = 0;
    int buffersize = MULTI_CMDS_SIZE;
    char **cmds = (char**)malloc(buffersize * sizeof(char *));
    int i;
    for(i = 0;i<strlen(line);i++){
        char *cmd = (char*)malloc(SINGLE_CMD_SIZE * sizeof(char));
        int k = 0;
        int j;
        for(j = i;line[j]!='&' && line[j]!=';' && line[j]!='\0';j++){
            cmd[k] = line[j];
            k++;
        }
        if(line[j] == '&'){
            cmd[k] = '&';
            k++;
        }
        cmd[k] = '\0';
        while(*cmd == ' '){
            cmd++;
        }
        cmds[*num] = cmd;
        *num +=1;
        i=j;
    }
    return cmds;
}

char **split_cmd_to_args(char * cmd,int *count){
    *count = 0;
    int buffersize = ARGS_SIZE;
    char **args = (char**)malloc(buffersize * sizeof(char *));
    int i;
    for(i = 0;i<strlen(cmd);i++){
        char *parameter = (char*)malloc(SINGLE_CMD_SIZE * sizeof(char));
        int k = 0;
        int j;
        for(j = i;cmd[j]!='>' && cmd[j]!='<' && cmd[j]!='|' && cmd[j]!=' ' && cmd[j]!='\0';j++){
            parameter[k] = cmd[j];
            k++;
        }
        parameter[k] = '\0';
        while(*parameter == ' ' && *parameter != '\0'){
            parameter++;
        }
        if(*parameter != '\0'){
            args[*count] = parameter;
            *count +=1 ;
        }
        if(cmd[j] == '|' || cmd[j] == '<' || cmd[j] == '>'){
            printf("HERE\n");
            char *special = (char*)malloc(SINGLE_CMD_SIZE * sizeof(char));
            if(cmd[j] == '|'){
                args[*count] = "|\0";
            }
            else if(cmd[j] == '>'){
                args[*count] = ">\0";
            }
            else if(cmd[j] == '<'){
                args[*count] = "<\0";
            }
            //args[*count] = special;
            *count += 1;
        }
        i=j;
    }
    args[*count] = NULL;
    return args;
}

int built_in(char **args){
    if(strcmp(args[0],"cd") == 0){
        if(args[1] == NULL || args[2] != NULL){
            return -1;
        }
        if(strcmp(args[1],"~") == 0){
            const char *home = getenv("HOME");
            if(home == NULL){
                home = "/";
            }
            if(chdir(home) != 0){
                return -1;
            }
        }
        else{
            if(chdir(args[1]) != 0){
                return -1;
            }
        }
        return 1;
    }
    else if(strcmp(args[0],"exit") == 0){
        exit(0);
    }
    else if(strcmp(args[0],"bg") == 0){
        return 1;
    }
    else if(strcmp(args[0],"path") == 0){
        return 1;
    }
    else{
        return 0;
    }
}

int outside_cmd(char **args){
    pid_t pid,wpid;
    int status;
    pid = fork();
    if(pid == 0){
        if(execvp(args[0],args) < 0){
            return -1;
        }
    }
    else{
        do
        {
            wpid = waitpid(pid,&status,WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));    
    }
    return 1;
}

void start_execute(char **args){
    if(args[0] == NULL){
        return;
    }
    int priority = built_in(args);
    if(priority == -1){//builtin cmd execute failed
        print_error_info();
        return;
    }
    else if(priority == 0){
        int priority2 = outside_cmd(args);
        if(priority2 == -1){
            print_error_info();
        }
    }
}

int main() {
    char *line;// cmd "cd a"
    char **args;//split {"cd","a"}
    char **cmds;
    int status;
    int cmd_num;
    init_path();

    while(1) {
        printf("esh > ");
        fflush(stdout);
        line = readline();
        if(line == NULL){
            continue;
        }
        cmds = split_multi_cmds(line,&cmd_num);
        for(int i = 0;i < cmd_num;i++){
            //printf("cmd:%s#\n",cmds[i]);
            int count; 
            args = split_cmd_to_args(cmds[i],&count);
            //for(int j = 0;j <= count ;j++){
            //    printf("prm:%s#\n",args[j]);
            //}
            start_execute(args);
        }
        // TODO
        // Show your intelligence
    }
    return 0;
}