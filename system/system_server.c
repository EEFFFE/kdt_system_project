#include <stdio.h>
#include <sys/prctl.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <assert.h>
#include <pthread.h>

#include <system_server.h>
#include <gui.h>
#include <input.h>
#include <web_server.h>
#include <camera_HAL.h>
#include <bits/sigevent-consts.h>

pthread_mutex_t system_loop_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  system_loop_cond  = PTHREAD_COND_INITIALIZER;
bool            system_loop_exit = false;    ///< true if main loop should exit

static int toy_timer = 0;

void timer_sighandler();
int posix_sleep_ms(unsigned int overp_time, unsigned int underp_time);

void *watchdog_thread();
void *monitor_thread();
void *disk_service_thread();
void *camera_service_thread();

void signal_exit(void);
static void timer_expire_signal_handler();

int system_server();


int system_server()
{
    struct itimerspec it;
    struct sigaction  sa;
    struct sigevent   sev;
    timer_t tidlist;
    int retcode;
    pthread_t watchdog_thread_tid, monitor_thread_tid, disk_service_thread_tid, camera_service_thread_tid;

    printf("나 system_server 프로세스!\n");

    signal(SIGALRM, timer_expire_signal_handler);
    

    sigemptyset(&sa.sa_mask);
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
    if(pthread_mutex_unlock(&system_loop_mutex) < 0){
        perror("System server mutex unlock error");
        exit(0);
    }
    while (system_loop_exit == false) {
        sleep(1);
    }
    
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
    toy_timer ++;
    signal_exit();
    //printf("System timer : %d sec\n", timer );
}   

int posix_sleep_ms(unsigned int overp_time, unsigned int underp_time)
{
    struct timespec sleep_time;

    sleep_time.tv_sec = overp_time;
    sleep_time.tv_nsec = underp_time;

    return nanosleep(&sleep_time, NULL);
}

void *watchdog_thread(){
    printf("Watchdog thread started!\n");
    while(1){
        posix_sleep_ms(60, 0);
    }
}

void *monitor_thread(){
    printf("Monitor thread started!\n");
    while(1){
        posix_sleep_ms(60, 0);
    }

}

void *disk_service_thread(){
    printf("Disk service thread started!\n");
    while(1){
        posix_sleep_ms(60, 0);
    }
}

void *camera_service_thread(){
    printf("Camera service thread started!\n");
    toy_camera_open();
    toy_camera_take_picture();
    while(1){
        posix_sleep_ms(60, 0);
    }

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

static void timer_expire_signal_handler()
{
    toy_timer++;
    signal_exit();
}
