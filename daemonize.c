/* Code based on:
 *       daemon.c
 *
 *       Copyright 2010 Vasudev Kamath <kamathvasudev@gmail.com>
 *     
 *       This program is free software; you can redistribute it and/or modify
 *       it under the terms of the GNU Lesser General Public License as published by
 *       the Free Software Foundation; either version 3 of the License, or
 *       (at your option) any later version.
 *     
 *       This program is distributed in the hope that it will be useful,
 *       but WITHOUT ANY WARRANTY; without even the implied warranty of
 *       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *       GNU General Public License for more details.
 *      
 *       You should have received a copy of the GNU General Public License
 *       along with this program; if not, write to the Free Software
 *       Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *       MA 02110-1301, USA.
 */

#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#include "daemonize.h"

char *get_time()
{
    time_t rawtime;
    struct tm *timeinfo;

    time (&rawtime);
    timeinfo = localtime (&rawtime);
    return (asctime (timeinfo));
}

void log_message(char *filename, char *message){
    FILE *logfile;
    logfile = fopen(filename,"a");
    if(!logfile)
        return;
    fprintf(logfile,"%s", message);
    fflush(logfile);
    fclose(logfile);
}

void signal_handler(int sig){
    switch(sig){
    case SIGHUP:
        log_message(LOG_FILE,"Hangup Signal Catched");
        break;
    case SIGTERM:
        log_message(LOG_FILE,"Terminate Signal Catched");
        exit(0);
        break;
    }
}

void daemonize(){
    int i,lfp;
    char str[10];

    if(getppid() == 1)
        return;
    i = fork();

    if(i < 0)
        exit(1);
    if(i > 0)
        exit(0);


    if (setsid() < 0)
      exit(EXIT_FAILURE);

    for(i = getdtablesize(); i >= 0; --i)
        close(i);

    i = open("/dev/null",O_RDWR);
    dup(i);
    dup(i);
    dup(i);
    umask(027);

    chdir(RUNNING_DIR);

    signal(SIGCHLD,SIG_IGN);
    signal(SIGTSTP,SIG_IGN);
    signal(SIGTTOU,SIG_IGN);
    signal(SIGTTIN,SIG_IGN);

    signal(SIGHUP,signal_handler);
    signal(SIGTERM,signal_handler);
    
    /* Fork off for the second time*/
    i = fork();

    char buff[512];
    sprintf(buff, "i = %d\n", i);
    log_message(LOG_FILE, buff);    
    
    /* An error occurred */
    if (i < 0)
        exit(EXIT_FAILURE);

    /* Success: Let the parent terminate */
    if (i > 0)
        exit(EXIT_SUCCESS);

    lfp = open(LOCK_FILE,O_RDWR|O_CREAT,0640);
    if(lfp < 0)
        exit(1);
    if(lockf(lfp,F_TLOCK,0) < 0)
        exit(1);

    sprintf(str,"%d\n",getpid());
    write(lfp,str,strlen(str));


    
}
