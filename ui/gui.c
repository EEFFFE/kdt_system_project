#include <stdio.h>
#include <errno.h>

#include <system_server.h>
#include <gui.h>
#include <input.h>
#include <web_server.h>

int create_gui()
{
    pid_t systemPid;

    printf("Creating GUI process \n");

    sleep(3);
    
    switch(systemPid = fork()){
        case -1 : 
            perror("GUI process fork failed \n");

            case 0 :
            if(execl("/usr/bin/google-chrome-stable", "google-chrome-stable", "http://localhost:8282", NULL)) {
                perror("GUI process execl failed \n");
            }
            printf("Finish GUI Process\n");
            break;
        default :
            break;
    }
    return 0;
}
