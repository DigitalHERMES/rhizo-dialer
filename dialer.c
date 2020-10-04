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
#include <stdbool.h>

#include <telepathy-glib/telepathy-glib.h>
#include <telepathy-glib/telepathy-glib-dbus.h>

#include "ui.h"
#include "tp.h"

#define MODE_NONE 0
#define MODE_DIAL 1
#define MODE_ANSWER 2
#define MODE_HANGUP 3
#define MODE_DIAL_PAD 4

FILE *modem;

gint incoming_call_checker (gpointer data)
{
    char cmd[] = "AT+CPAS";
    int res;
    char buf[MAX_BUF_SIZE];
    char *line;

    res = fputs(cmd, modem);
    if (res < 0)
    {
        fprintf(stderr, "Error writing to the modem\n");
        return false;
    }

    line = fgets(buf, MAX_BUF_SIZE, modem);

    if (line == NULL)
    {
        fprintf(stderr, "output is NULL\n");
    }
    else
    {
        fprintf(stderr, "output: %s\n", line);
    }
    return true;

}

void callback_button_pressed(GtkWidget * widget, char key_pressed)
{
    gint result;
    char cmd[MAX_BUF_SIZE];
    int res;

    fprintf(stderr, "Pressed %c\n", key_pressed);

    if (key_pressed == 'D')
    {
        sprintf(cmd, "ATD%s;\n", dial_pad);
        res = fputs(cmd, modem);
    }

    if (key_pressed == 'H'){
        sprintf(cmd, "ATH\n");
        res = fputs(cmd, modem);
        memset (dial_pad, 0, MAX_PHONE_SIZE);
    }

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
// /usr/share/ofono/scripts/enable-modem /usr/share/ofono/scripts/online-modem

// Querying IMEI:
//AT+CGSN

// Query IMSI
//AT+CIMI

// Signal quality
// AT+CSQ

// Query registration
// AT+CREG?

// Query operator
// AT+COPS?

// Query call state
// AT+CLCC

// with this I can see if someone is calling, and drop an ATA...
// Query ME state
// AT+CPAS



int main(int argc, char *argv[])
{
    char modem_path[MAX_MODEM_PATH];
    char msisdn[MAX_PHONE_SIZE];
    int mode = MODE_NONE;
    bool set_alsa = false;

    if (argc < 2){
    usage_info:
        fprintf(stderr, "Usage: %s [-d <phone_number>] [-a] [-h] [-p]\n", argv[0]);
        fprintf(stderr, "Usage example: %s -d 99991234\n\n", argv[0]);
        fprintf(stderr, "OPTIONS:\n");
        fprintf(stderr, "    -d <phone_number>       Dial to a given phone number\n");
        fprintf(stderr, "    -a                      Answer an incoming call\n");
        fprintf(stderr, "    -h                      Hangup current call\n");
        fprintf(stderr, "    -p                      Open Dial Pad\n");
        fprintf(stderr, "    -m <modem AT device>    Modem AT device\n");
        fprintf(stderr, "    -s                      Set alsa routing option (right now - no option yet!)\n");
        return EXIT_SUCCESS;
    }
    int opt;
    while ((opt = getopt(argc, argv, "d:ahpm:s")) != -1){
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
        case 'm':
            strncpy (modem_path, optarg, MAX_MODEM_PATH);
            break;
        case 's':
	    set_alsa = true;
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

    hildon_gtk_window_set_portrait_flags(GTK_WINDOW(window), HILDON_PORTRAIT_MODE_SUPPORT); // or HILDON_PORTRAIT_MODE_REQUEST  ?

    // TODO: Use Hildon widgets!
    // http://maemo.org/api_refs/5.0/5.0-final/hildon/
    /* Create buttons and add it to main view */
    vbox = gtk_vbox_new(TRUE, 5);
    hbox1 = gtk_hbox_new(TRUE, 5);
    hbox2 = gtk_hbox_new(TRUE, 5);
    hbox3 = gtk_hbox_new(TRUE, 5);
    hbox4 = gtk_hbox_new(TRUE, 5);
    hbox5 = gtk_hbox_new(TRUE, 5);

    display = gtk_entry_new();
    gtk_entry_set_alignment (GTK_ENTRY(display), 0.5);
    gtk_editable_set_editable (GTK_EDITABLE (display), FALSE);

    button9 = gtk_button_new_with_label("9");
//    button9 = hildon_gtk_button_new(HILDON_SIZE_AUTO);
// hildon_gtk_button_new (HILDON_SIZE_FINGER_HEIGHT | HILDON_SIZE_AUTO_WIDTH);
// gtk_button_set_label (GTK_BUTTON (button9), "9"); // ?
    button8 = gtk_button_new_with_label("8");
    button7 = gtk_button_new_with_label("7");
    button6 = gtk_button_new_with_label("6");
    button5 = gtk_button_new_with_label("5");
    button4 = gtk_button_new_with_label("4");
    button3 = gtk_button_new_with_label("3");
    button2 = gtk_button_new_with_label("2");
    button1 = gtk_button_new_with_label("1");
    button0 = gtk_button_new_with_label("0");
    buttonStar = gtk_button_new_with_label("*");
    buttonHash = gtk_button_new_with_label("#");
    buttonDial = gtk_button_new_with_label("Call");
    buttonHangup = gtk_button_new_with_label("Hangup");

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

    /* Add VBox to Window */
    gtk_container_add(GTK_CONTAINER(window), vbox);

    /* Connect signal to X in the upper corner */
    g_signal_connect(G_OBJECT(window), "delete_event",
      G_CALLBACK(gtk_main_quit), NULL);

    //    g_timeout_add(1000, incoming_call_checker, NULL);

    modem = fopen(modem_path, "r+");

    if (modem == NULL)
    {
        fprintf(stderr, "Could not open modem\n");
    }
    
    // Lets dial?
    //https://megous.com/dl/tmp/modem.txt

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

    fclose(modem);

  return exit_code;
}
