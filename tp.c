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
 * @file tp.c
 * @author Rafael Diniz
 * @date 07 Feb 2020
 * @brief Telepathy functions
 *
 * Telepathy functions
 *
 */

#include "tp.h"

void got_connections (const gchar * const *bus_names,
                 gsize n,
                 const gchar * const *cms,
                 const gchar * const *protocols,
                 const GError *error,
                 gpointer user_data,
                 GObject *unused)
{
    GMainLoop *mainloop = user_data;

    if (error != NULL)
    {
        g_warning ("%s", error->message);
    }
    else
    {
        gsize i;

        g_message ("Found %" G_GSIZE_FORMAT " connections:", n);

        for (i = 0; i < n; i++)
        {
            g_message ("%s", bus_names[i]);
            g_message ("- CM %s, protocol %s", cms[i], protocols[i]);
        }

        /* all the arrays are also NULL-terminated */
        g_assert (bus_names[n] == NULL && cms[n] == NULL &&
                  protocols[n] == NULL);
    }

    g_main_loop_quit (mainloop);
}

void got_connection_managers (GObject *source,
    GAsyncResult *result,
    gpointer user_data)
{
    GMainLoop *mainloop = user_data;
    GList *cms;
    GError *error = NULL;

    cms = tp_list_connection_managers_finish (result, &error);
    if (error != NULL)
    {
        g_warning ("%s", error->message);
        g_clear_error (&error);
    }
    else
    {
        g_message ("Found %u connection managers:", g_list_length (cms));

        while (cms != NULL)
        {
            TpConnectionManager *cm = cms->data;

            g_message ("- %s", tp_connection_manager_get_name (cm));

            g_object_unref (cm);
            cms = g_list_delete_link (cms, cms);
        }
    }

    g_main_loop_quit (mainloop);
}
