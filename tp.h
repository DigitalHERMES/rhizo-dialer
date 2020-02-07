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

/**
 * @file tp.h
 * @author Rafael Diniz
 * @date 07 Feb 2020
 * @brief Telepathy functions
 *
 * Telepathy functions
 *
 */

#ifndef HAVE_TP_H__
#define HAVE_TP_H__

#include <telepathy-glib/telepathy-glib.h>
#include <telepathy-glib/telepathy-glib-dbus.h>


void got_connections (const gchar * const *bus_names,
                             gsize n,
                             const gchar * const *cms,
                             const gchar * const *protocols,
                             const GError *error,
                             gpointer user_data,
                             GObject *unused);


void got_connection_managers (GObject *source,
    GAsyncResult *result,
    gpointer user_data);

#endif /* HAVE_TP_H__ */
