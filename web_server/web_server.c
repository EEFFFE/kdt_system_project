#include <stdio.h>

#include <system_server.h>
#include <gui.h>
#include <input.h>
#include <web_server.h>

int create_web_server()
{
    pid_t systemPid;

    printf("Creating web_server process\n");

    sleep(5);

    switch(systemPid = fork()){
        case -1 : 
            printf("web_server process fork failed \n");

        case 0 :
            if(execl("/usr/local/bin/filebrowser", "filebrowser", "-p", "8282", (char *) NULL)) {
                perror("web_server process execl failed \n");
            }
            printf("Finish web_server process\n");
            break;
        default :
            break;
    }

    return 0;
}
