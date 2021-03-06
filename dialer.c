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
#include <signal.h>
#include <threads.h>

#include "ui.h"
#include "at.h"
#include "audio_setup.h"
#include "ring-audio.h"
#include "daemonize.h"

#define MODE_NONE 0
#define MODE_DIAL_PAD 1

int modem_fd;

bool set_alsa;

void sig_handler(int sig_num)
{

    if(sig_num == SIGINT)
    {
        close(modem_fd);
        exit(EXIT_SUCCESS);
    }
    else if (sig_num == SIGUSR1)
    {
        gtk_widget_show(GTK_WIDGET(window));
    }

}

gboolean hide_instead(GtkWidget * widget, char key_pressed)
{
    gtk_widget_hide(GTK_WIDGET(window));
    return TRUE;
}

void callback_button_pressed(GtkWidget * widget, char key_pressed)
{
    char cmd[MAX_BUF_SIZE];
    int res;

    if (key_pressed == 'D')
    {
      snprintf(cmd, MAX_BUF_SIZE, "ATD%s;\r", dial_pad);
      res = write(modem_fd, cmd, strlen(cmd));
      if (res < 0)
      {
          log_message(LOG_FILE,"Error writing to the modem\n");
          return;
      }

      if (set_alsa)
      {
          // save status first ?
          call_audio_setup();
      }
    }

    if (key_pressed == 'H'){
        sprintf(cmd, "ATH\r");
        res = write(modem_fd, cmd, strlen(cmd));
        if (res < 0)
        {
            log_message(LOG_FILE,"Error writing to the modem\n");
            return;
        }
        memset (dial_pad, 0, MAX_BUF_SIZE);
    }

    if (key_pressed == 'A')
    {
        sprintf(cmd, "ATA\r");
        res = write(modem_fd, cmd, strlen(cmd));
        if (res < 0)
        {
            log_message(LOG_FILE,"Error writing to the modem\n");
            return;
        }

        if (set_alsa) // save status first?
            call_audio_setup();
    }

    if ((key_pressed >= '0' && key_pressed <= '9') ||
        key_pressed == '*' ||
        key_pressed == '#' ||
        key_pressed == '+')
    {
        dial_pad[strlen(dial_pad)] = key_pressed;
        dial_pad[strlen(dial_pad)] = 0;
    }

    if (key_pressed == 'B')
    {
        dial_pad[strlen(dial_pad)-1] = 0;
    }

    hildon_entry_set_text((HildonEntry *)display, dial_pad);

#if 0
// play correct DTMF event
    switch(key_pressed)
    {
    case '1':
        // play DTMF...
        break;
        ...
#endif

}


// check:
// https://git.sailfishos.org/mer-core/voicecall/tree/master/plugins/providers/telepathy
// https://git.sailfishos.org/mer-core/voicecall/blob/master/plugins/providers/telepathy/src/telepathyproviderplugin.cpp#L106
// /usr/share/ofono/scripts/enable-modem /usr/share/ofono/scripts/online-modem

// Querying IMEI:
// AT+CGSN

// Query IMSI
// AT+CIMI

// Signal quality
// AT+CSQ

// Query registration
// AT+CREG?

// Query operator
// AT+COPS?

// Query call state (to check who is calling:)
// AT+CLCC

// with this I can see if someone is calling, and drop an ATA...
// Query ME state
// AT+CPAS


int main(int argc, char *argv[])
{
    char modem_path[MAX_MODEM_PATH];
    int mode = MODE_NONE;
    bool daemonize_flag = false;
    set_alsa = false;
    int backend = BACKEND_AT;

    if (argc < 2){
    usage_info:
        fprintf(stderr, "Usage: %s [-h] [-p] [-s] [-d] -m modem_dev\n", argv[0]);
        fprintf(stderr, "Usage example: %s -m /dev/EG25.AT -s -d\n\n", argv[0]);
        fprintf(stderr, "OPTIONS:\n");
        fprintf(stderr, "    -h                      Show this help\n");
        fprintf(stderr, "    -p                      Open Dial Pad\n");
        fprintf(stderr, "    -s                      Set alsa routing option (right now - no option yet!)\n");
        fprintf(stderr, "    -d                      Daemonize\n");
        fprintf(stderr, "    -m <modem AT device>    Modem AT device\n");
        fprintf(stderr, "    -b <at, ofono>          Choose between AT and ofono backends (ofono not implemented yet!)\n");
        return EXIT_SUCCESS;
    }
    int opt;
    while ((opt = getopt(argc, argv, "hpm:sdb:")) != -1){
        switch (opt){
        case 'h':
            goto usage_info;
            break;
        case 'p':
            mode = MODE_DIAL_PAD;
            break;
        case 'm':
            strncpy (modem_path, optarg, MAX_MODEM_PATH);
            break;
        case 'b':
            // TODO: set backend
            break;
        case 's':
            set_alsa = true;
            break;
        case 'd':
            daemonize_flag = true;
            break;
        default:
            fprintf(stderr, "Wrong command line.\n");
            goto usage_info;
        }
    }

    if (daemonize_flag == true)
        daemonize();


    if (!gtk_init_check (&argc, &argv)) {
        log_message(LOG_FILE,"Display cannot be initialized, wait for the curses interface :-P\n");
        exit (EXIT_FAILURE);
    }

    signal(SIGINT, sig_handler);
    signal(SIGUSR1, sig_handler);

    /* Create the hildon program and setup the title */
    program = HILDON_PROGRAM(hildon_program_get_instance());
    g_set_application_name("Rhizomatica's Dialer");

    /* Create HildonWindow and set it to HildonProgram */
    window = HILDON_WINDOW(hildon_window_new());
    hildon_program_add_window(program, window);

    hildon_gtk_window_set_portrait_flags(GTK_WINDOW(window), HILDON_PORTRAIT_MODE_REQUEST); // or HILDON_PORTRAIT_MODE_SUPPORT  ?

    // TODO: Use Hildon widgets!
    // http://maemo.org/api_refs/5.0/5.0-final/hildon/
    /* Create buttons and add it to main view */
    vbox = gtk_vbox_new(TRUE, 5);
    hbox0 = gtk_table_new(1, 3, FALSE);
    hbox1 = gtk_hbox_new(TRUE, 5);
    hbox2 = gtk_hbox_new(TRUE, 5);
    hbox3 = gtk_hbox_new(TRUE, 5);
    hbox4 = gtk_hbox_new(TRUE, 5);
    hbox5 = gtk_hbox_new(TRUE, 5);

    display = hildon_entry_new (HILDON_SIZE_AUTO);
    gtk_entry_set_alignment (GTK_ENTRY(display), 0.5);
    gtk_editable_set_editable (GTK_EDITABLE (display), TRUE); // may be this should be false?

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
    buttonAnswer = gtk_button_new_with_label("Answer");
    buttonBack = hildon_gtk_button_new(HILDON_SIZE_THUMB_HEIGHT);
    gtk_button_set_label (GTK_BUTTON(buttonBack),"<--");

    buttonPlus = hildon_gtk_button_new(HILDON_SIZE_AUTO); // was HILDON_SIZE_FINGER_HEIGHT
    gtk_button_set_label (GTK_BUTTON(buttonPlus),"+");


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
    g_signal_connect(G_OBJECT(buttonAnswer), "clicked", G_CALLBACK(callback_button_pressed), (void *) 'A');
    g_signal_connect(G_OBJECT(buttonBack), "clicked", G_CALLBACK(callback_button_pressed), (void *) 'B');
    g_signal_connect(G_OBJECT(buttonPlus), "clicked", G_CALLBACK(callback_button_pressed), (void *) '+');


    gtk_table_attach(GTK_TABLE(hbox0), display, 0, 1, 0, 1, GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 5, 5);
    gtk_table_attach(GTK_TABLE(hbox0), buttonBack, 1, 2, 0, 1, GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 5, 5);

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
    gtk_container_add(GTK_CONTAINER(hbox5), buttonAnswer);
    gtk_container_add(GTK_CONTAINER(hbox5), buttonPlus);

    gtk_container_add(GTK_CONTAINER(vbox), hbox0);
    gtk_container_add(GTK_CONTAINER(vbox), hbox1);
    gtk_container_add(GTK_CONTAINER(vbox), hbox2);
    gtk_container_add(GTK_CONTAINER(vbox), hbox3);
    gtk_container_add(GTK_CONTAINER(vbox), hbox4);
    gtk_container_add(GTK_CONTAINER(vbox), hbox5);

    /* Add VBox to Window */
    gtk_container_add(GTK_CONTAINER(window), vbox);

    /* Connect signal to X in the upper corner */
    //    g_signal_connect(G_OBJECT(window), "delete_event", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(G_OBJECT(window), "delete-event", G_CALLBACK(hide_instead), NULL);

    if (backend == BACKEND_AT)
    {
        log_message(LOG_FILE,"Starting AT backend\n");
        // Modem initialization
        modem_fd = open_serial_port(modem_path);

        if (modem_fd == -1)
        {
            log_message(LOG_FILE, "Could not open modem\n");
            return EXIT_FAILURE;
        }

        // make this a parameter?
        set_fixed_baudrate("115200", modem_fd);

        bool at_res = run_at_backend(modem_fd);
        if (at_res == false)
        {
            log_message(LOG_FILE, "AT Error\n");
            return EXIT_FAILURE;
        }
    }

    log_message(LOG_FILE,"aqui 4444\n");

    /* Begin the main application */
    gtk_widget_show_all(GTK_WIDGET(window));

    if (mode != MODE_DIAL_PAD)
        gtk_widget_hide(GTK_WIDGET(window));

    gtk_main();

    close (modem_fd);
    return EXIT_SUCCESS;
}
