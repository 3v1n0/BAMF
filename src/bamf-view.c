/*
 * Copyright (C) 2010-2011 Canonical Ltd
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Jason Smith <jason.smith@canonical.com>
 *              Marco Trevisan (Treviño) <3v1n0@ubuntu.com>
 *
 */

#include "bamf-view.h"

#define BAMF_VIEW_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE(obj, \
                                    BAMF_TYPE_VIEW, BamfViewPrivate))

static void bamf_view_dbus_view_iface_init (BamfDBusItemViewIface *iface);
G_DEFINE_TYPE_WITH_CODE (BamfView, bamf_view, BAMF_DBUS_ITEM_TYPE_OBJECT_SKELETON,
                         G_IMPLEMENT_INTERFACE (BAMF_DBUS_ITEM_TYPE_VIEW,
                                                bamf_view_dbus_view_iface_init));

enum
{
  PROP_0,

  PROP_ACTIVE,
  PROP_RUNNING,
  PROP_URGENT,
  PROP_USER_VISIBLE,
};

enum
{
  CLOSED_INTERNAL,
  EXPORTED,

  LAST_SIGNAL,
};

static guint view_signals[LAST_SIGNAL] = { 0 };

struct _BamfViewPrivate
{
  BamfDBusItemView *dbus_iface;
  char * name;
  char * path;
  GList * children;
  GList * parents;
  gboolean closed;
};

static void
bamf_view_active_changed (BamfView *view, gboolean active)
{
  gboolean emit = TRUE;
  if (BAMF_VIEW_GET_CLASS (view)->active_changed)
    {
      emit = !BAMF_VIEW_GET_CLASS (view)->active_changed (view, active);
    }

  if (emit)
    g_signal_emit_by_name (view, "active-changed", active);

}

static void
bamf_view_name_changed (BamfView *view,
                        const gchar *old_name,
                        const gchar *new_name)
{
  g_signal_emit_by_name (view, "name-changed", old_name, new_name);

  if (view->priv->name)
    g_free (view->priv->name);

  view->priv->name = g_strdup (new_name);
}

static void
bamf_view_user_visible_changed (BamfView *view, gboolean user_visible)
{
  gboolean emit = TRUE;
  if (BAMF_VIEW_GET_CLASS (view)->user_visible_changed)
    {
      emit = !BAMF_VIEW_GET_CLASS (view)->user_visible_changed (view, user_visible);
    }

  if (emit)
    g_signal_emit_by_name (view, "user-visible-changed", user_visible);
}

static void
bamf_view_running_changed (BamfView *view, gboolean running)
{
  gboolean emit = TRUE;
  if (BAMF_VIEW_GET_CLASS (view)->running_changed)
    {
      emit = !BAMF_VIEW_GET_CLASS (view)->running_changed (view, running);
    }

  if (emit)
    g_signal_emit_by_name (view, "running-changed", running);
}

static void
bamf_view_urgent_changed (BamfView *view, gboolean urgent)
{
  gboolean emit = TRUE;
  if (BAMF_VIEW_GET_CLASS (view)->urgent_changed)
    {
      emit = !BAMF_VIEW_GET_CLASS (view)->urgent_changed (view, urgent);
    }

  if (emit)
    g_signal_emit_by_name (view, "urgent-changed", urgent);
}

void
bamf_view_close (BamfView *view)
{
  BamfViewPrivate *priv;
  gboolean emit = TRUE;
  GList *l;
  
  priv = view->priv;

  if (priv->closed)
    return;

  priv->closed = TRUE;

  if (BAMF_VIEW_GET_CLASS (view)->closed)
    {
      emit = !BAMF_VIEW_GET_CLASS (view)->closed (view);
    }

  if (priv->children)
    {
      for (l = priv->children; l; l = l->next)
        {
          if (BAMF_IS_VIEW (l->data))
            bamf_view_remove_child (view, l->data);
        }
      g_list_free (priv->children);
      priv->children = NULL;
    }

  if (emit)
    {
      g_object_ref (G_OBJECT (view));
      g_signal_emit_by_name (view, "closed");
      g_signal_emit (view, view_signals[CLOSED_INTERNAL], 0);
      g_object_unref (G_OBJECT (view));
    }
}

const char *
bamf_view_get_path (BamfView *view)
{
  g_return_val_if_fail (BAMF_IS_VIEW (view), NULL);

  return view->priv->path;
}

GVariant *
bamf_view_get_children_paths (BamfView *view)
{
  GVariantBuilder b;
  GList *l;

  g_return_val_if_fail (BAMF_IS_VIEW (view), NULL);

  g_variant_builder_init (&b, G_VARIANT_TYPE ("(as)"));
  g_variant_builder_open (&b, G_VARIANT_TYPE ("as"));

  for (l = view->priv->children; l; l = l->next)
    {
      BamfView *child = l->data;
      const char *path = bamf_view_get_path (child);

      if (!path)
        continue;

      g_variant_builder_add (&b, "s", path);
    }

  g_variant_builder_close (&b);

  return g_variant_builder_end (&b);
}

const GList *
bamf_view_get_children (BamfView *view)
{
  g_return_val_if_fail (BAMF_IS_VIEW (view), NULL);

  return view->priv->children;
}

GVariant *
bamf_view_get_parent_paths (BamfView *view)
{
  GVariantBuilder b;
  GList *l;

  g_return_val_if_fail (BAMF_IS_VIEW (view), NULL);

  g_variant_builder_init (&b, G_VARIANT_TYPE ("(as)"));
  g_variant_builder_open (&b, G_VARIANT_TYPE ("as"));

  for (l = view->priv->parents; l; l = l->next)
    {
      BamfView *parent = l->data;
      const char *path = bamf_view_get_path (parent);

      if (!path)
        continue;

      g_variant_builder_add (&b, "s", path);
    }

  g_variant_builder_close (&b);

  return g_variant_builder_end (&b);
}

const GList *
bamf_view_get_parents (BamfView *view)
{
  g_return_val_if_fail (BAMF_IS_VIEW (view), NULL);

  return view->priv->parents;
}

static void
bamf_view_handle_child_closed (BamfView *child,
                               BamfView *view)
{
  bamf_view_remove_child (view, child);
}

void
bamf_view_add_child (BamfView *view,
                     BamfView *child)
{
  const char * added;

  g_return_if_fail (BAMF_IS_VIEW (view));
  g_return_if_fail (BAMF_IS_VIEW (child));

  g_signal_connect (G_OBJECT (child), "closed-internal",
                    (GCallback) bamf_view_handle_child_closed, view);

  /* Make sure our parent child lists are ok, pay attention to whose list you add parents to */
  view->priv->children = g_list_prepend (view->priv->children, child);
  child->priv->parents = g_list_prepend (child->priv->parents, view);

  // Do this by hand so we can pass and object instead of a string
  if (BAMF_VIEW_GET_CLASS (view)->child_added)
    BAMF_VIEW_GET_CLASS (view)->child_added (view, child);

  added = bamf_view_get_path (child);
  g_signal_emit_by_name (view, "child-added", added);
}

void
bamf_view_remove_child (BamfView *view,
                        BamfView *child)
{
  const char * removed;

  g_return_if_fail (BAMF_IS_VIEW (view));
  g_return_if_fail (BAMF_IS_VIEW (child));

  g_signal_handlers_disconnect_by_func (child, bamf_view_handle_child_closed, view);

  /* Make sure our parent child lists are ok, pay attention to whose list you add parents to */
  view->priv->children = g_list_remove (view->priv->children, child);
  child->priv->parents = g_list_remove (child->priv->parents, view);

  // Do this by hand so we can pass and object instead of a string
  if (BAMF_VIEW_GET_CLASS (view)->child_removed)
    BAMF_VIEW_GET_CLASS (view)->child_removed (view, child);

  removed = bamf_view_get_path (child);
  g_signal_emit_by_name (view, "child-removed", removed);
}

gboolean
bamf_view_is_active (BamfView *view)
{
  g_return_val_if_fail (BAMF_IS_VIEW (view), FALSE);

  return bamf_dbus_item_view_get_active (view->priv->dbus_iface);
}

void
bamf_view_set_active (BamfView *view,
                      gboolean active)
{
  g_return_if_fail (BAMF_IS_VIEW (view));

  if (active == bamf_view_is_active (view))
    return;

  bamf_dbus_item_view_set_active (view->priv->dbus_iface, active);
  bamf_view_active_changed (view, active);
}

gboolean
bamf_view_is_urgent (BamfView *view)
{
  g_return_val_if_fail (BAMF_IS_VIEW (view), FALSE);

  return bamf_dbus_item_view_get_urgent (view->priv->dbus_iface);
}

void
bamf_view_set_urgent (BamfView *view,
                       gboolean urgent)
{
  g_return_if_fail (BAMF_IS_VIEW (view));

  if (urgent == bamf_view_is_urgent (view))
    return;

  bamf_dbus_item_view_set_urgent (view->priv->dbus_iface, urgent);
  bamf_view_urgent_changed (view, urgent);
}

gboolean
bamf_view_is_running (BamfView *view)
{
  g_return_val_if_fail (BAMF_IS_VIEW (view), FALSE);

  return bamf_dbus_item_view_get_running (view->priv->dbus_iface);
}

void
bamf_view_set_running (BamfView *view,
                       gboolean running)
{
  g_return_if_fail (BAMF_IS_VIEW (view));

  if (running == bamf_view_is_running (view))
    return;

  bamf_dbus_item_view_set_running (view->priv->dbus_iface, running);
  bamf_view_running_changed (view, running);
}

gboolean
bamf_view_user_visible (BamfView *view)
{
  g_return_val_if_fail (BAMF_IS_VIEW (view), FALSE);

  return bamf_dbus_item_view_get_user_visible (view->priv->dbus_iface);
}

void
bamf_view_set_user_visible (BamfView *view, gboolean user_visible)
{
  g_return_if_fail (BAMF_IS_VIEW (view));

  if (user_visible == bamf_view_user_visible (view))
    return;

  bamf_dbus_item_view_set_user_visible (view->priv->dbus_iface, user_visible);
  bamf_view_user_visible_changed (view, user_visible);
}

char *
bamf_view_get_icon (BamfView *view)
{
  g_return_val_if_fail (BAMF_IS_VIEW (view), NULL);

  if (BAMF_VIEW_GET_CLASS (view)->get_icon)
    return BAMF_VIEW_GET_CLASS (view)->get_icon (view);

  return NULL;
}

char *
bamf_view_get_name (BamfView *view)
{
  g_return_val_if_fail (BAMF_IS_VIEW (view), NULL);

  return g_strdup (view->priv->name);
}

void
bamf_view_set_name (BamfView *view,
                    const char * name)
{
  g_return_if_fail (BAMF_IS_VIEW (view));

  if (!g_strcmp0 (name, view->priv->name))
    return;

  bamf_view_name_changed (view, view->priv->name, name);
}

const char *
bamf_view_get_view_type (BamfView *view)
{
  g_return_val_if_fail (BAMF_IS_VIEW (view), NULL);

  if (BAMF_VIEW_GET_CLASS (view)->view_type)
    return BAMF_VIEW_GET_CLASS (view)->view_type (view);

  return "view";
}

char *
bamf_view_get_stable_bus_name (BamfView *view)
{
  g_return_val_if_fail (BAMF_IS_VIEW (view), NULL);

  if (BAMF_VIEW_GET_CLASS (view)->stable_bus_name)
    return BAMF_VIEW_GET_CLASS (view)->stable_bus_name (view);
    
  return g_strdup_printf ("view%p", view);
}

const char *
bamf_view_export_on_bus (BamfView *view, GDBusConnection *connection)
{
  char *path = NULL;
  GList *ifaces, *l;
  GError *error = NULL;

  g_return_val_if_fail (BAMF_IS_VIEW (view), NULL);
  g_return_val_if_fail (G_IS_DBUS_CONNECTION (connection), NULL);

  if (!view->priv->path)
    {
      char *stable_name = bamf_view_get_stable_bus_name (view);
      path = g_strdup_printf ("%s/%s", BAMF_DBUS_PATH, stable_name);
      g_free (stable_name);

      BAMF_VIEW_GET_CLASS (view)->names = g_list_prepend (BAMF_VIEW_GET_CLASS (view)->names, path);
      view->priv->path = path;

      ifaces = g_dbus_object_get_interfaces (G_DBUS_OBJECT (view));

      for (l = ifaces; l; l = l->next)
        {
          g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (l->data),
                                            connection, path, &error);
        }

      if (error)
        {
          g_critical ("Can't register BAMF view: %s", error->message);
          g_error_free (error);
        }
      else
        {
          g_signal_emit (view, view_signals[EXPORTED], 0);
        }

      g_list_free_full (ifaces, g_object_unref);
    }

  return view->priv->path;
}

gboolean
bamf_view_is_on_bus (BamfView *view)
{
  g_return_val_if_fail (BAMF_IS_VIEW (view), FALSE);
  GDBusInterfaceSkeleton *dbus_iface;
  const gchar *exported_path;

  dbus_iface = G_DBUS_INTERFACE_SKELETON (view->priv->dbus_iface);
  exported_path = g_dbus_interface_skeleton_get_object_path (dbus_iface);

  return (view->priv->path != NULL && exported_path != NULL);
}

static void
on_view_active_changed (BamfView *view, gboolean active, gpointer _not_used)
{
  g_return_if_fail (BAMF_IS_VIEW (view));
  g_signal_emit_by_name (view->priv->dbus_iface, "active-changed", active);
}

static void
on_view_running_changed (BamfView *view, gboolean running, gpointer _not_used)
{
  g_return_if_fail (BAMF_IS_VIEW (view));
  g_signal_emit_by_name (view->priv->dbus_iface, "running-changed", running);
}

static void
on_view_urgent_changed (BamfView *view, gboolean urgent, gpointer _not_used)
{
  g_return_if_fail (BAMF_IS_VIEW (view));
  g_signal_emit_by_name (view->priv->dbus_iface, "urgent-changed", urgent);
}

static void
on_view_user_visible_changed (BamfView *view, gboolean user_visible, gpointer _not_used)
{
  g_return_if_fail (BAMF_IS_VIEW (view));
  g_signal_emit_by_name (view->priv->dbus_iface, "user-visible-changed", user_visible);
}

static void
on_view_name_changed (BamfView *view, const gchar *old_name, const gchar *new_name, gpointer _not_used)
{
  g_return_if_fail (BAMF_IS_VIEW (view));
  g_signal_emit_by_name (view->priv->dbus_iface, "name-changed",
                         old_name ? old_name : "", new_name ? new_name : "");
}

static void
on_view_child_added (BamfView *view, const gchar *child_path, gpointer _not_used)
{
  g_return_if_fail (BAMF_IS_VIEW (view));
  g_signal_emit_by_name (view->priv->dbus_iface, "child-added",
                         child_path ? child_path : "");
}

static void
on_view_child_removed (BamfView *view, const gchar *child_path, gpointer _not_used)
{
  g_return_if_fail (BAMF_IS_VIEW (view));
  g_signal_emit_by_name (view->priv->dbus_iface, "child-removed",
                         child_path ? child_path : "");
}

static void
on_view_closed (BamfView *view, gpointer _not_used)
{
  g_return_if_fail (BAMF_IS_VIEW (view));
  g_signal_emit_by_name (view->priv->dbus_iface, "closed");
}

static gboolean
on_dbus_handle_view_type (BamfDBusItemView *interface,
                          GDBusMethodInvocation *invocation,
                          BamfView *view)
{
  const char *type = bamf_view_get_view_type (view);
  g_dbus_method_invocation_return_value (invocation,
                                         g_variant_new ("(s)", type));

  return TRUE;
}

static gboolean
on_dbus_handle_user_visible (BamfDBusItemView *interface,
                             GDBusMethodInvocation *invocation,
                             BamfView *view)
{
  gboolean user_visible = bamf_view_user_visible (view);
  g_dbus_method_invocation_return_value (invocation,
                                         g_variant_new ("(b)", user_visible));

  return TRUE;
}

static gboolean
on_dbus_handle_icon (BamfDBusItemView *interface,
                     GDBusMethodInvocation *invocation,
                     BamfView *view)
{
  char *icon = bamf_view_get_icon (view);
  g_dbus_method_invocation_return_value (invocation,
                                         g_variant_new ("(s)", icon ? icon : ""));
  g_free (icon);

  return TRUE;
}

static gboolean
on_dbus_handle_name (BamfDBusItemView *interface,
                     GDBusMethodInvocation *invocation,
                     BamfView *view)
{
  char *name = bamf_view_get_name (view);
  g_dbus_method_invocation_return_value (invocation,
                                         g_variant_new ("(s)", name ? name : ""));
  g_free (name);

  return TRUE;
}

static gboolean
on_dbus_handle_is_urgent (BamfDBusItemView *interface,
                          GDBusMethodInvocation *invocation,
                          BamfView *view)
{
  gboolean is_urgent = bamf_view_is_urgent (view);
  g_dbus_method_invocation_return_value (invocation,
                                         g_variant_new ("(b)", is_urgent));

  return TRUE;
}

static gboolean
on_dbus_handle_is_running (BamfDBusItemView *interface,
                           GDBusMethodInvocation *invocation,
                           BamfView *view)
{
  gboolean is_running = bamf_view_is_running (view);
  g_dbus_method_invocation_return_value (invocation,
                                         g_variant_new ("(b)", is_running));

  return TRUE;
}

static gboolean
on_dbus_handle_is_active (BamfDBusItemView *interface,
                          GDBusMethodInvocation *invocation,
                          BamfView *view)
{
  gboolean is_active = bamf_view_is_active (view);
  g_dbus_method_invocation_return_value (invocation,
                                         g_variant_new ("(b)", is_active));

  return TRUE;
}

static gboolean
on_dbus_handle_parents (BamfDBusItemView *interface,
                        GDBusMethodInvocation *invocation,
                        BamfView *view)
{
  GVariant *parents = bamf_view_get_parent_paths (view);
  g_dbus_method_invocation_return_value (invocation, parents);

  return TRUE;
}

static gboolean
on_dbus_handle_children (BamfDBusItemView *interface,
                         GDBusMethodInvocation *invocation,
                         BamfView *view)
{
  GVariant *children = bamf_view_get_children_paths (view);
  g_dbus_method_invocation_return_value (invocation, children);

  return TRUE;
}

static void
bamf_view_dispose (GObject *object)
{
  BamfView *view = BAMF_VIEW (object);
  BamfViewPrivate *priv = view->priv;
  GList *ifaces, *l;

  ifaces = g_dbus_object_get_interfaces (G_DBUS_OBJECT (view));

  for (l = ifaces; l; l = l->next)
    {
      GDBusInterfaceSkeleton *iface = G_DBUS_INTERFACE_SKELETON (l->data);

      if (g_dbus_interface_skeleton_get_object_path (iface))
        g_dbus_interface_skeleton_unexport (iface);
    }
  g_list_free (ifaces);

  if (priv->name)
    {
      g_free (priv->name);
      priv->name = NULL;
    }

  if (priv->path)
    {
      g_free (priv->path);
      priv->path = NULL;
    }

  if (priv->children)
    {
      g_list_free (priv->children);
      priv->children = NULL;
    }

  if (priv->parents)
    {
      g_list_free (priv->parents);
      priv->parents = NULL;
    }

  G_OBJECT_CLASS (bamf_view_parent_class)->dispose (object);
}

static void
bamf_view_finalize (GObject *object)
{
  BamfView *view = BAMF_VIEW (object);

  g_object_unref (view->priv->dbus_iface);

  G_OBJECT_CLASS (bamf_view_parent_class)->finalize (object);
}

static void
bamf_view_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
  BamfView *view = BAMF_VIEW (object);

  switch (property_id)
    {
      case PROP_ACTIVE:
        g_value_set_boolean (value, bamf_view_is_active (view));
        break;
      case PROP_URGENT:
        g_value_set_boolean (value, bamf_view_is_urgent (view));
        break;
      case PROP_USER_VISIBLE:
        g_value_set_boolean (value, bamf_view_user_visible (view));
        break;
      case PROP_RUNNING:
        g_value_set_boolean (value, bamf_view_is_running (view));
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
    }
}

static void
bamf_view_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
  switch (property_id)
    {
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
}

static void
bamf_view_init (BamfView * self)
{
  self->priv = BAMF_VIEW_GET_PRIVATE (self);

  /* Initializing the dbus interface */
  self->priv->dbus_iface = bamf_dbus_item_view_skeleton_new ();

  /* We need to connect to the object own signals to redirect them to the dbus
   * interface                                                                */
  g_signal_connect (self, "active-changed", G_CALLBACK (on_view_active_changed), NULL);
  g_signal_connect (self, "running-changed", G_CALLBACK (on_view_running_changed), NULL);
  g_signal_connect (self, "urgent-changed", G_CALLBACK (on_view_urgent_changed), NULL);
  g_signal_connect (self, "user-visible-changed", G_CALLBACK (on_view_user_visible_changed), NULL);
  g_signal_connect (self, "name-changed", G_CALLBACK (on_view_name_changed), NULL);
  g_signal_connect (self, "child-added", G_CALLBACK (on_view_child_added), NULL);
  g_signal_connect (self, "child-removed", G_CALLBACK (on_view_child_removed), NULL);
  g_signal_connect (self, "closed", G_CALLBACK (on_view_closed), NULL);

  /* Registering signal callbacks to reply to dbus method calls */
  g_signal_connect (self->priv->dbus_iface, "handle-view-type",
                    G_CALLBACK (on_dbus_handle_view_type), self);

  g_signal_connect (self->priv->dbus_iface, "handle-user-visible",
                    G_CALLBACK (on_dbus_handle_user_visible), self);

  g_signal_connect (self->priv->dbus_iface, "handle-icon",
                    G_CALLBACK (on_dbus_handle_icon), self);

  g_signal_connect (self->priv->dbus_iface, "handle-name",
                    G_CALLBACK (on_dbus_handle_name), self);

  g_signal_connect (self->priv->dbus_iface, "handle-is-urgent",
                    G_CALLBACK (on_dbus_handle_is_urgent), self);

  g_signal_connect (self->priv->dbus_iface, "handle-is-running",
                    G_CALLBACK (on_dbus_handle_is_running), self);

  g_signal_connect (self->priv->dbus_iface, "handle-is-active",
                    G_CALLBACK (on_dbus_handle_is_active), self);

  g_signal_connect (self->priv->dbus_iface, "handle-parents",
                    G_CALLBACK (on_dbus_handle_parents), self);

  g_signal_connect (self->priv->dbus_iface, "handle-children",
                    G_CALLBACK (on_dbus_handle_children), self);

  /* Setting the interface for the dbus object */
  bamf_dbus_item_object_skeleton_set_view (BAMF_DBUS_ITEM_OBJECT_SKELETON (self),
                                           self->priv->dbus_iface);
}

static void
bamf_view_dbus_view_iface_init (BamfDBusItemViewIface *iface)
{
}

static void
bamf_view_class_init (BamfViewClass * klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = bamf_view_dispose;
  object_class->finalize = bamf_view_finalize;
  object_class->get_property = bamf_view_get_property;
  object_class->set_property = bamf_view_set_property;

  g_type_class_add_private (klass, sizeof (BamfViewPrivate));

  /* Overriding the properties defined in the interface, this is needed
   * but we actually don't use these properties, as we act like a proxy       */
  g_object_class_override_property (object_class, PROP_ACTIVE, "active");
  g_object_class_override_property (object_class, PROP_URGENT, "urgent");
  g_object_class_override_property (object_class, PROP_RUNNING, "running");
  g_object_class_override_property (object_class, PROP_USER_VISIBLE, "user-visible");

  view_signals [CLOSED_INTERNAL] =
    g_signal_new ("closed-internal",
                  G_OBJECT_CLASS_TYPE (klass),
                  0,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  view_signals [EXPORTED] =
    g_signal_new ("exported",
                  G_OBJECT_CLASS_TYPE (klass),
                  0,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
}
