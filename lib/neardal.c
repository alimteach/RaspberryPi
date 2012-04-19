/*
 *     NEARDAL (Neard Abstraction Library)
 *
 *     Copyright 2012 Intel Corporation. All rights reserved.
 *
 *     This program is free software; you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License version 2
 *     as published by the Free Software Foundation.
 *
 *     This program is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with this program; if not, write to the Free Software Foundation,
 *     Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <glib-object.h>

#include "neard_manager_proxy.h"
#include "neard_adapter_proxy.h"

#include "neardal.h"
#include "neardal_prv.h"

#include <glib-2.0/glib/glist.h>
#include <glib-2.0/glib/gerror.h>

neardalCtx neardalMgr = {NULL, NULL, {NULL}, NULL, NULL, NULL, NULL, NULL, NULL,
NULL, NULL, NULL, NULL, NULL, NULL, 0, NULL};

/******************************************************************************
 * neardal_prv_construct: create NEARDAL object instance, Neard Dbus connection,
 * register Neard's events
 *****************************************************************************/
void neardal_prv_construct(errorCode_t *ec)
{
	errorCode_t	err = NEARDAL_SUCCESS;

	if (neardalMgr.proxy != NULL)
		return;

	NEARDAL_TRACEIN();
	/* Create DBUS connection */
	g_type_init();
	neardalMgr.conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL,
					   &neardalMgr.gerror);
	if (neardalMgr.conn != NULL) {
		/* We have a DBUS connection, create proxy on Neard Manager */
		err =  neardal_mgr_create();
		if (err != NEARDAL_SUCCESS) {
			NEARDAL_TRACEF(
				"neardal_mgr_create() exit (err %d: %s)\n",
				err, neardal_error_get_text(err));

			/* No Neard daemon, destroying neardal object... */
			if (err == NEARDAL_ERROR_DBUS_CANNOT_CREATE_PROXY)
				neardal_tools_prv_free_gerror(&neardalMgr);
		}
	} else {
		NEARDAL_TRACE_ERR("Unable to connect to dbus: %s\n",
				 neardalMgr.gerror->message);
		neardal_tools_prv_free_gerror(&neardalMgr);
		err = NEARDAL_ERROR_DBUS;
	}

	if (ec != NULL)
		*ec = err;

	NEARDAL_TRACEF("Exit\n");
	return;
}


/******************************************************************************
 * neardal_destroy: destroy NEARDAL object instance, Disconnect Neard Dbus
 * connection, unregister Neard's events
 *****************************************************************************/
void neardal_destroy(void)
{
	NEARDAL_TRACEIN();
	if (neardalMgr.proxy != NULL) {
		neardal_tools_prv_free_gerror(&neardalMgr);
		neardal_mgr_destroy();
	}
}

/******************************************************************************
 * neardal_set_cb_adapter_added: setup a client callback for
 * 'NEARDAL adapter added'.
 * cb_adp_added = NULL to remove actual callback.
 *****************************************************************************/
errorCode_t neardal_set_cb_adapter_added(adapter_cb cb_adp_added,
					 void *user_data)
{
	neardalMgr.cb_adp_added		= cb_adp_added;
	neardalMgr.cb_adp_added_ud	= user_data;

	return NEARDAL_SUCCESS;
}

/******************************************************************************
 * neardal_set_cb_adapter_removed: setup a client callback for
 * 'NEARDAL adapter added'.
 * cb_adp_removed = NULL to remove actual callback.
 *****************************************************************************/
errorCode_t neardal_set_cb_adapter_removed(adapter_cb cb_adp_removed,
					   void *user_data)
{

	neardalMgr.cb_adp_removed	= cb_adp_removed;
	neardalMgr.cb_adp_removed_ud	= user_data;

	return NEARDAL_SUCCESS;
}

/******************************************************************************
 * neardal_set_manager_cb_property_changed: setup a client callback for
 * 'NEARDAL Adapter Property Change'.
 * cb_mgr_adp_property_changed = NULL to remove actual callback.
 *****************************************************************************/
errorCode_t neardal_set_cb_adapter_property_changed(
					adapter_prop_cb cb_adp_property_changed,
					void *user_data)
{
	neardalMgr.cb_adp_prop_changed		= cb_adp_property_changed;
	neardalMgr.cb_adp_prop_changed_ud	= user_data;

	return NEARDAL_SUCCESS;
}

/******************************************************************************
 * neardal_set_cb_adapter_added: setup a client callback for
 * 'NEARDAL adapter added'.
 * cb_adp_added = NULL to remove actual callback.
 *****************************************************************************/
errorCode_t neardal_set_cb_target_found(target_cb cb_tgt_found,
					void *user_data)
{
	neardalMgr.cb_tgt_found		= cb_tgt_found;
	neardalMgr.cb_tgt_found_ud	= user_data;

	return NEARDAL_SUCCESS;
}

/******************************************************************************
 * neardal_set_cb_adapter_removed: setup a client callback for
 * 'NEARDAL adapter added'.
 * cb_adp_removed = NULL to remove actual callback.
 *****************************************************************************/
errorCode_t neardal_set_cb_target_lost(target_cb cb_tgt_lost,
				       void *user_data)
{
	neardalMgr.cb_tgt_lost		= cb_tgt_lost;
	neardalMgr.cb_tgt_lost_ud	= user_data;

	return NEARDAL_SUCCESS;
}


/******************************************************************************
 * neardal_set_cb_record_found: setup a client callback for
 * 'NEARDAL target record found'.
 * cb_rcd_found = NULL to remove actual callback.
 *****************************************************************************/
errorCode_t neardal_set_cb_record_found(record_cb cb_rcd_found,
					void *user_data)
{
	neardalMgr.cb_rcd_found		= cb_rcd_found;
	neardalMgr.cb_rcd_found_ud	= user_data;

	return NEARDAL_SUCCESS;
}

/******************************************************************************
 * neardal_free_array: free adapters array, targets array or records array
 *****************************************************************************/
errorCode_t neardal_free_array(char ***array)
{
	errorCode_t	err = NEARDAL_SUCCESS;
	char		**adps;

	if (array == NULL)
		return NEARDAL_ERROR_INVALID_PARAMETER;

	adps = *array;
	while ((*adps) != NULL) {
		g_free(*adps);
		adps++;
	}
	g_free(*array);
	*array = NULL;

	return err;
}


/******************************************************************************
 * neardal_start_poll: Request Neard to start polling
 *****************************************************************************/
errorCode_t neardal_start_poll(char *adpName)
{
	errorCode_t	err		= NEARDAL_SUCCESS;
	AdpProp		*adpProp	= NULL;

	if (neardalMgr.proxy == NULL)
		neardal_prv_construct(&err);
	if (err != NEARDAL_SUCCESS)
		return err;

	err = neardal_mgr_prv_get_adapter(adpName, &adpProp);

	err = NEARDAL_ERROR_NO_ADAPTER;
	if (adpProp == NULL)
		goto exit;

	if (adpProp->proxy == NULL)
		goto exit;

	if (!adpProp->polling) {
		org_neard_adp__call_start_poll_sync(adpProp->proxy, NULL,
						    &neardalMgr.gerror);

		err = NEARDAL_SUCCESS;
		if (neardalMgr.gerror != NULL) {
			NEARDAL_TRACE_ERR(
				"Error with neard dbus method (err:%d:'%s')\n"
					, neardalMgr.gerror->code
					, neardalMgr.gerror->message);
			err = NEARDAL_ERROR_DBUS_INVOKE_METHOD_ERROR;
			neardal_tools_prv_free_gerror(&neardalMgr);
		}
	} else
		err = NEARDAL_ERROR_POLLING_ALREADY_ACTIVE;

exit:
	return err;
}

/******************************************************************************
 * neardal_stop_poll: Request Neard to stop polling
 *****************************************************************************/
errorCode_t neardal_stop_poll(char *adpName)
{
	errorCode_t	err = NEARDAL_SUCCESS;
	AdpProp		*adpProp	= NULL;

	if (neardalMgr.proxy == NULL)
		neardal_prv_construct(&err);

	if (err == NEARDAL_SUCCESS)
		err = neardal_mgr_prv_get_adapter(adpName, &adpProp);

	if (adpProp == NULL)
		goto exit;

	if (adpProp->proxy == NULL)
		goto exit;

	if (adpProp->polling) {
		org_neard_adp__call_stop_poll_sync(adpProp->proxy, NULL,
						   &neardalMgr.gerror);

		err = NEARDAL_SUCCESS;
		if (neardalMgr.gerror != NULL) {
			NEARDAL_TRACE_ERR(
				"Error with neard dbus method (err:%d:'%s')\n"
					, neardalMgr.gerror->code
					, neardalMgr.gerror->message);
			err = NEARDAL_ERROR_DBUS_INVOKE_METHOD_ERROR;
			neardal_tools_prv_free_gerror(&neardalMgr);
		}
	}

exit:
	return err;
}

/******************************************************************************
 * neardal_publish: Write NDEF record to an NFC tag
 *****************************************************************************/
errorCode_t neardal_publish(neardal_record *record)
{
	errorCode_t	err	= NEARDAL_SUCCESS;
	AdpProp		*adpProp;
	RcdProp		rcd;


	if (neardalMgr.proxy == NULL)
		neardal_prv_construct(&err);

	if (err != NEARDAL_SUCCESS || record == NULL)
		goto exit;

	err = neardal_mgr_prv_get_adapter((gchar *) record->name, &adpProp);
	if (err != NEARDAL_SUCCESS)
		goto exit;
	rcd.name		= (gchar *) record->name;
	rcd.action		= (gchar *) record->action;
	rcd.encoding		= (gchar *) record->encoding;
	rcd.language		= (gchar *) record->language;
	rcd.type		= (gchar *) record->type;
	rcd.representation	= (gchar *) record->representation;
	rcd.uri			= (gchar *) record->uri;
	rcd.uriObjSize		= record->uriObjSize;
	rcd.mime		= (gchar *) record->mime;

	 neardal_adp_publish(adpProp, &rcd);
exit:
	return err;
}

/******************************************************************************
 * neardal_get_adapter_properties: Get properties of a specific NEARDAL adapter
 *****************************************************************************/
errorCode_t neardal_get_adapter_properties(const char *adpName,
					   neardal_adapter *adapter)
{
	errorCode_t	err		= NEARDAL_SUCCESS;
	AdpProp		*adpProp	= NULL;
	TgtProp		*target		= NULL;
	int		ct		= 0;	/* counter */
	gsize		size;

	if (neardalMgr.proxy == NULL)
		neardal_prv_construct(&err);

	if (err != NEARDAL_SUCCESS || adpName == NULL || adapter == NULL)
		goto exit;

	err = neardal_mgr_prv_get_adapter((gchar *) adpName, &adpProp);
	if (err != NEARDAL_SUCCESS)
		goto exit;

	adapter->name		= (char *) adpProp->name;
	adapter->polling	= (short) adpProp->polling;
	adapter->powered	= (short) adpProp->powered;

	adapter->nbProtocols	= adpProp->lenProtocols;
	adapter->protocols	= NULL;


	if (adapter->nbProtocols > 0) {
		err = NEARDAL_ERROR_NO_MEMORY;
		size = (adapter->nbProtocols + 1) * sizeof(char *);
		adapter->protocols = g_try_malloc0(size);
		if (adapter->protocols != NULL) {
			ct = 0;
			while (ct < adapter->nbProtocols) {
				gchar *tmp = g_strdup(adpProp->protocols[ct]);
				adapter->protocols[ct++] = (char *) tmp;
			}
			err = NEARDAL_SUCCESS;
		}
	}

	adapter->nbTargets	= (int) adpProp->tgtNb;
	adapter->targets	= NULL;
	if (adapter->nbTargets <= 0)
		goto exit;

	err = NEARDAL_ERROR_NO_MEMORY;
	size = (adapter->nbTargets + 1) * sizeof(char *);
	adapter->targets = g_try_malloc0(size);
	if (adapter->targets == NULL)
		goto exit;

	ct = 0;
	while (ct < adapter->nbTargets) {
		target = g_list_nth_data(adpProp->tgtList, ct);
		if (target != NULL)
			adapter->targets[ct++] = g_strdup(target->name);
	}
	err = NEARDAL_SUCCESS;

exit:
	return err;
}

/******************************************************************************
 * neardal_get_adapter_properties: Get properties of a specific NEARDAL adapter
 *****************************************************************************/
errorCode_t neardal_get_target_properties(const char *tgtName,
					  neardal_target *target)
{
	errorCode_t	err		= NEARDAL_SUCCESS;
	AdpProp		*adpProp	= NULL;
	TgtProp		*tgtProp	= NULL;
	int		ct		= 0;	/* counter */
	RcdProp		*record		= NULL;
	gsize		size;

	if (neardalMgr.proxy == NULL)
		neardal_prv_construct(&err);

	if (err != NEARDAL_SUCCESS || tgtName == NULL || target == NULL)
		goto exit;

	target->records	= NULL;
	target->tagType	= NULL;
	err = neardal_mgr_prv_get_adapter((gchar *) tgtName, &adpProp);
	if (err != NEARDAL_SUCCESS)
		goto exit;

	err = neardal_mgr_prv_get_target(adpProp, (gchar *) tgtName, &tgtProp);
	if (err != NEARDAL_SUCCESS)
		goto exit;

	target->name		= (const char *) tgtProp->name;
	target->type		= (const char *) tgtProp->type;
	target->readOnly	= (short) tgtProp->readOnly;
	target->nbRecords	= (int) tgtProp->rcdLen;
	if (target->nbRecords > 0) {
		err = NEARDAL_ERROR_NO_MEMORY;
		size = (target->nbRecords + 1) * sizeof(char *);
		target->records = g_try_malloc0(size);
		if (target->records == NULL)
			goto exit;

		ct = 0;
		while (ct < target->nbRecords) {
			record = g_list_nth_data(tgtProp->rcdList, ct);
			if (record != NULL)
				target->records[ct++] = g_strdup(record->name);
		}
		err = NEARDAL_SUCCESS;
	}

	target->nbTagTypes = 0;
	target->tagType = NULL;
	/* Count TagTypes */
	target->nbTagTypes = (int) tgtProp->tagTypeLen;

	if (target->nbTagTypes <= 0)
		goto exit;

	err = NEARDAL_ERROR_NO_MEMORY;
	size = (target->nbTagTypes + 1) * sizeof(char *);
	target->tagType = g_try_malloc0(size);
	if (target->tagType == NULL)
		goto exit;

	ct = 0;
	while (ct < target->nbTagTypes) {
		target->tagType[ct] = g_strdup(tgtProp->tagType[ct]);
		ct++;
	}
	err = NEARDAL_SUCCESS;

exit:
	return err;
}

 /******************************************************************************
 * neardal_get_record_properties: Get values of a specific target record
  *****************************************************************************/
errorCode_t neardal_get_record_properties(const char *recordName,
					  neardal_record *record)
{
	errorCode_t	err		= NEARDAL_SUCCESS;
	AdpProp		*adpProp	= NULL;
	TgtProp		*tgtProp	= NULL;
	RcdProp		*rcdProp	= NULL;

	if (recordName == NULL || record == NULL)
		goto exit;

	if (neardalMgr.proxy == NULL)
		neardal_prv_construct(&err);

	if (err != NEARDAL_SUCCESS)
		goto exit;

	err = neardal_mgr_prv_get_adapter((gchar *) recordName, &adpProp);
	if (err != NEARDAL_SUCCESS)
		goto exit;

	err = neardal_mgr_prv_get_target(adpProp, (gchar *) recordName,
					 &tgtProp);
	if (err != NEARDAL_SUCCESS)
		goto exit;

	err = neardal_mgr_prv_get_record(tgtProp, (gchar *) recordName,
					 &rcdProp);
	if (err != NEARDAL_SUCCESS)
		goto exit;

	record->name		= (const char *) rcdProp->name;
	record->encoding	= (const char *) rcdProp->encoding;
	record->language	= (const char *) rcdProp->language;
	record->action		= (const char *) rcdProp->action;

	record->type		= (const char *) rcdProp->type;
	record->representation	= (const char *) rcdProp->representation;
	record->uri		= (const char *) rcdProp->uri;
	record->uriObjSize	= (uint) rcdProp->uriObjSize;
	record->mime		= (const char *) rcdProp->mime;

exit:
	return err;
}