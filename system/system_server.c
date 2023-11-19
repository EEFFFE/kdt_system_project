#include <stdio.h>
#include <sys/prctl.h>

#include <system_server.h>
#include <gui.h>
#include <input.h>
#include <web_server.h>

int system_server()
{
    printf("Enter system_server process\n");

    while (1) {
        //printf("system_server process is operating \n");
        sleep(1);
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
            system_server();
            break;
        
        default:
            break;
    }

    return 0;
}
