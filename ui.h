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
 * @file ui.h
 * @author Rafael Diniz
 * @date 07 Feb 2020
 * @brief Hildon/gtk UI widgets
 *
 * Hildon/gtk UI widgets for the dialer
 *
 */

#ifndef HAVE_UI_H__
#define HAVE_UI_H__

// max msisdn number size
#define MAX_PHONE_SIZE 512
#define MAX_MODEM_PATH 4096
#define MAX_BUF_SIZE 4096

// Hildon stuff
#include <hildon/hildon-banner.h>
#include <hildon/hildon-program.h>
#include <hildon/hildon.h>
#include <gtk/gtk.h>

/* Hildon/GTK stuff */
HildonProgram *program;
HildonWindow *window;

GtkWidget *display;
char dial_pad[MAX_PHONE_SIZE];

/* Create buttons and add it to main view */
GtkWidget *vbox;
GtkWidget *hbox1;
GtkWidget *hbox2;
GtkWidget *hbox3;
GtkWidget *hbox4;
GtkWidget *hbox5;

GtkWidget *button9;
GtkWidget *button8;
GtkWidget *button7;
GtkWidget *button6;
GtkWidget *button5;
GtkWidget *button4;
GtkWidget *button3;
GtkWidget *button2;
GtkWidget *button1;
GtkWidget *button0;
GtkWidget *buttonStar;
GtkWidget *buttonHash;
GtkWidget *buttonDial;
GtkWidget *buttonHangup;

#endif /* HAVE_UI_H__ */
