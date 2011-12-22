//  
//  Copyright (C) 2009 Canonical Ltd.
// 
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
// 
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
// 
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
// 

#include <stdio.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
/*
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
*/
#include <glib.h>
#include <gio/gio.h>
#include <sys/types.h>
#include <unistd.h>
#include <glibtop.h>
#include "bamf.h"

void test_application_create_suite (GDBusConnection *connection);
void test_matcher_create_suite (void);
void test_view_create_suite (GDBusConnection *connection);
void test_window_create_suite (void);

static int result = 0;

static void
on_bus_acquired (GDBusConnection *connection, const gchar *name, gpointer data)
{
  GMainLoop *loop = data;

  test_matcher_create_suite ();
  test_view_create_suite (connection);
  test_window_create_suite ();
  test_application_create_suite (connection);
  result = g_test_run ();

  g_main_loop_quit (loop);
}

static void
on_name_lost (GDBusConnection *connection, const gchar *name, gpointer data)
{
  GMainLoop *loop = data;
  g_main_loop_quit (loop);
}

gint
main (gint argc, gchar *argv[])
{
  GMainLoop *loop;

  g_type_init ();
  g_test_init (&argc, &argv, NULL);

  gtk_init (&argc, &argv);
  glibtop_init ();

  loop = g_main_loop_new (NULL, FALSE);

  g_bus_own_name (G_BUS_TYPE_SYSTEM,
                  BAMF_DBUS_SERVICE".test",
                  G_BUS_NAME_OWNER_FLAGS_NONE,
                  on_bus_acquired,
                  NULL,
                  on_name_lost,
                  loop,
                  NULL);

  g_main_loop_run (loop);

  return result;
}
