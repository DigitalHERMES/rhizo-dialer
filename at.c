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

#include <string.h>
#include <stdbool.h>

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
	        //		if (strcmp(&response[1], "O ANSWER\r\n") == 0) {
	        //return true;
	        //}
		//		if (strcmp(&response[1], "O CARRIER\r\n") == 0) {
		//			return true;
		//		}
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
