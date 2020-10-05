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

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/ioctl.h>

#include "at.h"

#define MAX_BUF_SIZE 4096

bool get_response(char *response, FILE *modem)
{
    char buf[MAX_BUF_SIZE] = {};
    char buf2[MAX_BUF_SIZE] = {};
    char *line;
    int res;

    fprintf(stderr, "fileno = %d\n", fileno(modem));
    
    do {
#if 0
      if (ioctl(fileno(modem), FIONREAD, &res) < 0)
        {
            fprintf(stderr, "Error in ioctl()\n");
            clearerr(modem);
            return false;
        }

        if (res < 1)
        {
	    fprintf(stderr, "no data!\n");
            return false;
        }
#endif
        line = fgets(buf, (int)sizeof(buf), modem);
        if (line == NULL) {
            fprintf(stderr, "EOF from modem\n");
            clearerr(modem);
            return false;
        }
        strcat(buf2, line);

    } while ( !is_final_result(line));

    strip_cr(buf2);
    strcpy(response, buf2);
    return true;
}


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
		/* no break */
	default:
		return false;
	}

}
