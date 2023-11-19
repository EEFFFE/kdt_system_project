#include <stdio.h>
#include <sys/prctl.h>

#include <system_server.h>
#include <gui.h>
#include <input.h>
#include <web_server.h>

int input()
{
    printf("Enter input process\n");

    while (1) {
        sleep(1);
    }

    return 0;
}

int create_input()
{
    pid_t systemPid;
    const char *name = "input";

    printf("Creating input process\n");

    switch(systemPid = fork()){
        case -1:
            perror("input process create failed \n");
        
        case 0 :
            input();
            break;

        default :
            break;
    }

    return 0;
}
