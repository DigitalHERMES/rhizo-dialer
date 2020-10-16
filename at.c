/*
 * Copyright (C) 2020 Rhizomatica <rafael@rhizomatica.org>
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 *
 * Rhizo-dialer is an experimental dialer for Maemo.
 *
 */

#include <sys/types.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <strings.h>
#include <asm/termbits.h>
#include <errno.h>
#include <threads.h>

#include "at.h"
#include "ring-audio.h"
#include "daemonize.h"

#include <hildon/hildon-banner.h>
#include <hildon/hildon-program.h>
#include <hildon/hildon.h>
#include <gtk/gtk.h>

extern GtkWidget *window;
extern char dial_pad[MAX_BUF_SIZE];
extern GtkWidget *display;

struct baudrate {
    char       *name;
    int         termios_code;
    int         nonstd_speed;
    int         bootrom_code;
    int         xram_records;
};

void strip_cr(char *s)
{
    char *from, *to;
    from = to = s;
    while (*from != '\0') {
        if (*from == '\r') {
            from++;
            continue;
        }
        *to++ = *from++;
    }
    *to = '\0';
}

bool is_final_result(const char * const response)
{
#define STARTS_WITH(a, b) ( strncmp((a), (b), strlen(b)) == 0)
    switch (response[0]) {
    case '+':
        if (STARTS_WITH(&response[1], "CME ERROR:")) {
            return true;
        }
        if (STARTS_WITH(&response[1], "CMS ERROR:")) {
            return true;
        }
        return false;
    case 'B':
        if (strcmp(&response[1], "USY\r\n") == 0) {
            return true;
        }
        return false;
    case 'E':
        if (strcmp(&response[1], "RROR\r\n") == 0) {
            return true;
        }
        return false;
    case 'N':
        if (strcmp(&response[1], "O ANSWER\r\n") == 0) {
            return true;
        }
        if (strcmp(&response[1], "O CARRIER\r\n") == 0) {
            return true;
        }
        if (strcmp(&response[1], "O DIALTONE\r\n") == 0) {
            return true;
        }
        return false;
    case 'O':
        if (strcmp(&response[1], "K\r\n") == 0) {
            return true;
        }
        return false;
    case 'R':
        if (strcmp(&response[1], "ING\r\n") == 0) {
            return true;
        }
        /* no break */
    default:
        return false;
    }
}


int open_serial_port(char *ttyport)
{
    int target_fd = open(ttyport, O_RDWR|O_NONBLOCK);
    if (target_fd < 0)
    {
        log_message(LOG_FILE, "open() serial port error\n");
        // perror(ttyport);
        exit(EXIT_FAILURE);
    }

    ioctl(target_fd, TIOCEXCL);
    return target_fd;
}


struct baudrate baud_rate_table[] = {
    /* the first listed rate will be our default */
    {"115200",	B115200,	0,	0,	100},
    {"57600",	B57600,		0,	1,	100},
    {"38400",	B38400,		0,	2,	100},
    {"19200",	B19200,		0,	4,	50},
    /* Non-standard high baud rates */
    {"812500",	BOTHER,		812500,	-1,	1000},
    {"406250",	BOTHER,		406250,	-1,	500},
    {"203125",	BOTHER,		203125,	-1,	250},
    /* table search terminator */
    {NULL,		B0,		0,	-1,	0},
};

struct baudrate *find_baudrate_by_name(char *srch_name)
{
    struct baudrate *br;

    for (br = baud_rate_table; br->name; br++)
        if (!strcmp(br->name, srch_name))
            break;
    if (br->name)
        return(br);
    else
    {
        log_message(LOG_FILE, "error: baud rate not known\n");
//        fprintf(stderr, "error: baud rate \"%s\" not known\n", srch_name);
        return(NULL);
    }
}

struct baudrate *set_serial_baudrate(struct baudrate *br, int target_fd)
{
    struct termios2 target_termios;

    target_termios.c_iflag = IGNBRK;
    target_termios.c_oflag = 0;
    target_termios.c_cflag = br->termios_code | CLOCAL|HUPCL|CREAD|CS8;
    target_termios.c_lflag = 0;
    target_termios.c_cc[VMIN] = 1;
    target_termios.c_cc[VTIME] = 0;
    target_termios.c_ispeed = br->nonstd_speed;
    target_termios.c_ospeed = br->nonstd_speed;
    if (ioctl(target_fd, TCSETSF2, &target_termios) < 0) {
        log_message(LOG_FILE, "ioctl() TCSETSF2 error\n");
        // perror("TCSETSF2");
        exit(1);
    }

    return br;
}

void set_fixed_baudrate(char *baudname, int target_fd)
{
    struct baudrate *br;

    br = find_baudrate_by_name(baudname);
    if (!br)
        exit(1); /* error msg already printed */
    set_serial_baudrate(br, target_fd);
}

void safe_output(unsigned char *buf, int cc)
{
    int i, c;

    for (i = 0; i < cc; i++) {
        c = buf[i];
        if (c == '\r' || c == '\n' || c == '\t' || c == '\b') {
            continue;
        }
        if (c & 0x80) {
            c &= 0x7F;
        }
        if (c < 0x20) {
            buf[i] = '^';
        } else if (c == 0x7F) {
            buf[i] = '?';
        }
    }
}

int loop(void *arg)
{
    int *target_fd_ptr = (int *)arg;
    int target_fd = *target_fd_ptr;
    char buf[MAX_BUF_SIZE];
    fd_set fds, fds1;
    int i, cc, max;

    if (thrd_detach(thrd_current()) != thrd_success) {
        /* Handle error */
    }

    FD_ZERO(&fds);
    FD_SET(target_fd, &fds);
    max = target_fd + 1;
    for (;;) {
        bcopy(&fds, &fds1, sizeof(fd_set));
        i = select(max, &fds1, NULL, NULL, NULL);
        if (i < 0) {
            if (errno == EINTR)
                continue;
            log_message(LOG_FILE, "select() error\n");
            // perror("select");
            exit(1);
        }

        if (FD_ISSET(target_fd, &fds1)) {
            cc = read(target_fd, buf, sizeof buf);
            if (cc <= 0) {
                fprintf(stderr, "EOF/error on target tty\n");
                exit(1);
            }
            buf[cc] = 0;
            if (strstr(buf,"ING") != NULL)
            {
                char ring_str[512];
                sprintf(ring_str, "%s: RINGING\n", get_time());
                log_message(LOG_FILE,ring_str);
                if (!gtk_widget_get_visible (GTK_WIDGET(window)))
                {
                    gtk_widget_show(GTK_WIDGET(window));
                    // sprintf(dial_pad, "!! RINGING !!");
                }
                hildon_entry_set_text((HildonEntry *)display, "!! RINGING !!");
                ring(1, 1800.0);
            }
            safe_output(buf, cc);
            log_message(LOG_FILE, buf);
        }
    }
}

bool run_at_backend(int modem_fd)
{
    thrd_t rx_thread;
    int result;

    char cmd[MAX_BUF_SIZE];

    sprintf(cmd, "ATZ\r");
    int res = write(modem_fd, cmd, strlen(cmd));

    if (res < 0)
    {
        log_message(LOG_FILE, "Error writing to the modem\n");
        return false;
    }

    thrd_create(&rx_thread, loop, &modem_fd );

    return true;
}
