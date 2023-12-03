#define STACK_SIZE (1024 * 1024)
#define _GNU_SOURCE

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sched.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>

#include <system_server.h>
#include <gui.h>
#include <input.h>
#include <web_server.h>


static int child_func(){

    printf("\n\n______________________________\n\n");
    printf("web server PID: %d, PPID %d \n", getpid(), getppid());

    
    printf("user id : %d, euid : %d ", getuid(), geteuid());
    printf("\n\n______________________________\n\n");

    if(execl("/usr/local/bin/filebrowser", "filebrowser", "-p", "8282", (char *) NULL) < 0){
        perror("execl failed : child_func");
    }
}

int create_web_server()
{
    pid_t systemPid;

    printf("Creating web_server process\n");

    sleep(3);
    char *stack = mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE, 
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
    if (stack == MAP_FAILED){
        perror("mmap failed : create web server");
        exit(0);
    }

    pid_t child_pid = clone(child_func, stack + STACK_SIZE, CLONE_NEWUTS | CLONE_NEWIPC | CLONE_NEWPID | 
                                CLONE_NEWNS | SIGCHLD, NULL);

    if(child_pid < 0){
        perror("clone failed : create_webserver");
        return -1;
    }

    munmap(stack, STACK_SIZE);

    waitpid(child_pid, NULL, 0);
    
    return 0;
}
