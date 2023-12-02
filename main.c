#define NUM_MESSAGES 10

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <mqueue.h>
#include <asm-generic/signal-defs.h>

#include <system_server.h>
#include <gui.h>
#include <input.h>
#include <web_server.h>
#include <toy_message.h>

#define NUM_MESSAGES 10

static mqd_t watchdog_queue;
static mqd_t monitor_queue;
static mqd_t disk_queue;
static mqd_t camera_queue;

static void sigcldHandler(int sig);
static void create_msq(mqd_t *msq, const char *name, int max_num, int max_size);

int main()
{
    pid_t spid, gpid, ipid, wpid;
    int status, savedErrno;
    int sigCnt;
    sigset_t blockMask, emptyMask;
    struct sigaction sa;

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sa.sa_handler = sigcldHandler;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        printf("sigaction");
        return 0;
    }

    create_msq(&watchdog_queue, "/watchdog_queue", NUM_MESSAGES, sizeof(toy_msg_t));
    create_msq(&monitor_queue, "/monitor_queue", NUM_MESSAGES, sizeof(toy_msg_t));
    create_msq(&disk_queue, "/disk_queue", NUM_MESSAGES, sizeof(toy_msg_t));
    create_msq(&camera_queue, "/camera_queue", NUM_MESSAGES, sizeof(toy_msg_t));

    printf("메인 함수입니다.\n");
    printf("시스템 서버를 생성합니다.\n");
    spid = create_system_server();
    printf("웹 서버를 생성합니다.\n");
    wpid = create_web_server();
    printf("입력 프로세스를 생성합니다.\n");
    ipid = create_input();
    printf("GUI를 생성합니다.\n");
    gpid = create_gui();

    waitpid(spid, &status, 0);
    waitpid(gpid, &status, 0);
    waitpid(ipid, &status, 0);
    waitpid(wpid, &status, 0);

    return 0;
}

static void sigcldHandler(int sig){
    int status, savedErrno;
    pid_t childPid;

    savedErrno = errno;

    printf("handler: Caught SIGCHLD : %d\n", sig);

    while ((childPid = waitpid(-1, &status, WNOHANG)) > 0) {
        printf("handler: Reaped child %ld - ", (long) childPid);
        (NULL, status);
    }

    if (childPid == -1 && errno != ECHILD)
        printf("waitpid");

    printf("handler: returning\n");

    errno = savedErrno;
}

static void create_msq(mqd_t *msq, const char *name, int max_num, int max_size){
    struct mq_attr msqinfo;
    int flags = O_CREAT | O_RDWR | O_CLOEXEC;


    msqinfo.mq_maxmsg = max_num;
    msqinfo.mq_msgsize = max_size;
    *msq = mq_open(name, flags, 0777, &msqinfo);
    if ( *msq < 0){
        perror("Message queue open error");
        exit(0);
    }

}