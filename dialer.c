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
 */

#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <telepathy-glib/telepathy-glib.h>
#include <telepathy-glib/telepathy-glib-dbus.h>

#define MAX_PHONE_SIZE 512

#define MODE_NONE 0
#define MODE_DIAL 1
#define MODE_ANSWER 2
#define MODE_HANGUP 3

static void got_connections (const gchar * const *bus_names,
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

static void got_connection_managers (GObject *source,
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

// check:
// https://git.sailfishos.org/mer-core/voicecall/tree/master/plugins/providers/telepathy
// https://git.sailfishos.org/mer-core/voicecall/blob/master/plugins/providers/telepathy/src/telepathyproviderplugin.cpp#L106

int main(int argc, char *argv[])
{
    char msisdn[MAX_PHONE_SIZE];
    int mode = MODE_NONE;

    if (argc < 2){
    usage_info:
        fprintf(stderr, "Usage: %s -d <phone_number>\n", argv[0]);
        fprintf(stderr, "%s -a\n", argv[0]);
        fprintf(stderr, "%s -h\n\n", argv[0]);
        fprintf(stderr, "Usage example: %s -d 99991234\n\n", argv[0]);
        fprintf(stderr, "OPTIONS:\n");
        fprintf(stderr, "    -d <phone_number>       Dial to a given phone number\n");
        fprintf(stderr, "    -a                      Answer an incoming call\n");
        fprintf(stderr, "    -h                      Hangup current call\n");
        return EXIT_SUCCESS;
    }
    int opt;
    while ((opt = getopt(argc, argv, "d:ah")) != -1){
        switch (opt){
        case 'd':
            strncpy (msisdn, optarg, MAX_PHONE_SIZE);
            mode = MODE_DIAL;
            break;
        case 'a':
            mode = MODE_ANSWER;
            break;
        case 'h':
            mode = MODE_HANGUP;
            break;
        default:
            fprintf(stderr, "Wrong command line.\n");
            goto usage_info;
        }
    }

    // nothing works yet, just playing with tekepathy...

    GMainLoop *mainloop = g_main_loop_new (NULL, FALSE);
    int exit_code = EXIT_SUCCESS;
    TpDBusDaemon *bus_daemon;
    GError *error = NULL;

    bus_daemon = tp_dbus_daemon_dup (&error);

    if (bus_daemon == NULL)
    {
        g_warning ("%s", error->message);
        g_error_free (error);
        exit_code = EXIT_FAILURE;
        goto out;
    }

    // set for verbosity
    tp_debug_set_flags ("all");

    tp_list_connection_names (bus_daemon, got_connections, mainloop, NULL, NULL);
    tp_list_connection_managers_async (bus_daemon, got_connection_managers, mainloop);

// playground
    TpConnectionManager *phone_manager = tp_connection_manager_new (bus_daemon, "phone_manager", "ofono", &error);
    TpConnectionManagerProtocol phone_protocol;
    phone_protocol.name = "tel";

    tp_connection_manager_activate(phone_manager);
    tp_connection_init_known_interfaces ();
// end playground

    g_main_loop_run (mainloop);

out:
    if (mainloop != NULL)
        g_main_loop_unref (mainloop);

    if (bus_daemon != NULL)
        g_object_unref (bus_daemon);


  return exit_code;
}
