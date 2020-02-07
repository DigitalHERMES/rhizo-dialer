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

#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

// Hildon stuff
#include <hildon/hildon-banner.h>
#include <hildon/hildon-program.h>
#include <gtk/gtk.h>

// Telepathy stuff
#include <telepathy-glib/telepathy-glib.h>
#include <telepathy-glib/telepathy-glib-dbus.h>

// ms
#define MAX_PHONE_SIZE 512

#define MODE_NONE 0
#define MODE_DIAL 1
#define MODE_ANSWER 2
#define MODE_HANGUP 3
#define MODE_DIAL_PAD 4

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

GtkWidget *display;
char dial_pad[MAX_PHONE_SIZE];

void callback_button_pressed(GtkWidget * widget, char key_pressed)
{
    gint result;

    fprintf(stderr, "Pressed %c\n", key_pressed);

    if (key_pressed == 'H')
        memset (dial_pad, 0, MAX_PHONE_SIZE);

    if ((key_pressed >= '0' && key_pressed <= '9') ||
        key_pressed == '*' ||
        key_pressed == '#')
    {
        dial_pad[strlen(dial_pad)] = key_pressed;
        dial_pad[strlen(dial_pad)] = 0;
    }

    gtk_entry_set_text (GTK_ENTRY(display), dial_pad);

#if 0
// play correct DTMF event
    switch(key_pressed)
    {
    case '1':
        TP_DTMF_EVENT_DIGIT_1;
        break;
        ...
TP_DTMF_EVENT_DIGIT_2
TP_DTMF_EVENT_DIGIT_3
TP_DTMF_EVENT_DIGIT_4
TP_DTMF_EVENT_DIGIT_5
TP_DTMF_EVENT_DIGIT_6
TP_DTMF_EVENT_DIGIT_7
TP_DTMF_EVENT_DIGIT_8
TP_DTMF_EVENT_DIGIT_9
TP_DTMF_EVENT_HASH
TP_DTMF_EVENT_DIGIT_0
TP_DTMF_EVENT_ASTERISK
#endif

    /* Show the dialog */
    //gtk_widget_show_all(GTK_WIDGET(dialog));
    /* Wait for user to select OK or CANCEL */
    //result = gtk_dialog_run(GTK_DIALOG(dialog));
    /* Close the dialog */
    // gtk_widget_destroy(GTK_WIDGET(dialog));

}


// check:
// https://git.sailfishos.org/mer-core/voicecall/tree/master/plugins/providers/telepathy
// https://git.sailfishos.org/mer-core/voicecall/blob/master/plugins/providers/telepathy/src/telepathyproviderplugin.cpp#L106

int main(int argc, char *argv[])
{
    /* Hildon/GTK stuff */
    HildonProgram *program;
    HildonWindow *window;

    char msisdn[MAX_PHONE_SIZE];
    int mode = MODE_NONE;

    if (argc < 2){
    usage_info:
        fprintf(stderr, "Usage: %s [-d <phone_number>] [-a] [-h] [-p]\n", argv[0]);
        fprintf(stderr, "Usage example: %s -d 99991234\n\n", argv[0]);
        fprintf(stderr, "OPTIONS:\n");
        fprintf(stderr, "    -d <phone_number>       Dial to a given phone number\n");
        fprintf(stderr, "    -a                      Answer an incoming call\n");
        fprintf(stderr, "    -h                      Hangup current call\n");
        fprintf(stderr, "    -p                      Open Dial Pad\n");
        return EXIT_SUCCESS;
    }
    int opt;
    while ((opt = getopt(argc, argv, "d:ahp")) != -1){
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
        case 'p':
            mode = MODE_DIAL_PAD;
            break;
        default:
            fprintf(stderr, "Wrong command line.\n");
            goto usage_info;
        }
    }

    if (!gtk_init_check (&argc, &argv)) {
        fprintf(stderr, "Display cannot be initialized, wait for the curses interface :-P\n");
        exit (-1);
    }


    /* Create the hildon program and setup the title */
    program = HILDON_PROGRAM(hildon_program_get_instance());
    g_set_application_name("Rhizomatica Dialer");

    /* Create HildonWindow and set it to HildonProgram */
    window = HILDON_WINDOW(hildon_window_new());
    hildon_program_add_window(program, window);

    // TODO: Use Hildon widgets!
    // http://maemo.org/api_refs/5.0/5.0-final/hildon/
    /* Create buttons and add it to main view */
    GtkWidget *vbox = gtk_vbox_new(TRUE, 5);
    GtkWidget *hbox1 = gtk_hbox_new(TRUE, 5);
    GtkWidget *hbox2 = gtk_hbox_new(TRUE, 5);
    GtkWidget *hbox3 = gtk_hbox_new(TRUE, 5);
    GtkWidget *hbox4 = gtk_hbox_new(TRUE, 5);
    GtkWidget *hbox5 = gtk_hbox_new(TRUE, 5);

    display = gtk_entry_new();
    gtk_entry_set_alignment (GTK_ENTRY(display), 0.5);
    gtk_editable_set_editable (GTK_EDITABLE (display), FALSE);

    GtkWidget *button9 = gtk_button_new_with_label("9");
    GtkWidget *button8 = gtk_button_new_with_label("8");
    GtkWidget *button7 = gtk_button_new_with_label("7");
    GtkWidget *button6 = gtk_button_new_with_label("6");
    GtkWidget *button5 = gtk_button_new_with_label("5");
    GtkWidget *button4 = gtk_button_new_with_label("4");
    GtkWidget *button3 = gtk_button_new_with_label("3");
    GtkWidget *button2 = gtk_button_new_with_label("2");
    GtkWidget *button1 = gtk_button_new_with_label("1");
    GtkWidget *button0 = gtk_button_new_with_label("0");
    GtkWidget *buttonStar = gtk_button_new_with_label("*");
    GtkWidget *buttonHash = gtk_button_new_with_label("#");
    GtkWidget *buttonDial = gtk_button_new_with_label("Call");
    GtkWidget *buttonHangup = gtk_button_new_with_label("Hangup");

    g_signal_connect(G_OBJECT(button9), "clicked", G_CALLBACK(callback_button_pressed), (void *) '9');
    g_signal_connect(G_OBJECT(button8), "clicked", G_CALLBACK(callback_button_pressed), (void *) '8');
    g_signal_connect(G_OBJECT(button7), "clicked", G_CALLBACK(callback_button_pressed), (void *) '7');
    g_signal_connect(G_OBJECT(button6), "clicked", G_CALLBACK(callback_button_pressed), (void *) '6');
    g_signal_connect(G_OBJECT(button5), "clicked", G_CALLBACK(callback_button_pressed), (void *) '5');
    g_signal_connect(G_OBJECT(button4), "clicked", G_CALLBACK(callback_button_pressed), (void *) '4');
    g_signal_connect(G_OBJECT(button3), "clicked", G_CALLBACK(callback_button_pressed), (void *) '3');
    g_signal_connect(G_OBJECT(button2), "clicked", G_CALLBACK(callback_button_pressed), (void *) '2');
    g_signal_connect(G_OBJECT(button1), "clicked", G_CALLBACK(callback_button_pressed), (void *) '1');
    g_signal_connect(G_OBJECT(button0), "clicked", G_CALLBACK(callback_button_pressed), (void *) '0');
    g_signal_connect(G_OBJECT(buttonStar), "clicked", G_CALLBACK(callback_button_pressed), (void *) '*');
    g_signal_connect(G_OBJECT(buttonHash), "clicked", G_CALLBACK(callback_button_pressed), (void *) '#');
    g_signal_connect(G_OBJECT(buttonDial), "clicked", G_CALLBACK(callback_button_pressed), (void *) 'D');
    g_signal_connect(G_OBJECT(buttonHangup), "clicked", G_CALLBACK(callback_button_pressed), (void *) 'H');


    gtk_container_add(GTK_CONTAINER(hbox1), button1);
    gtk_container_add(GTK_CONTAINER(hbox1), button2);
    gtk_container_add(GTK_CONTAINER(hbox1), button3);
    gtk_container_add(GTK_CONTAINER(hbox2), button4);
    gtk_container_add(GTK_CONTAINER(hbox2), button5);
    gtk_container_add(GTK_CONTAINER(hbox2), button6);
    gtk_container_add(GTK_CONTAINER(hbox3), button7);
    gtk_container_add(GTK_CONTAINER(hbox3), button8);
    gtk_container_add(GTK_CONTAINER(hbox3), button9);
    gtk_container_add(GTK_CONTAINER(hbox4), buttonStar);
    gtk_container_add(GTK_CONTAINER(hbox4), button0);
    gtk_container_add(GTK_CONTAINER(hbox4), buttonHash);
    gtk_container_add(GTK_CONTAINER(hbox5), buttonDial);
    gtk_container_add(GTK_CONTAINER(hbox5), buttonHangup);

    gtk_container_add(GTK_CONTAINER(vbox), display);
    gtk_container_add(GTK_CONTAINER(vbox), hbox1);
    gtk_container_add(GTK_CONTAINER(vbox), hbox2);
    gtk_container_add(GTK_CONTAINER(vbox), hbox3);
    gtk_container_add(GTK_CONTAINER(vbox), hbox4);
    gtk_container_add(GTK_CONTAINER(vbox), hbox5);

    /* Add VBox to AppView */
    gtk_container_add(GTK_CONTAINER(window), vbox);

    /* Connect signal to X in the upper corner */
    g_signal_connect(G_OBJECT(window), "delete_event",
      G_CALLBACK(gtk_main_quit), NULL);

    // nothing works yet, just playing with telepathy...

    GMainLoop *mainloop = g_main_loop_new (NULL, FALSE);
    int exit_code = EXIT_SUCCESS;
    TpDBusDaemon *bus_daemon;
    TpDTMFEvent DTMFevent;
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

    /* Begin the main application */
    gtk_widget_show_all(GTK_WIDGET(window));
    gtk_main();

out:
    if (mainloop != NULL)
        g_main_loop_unref (mainloop);

    if (bus_daemon != NULL)
        g_object_unref (bus_daemon);


  return exit_code;
}
