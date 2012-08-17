/*
 * Copyright 2010 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of either or both of the following licenses:
 *
 * 1) the GNU Lesser General Public License version 3, as published by the
 * Free Software Foundation; and/or
 * 2) the GNU Lesser General Public License version 2.1, as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the applicable version of the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of both the GNU Lesser General Public
 * License version 3 and version 2.1 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Jason Smith <jason.smith@canonical.com>
 *              Neil Jagdish Patel <neil.patel@canonical.com>
 *
 */
/**
 * SECTION:bamf-application
 * @short_description: The base class for all applications
 *
 * #BamfApplication is the base class that all applications need to derive from.
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include "bamf-application.h"
#include "bamf-window.h"
#include "bamf-factory.h"
#include "bamf-application-private.h"
#include "bamf-view-private.h"

#include <gio/gdesktopappinfo.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <string.h>

G_DEFINE_TYPE (BamfApplication, bamf_application, BAMF_TYPE_VIEW);

#define BAMF_APPLICATION_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BAMF_TYPE_APPLICATION, BamfApplicationPrivate))

enum
{
  WINDOW_ADDED,
  WINDOW_REMOVED,

  LAST_SIGNAL,
};

static guint application_signals[LAST_SIGNAL] = { 0 };

struct _BamfApplicationPrivate
{
  DBusGConnection *connection;
  DBusGProxy      *proxy;
  gchar           *application_type;
  gchar           *desktop_file;
  GList           *cached_xids;
  gchar           **cached_mimes;
  gboolean        mimes_initialized;
  int             show_stubs;
};

gchar **
bamf_application_get_supported_mime_types (BamfApplication *application)
{
  GError *error = NULL;
  gchar **mimes = NULL;

  if (application->priv->mimes_initialized)
    return g_strdupv (application->priv->cached_mimes);

  if (!_bamf_view_remote_ready (BAMF_VIEW (application)))
    return NULL;

  if (!dbus_g_proxy_call (application->priv->proxy,
                          "SupportedMimeTypes",
                          &error,
                          G_TYPE_INVALID,
                          G_TYPE_STRV, &mimes,
                          G_TYPE_INVALID))
    {
      g_warning ("Failed to fetch mimes: %s", error->message);
      g_error_free (error);

      return NULL;
    }
  application->priv->mimes_initialized = TRUE;
  application->priv->cached_mimes = g_strdupv (mimes);

  return mimes;
}

/**
 * bamf_application_get_desktop_file:
 * @application: a #BamfApplication
 *
 * Used to fetch the path to the .desktop file associated with the passed application. If
 * none exists, the result is NULL.
 *
 * Returns: A string representing the path to the desktop file.
 */
const gchar *
bamf_application_get_desktop_file (BamfApplication *application)
{
  BamfApplicationPrivate *priv;
  gchar *file;
  GError *error = NULL;

  g_return_val_if_fail (BAMF_IS_APPLICATION (application), FALSE);
  priv = application->priv;

  if (priv->desktop_file)
    return priv->desktop_file;

  if (!_bamf_view_remote_ready (BAMF_VIEW (application)))
    return NULL;

  if (!dbus_g_proxy_call (priv->proxy,
                          "DesktopFile",
                          &error,
                          G_TYPE_INVALID,
                          G_TYPE_STRING, &file,
                          G_TYPE_INVALID))
    {
      g_warning ("Failed to fetch path: %s", error->message);
      g_error_free (error);

      return NULL;
    }

  if (file && file[0] == '\0')
    {
      g_free (file);
      file = NULL;
    }

  priv->desktop_file = file;
  return file;
}

gboolean
bamf_application_get_application_menu (BamfApplication *application,
                                      gchar **name,
                                      gchar **object_path)
{
  BamfApplicationPrivate *priv;
  GError *error = NULL;
  
  g_return_val_if_fail (BAMF_IS_APPLICATION (application), FALSE);
  
  priv = application->priv;
  
  if (!_bamf_view_remote_ready (BAMF_VIEW (application)))
    return FALSE;

  if (!dbus_g_proxy_call (priv->proxy,
                          "ApplicationMenu",
                          &error,
                          G_TYPE_INVALID,
                          G_TYPE_STRING, name,
                         G_TYPE_STRING, object_path,
                          G_TYPE_INVALID))
    {
      g_warning ("Failed to fetch application menu path: %s", error->message);
      g_error_free (error);
      
      return FALSE;
    }

  return TRUE;
}

/**
 * bamf_application_get_applicaton_type:
 * @application: a #BamfApplication
 *
 * Used to determine what type of application a .desktop file represents. Current values are:
 *  "system" : A normal application, like firefox or evolution
 *  "web"    : A web application, like facebook or twitter
 *
 * Returns: A string
 */
const gchar *
bamf_application_get_application_type (BamfApplication *application)
{
  BamfApplicationPrivate *priv;
  gchar *type;
  GError *error = NULL;

  g_return_val_if_fail (BAMF_IS_APPLICATION (application), FALSE);
  priv = application->priv;

  if (priv->application_type)
    return priv->application_type;

  if (!_bamf_view_remote_ready (BAMF_VIEW (application)))
    return NULL;

  if (!dbus_g_proxy_call (priv->proxy,
                          "ApplicationType",
                          &error,
                          G_TYPE_INVALID,
                          G_TYPE_STRING, &type,
                          G_TYPE_INVALID))
    {
      g_warning ("Failed to fetch path: %s", error->message);
      g_error_free (error);

      return NULL;
    }

  priv->application_type = type;
  return type;
}

/**
 * bamf_application_get_xids:
 * @application: a #BamfApplication
 *
 * Used to fetch all #BamfWindow's xids associated with the passed #BamfApplication.
 *
 * Returns: (element-type guint) (transfer full): An array of xids.
 */
GArray *
bamf_application_get_xids (BamfApplication *application)
{
  BamfApplicationPrivate *priv;
  GArray *xids;
  GError *error = NULL;

  g_return_val_if_fail (BAMF_IS_APPLICATION (application), FALSE);
  priv = application->priv;

  if (!_bamf_view_remote_ready (BAMF_VIEW (application)))
    return NULL;

  if (!dbus_g_proxy_call (priv->proxy,
                          "Xids",
                          &error,
                          G_TYPE_INVALID,
                          DBUS_TYPE_G_UINT_ARRAY, &xids,
                          G_TYPE_INVALID))
    {
      g_warning ("Failed to fetch xids: %s", error->message);
      g_error_free (error);

      return NULL;
    }

  return xids;
}

/**
 * bamf_application_get_windows:
 * @application: a #BamfApplication
 *
 * Used to fetch all #BamfWindow's associated with the passed #BamfApplication.
 *
 * Returns: (element-type Bamf.Window) (transfer container): A list of #BamfWindow's.
 */
GList *
bamf_application_get_windows (BamfApplication *application)
{
  GList *children, *l;
  GList *windows = NULL;
  BamfView *view;

  g_return_val_if_fail (BAMF_IS_APPLICATION (application), NULL);

  children = bamf_view_get_children (BAMF_VIEW (application));

  for (l = children; l; l = l->next)
    {
      view = l->data;

      if (BAMF_IS_WINDOW (view));
        {
          windows = g_list_prepend (windows, view);
        }
    }

  g_list_free (children);
  return windows;
}

/**
 * bamf_application_get_show_menu_stubs:
 * @application: a #BamfApplication
 *
 * Used to discover whether the application wants menu stubs shown.
 *
 * Returns: Whether the stubs should be shown.
 */
gboolean
bamf_application_get_show_menu_stubs (BamfApplication * application)
{
  BamfApplicationPrivate *priv;
  GError *error = NULL;
  gboolean result;

  g_return_val_if_fail (BAMF_IS_APPLICATION (application), TRUE);

  priv = application->priv;

  if (!_bamf_view_remote_ready (BAMF_VIEW (application)))
    return TRUE;

  if (priv->show_stubs == -1)
    {
      if (!dbus_g_proxy_call (application->priv->proxy,
                              "ShowStubs",
                              &error,
                              G_TYPE_INVALID,
                              G_TYPE_BOOLEAN, &result,
                              G_TYPE_INVALID))
        {
          g_warning ("Failed to fetch show_stubs: %s", error->message);
          g_error_free (error);

          return TRUE;
        }

      if (result)
        priv->show_stubs = 1;
      else
        priv->show_stubs = 0;
    }

  return priv->show_stubs;
}

static BamfClickBehavior
bamf_application_get_click_suggestion (BamfView *view)
{
  if (!bamf_view_is_running (view))
    return BAMF_CLICK_BEHAVIOR_OPEN;
  return 0;
}

static void
bamf_application_on_supported_mime_types_changed (DBusGProxy *proxy, const gchar *const *mimes, BamfApplication *self)
{
  if (self->priv->cached_mimes)
    g_strfreev (self->priv->cached_mimes);

  self->priv->cached_mimes = g_strdupv ((gchar**)mimes);
  self->priv->mimes_initialized = TRUE;
}

static void
bamf_application_on_window_added (DBusGProxy *proxy, char *path, BamfApplication *self)
{
  BamfView *view;
  BamfFactory *factory;

  g_return_if_fail (BAMF_IS_APPLICATION (self));

  factory = _bamf_factory_get_default ();
  view = _bamf_factory_view_for_path_type (factory, path, BAMF_FACTORY_WINDOW);

  if (BAMF_IS_WINDOW (view))
    {
      guint32 xid = bamf_window_get_xid (BAMF_WINDOW (view));

      if (!g_list_find (self->priv->cached_xids, GUINT_TO_POINTER (xid)))
      {
        self->priv->cached_xids = g_list_prepend (self->priv->cached_xids, GUINT_TO_POINTER (xid));
      }

      g_signal_emit (G_OBJECT (self), application_signals[WINDOW_ADDED], 0, view);
    }
}

static void
bamf_application_on_window_removed (DBusGProxy *proxy, char *path, BamfApplication *self)
{
  BamfView *view;
  BamfFactory *factory;

  g_return_if_fail (BAMF_IS_APPLICATION (self));

  factory = _bamf_factory_get_default ();
  view = _bamf_factory_view_for_path_type (factory, path, BAMF_FACTORY_WINDOW);

  if (BAMF_IS_WINDOW (view))
    {
      guint32 xid = bamf_window_get_xid (BAMF_WINDOW (view));
      self->priv->cached_xids = g_list_remove (self->priv->cached_xids, GUINT_TO_POINTER (xid));

      g_signal_emit (G_OBJECT (self), application_signals[WINDOW_REMOVED], 0, view);
    }
}

BamfView *
bamf_application_get_focus_child (BamfApplication *application)
{
  BamfApplicationPrivate *priv;
  BamfView *ret;
  gchar *path;
  GError *error = NULL;

  g_return_val_if_fail (BAMF_IS_APPLICATION (application), FALSE);
  priv = application->priv;
  
  if (!_bamf_view_remote_ready (BAMF_VIEW (application)))
    return NULL;

  if (!dbus_g_proxy_call (priv->proxy,
                          "FocusChild",
                          &error,
                          G_TYPE_INVALID,
                          G_TYPE_STRING, &path,
                          G_TYPE_INVALID))
    {
      g_warning ("Failed to fetch focus xids: %s", error->message);
      g_error_free (error);
      
      return NULL;
    }

  ret = _bamf_factory_view_for_path (_bamf_factory_get_default (), path);
  
  g_free (path);
  
  return ret;
}

GList *
_bamf_application_get_cached_xids (BamfApplication *self)
{
  g_return_val_if_fail (BAMF_IS_APPLICATION (self), NULL);

  return self->priv->cached_xids;
}

static void
bamf_application_unset_proxy (BamfApplication* self)
{
  BamfApplicationPrivate *priv;

  g_return_if_fail (BAMF_IS_APPLICATION (self));
  priv = self->priv;

  if (!priv->proxy)
    return;

  dbus_g_proxy_disconnect_signal (priv->proxy,
                                 "WindowAdded",
                                 (GCallback) bamf_application_on_window_added,
                                 self);

  dbus_g_proxy_disconnect_signal (priv->proxy,
                                 "WindowRemoved",
                                 (GCallback) bamf_application_on_window_removed,
                                 self);
  dbus_g_proxy_disconnect_signal (priv->proxy,
				  "SupportedMimeTypesChanged",
				  (GCallback) bamf_application_on_supported_mime_types_changed,
				  self);

  g_object_unref (priv->proxy);
  priv->proxy = NULL;
}

static void
bamf_application_dispose (GObject *object)
{
  BamfApplication *self;
  BamfApplicationPrivate *priv;

  self = BAMF_APPLICATION (object);
  priv = self->priv;

  if (priv->application_type)
    {
      g_free (priv->application_type);
      priv->application_type = NULL;
    }

  if (priv->desktop_file)
    {
      g_free (priv->desktop_file);
      priv->desktop_file = NULL;
    }

  if (priv->cached_xids)
    {
      g_list_free (priv->cached_xids);
      priv->cached_xids = NULL;
    }
  
  bamf_application_unset_proxy (self);

  if (G_OBJECT_CLASS (bamf_application_parent_class)->dispose)
    G_OBJECT_CLASS (bamf_application_parent_class)->dispose (object);
}

static void
bamf_application_set_path (BamfView *view, const char *path)
{
  BamfApplication *self;
  BamfApplicationPrivate *priv;

  self = BAMF_APPLICATION (view);
  priv = self->priv;

  bamf_application_unset_proxy (self);

  priv->proxy = dbus_g_proxy_new_for_name (priv->connection,
                                           "org.ayatana.bamf",
                                           path,
                                           "org.ayatana.bamf.application");
  if (priv->proxy == NULL)
    {
      g_critical ("Unable to get org.ayatana.bamf.application application");
      return;
    }

  dbus_g_proxy_add_signal (priv->proxy,
                           "WindowAdded",
                           G_TYPE_STRING,
                           G_TYPE_INVALID);

  dbus_g_proxy_add_signal (priv->proxy,
                           "WindowRemoved",
                           G_TYPE_STRING,
                           G_TYPE_INVALID);

  dbus_g_proxy_add_signal (priv->proxy,
                           "SupportedMimeTypesChanged",
                           G_TYPE_STRV,
                           G_TYPE_INVALID);

  dbus_g_proxy_connect_signal (priv->proxy,
                               "WindowAdded",
                               (GCallback) bamf_application_on_window_added,
                               self,
                               NULL);

  dbus_g_proxy_connect_signal (priv->proxy,
                               "WindowRemoved",
                               (GCallback) bamf_application_on_window_removed,
                               self,
                               NULL);

  dbus_g_proxy_connect_signal (priv->proxy,
                               "SupportedMimeTypesChanged",
                               (GCallback) bamf_application_on_supported_mime_types_changed,
                               self,
                               NULL);

  GList *children, *l;
  children = bamf_view_get_children (view);

  if (priv->cached_xids)
    {
      g_list_free (priv->cached_xids);
      priv->cached_xids = NULL;
    }

  for (l = children; l; l = l->next)
    {
      if (!BAMF_IS_WINDOW (l->data))
        continue;

      guint32 xid = bamf_window_get_xid (BAMF_WINDOW (l->data));
      priv->cached_xids = g_list_prepend (priv->cached_xids, GUINT_TO_POINTER (xid));
    }

  g_list_free (children);
}

static void
bamf_application_load_data_from_file (BamfApplication *self)
{
  GDesktopAppInfo *desktop_info;
  GIcon *gicon;
  char *name;
  char *icon;
  GKeyFile * keyfile;
  GError *error;

  keyfile = g_key_file_new();
  if (!g_key_file_load_from_file(keyfile, self->priv->desktop_file, G_KEY_FILE_NONE, NULL)) {
      g_key_file_free(keyfile);
    return;
  }

  desktop_info = g_desktop_app_info_new_from_keyfile (keyfile);

  if (!desktop_info)
    return;

  name = g_strdup (g_app_info_get_name (G_APP_INFO (desktop_info)));

  if (g_key_file_has_key(keyfile, G_KEY_FILE_DESKTOP_GROUP, "X-GNOME-FullName", NULL))
		{
		  /* Grab the better name if its available */
		  gchar *fullname = NULL;
		  error = NULL;
		  fullname = g_key_file_get_locale_string (keyfile, G_KEY_FILE_DESKTOP_GROUP, "X-GNOME-FullName", NULL, &error);
		  if (error != NULL)
		    {
		      g_error_free (error);
		      if (fullname)
		        g_free (fullname);
		    }
		  else
		    {
		      g_free (name);
		      name = fullname;
		    }
		}

  _bamf_view_set_name (BAMF_VIEW (self), name);

  gicon = g_app_info_get_icon (G_APP_INFO (desktop_info));
  icon = g_icon_to_string (gicon);

  if (!icon)
    icon = g_strdup ("application-default-icon");

  _bamf_view_set_icon (BAMF_VIEW (self), icon);
  g_free (icon);
  g_key_file_free (keyfile);
  g_free (name);
  g_object_unref (desktop_info);
}

static void
bamf_application_class_init (BamfApplicationClass *klass)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);
  BamfViewClass *view_class = BAMF_VIEW_CLASS (klass);

  obj_class->dispose     = bamf_application_dispose;
  view_class->set_path   = bamf_application_set_path;
  view_class->click_behavior = bamf_application_get_click_suggestion;

  g_type_class_add_private (obj_class, sizeof (BamfApplicationPrivate));

  application_signals [WINDOW_ADDED] =
  	g_signal_new ("window-added",
  	              G_OBJECT_CLASS_TYPE (klass),
  	              0,
  	              0, NULL, NULL,
  	              g_cclosure_marshal_VOID__OBJECT,
  	              G_TYPE_NONE, 1,
  	              BAMF_TYPE_VIEW);

  application_signals [WINDOW_REMOVED] =
  	g_signal_new ("window-removed",
  	              G_OBJECT_CLASS_TYPE (klass),
  	              0,
  	              0, NULL, NULL,
  	              g_cclosure_marshal_VOID__OBJECT,
  	              G_TYPE_NONE, 1,
  	              BAMF_TYPE_VIEW);
}


static void
bamf_application_init (BamfApplication *self)
{
  BamfApplicationPrivate *priv;
  GError           *error = NULL;

  priv = self->priv = BAMF_APPLICATION_GET_PRIVATE (self);
  priv->show_stubs = -1;

  priv->connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
  if (priv->connection == NULL)
    {
      g_critical ("Failed to open connection to bus: %s",
               error != NULL ? error->message : "Unknown");
      if (error)
        g_error_free (error);
      return;
    }
}

BamfApplication *
bamf_application_new (const char * path)
{
  BamfApplication *self;
  self = g_object_new (BAMF_TYPE_APPLICATION, NULL);

  _bamf_view_set_path (BAMF_VIEW (self), path);

  return self;
}

BamfApplication *
bamf_application_new_favorite (const char * favorite_path)
{
  BamfApplication *self;
  GKeyFile        *desktop_keyfile;
  GKeyFileFlags    flags;
  gchar           *type;
  gboolean         supported = FALSE;

  // check that we support this kind of desktop file
  desktop_keyfile = g_key_file_new ();
  flags = G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS;
  if (g_key_file_load_from_file (desktop_keyfile, favorite_path, flags, NULL))
    {
      type = g_key_file_get_string (desktop_keyfile, "Desktop Entry", "Type", NULL);
      if (g_strcmp0 (type, "Application") == 0)
        supported = TRUE;

      g_key_file_free (desktop_keyfile);
      g_free (type);
    }
  if (!supported)
    return NULL;

  self = g_object_new (BAMF_TYPE_APPLICATION, NULL);

  self->priv->desktop_file = g_strdup (favorite_path);
  bamf_application_load_data_from_file (self);

  return self;
}
