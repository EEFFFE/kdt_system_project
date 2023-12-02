#define CAMERA_TAKE_PICTURE 1

#include <stdio.h>
#include <sys/prctl.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <assert.h>
#include <pthread.h>
#include <mqueue.h>
#include <semaphore.h>

#include <system_server.h>
#include <gui.h>
#include <input.h>
#include <web_server.h>
#include <camera_HAL.h>
#include <toy_message.h>
#include <bits/sigevent-consts.h>

pthread_mutex_t system_loop_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  system_loop_cond  = PTHREAD_COND_INITIALIZER;
bool            system_loop_exit = false;    ///< true if main loop should exit

static mqd_t watchdog_queue;
static mqd_t monitor_queue;
static mqd_t disk_queue;
static mqd_t camera_queue;

static int toy_timer = 0;
pthread_mutex_t toy_timer_mutex = PTHREAD_MUTEX_INITIALIZER;
static sem_t global_timer_sem;
static bool global_timer_stopped;

void timer_sighandler();
int posix_sleep_ms(unsigned int overp_time, unsigned int underp_time);


void print_msq(toy_msg_t *msg);
void *watchdog_thread();
void *monitor_thread();
void *disk_service_thread();
void *camera_service_thread();
static void *timer_thread(void *not_used);

void signal_exit(void);
static void timer_expire_signal_handler();
void set_periodic_timer(long sec_delay, long usec_delay);

int system_server();


int system_server()
{
    struct itimerspec it;
    struct sigaction  sa;
    struct sigevent   sev;
    timer_t tidlist;
    int retcode;
    pthread_t watchdog_thread_tid, monitor_thread_tid, disk_service_thread_tid, camera_service_thread_tid, timer_thread_tid;;

    printf("나 system_server 프로세스!\n");

    /*sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;
    sa.sa_handler = timer_sighandler;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGALRM, &sa, NULL) < 0)
        perror("Sigaction error");

    it.it_value.tv_sec = 10;
    it.it_value.tv_nsec = 0;
    it.it_interval.tv_sec = 10;
    it.it_interval.tv_nsec = 0;

    sev.sigev_notify = SIGEV_SIGNAL;    
    sev.sigev_signo = SIGALRM;        
    sev.sigev_value.sival_ptr = &tidlist;

    if (timer_create(CLOCK_REALTIME, &sev, &tidlist) < 0) {
        perror("timer_create");
        exit(-1);
    }

    if (timer_settime(tidlist, 0, &it, NULL) < 0) {
        perror("timer_settime");
        exit(-1);
    }    
    */
    watchdog_queue = mq_open("/watchdog_queue", O_RDWR);
    monitor_queue = mq_open("/monitor_queue", O_RDWR);
    disk_queue = mq_open("/disk_queue", O_RDWR);
    camera_queue = mq_open("/camera_queue", O_RDWR);
    

    retcode = pthread_create(&watchdog_thread_tid, NULL, watchdog_thread, NULL);
    if(retcode < 0){
        perror("Watchdog thread create error");
        exit(1);
    }

    retcode = pthread_create(&monitor_thread_tid, NULL, monitor_thread, NULL);
    if(retcode < 0){
        perror("Watchdog thread create error");
        exit(1);
    }

    retcode = pthread_create(&disk_service_thread_tid, NULL, disk_service_thread, NULL);
    if(retcode < 0){
        perror("Disk service thread create error");
        exit(1);
    }

    retcode = pthread_create(&camera_service_thread_tid, NULL, camera_service_thread, NULL);
    if(retcode < 0){
        perror("Camera service thread create error");
        exit(1);
    }

    retcode = pthread_create(&timer_thread_tid, NULL, timer_thread, NULL);
    if(retcode < 0){
        perror("Camera service thread create error");
        exit(1);
    }

    if(pthread_mutex_lock(&system_loop_mutex) < 0){
        perror("System server mutex lock error");
        exit(0);
    }
    while (system_loop_exit == false) {
        int result = pthread_cond_wait(&system_loop_cond, &system_loop_mutex);
        if(result == ETIMEDOUT){
            printf("System exit timed out!\n");
        }
    }
    while (system_loop_exit == false) {
        pthread_cond_wait(&system_loop_cond, &system_loop_mutex);
    }
    if(pthread_mutex_unlock(&system_loop_mutex) < 0){
        perror("System server mutex unlock error");
        exit(0);
    }
    while (system_loop_exit == false) {
        sleep(1);
    }
    
    printf("<== system\n");

    while (1) {
        posix_sleep_ms(5, 0);
    }

    return 0;
}

int create_system_server()
{
    pid_t systemPid;
    const char *name = "system_server";

    printf("Creating system_server process \n");

    switch(systemPid = fork()) {
        case -1:
            perror("system_sever process folk failed \n");

        case 0:
            if (prctl(PR_SET_NAME, (unsigned long) name) < 0)
                perror("prctl()");
            system_server();
            break;
        
        default:
            break;
    }

    return 0;
}

void timer_sighandler(){
    pthread_mutex_lock(&toy_timer_mutex);
    toy_timer++;
    printf("toy_timer: %d\n", toy_timer);
    pthread_mutex_unlock(&toy_timer_mutex);
}   

int posix_sleep_ms(unsigned int overp_time, unsigned int underp_time)
{
    struct timespec sleep_time;

    sleep_time.tv_sec = overp_time;
    sleep_time.tv_nsec = underp_time;

    return nanosleep(&sleep_time, NULL);
}

void print_msq(toy_msg_t *msg){
    printf("msg_type : %d\n", msg->msg_type);
    printf("param1 : %d\n", msg->param1);
    printf("param2 : %d\n", msg->param2);

}

void *watchdog_thread(){
    printf("Watchdog thread started!\n");
    toy_msg_t msg;
    while(1){
        if(mq_receive(watchdog_queue, (void *)&msg, sizeof(toy_msg_t), 0) < 0){
            perror("Message queue recevie error : watchdog");
            exit(0);
        }
        printf("Watchdog msq arrived\n ");
        print_msq(&msg);

    }
}

void *monitor_thread(){
    printf("Monitor thread started!\n");
    toy_msg_t msg;
    while(1){
        if(mq_receive(monitor_queue, (void *)&msg, sizeof(toy_msg_t), 0) < 0){
            perror("Message queue recevie error : monitor");
            exit(0);
        }
        printf("Monitor msq arrived\n");
        print_msq(&msg);
        
    }

}

void *disk_service_thread(){
    printf("Disk service thread started!\n");

    FILE* apipe;
    char buf[1024];
    char cmd[]="df -h ./";

    toy_msg_t msg;

    while(1){
        if(mq_receive(monitor_queue, (void *)&msg, sizeof(toy_msg_t), 0) < 0){
            perror("Message queue recevie error : disk");
            exit(0);
        }
        printf("Disk msq arrived\n");

        print_msq(&msg);

        apipe = popen(cmd, "r");
        if (apipe == NULL) {
            perror("popen");
            exit(0);
        }
        while (fgets(buf, 1024, apipe) != NULL) {
            printf("%s", buf);
        }
        pclose(apipe);
        //posix_sleep_ms(10, 0);
    }
}

void *camera_service_thread(){
    printf("Camera service thread started!\n");
    toy_camera_open();
    toy_camera_take_picture();
    toy_msg_t msg;

    while(1){
        if(mq_receive(camera_queue, (void *)&msg, sizeof(toy_msg_t), 0) < 0){
            perror("Message queue recevie error : camera");
            exit(0);
        }
        printf("Camera msq arrived\n");

        print_msq(&msg);
        if(msg.msg_type == CAMERA_TAKE_PICTURE){
            toy_camera_take_picture();
        }
    }

}

static void *timer_thread(void *not_used)
{
    signal(SIGALRM, timer_sighandler);
    set_periodic_timer(1, 1);

    sem_init(&global_timer_sem, 0, 0);

    while (!global_timer_stopped) {
        sem_wait(&global_timer_sem);
        timer_sighandler();
    }
	return 0;
}

void signal_exit(void)
{
    if(pthread_mutex_lock(&system_loop_mutex) < 0){
        perror("Signal exit mutex lock error");
        exit(0);    
    }
    system_loop_exit = true;
    pthread_cond_broadcast(&system_loop_cond);
    
    pthread_cond_signal(&system_loop_cond);
    if(pthread_mutex_unlock(&system_loop_mutex) < 0){
        perror("Signal exit mutex unlock error");
        exit(0);    
    }
}

static void timer_expire_signal_handler(){
    sem_post(&global_timer_sem);
}

void set_periodic_timer(long sec_delay, long usec_delay)
{
	struct itimerval itimer_val = {
		 .it_interval = { .tv_sec = sec_delay, .tv_usec = usec_delay },
		 .it_value = { .tv_sec = sec_delay, .tv_usec = usec_delay }
    };

	setitimer(ITIMER_REAL, &itimer_val, (struct itimerval*)0);
}