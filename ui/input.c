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
#include <mqueue.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <execinfo.h>

#include <system_server.h>
#include <gui.h>
#include <input.h>
#include <web_server.h>
#include <toy_message.h>
#include <bits/sigaction.h>
#include <shared_memory.h>

#define TOY_TOK_BUFSIZE 64
#define TOY_TOK_DELIM " \t\r\n\a"

#define TOY_TOK_BUFSIZE 64
#define TOY_TOK_DELIM " \t\r\n\a"
#define TOY_BUFFSIZE 1024

#define MAX 30
#define NUMTHREAD 3 /* number of threads */

#define DUMP_STATE 2

static pthread_mutex_t global_message_mutex  = PTHREAD_MUTEX_INITIALIZER;
static char global_message[TOY_BUFFSIZE];

char buffer[TOY_BUFFSIZE];
int read_count = 0, write_count = 0;
int buflen;
pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;
int thread_id[NUMTHREAD] = {0, 1, 2};
int producer_count = 0, consumer_count = 0;

static shm_sensor_t *the_sensor_info = NULL;
int shmid;
struct shmseg *shmp;

static mqd_t watchdog_queue;
static mqd_t monitor_queue;
static mqd_t disk_queue;
static mqd_t camera_queue;

typedef struct _sig_ucontext {
    unsigned long uc_flags;
    struct ucontext *uc_link;
    stack_t uc_stack;
    struct sigcontext uc_mcontext;
    sigset_t uc_sigmask;
} sig_ucontext_t;

int toy_send(char **args);
int toy_shell(char **args);
int toy_exit(char **args);
int toy_mutex(char **args);
int toy_message_queue(char **args);
int toy_read_elf_header(char **args);
int toy_dump_state(char **args);

char *builtin_str[] = {
    "send",
    "mu",
    "sh",
    "mq",
    "elf",
    "dump",
    "exit"
};


int (*builtin_func[]) (char **) = {
    &toy_send,
    &toy_mutex,
    &toy_shell,
    &toy_message_queue,
    &toy_read_elf_header,
    &toy_dump_state,
    &toy_exit
};

static int posix_sleep_ms(unsigned int overp_time, unsigned int underp_time);

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
int toy_mutex(char **args);

void toy_loop(void);
void *command_thread(void* arg);


void segfault_handler(int sig_num, siginfo_t * info, void * ucontext);

int input();


/*  comsumer & producer */
void *toy_consumer(int *id);
void *toy_producer(int *id);



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
    int i;
    pthread_t thread[NUMTHREAD];


    siga.sa_sigaction = segfault_handler;

    sigaction(SIGSEGV, &siga, NULL);

    shmid = shmget(SHM_KEY_SENSOR, sizeof(shm_sensor_t), IPC_CREAT | 0777);
    if (shmid < 0){
        perror("shmget failed : sensor thread");
        exit(0);
    }
    the_sensor_info = (shm_sensor_t *)shmat(shmid, NULL, 0);
    if (shmp < 0){
        perror("shmat failed : sensor thread");
        exit(0);
    }

    watchdog_queue = mq_open("/watchdog_queue", O_RDWR);
    monitor_queue = mq_open("/monitor_queue", O_RDWR);
    disk_queue = mq_open("/disk_queue", O_RDWR);
    camera_queue = mq_open("/camera_queue", O_RDWR);

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

    /*pthread_mutex_lock(&global_message_mutex);
    strcpy(global_message, "hello world!");
    buflen = strlen(global_message);
    pthread_mutex_unlock(&global_message_mutex);
    pthread_create(&thread[0], NULL, (void *)toy_consumer, &thread_id[0]);
    pthread_create(&thread[1], NULL, (void *)toy_producer, &thread_id[1]);
    pthread_create(&thread[2], NULL, (void *)toy_producer, &thread_id[2]);*/


    /*for (i = 0; i < NUMTHREAD; i++) {
        pthread_join(thread[i], NULL);
    }*/


    while (1) {
        sleep(1);
    }

    return 0;
}

static int posix_sleep_ms(unsigned int overp_time, unsigned int underp_time)
{
    struct timespec sleep_time;

    sleep_time.tv_sec = overp_time;
    sleep_time.tv_nsec = underp_time;

    return nanosleep(&sleep_time, NULL);
}

/*  sensor thread   */

void *sensor_thread(void* arg)
{
    char *s = arg;
    toy_msg_t msg;
    int mqretcode;
    // 여기 추가: 공유메모리 키
    

    printf("%s", s);

    the_sensor_info->temp = 1;
    the_sensor_info->press = 2;
    the_sensor_info->humidity = 4;

    while (1) {
        posix_sleep_ms(5, 0);
        // 여기에 구현해 주세요.
        // 현재 고도/온도/기압 정보를  SYS V shared memory에 저장 후
        // monitor thread에 메시지 전송한다.

        msg.msg_type = 1;
        msg.param1 = shmid;
        msg.param2 = 0;
        mqretcode = mq_send(monitor_queue, (char *)&msg, sizeof(msg), 0);
        assert(mqretcode == 0);
    }

    return 0;
}

/*  command thread   */

int toy_send(char **args);
int toy_shell(char **args);
int toy_exit(char **args);

int toy_num_builtins()
{
    return sizeof(builtin_str) / sizeof(char *);
}

int toy_send(char **args)
{
    printf("send message: %s\n", args[1]);

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

    do {
        printf("TOY>");
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

int toy_message_queue(char **args)
{
    int mqretcode;
    toy_msg_t msg;

    if (args[1] == NULL || args[2] == NULL) {
        return 1;
    }

    if (!strcmp(args[1], "camera")) {
        msg.msg_type = atoi(args[2]);
        msg.param1 = 0;
        msg.param2 = 0;
        mqretcode = mq_send(camera_queue, (char *)&msg, sizeof(msg), 0);
        assert(mqretcode == 0);
    }

    return 1;
}

/* read_elf */
int toy_read_elf_header(char **args)
{
    int mqretcode;
    toy_msg_t msg;
    int in_fd;
    char *contents = NULL;
    size_t contents_sz;
    struct stat st;
    Elf64Hdr *map;

    in_fd = open("./sample/sample.elf", O_RDONLY);

	if ( in_fd < 0 ) {
        perror("File is not available");
        return 0;
    }
    if (fstat(in_fd, &st) == 0) {
                
        map = (Elf64Hdr *)mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, in_fd, 0);
        if(map == MAP_FAILED){
            perror("mmap failed : read_elf");
            exit(0);
        }
        printf("file type : %d\n", map->e_type);
        printf("Architecture : %d\n", map->e_machine);
        printf("file version : %d\n", map->e_version);
        printf("virtual address : %ld\n", map->e_entry);
        printf("header table file offset : %ld\n", map->e_phoff);
        munmap(map, contents_sz);
    }


    return 1;
}

int toy_dump_state(char **args)
{
    int mqretcode;
    toy_msg_t msg;

    msg.msg_type = DUMP_STATE;
    msg.param1 = 0;
    msg.param2 = 0;
    mqretcode = mq_send(camera_queue, (char *)&msg, sizeof(msg), 0);
    assert(mqretcode == 0);
    mqretcode = mq_send(monitor_queue, (char *)&msg, sizeof(msg), 0);
    assert(mqretcode == 0);

    return 1;
}

int toy_mutex(char **args)
{
    if (args[1] == NULL) {
        return 1;
    }

    printf("save message: %s\n", args[1]);
    pthread_mutex_lock(&global_message_mutex);
    strcpy(global_message, args[1]);
    pthread_mutex_unlock(&global_message_mutex);
    return 1;
}

/*  consumer & producer */

void *toy_consumer(int *id)
{
    pthread_mutex_lock(&count_mutex);
    while (consumer_count < MAX) {
        pthread_cond_wait(&empty, &count_mutex);
        // get from queue
        printf("                           소비자[%d]: %c\n", *id, buffer[read_count]);
        read_count = (read_count + 1) % TOY_BUFFSIZE;
        fflush(stdout);
        consumer_count++;
    }
    pthread_mutex_unlock(&count_mutex);
}

void *toy_producer(int *id)
{
    while (producer_count < MAX) {
        pthread_mutex_lock(&count_mutex);
        strcpy(buffer, "");
        buffer[write_count] = global_message[write_count % buflen];
        // push at queue
        printf("%d - 생산자[%d]: %c \n", producer_count, *id, buffer[write_count]);
        fflush(stdout);
        write_count = (write_count + 1) % TOY_BUFFSIZE;
        producer_count++;
        pthread_cond_signal(&empty);
        pthread_mutex_unlock(&count_mutex);
        sleep(rand() % 3);
    }
}
