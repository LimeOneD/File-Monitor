#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <signal.h>
#include <sys/inotify.h>
#include <libnotify/notify.h>
#include <unistd.h>


#define EXT_SUCCESS 0
#define EXT_ERR_TOO_FEW_ARGS 1
#define EXT_ERR_INIT_INOTIFY 2
#define EXT_ERR_ADD_WATCH 3
#define EXT_ERR_BASE_PATH_NULL 4
#define EXT_ERR_READ_INOTIFY 5
#define EXT_ERR_LIBNOTIFY 6

int IeventStatus =-1;
int IeventQueue =-1;
char *ProgramTitle = "wifi_daemon";

void signal_handler(int signal)
{
    int closeStatus = -1;
    printf("signal recieved: cleaning up");
    closeStatus = inotify_rm_watch(IeventQueue, IeventStatus);

    if(closeStatus == -1 ) 
    {
        fprintf(stderr, "Error removing from watch queue! \n");
    
    }
    close(IeventQueue);

    exit(EXIT_SUCCESS);
}


int main(int argc, char** argv)
{  
    bool libnotifyInitStatus = false;
    char *basePath = NULL;
    char *token = NULL;
    char buffer[4096];
    int readLength;
    const struct inotify_event *watchEvent;
    char *notificationMessage = NULL;
    NotifyNotification *notifyHandle;


    const uint32_t watchMask = IN_CREATE | IN_DELETE | IN_ACCESS | IN_CLOSE_WRITE | IN_MODIFY | IN_MOVE_SELF;
    if(argc<2) 
    {
        fprintf(stderr, "USAGE: rolexhound PATH\n");
        exit(EXT_ERR_TOO_FEW_ARGS);
    }
    basePath = (char *) malloc(sizeof(char) * (strlen(argv[1])+1));
    strcpy(basePath, argv[1]);
    
    token = strtok(basePath, "/");
    
    while(token != NULL)
    {
        basePath = token;
        token = strtok(NULL, "/");
    }
    if(basePath == NULL)
    {
        fprintf(stderr, "error getting base path\n");
        exit(EXT_ERR_BASE_PATH_NULL);
    }
    libnotifyInitStatus = notify_init(ProgramTitle);
    if(!libnotifyInitStatus) 
    {
        fprintf(stderr, "libnotify init error");
        exit(EXT_ERR_LIBNOTIFY);
    }


    IeventQueue = inotify_init();
    if(IeventQueue == -1) 
    {
        fprintf(stderr, "Error initialising inotify instance\n");
        exit(EXT_ERR_INIT_INOTIFY);

    }

    IeventStatus = inotify_add_watch(IeventQueue, argv[1], watchMask);
    if(IeventStatus == -1)
    {
        fprintf(stderr, "error adding file to watchlist");
        exit(EXT_ERR_ADD_WATCH);
    }
    signal(SIGABRT, signal_handler);
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    while(true)
    {
        printf("waiting for ievent...\n");
        readLength = read(IeventQueue, buffer, sizeof(buffer));
        if(readLength == -1)
        {
            fprintf(stderr, "error reading from inotify instance\n");
            exit(EXT_ERR_READ_INOTIFY);
        }
        for(char *buffPointer= buffer; buffPointer < buffer + readLength; )//buffPointer+=sizeof(struct inotify_event) + watchEvent->len)
        {
            watchEvent = (const struct inotify_event*) buffPointer;
            printf("Processing event: mask=%u, len=%u\n", watchEvent->mask, watchEvent->len);

            notificationMessage = NULL;

            if(watchEvent->mask & IN_CREATE)
                notificationMessage = "File created\n";

            else if(watchEvent->mask & IN_DELETE)
                notificationMessage = "File deleted\n";

            else if(watchEvent->mask & IN_ACCESS)
                notificationMessage = "File accessed\n";

            else if(watchEvent->mask & IN_CLOSE_WRITE )
                notificationMessage = "File written and closed\n";

            else if(watchEvent->mask & IN_MODIFY)
                notificationMessage = "File modified\n";

            else if(watchEvent->mask & IN_MOVE_SELF)
                notificationMessage = "File moved\n";
            

            if(notificationMessage != NULL)
            {

                printf("%s\n", notificationMessage);

                notifyHandle = notify_notification_new(basePath, notificationMessage, "dialog-information");
                if(notifyHandle == NULL)
                {
                    fprintf(stderr, "notifyHandle was NULL");
                    continue;
                }
           
                else notify_notification_show(notifyHandle, NULL);
            }
            
            buffPointer+=sizeof(struct inotify_event) + watchEvent->len;
            printf("Advancing pointer by %lu bytes\n", sizeof(struct inotify_event) + watchEvent->len);///
            
            usleep(10000);

        }
                    

    }
}
