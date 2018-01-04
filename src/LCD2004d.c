#include <stdio.h>
#include <stdbool.h>
#include <signal.h>
#include <syslog.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <linux/uinput.h>
#include "lib/mcp3008.h"


#define UI_GET_VERSION		_IOR(UINPUT_IOCTL_BASE, 45, unsigned int)
#define DAEMON_NAME "LCD2004d"

#define NORMAL 8
#define UP     4
#define BACK   0
#define MENU   9
#define ENTER  7
#define DOWN   2

 
void daemonShutdown();
void signal_handler(int sig);
void daemonize(char *rundir, char *pidfile);


bool verbose = true;

int pidFilehandle,version, rc, fd , spi_fx , level;
struct uinput_user_dev uud;
char spi_dev[20] = "/dev/spidev0.0";


void signal_handler(int sig)
{
    switch(sig)
    {
        case SIGHUP:
            syslog(LOG_WARNING, "Received SIGHUP signal.");
            break;
        case SIGINT:
        case SIGTERM:
            syslog(LOG_INFO, "Daemon exiting");
            daemonShutdown();
            exit(EXIT_SUCCESS);
            break;
        default:
            syslog(LOG_WARNING, "Unhandled signal %s", strsignal(sig));
            break;
    }
}

void daemonShutdown()
{

    close(pidFilehandle);
    ioctl(fd, UI_DEV_DESTROY);
    close(fd);
}

void daemonize(char *rundir, char *pidfile)
{
    int pid, sid, i;
    char str[10];
    struct sigaction newSigAction;
    sigset_t newSigSet;

    /* Check if parent process id is set */
    if (getppid() == 1)
    {
        /* PPID exists, therefore we are already a daemon */
        return;
    }

    /* Set signal mask - signals we want to block */
    sigemptyset(&newSigSet);
    sigaddset(&newSigSet, SIGCHLD);  /* ignore child - i.e. we don't need to wait for it */
    sigaddset(&newSigSet, SIGTSTP);  /* ignore Tty stop signals */
    sigaddset(&newSigSet, SIGTTOU);  /* ignore Tty background writes */
    sigaddset(&newSigSet, SIGTTIN);  /* ignore Tty background reads */
    sigprocmask(SIG_BLOCK, &newSigSet, NULL);   /* Block the above specified signals */

    /* Set up a signal handler */
    newSigAction.sa_handler = signal_handler;
    sigemptyset(&newSigAction.sa_mask);
    newSigAction.sa_flags = 0;

        /* Signals to handle */
        sigaction(SIGHUP, &newSigAction, NULL);     /* catch hangup signal */
        sigaction(SIGTERM, &newSigAction, NULL);    /* catch term signal */
        sigaction(SIGINT, &newSigAction, NULL);     /* catch interrupt signal */


    /* Fork*/
    pid = fork();

    if (pid < 0)
    {
        /* Could not fork */
        exit(EXIT_FAILURE);
    }

    if (pid > 0)
    {
        /* Child created ok, so exit parent process */
        printf("Child process created: %d\n", pid);
        exit(EXIT_SUCCESS);
    }

    /* Child continues */

    umask(027); /* Set file permissions 750 */

    /* Get a new process group */
    sid = setsid();

    if (sid < 0)
    {
        exit(EXIT_FAILURE);
    }

    /* close all descriptors */
    for (i = getdtablesize(); i >= 0; --i)
    {
        close(i);
    }

    /* Route I/O connections */

    /* Open STDIN */
    i = open("/dev/null", O_RDWR);

    /* STDOUT */
    dup(i);

    /* STDERR */
    dup(i);

    chdir(rundir); /* change running directory */

    /* Ensure only one copy */
    pidFilehandle = open(pidfile, O_RDWR|O_CREAT, 0600);

    if (pidFilehandle == -1 )
    {
        /* Couldn't open lock file */
        syslog(LOG_INFO, "Could not open PID lock file %s, exiting", pidfile);
        exit(EXIT_FAILURE);
    }

    /* Try to lock file */
    if (lockf(pidFilehandle,F_TLOCK,0) == -1)
    {
        /* Couldn't get lock on lock file */
        syslog(LOG_INFO, "Could not lock PID lock file %s, exiting", pidfile);
        exit(EXIT_FAILURE);
    }


    /* Get and format PID */
    sprintf(str,"%d\n",getpid());

    /* write pid to lockfile */
    write(pidFilehandle, str, strlen(str));
}





/* emit function is identical to of the first example */

void emit(int fd, int type, int code, int val)
{
   struct input_event ie;

   ie.type = type;
   ie.code = code;
   ie.value = val;
   /* timestamp values below are ignored */
   ie.time.tv_sec = 0;
   ie.time.tv_usec = 0;

   int res = write(fd, &ie, sizeof(ie));
   printf("emit write bytes=%d fd=%d code=%d val=%d\n",res, fd, code, val);
}


int main()
{
    struct uinput_user_dev uud;
    int version, rc, fd , lastkey ;
    bool pressed = false;



    fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    printf("fd=%d\n",fd);

    rc = ioctl(fd, UI_GET_VERSION, &version);
    printf("rd=%d\n",rc);

    if (rc == 0 && version >= 5)
    {
    printf("Error! version=%d\n",version);
      //return 0;
    }

    /*
    * The ioctls below will enable LCD2004-keypad to pass key events
    */
    int i1 = ioctl(fd, UI_SET_EVBIT, EV_KEY);
    int i2 = ioctl(fd, UI_SET_EVBIT, EV_SYN);
    int i3 = ioctl(fd, UI_SET_KEYBIT, KEY_UP);
    int i4 = ioctl(fd, UI_SET_KEYBIT, KEY_DOWN);
    int i5 = ioctl(fd, UI_SET_KEYBIT, KEY_BACKSPACE);
    int i6 = ioctl(fd, UI_SET_KEYBIT, KEY_HOME);
    int i7 = ioctl(fd, UI_SET_KEYBIT, KEY_ENTER);

    //  printf("ioctl = %d, %d, %d ,%d ,%d ,%d, %d\n", i1,i2,i3,i4,i5,i6,i7);

    memset(&uud, 0, sizeof(uud));
    snprintf(uud.name, UINPUT_MAX_NAME_SIZE, "LCD2004-keypad");
    uud.id.bustype = BUS_HOST;
    uud.id.vendor  = 0x1;
    uud.id.product = 0x2;
    uud.id.version = 1;

    write(fd, &uud, sizeof(uud));
    sleep(2);

    int i = ioctl(fd, UI_DEV_CREATE);
    syslog(LOG_INFO, "dev create =%d\n", i);

    sleep(2);

        /* Debug logging
        setlogmask(LOG_UPTO(LOG_DEBUG));
        openlog(DAEMON_NAME, LOG_CONS, LOG_USER);
        */

        /* Logging */
        setlogmask(LOG_UPTO(LOG_INFO));
        openlog(DAEMON_NAME, LOG_CONS | LOG_PERROR, LOG_USER);

        syslog(LOG_INFO, "LCD2004 daemon starting up");

        /* Deamonize */
        daemonize("/tmp/", "/tmp/daemon.pid");

        syslog(LOG_INFO, "LCD2004 daemon running");


        if ((spi_fx = mcp3008_open(spi_dev)) < 0) {
            syslog(LOG_WARNING, "LCD2004 spidev error");
            perror("mcp3008_open");
            exit(EXIT_FAILURE);
        }

        syslog(LOG_INFO, "LCD2004 spidev created");


        while (1)
        {


            level =  (mcp3008_read(spi_fx, 0));// /100 ;
            float voltage = mcp3008_read(spi_fx, 0) * (3.3 / 1020.0);
            usleep(7500);
            sleep(1);
            //syslog(LOG_INFO, "level : %d voltage : %f  \n" ,  level , voltage) ;

            if((voltage > 0.2 && voltage < 0.4)){

                emit(fd, EV_KEY, KEY_BACKSPACE, 1);
                emit(fd, EV_SYN, SYN_REPORT, 0);
                lastkey = KEY_BACKSPACE;
                if (verbose) syslog(LOG_INFO,"BACK");

            }
            else if(voltage > 1.9 && voltage <2.5){

                emit(fd, EV_KEY, KEY_UP, 1);
                emit(fd, EV_SYN, SYN_REPORT, 0);
                lastkey = KEY_UP;
                if (verbose) syslog(LOG_INFO,"UP");
            }
            else if(voltage > 1.3 && voltage < 1.9){

                emit(fd, EV_KEY, KEY_ENTER, 1);
                emit(fd, EV_SYN, SYN_REPORT, 0);
                lastkey = KEY_ENTER;
                if (verbose) syslog(LOG_INFO,"ENTER");
            }
            else if(voltage > 0.4 && voltage < 0.9){

                emit(fd, EV_KEY, KEY_DOWN, 1);
                emit(fd, EV_SYN, SYN_REPORT, 0);
                lastkey = KEY_DOWN;
                if (verbose) syslog(LOG_INFO,"DOWN");

            }
            else if(voltage > 0.9 && voltage < 1.3){

                emit(fd, EV_KEY, KEY_HOME, 1);
                emit(fd, EV_SYN, SYN_REPORT, 0);
                lastkey = KEY_HOME;
                if (verbose) syslog(LOG_INFO,"MENU");

            }
/*
                switch (voltage){

                    case (voltage > 0.2 && voltage < 0.4)  :
                        emit(fd, EV_KEY, KEY_BACKSPACE, 1);
                        emit(fd, EV_SYN, SYN_REPORT, 0);
                        lastkey = KEY_BACKSPACE;
                        if (verbose) syslog(LOG_INFO,"BACK");
                        break;
                    case (voltage > 1.9 && voltage <2.5)    :
                        emit(fd, EV_KEY, KEY_UP, 1);
                        emit(fd, EV_SYN, SYN_REPORT, 0);
                        lastkey = KEY_UP;
                        if (verbose) syslog(LOG_INFO,"UP");
                        break;
                    case (voltage > 1.3 && voltage < 1.9) :
                        emit(fd, EV_KEY, KEY_ENTER, 1);
                        emit(fd, EV_SYN, SYN_REPORT, 0);
                        lastkey = KEY_ENTER;
                        if (verbose) syslog(LOG_INFO,"ENTER");
                        break;
                    case (voltage > 0.4 && voltage < 0.9)  :
                        emit(fd, EV_KEY, KEY_DOWN, 1);
                        emit(fd, EV_SYN, SYN_REPORT, 0);
                        lastkey = KEY_DOWN;
                        if (verbose) syslog(LOG_INFO,"DOWN");
                        break;

                    case (voltage > 0.9 && voltage < 1.3)  :
                        emit(fd, EV_KEY, KEY_HOME, 1);
                        emit(fd, EV_SYN, SYN_REPORT, 0);
                        lastkey = KEY_HOME;
                        if (verbose) syslog(LOG_INFO,"MENU");
                        break;
                }

*/
                if (voltage < 3 ){


                emit(fd, EV_KEY, lastkey , 0);
                emit(fd, EV_SYN, SYN_REPORT, 0);
                if (verbose) syslog(LOG_INFO,"%d released ", lastkey);
                pressed = false ;
            }





        }
}
