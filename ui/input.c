#include <assert.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <execinfo.h>
#include <signal.h>
#include <ucontext.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include <system_server.h>
#include <gui.h>
#include <input.h>
#include <web_server.h>
#include <bits/sigaction.h>

#define TOY_TOK_BUFSIZE 64
#define TOY_TOK_DELIM " \t\r\n\a"
#define TOY_BUFFSIZE 1024

static pthread_mutex_t global_message_mutex  = PTHREAD_MUTEX_INITIALIZER;

// global_message <~ 모든 문제를 만드는 전역 변수
static char global_message[TOY_BUFFSIZE];//=  "Hello Sir";


typedef struct _sig_ucontext {
    unsigned long uc_flags;
    struct ucontext *uc_link;
    stack_t uc_stack;
    struct sigcontext uc_mcontext;
    sigset_t uc_sigmask;
} sig_ucontext_t;

char *builtin_str[] = {
    "send",
    "sh",
    "exit"
};

/*  sensor thread   */
void *sensor_thread(void* arg);

/*  command thread    */
int toy_num_builtins();
int toy_send(char **args);
int toy_exit(char **args);
int toy_shell(char **args);
int toy_execute(char **args);
char *toy_read_line(void);
char **toy_split_line(char *line);;

void toy_loop(void);
void *command_thread(void* arg);


void segfault_handler(int sig_num, siginfo_t * info, void * ucontext);

int input();

int (*builtin_func[]) (char **) = {
    &toy_send,
    &toy_shell,
    &toy_exit
};


int create_input()
{
    pid_t systemPid;
    const char *name = "input";

    printf("Creating input process\n");

    switch(systemPid = fork()){
        case -1:
            perror("input process create failed \n");
        
        case 0 :
            if (prctl(PR_SET_NAME, (unsigned long) name) < 0)
                perror("prctl()");
            input();
            break;

        default :
            break;
    }

    return 0;
}

void segfault_handler(int sig_num, siginfo_t * info, void * ucontext) {
  void * array[50];
  void * caller_address;
  char ** messages;
  int size, i;
  sig_ucontext_t * uc;

  uc = (sig_ucontext_t *) ucontext;

  /* Get the address at the time the signal was raised */
  caller_address = (void *) uc->uc_mcontext.rip;  // RIP: x86_64 specific     arm_pc: ARM

  fprintf(stderr, "\n");

  if (sig_num == SIGSEGV)
    printf("signal %d (%s), address is %p from %p\n", sig_num, strsignal(sig_num), info->si_addr,
           (void *) caller_address);
  else
    printf("signal %d (%s)\n", sig_num, strsignal(sig_num));

  size = backtrace(array, 50);
  /* overwrite sigaction with caller's address */
  array[1] = caller_address;
  messages = backtrace_symbols(array, size);

  /* skip first stack frame (points here) */
  for (i = 1; i < size && messages != NULL; ++i) {
    printf("[bt]: (%d) %s\n", i, messages[i]);
  }

  free(messages);

  exit(EXIT_FAILURE);
}


int input()
{
    printf("Enter input process\n");

    struct sigaction siga;
    int retcode;
    pthread_t command_thread_tid, sensor_thread_tid;

    siga.sa_sigaction = segfault_handler;

    sigaction(SIGSEGV, &siga, NULL);

    retcode = pthread_create(&command_thread_tid, NULL, command_thread, "command thread\n");
    if(retcode < 0){
        perror("Command thread error");
        return -1;
    }
    retcode = pthread_create(&sensor_thread_tid, NULL, sensor_thread, "sensor thread\n");
    if(retcode < 0){
        perror("Command thread error");
        return -1;
    }

    while (1) {
        sleep(1);
    }


    return 0;
}

/*  sensor thread   */

void *sensor_thread(void* arg)
{
    char saved_message[TOY_BUFFSIZE];
    char *s = arg;
    int i = 0;
    int mkey = 0;
    printf("%s", s); 
 
    while (1) {
        i = 0;
        mkey = 0;
        // 여기서 뮤텍스
        // 과제를 억지로 만들기 위해 한 글자씩 출력 후 슬립
        mkey = pthread_mutex_lock(&global_message_mutex);
        if (mkey != 0){
            perror("Sensor mutex lock error");
            exit(0);
        }
        while (global_message[i] != '\0') {
            printf("\t\t%c", global_message[i]);
            fflush(stdout);
            sleep(5);
            i++;
        }
        mkey = pthread_mutex_unlock(&global_message_mutex);
            if (mkey != 0){
                perror("Sensor mutex unlock error");
                exit(0);
            }
        sleep(5);
    }

    return 0;
}

/*  command thread   */

int toy_num_builtins()
{
    return sizeof(builtin_str) / sizeof(char *);
}

int toy_send(char **args)
{
    printf("send message: %s\n", args[1]);

    return 1;
}

int toy_mutex(char **args)
{
    int mkey = 0;
    if (args[1] == NULL) {
        return 1;
    }

    printf("save message: %s\n", args[1]);
    // 여기서 뮤텍스
    mkey = pthread_mutex_lock(&global_message_mutex);
    if(mkey != 0){
        perror("Toy mutex lock error");
        exit(0);
    }
    strcpy(global_message, args[1]);
    mkey = pthread_mutex_lock(&global_message_mutex);
    if(mkey != 0){
        perror("Toy mutex unlock error");
        exit(0);
    }
    return 1;
}
int toy_exit(char **args)
{
    return 0;
}

int toy_shell(char **args)
{
    pid_t pid;
    int status;

    pid = fork();
    if (pid == 0) {
        if (execvp(args[0], args) == -1) {
            perror("toy");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) {
        perror("toy");
    } else
{
        do
        {
            waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}

int toy_execute(char **args)
{
    int i;

    if (args[0] == NULL) {
        return 1;
    }

    for (i = 0; i < toy_num_builtins(); i++) {
        if (strcmp(args[0], builtin_str[i]) == 0) {
            return (*builtin_func[i])(args);
        }
    }

    return 1;
}

char *toy_read_line(void)
{
    char *line = NULL;
    ssize_t bufsize = 0;

    if (getline(&line, &bufsize, stdin) == -1) {
        if (feof(stdin)) {
            exit(EXIT_SUCCESS);
        } else {
            perror(": getline\n");
            exit(EXIT_FAILURE);
        }
    }
    return line;
}

char **toy_split_line(char *line)
{
    int bufsize = TOY_TOK_BUFSIZE, position = 0;
    char **tokens = malloc(bufsize * sizeof(char *));
    char *token, **tokens_backup;

    if (!tokens) {
        fprintf(stderr, "toy: allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, TOY_TOK_DELIM);
    while (token != NULL) {
        tokens[position] = token;
        position++;

        if (position >= bufsize) {
            bufsize += TOY_TOK_BUFSIZE;
            tokens_backup = tokens;
            tokens = realloc(tokens, bufsize * sizeof(char *));
            if (!tokens) {
                free(tokens_backup);
                fprintf(stderr, "toy: allocation error\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, TOY_TOK_DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}

void toy_loop(void)
{
    char *line;
    char **args;
    int status;
    int mkey;
    do {
        mkey = pthread_mutex_lock(&global_message_mutex);
        if (mkey != 0){
                perror("Toy loop mutex lock error");
                exit(0);
        }
        printf("TOY>");
        mkey = pthread_mutex_unlock(&global_message_mutex);
        if (mkey != 0){
                perror("Toy loop mutex unlock error");
                exit(0);
        }
        line = toy_read_line();
        args = toy_split_line(line);
        status = toy_execute(args);
        free(line);
        free(args);
    } while (status);
}

void *command_thread(void* arg)
{
    char *s = arg;

    printf("%s", s);

    toy_loop();

    return 0;
}