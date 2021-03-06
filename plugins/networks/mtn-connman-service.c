/*
 * mex-networks - Connection Manager UI for Media Explorer
 * Copyright © 2010-2011, Intel Corporation.
 * Copyright © 2012, sleep(5) ltd.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses>
 */

#include "mtn-connman-service.h"
#include "connman-service-introspection.h"

struct _MtnConnmanServicePrivate {
    GHashTable *properties;
};

static void mtn_connman_service_initable_init       (GInitableIface *initable_iface);
static void mtn_connman_service_async_initable_init (GAsyncInitableIface *async_initable_iface);

G_DEFINE_TYPE_WITH_CODE (MtnConnmanService, mtn_connman_service, G_TYPE_DBUS_PROXY,
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, mtn_connman_service_initable_init)
                         G_IMPLEMENT_INTERFACE (G_TYPE_ASYNC_INITABLE, mtn_connman_service_async_initable_init));

#define GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE ((o), MTN_TYPE_CONNMAN_SERVICE, MtnConnmanServicePrivate))

enum
{
    PROPERTY_CHANGED_SIGNAL,
    LAST_SIGNAL,
};

static guint signals[LAST_SIGNAL] = {0};

typedef struct {
    GSimpleAsyncResult *res;
    GCancellable *cancellable;
} InitData;

static void
_name_owner_notify_cb (MtnConnmanService *service,
                       GParamSpec        *pspec,
                       gpointer           user_data)
{
    const char *name;

    g_object_get (service, "g-name-owner", &name, NULL);

    if (!name) {
        g_hash_table_remove_all (service->priv->properties);
        /* Could notify "property-changed:NNN" here but it would be
         * very difficult to get right... better let owner connect
         * to this same signal as well*/
    } else {
        /* TODO: get_properties and notify each */
    }
}

static void
init_data_free (InitData *data)
{
    g_object_unref (data->res);
    g_free (data);
}

static void
_get_properties_cb (GObject      *obj,
                    GAsyncResult *res,
                    gpointer      user_data)
{
    MtnConnmanService *service;
    GError *error;
    GVariant *var;
    InitData *data;

    service = MTN_CONNMAN_SERVICE (obj);
    data = (InitData*)user_data;

    error = NULL;
    var = g_dbus_proxy_call_finish (G_DBUS_PROXY (obj), res, &error);
    if (!var) {
        g_warning ("Initial Service.GetProperties() failed: %s\n",
                   error->message);
        g_error_free (error);
    } else {
        GVariant *value;
        char *key;
        GVariantIter *iter;

        g_variant_get (var, "(a{sv})", &iter);
        while (g_variant_iter_next (iter, "{sv}", &key, &value)) {
            g_hash_table_insert (service->priv->properties,
                                 key, value);
        }
        g_variant_iter_free (iter);
        g_variant_unref (var);
    }

    g_simple_async_result_complete_in_idle (data->res);
    init_data_free (data);
}

static void
_parent_init_async_cb (GObject      *obj,
                       GAsyncResult *res,
                       gpointer      user_data)
{
    InitData *data;

    data = (InitData*)user_data;

    /* start our own initialization */
    g_dbus_proxy_call (G_DBUS_PROXY (obj),
                       "GetProperties",
                       NULL,
                       G_DBUS_CALL_FLAGS_NONE,
                       -1,
                       data->cancellable,
                       _get_properties_cb,
                       data);

    g_signal_connect (obj, "notify::g-name-owner",
                      G_CALLBACK (_name_owner_notify_cb), NULL);
}

static void
mtn_connman_service_initable_init_async (GAsyncInitable      *initable,
                                         gint                 io_priority,
                                         GCancellable        *cancellable,
                                         GAsyncReadyCallback  callback,
                                         gpointer             user_data)
{
    GAsyncInitableIface *iface_class, *parent_iface_class;
    InitData *data;

    data = g_new0 (InitData, 1);
    data->cancellable = cancellable;
    data->res = g_simple_async_result_new (G_OBJECT (initable),
                                           callback,
                                           user_data,
                                           NULL);

    /* Chain up the parent method */
    iface_class = G_ASYNC_INITABLE_GET_IFACE (initable);
    parent_iface_class = g_type_interface_peek_parent (iface_class);
    parent_iface_class->init_async (initable,
                                    io_priority,
                                    cancellable,
                                    _parent_init_async_cb,
                                    data);
}

static gboolean
mtn_connman_service_initable_init_finish (GAsyncInitable  *initable,
                                          GAsyncResult    *res,
                                          GError         **error)
{
    GAsyncInitableIface *iface_class, *parent_iface_class;

    /* Chain up the parent method */
    iface_class = G_ASYNC_INITABLE_GET_IFACE (initable);
    parent_iface_class = g_type_interface_peek_parent (iface_class);
    return parent_iface_class->init_finish (initable, res, error);
}

static void
mtn_connman_service_async_initable_init (GAsyncInitableIface *async_initable_iface)
{
    async_initable_iface->init_async = mtn_connman_service_initable_init_async;
    async_initable_iface->init_finish = mtn_connman_service_initable_init_finish;
}

static gboolean
mtn_connman_service_initable_init_sync (GInitable     *initable,
                                        GCancellable  *cancellable,
                                        GError       **error)
{
    GInitableIface *iface_class, *parent_iface_class;
    GVariant *var, *value;
    GVariantIter *iter;
    char *key;
    MtnConnmanService *service;

    service = MTN_CONNMAN_SERVICE (initable);

    /* Chain up the old method */
    iface_class = G_INITABLE_GET_IFACE (initable);
    parent_iface_class = g_type_interface_peek_parent (iface_class);
    if (!parent_iface_class->init (initable, cancellable, error)) {
        return FALSE;
    }

    g_signal_connect (service, "notify::g-name-owner",
                      G_CALLBACK (_name_owner_notify_cb), NULL);

    var = g_dbus_proxy_call_sync (G_DBUS_PROXY (service),
                                  "GetProperties",
                                  NULL,
                                  G_DBUS_CALL_FLAGS_NONE,
                                  -1,
                                  NULL,
                                  error);
    if (!var) {
        g_debug ("Service has no properties?!");
        return FALSE;
    }

    g_variant_get (var, "(a{sv})", &iter);
    while (g_variant_iter_next (iter, "{sv}", &key, &value)) {
        g_debug ("Got Service property %s", key);
        g_hash_table_insert (service->priv->properties,
                             key, value);
    }
    g_variant_iter_free (iter);
    g_variant_unref (var);

    return TRUE;
}

static void
mtn_connman_service_initable_init (GInitableIface *initable_iface)
{
    initable_iface->init = mtn_connman_service_initable_init_sync;
}

static GDBusInterfaceInfo *
mtn_connman_service_get_interface_info (void)
{
    static gsize has_info = 0;
    static GDBusInterfaceInfo *info = NULL;
    if (g_once_init_enter (&has_info)) {
        GDBusNodeInfo *introspection_data;
        introspection_data = g_dbus_node_info_new_for_xml (connman_service_introspection,
                                                           NULL);
        info = introspection_data->interfaces[0];
        g_once_init_leave (&has_info, 1);
    }
    return info;
}

static void
mtn_connman_service_handle_new_property (MtnConnmanService *service,
                                         char              *key,
                                         GVariant          *value)
{
    GVariant *old_value;

    old_value = g_hash_table_lookup (service->priv->properties, key);
    if (!old_value || !g_variant_equal (value, old_value)) {
        char *name;

        /* must use _replace here, since we need the key around afterwards */
        g_hash_table_replace (service->priv->properties,
                              key, value);

        /* emit changed signal*/
        name = g_strdup_printf ("property-changed::%s", key);
        g_signal_emit_by_name (service, name, value);
        g_free (name);
    }
}

static void
_get_properties_for_passphrase_cb (GObject      *obj,
                                   GAsyncResult *res,
                                   gpointer      user_data)
{
    MtnConnmanService *service;
    GError *error;
    GVariant *var;

    service = MTN_CONNMAN_SERVICE (obj);

    error = NULL;
    var = g_dbus_proxy_call_finish (G_DBUS_PROXY (obj), res, &error);
    if (!var) {
        g_warning ("Connman Service.GetProperties failed: %s",
                   error->message);
        g_error_free (error);
    } else {
        GVariant *value;
        char *key;
        GVariantIter *iter;

        g_variant_get (var, "(a{sv})", &iter);
        while (g_variant_iter_next (iter, "{sv}", &key, &value)) {
            if (g_strcmp0 (key, "Passphrase") == 0 ||
                g_strcmp0 (key, "PassphraseRequired") == 0) {
                mtn_connman_service_handle_new_property (service,
                                                         key, value);
            }
        }
        g_variant_iter_free (iter);
        g_variant_unref (var);
    }
}

static void
mtn_connman_service_g_signal (GDBusProxy   *proxy,
                              const gchar  *sender_name,
                              const gchar  *signal_name,
                              GVariant     *parameters)
{
    MtnConnmanService *service = MTN_CONNMAN_SERVICE (proxy);

    if (!service->priv->properties)
        return;

    if (g_strcmp0 (signal_name, "PropertyChanged") == 0) {
        char *key;
        GVariant *value;

        /* FIXME -- leaking value? */
        g_variant_get (parameters, "(sv)", &key, &value);
        if (g_strcmp0 (key, "PassphraseRequested") == 0) {
            /* get the new Passphrase value before emitting signal
             * (because there is no signal for Passphrase) */
            g_dbus_proxy_call (proxy,
                               "GetProperties",
                               NULL,
                               G_DBUS_CALL_FLAGS_NONE,
                               -1,
                               NULL,
                               _get_properties_for_passphrase_cb,
                               NULL);
        } else {
           mtn_connman_service_handle_new_property (service, key, value);
        }
    }
}

static void
mtn_connman_service_dispose (GObject *object)
{
    MtnConnmanService *service;

    service = MTN_CONNMAN_SERVICE (object);

    if (service->priv->properties) {
        g_hash_table_unref (service->priv->properties);
        service->priv->properties = NULL;
    }

    G_OBJECT_CLASS (mtn_connman_service_parent_class)->dispose (object);
}

static void
mtn_connman_service_class_init (MtnConnmanServiceClass *klass)
{
    GObjectClass *object_class;
    GDBusProxyClass *proxy_class;

    g_type_class_add_private (klass, sizeof (MtnConnmanServicePrivate));

    object_class = G_OBJECT_CLASS (klass);
    object_class->dispose = mtn_connman_service_dispose;

    proxy_class = G_DBUS_PROXY_CLASS (klass);
    proxy_class->g_signal = mtn_connman_service_g_signal;

    signals[PROPERTY_CHANGED_SIGNAL] =
            g_signal_new ("property-changed",
                          MTN_TYPE_CONNMAN_SERVICE,
                          G_SIGNAL_DETAILED|G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (MtnConnmanServiceClass, property_changed),
                          NULL,
                          NULL,
                          g_cclosure_marshal_VOID__VARIANT,
                          G_TYPE_NONE,
                          1,
                          G_TYPE_VARIANT);
}

static void
mtn_connman_service_init (MtnConnmanService *self)
{
    self->priv = GET_PRIVATE (self);

    self->priv->properties =
            g_hash_table_new_full (g_str_hash,
                                   g_str_equal,
                                   g_free,
                                   NULL);
}

GVariant*
mtn_connman_service_get_property (MtnConnmanService *service,
                                  const char        *key)
{
    g_return_val_if_fail (MTN_IS_CONNMAN_SERVICE (service), NULL);

    if (!service->priv->properties)
        return NULL;

    return g_hash_table_lookup (service->priv->properties, key);
}

static void
_set_property_cb (GObject *object,
                  GAsyncResult *res,
                  gpointer user_data)
{
    char *key;
    GVariant *var;
    GError *error;

    key = (char *)user_data;
    error = NULL;

    var = g_dbus_proxy_call_finish (G_DBUS_PROXY (object),
                                    res,
                                    &error);
    if (var) {
        g_variant_unref (var);
    } else if (error) {
        g_warning ("Connman Service.SetProperty() for '%s' failed: %s",
                   key, error->message);

        /* TODO: call a error handler method */
        g_error_free (error);
    }

    g_free (key);
}

void
mtn_connman_service_set_property (MtnConnmanService *service,
                                  const char        *key,
                                  GVariant          *value)
{
    GVariant *params;

    g_return_if_fail (MTN_IS_CONNMAN_SERVICE (service));
    g_return_if_fail (key);
    g_return_if_fail (value);

    params = g_variant_new ("(sv)", key, value);

    g_dbus_proxy_call (G_DBUS_PROXY (service),
                       "SetProperty",
                       params,
                       G_DBUS_CALL_FLAGS_NONE,
                       -1,
                       NULL,
                       _set_property_cb,
                       g_strdup (key));

    g_variant_unref (params);
}

MtnConnmanService *
mtn_connman_service_new_finish (GAsyncResult  *res,
                                GError       **error)
{
    return MTN_CONNMAN_SERVICE (g_dbus_proxy_new_finish (res, error));
}

void
mtn_connman_service_new (const char          *object_path,
                         GCancellable        *cancellable,
                         GAsyncReadyCallback  callback,
                         gpointer             user_data)
{
    g_async_initable_new_async (MTN_TYPE_CONNMAN_SERVICE,
                                G_PRIORITY_DEFAULT,
                                cancellable,
                                callback,
                                user_data,
                                "g-bus-type", G_BUS_TYPE_SYSTEM,
                                "g-interface-info", mtn_connman_service_get_interface_info (),
                                "g-interface-name", "net.connman.Service",
                                "g-name", "net.connman",
                                "g-object-path", object_path,
                                NULL);
}
