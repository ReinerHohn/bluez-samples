//#include <fwbluez.h>
//#include <fwcm.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
//#include <cl_dbus_bluetooth.h>
//#include <cl_delegate.h>
#include <glib.h>
#include <dbus/dbus.h>

#include "bluez.h"

#include "dbus-bt.h"

GDBusObjectManager* fwbluez_object_manager;

GMainContext* context;
GMainLoop* loop;
GDBusConnection* dbus;

//static FWBluezAgent1* _agent;
static FWBluezAdapter1* _adapter;

void fwcl_bt_register_hooks(void* loop, void* context, void* dbus);
void fwbluez_free_adapters(GList* adapters);

void init_bt()
{
	context = g_main_context_new();
		
	if (context)
	{
		loop = g_main_loop_new(context,FALSE);
	}

	dbus = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, NULL);

	fwcl_bt_register_hooks(loop, context, dbus);
}

// init
GError* fwbluez_init_system(GDBusConnection* bus)
{
	GError* error = NULL;
	fwbluez_object_manager = fwbluez_object_manager_client_new_sync(bus, G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE, "org.bluez", "/", NULL, &error);
	return error;
}

void fwbluez_exit_system()
{
	g_object_unref(fwbluez_object_manager);
}
//end init

GList* fwbluez_list_adapters(GError** error)
{
	GList* result = NULL;
	GList* l;
	GList* temp = g_dbus_object_manager_get_objects(fwbluez_object_manager);

	for (l = temp; l != NULL; l = l->next)
	{
		GDBusInterface* iface = g_dbus_object_get_interface((GDBusObject*)l->data, "org.bluez.Adapter1");

		if (FWBLUEZ_IS_ADAPTER1_PROXY(iface))
		{
			result = g_list_append(result,
			                       fwbluez_adapter1_proxy_new_sync(
			                          g_dbus_proxy_get_connection(G_DBUS_PROXY(iface)),
			                          G_DBUS_PROXY_FLAGS_NONE, /* allocate with all signals connected */
			                          "org.bluez",
			                          g_dbus_proxy_get_object_path(G_DBUS_PROXY(iface)),
			                          NULL,
			                          NULL));
			g_object_unref(iface);
		}
	}

	fwbluez_free_adapters(temp);
	return result;
}

void fwbluez_free_adapters(GList* adapters)
{
	GList* l;

	for (l = adapters; l != NULL; l = l->next)
	{
		g_object_unref(l->data);
	}

	g_list_free(adapters);
}

void fwcl_bt_register_hooks(void* loop, void* context, void* dbus)
{
	(void)loop;
	(void)context;

	if (fwbluez_init_system(dbus) != NULL)
	{
		return;
	}

	GList* adapters = fwbluez_list_adapters(NULL);

	if (adapters)
	{
		_adapter = g_object_ref(adapters->data);
		//g_signal_connect(_adapter, "notify::discovering", G_CALLBACK(_on_adapter_property_change), NULL);
		fwbluez_adapter1_set_discoverable( _adapter, TRUE );

		fwbluez_free_adapters(adapters);
	}

	//_agent = fwbluez_agent_new(dbus, "/com/hgs/bt5agent", _bt_agent_func, NULL, NULL);
	//_bt_enable(dbus);
}
