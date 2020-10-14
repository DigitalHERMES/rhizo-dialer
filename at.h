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

#ifndef HAVE_AT_H__
#define HAVE_AT_H__

#include <stdbool.h>

#define MAX_MODEM_PATH 4096
#define MAX_BUF_SIZE 4096

#define BACKEND_NONE 0
#define BACKEND_AT 1

void strip_cr(char *s);

bool is_final_result(const char * const response);

#if 0
bool get_response(char *response, FILE *modem);
#endif

int open_serial_port(char *ttyport);

void set_fixed_baudrate(char *baudname, int target_fd);

bool run_at_backend(int modem_fd);

#endif // HAVE_AT_H__
