/*
 * libdivecomputer
 *
 * Copyright (C) 2008 Jef Driesen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301 USA
 */

#include <stdlib.h>
#include <assert.h>

#include "suunto_d9.h"
#include "suunto_eon.h"
#include "suunto_eonsteel.h"
#include "suunto_solution.h"
#include "suunto_vyper2.h"
#include "suunto_vyper.h"
#include "reefnet_sensus.h"
#include "reefnet_sensuspro.h"
#include "reefnet_sensusultra.h"
#include "uwatec_aladin.h"
#include "uwatec_memomouse.h"
#include "uwatec_smart.h"
#include "oceanic_atom2.h"
#include "oceanic_atom2.h"
#include "oceanic_veo250.h"
#include "oceanic_vtpro.h"
#include "mares_darwin.h"
#include "mares_iconhd.h"
#include "mares_nemo.h"
#include "mares_puck.h"
#include "hw_frog.h"
#include "hw_ostc.h"
#include "hw_ostc3.h"
#include "cressi_edy.h"
#include "cressi_leonardo.h"
#include "cressi_goa.h"
#include "zeagle_n2ition3.h"
#include "atomics_cobalt.h"
#include "shearwater_petrel.h"
#include "shearwater_predator.h"
#include "diverite_nitekq.h"
#include "citizen_aqualand.h"
#include "divesystem_idive.h"
#include "cochran_commander.h"
#include "tecdiving_divecomputereu.h"
#include "mclean_extreme.h"
#include "liquivision_lynx.h"

// Not merged upstream yet
#include "garmin.h"
#include "deepblu.h"
#include "oceans_s1.h"
#include "deepsix.h"

#include "context-private.h"
#include "parser-private.h"
#include "device-private.h"

#define REACTPROWHITE 0x4354

static dc_status_t
dc_parser_new_internal (dc_parser_t **out, dc_context_t *context, dc_family_t family, unsigned int model, unsigned int serial, unsigned int devtime, dc_ticks_t systime)
{
	dc_status_t rc = DC_STATUS_SUCCESS;
	dc_parser_t *parser = NULL;

	if (out == NULL)
		return DC_STATUS_INVALIDARGS;

	switch (family) {
	case DC_FAMILY_SUUNTO_SOLUTION:
		rc = suunto_solution_parser_create (&parser, context);
		break;
	case DC_FAMILY_SUUNTO_EON:
		rc = suunto_eon_parser_create (&parser, context, 0);
		break;
	case DC_FAMILY_SUUNTO_VYPER:
		if (model == 0x01)
			rc = suunto_eon_parser_create (&parser, context, 1);
		else
			rc = suunto_vyper_parser_create (&parser, context);
		break;
	case DC_FAMILY_SUUNTO_VYPER2:
	case DC_FAMILY_SUUNTO_D9:
		rc = suunto_d9_parser_create (&parser, context, model, serial);
		break;
	case DC_FAMILY_SUUNTO_EONSTEEL:
		rc = suunto_eonsteel_parser_create(&parser, context, model);
		break;
	case DC_FAMILY_UWATEC_ALADIN:
	case DC_FAMILY_UWATEC_MEMOMOUSE:
		rc = uwatec_memomouse_parser_create (&parser, context, devtime, systime);
		break;
	case DC_FAMILY_UWATEC_SMART:
		rc = uwatec_smart_parser_create (&parser, context, model, devtime, systime);
		break;
	case DC_FAMILY_REEFNET_SENSUS:
		rc = reefnet_sensus_parser_create (&parser, context, devtime, systime);
		break;
	case DC_FAMILY_REEFNET_SENSUSPRO:
		rc = reefnet_sensuspro_parser_create (&parser, context, devtime, systime);
		break;
	case DC_FAMILY_REEFNET_SENSUSULTRA:
		rc = reefnet_sensusultra_parser_create (&parser, context, devtime, systime);
		break;
	case DC_FAMILY_OCEANIC_VTPRO:
		rc = oceanic_vtpro_parser_create (&parser, context, model);
		break;
	case DC_FAMILY_OCEANIC_VEO250:
		rc = oceanic_veo250_parser_create (&parser, context, model);
		break;
	case DC_FAMILY_OCEANIC_ATOM2:
		if (model == REACTPROWHITE)
			rc = oceanic_veo250_parser_create (&parser, context, model);
		else
			rc = oceanic_atom2_parser_create (&parser, context, model, serial);
		break;
	case DC_FAMILY_MARES_NEMO:
	case DC_FAMILY_MARES_PUCK:
		rc = mares_nemo_parser_create (&parser, context, model);
		break;
	case DC_FAMILY_MARES_DARWIN:
		rc = mares_darwin_parser_create (&parser, context, model);
		break;
	case DC_FAMILY_MARES_ICONHD:
		rc = mares_iconhd_parser_create (&parser, context, model);
		break;
	case DC_FAMILY_HW_OSTC:
		rc = hw_ostc_parser_create (&parser, context, serial);
		break;
	case DC_FAMILY_HW_FROG:
	case DC_FAMILY_HW_OSTC3:
		rc = hw_ostc3_parser_create (&parser, context, serial, model);
		break;
	case DC_FAMILY_CRESSI_EDY:
	case DC_FAMILY_ZEAGLE_N2ITION3:
		rc = cressi_edy_parser_create (&parser, context, model);
		break;
	case DC_FAMILY_CRESSI_LEONARDO:
		rc = cressi_leonardo_parser_create (&parser, context, model);
		break;
	case DC_FAMILY_CRESSI_GOA:
		rc = cressi_goa_parser_create (&parser, context, model);
		break;
	case DC_FAMILY_ATOMICS_COBALT:
		rc = atomics_cobalt_parser_create (&parser, context);
		break;
	case DC_FAMILY_SHEARWATER_PREDATOR:
		rc = shearwater_predator_parser_create (&parser, context, model, serial);
		break;
	case DC_FAMILY_SHEARWATER_PETREL:
		rc = shearwater_petrel_parser_create (&parser, context, model, serial);
		break;
	case DC_FAMILY_DIVERITE_NITEKQ:
		rc = diverite_nitekq_parser_create (&parser, context);
		break;
	case DC_FAMILY_CITIZEN_AQUALAND:
		rc = citizen_aqualand_parser_create (&parser, context);
		break;
	case DC_FAMILY_DIVESYSTEM_IDIVE:
		rc = divesystem_idive_parser_create (&parser, context, model);
		break;
	case DC_FAMILY_COCHRAN_COMMANDER:
		rc = cochran_commander_parser_create (&parser, context, model);
		break;
	case DC_FAMILY_TECDIVING_DIVECOMPUTEREU:
		rc = tecdiving_divecomputereu_parser_create (&parser, context);
		break;
	case DC_FAMILY_MCLEAN_EXTREME:
		rc = mclean_extreme_parser_create (&parser, context);
		break;
	case DC_FAMILY_LIQUIVISION_LYNX:
		rc = liquivision_lynx_parser_create (&parser, context, model);
		break;
	default:
		return DC_STATUS_INVALIDARGS;

	// Not merged upstream yet
	case DC_FAMILY_GARMIN:
		rc = garmin_parser_create (&parser, context);
		break;
	case DC_FAMILY_DEEPBLU:
		rc = deepblu_parser_create (&parser, context);
		break;
	case DC_FAMILY_OCEANS_S1:
		rc = oceans_s1_parser_create(&parser, context);
		break;
    case  DC_FAMILY_DEEPSIX:
        rc = deepsix_parser_create (&parser, context);
        break;
	}

	*out = parser;

	return rc;
}

dc_status_t
dc_parser_new (dc_parser_t **out, dc_device_t *device)
{
	if (device == NULL)
		return DC_STATUS_INVALIDARGS;

	return dc_parser_new_internal (out, device->context,
		dc_device_get_type (device),
		device->devinfo.model,
		device->devinfo.serial,
		device->clock.devtime, device->clock.systime);
}

dc_status_t
dc_parser_new2 (dc_parser_t **out, dc_context_t *context, dc_descriptor_t *descriptor, unsigned int devtime, dc_ticks_t systime)
{
	return dc_parser_new_internal (out, context,
		dc_descriptor_get_type (descriptor),
		dc_descriptor_get_model (descriptor),
		0,
		devtime, systime);
}

dc_parser_t *
dc_parser_allocate (dc_context_t *context, const dc_parser_vtable_t *vtable)
{
	dc_parser_t *parser = NULL;

	assert(vtable != NULL);
	assert(vtable->size >= sizeof(dc_parser_t));

	// Allocate memory.
	parser = (dc_parser_t *) malloc (vtable->size);
	if (parser == NULL) {
		ERROR (context, "Failed to allocate memory.");
		return parser;
	}

	// Initialize the base class.
	parser->vtable = vtable;
	parser->context = context;
	parser->data = NULL;
	parser->size = 0;

	return parser;
}

void
dc_parser_deallocate (dc_parser_t *parser)
{
	free (parser);
}

int
dc_parser_isinstance (dc_parser_t *parser, const dc_parser_vtable_t *vtable)
{
	if (parser == NULL)
		return 0;

	return parser->vtable == vtable;
}


dc_family_t
dc_parser_get_type (dc_parser_t *parser)
{
	if (parser == NULL)
		return DC_FAMILY_NULL;

	return parser->vtable->type;
}


dc_status_t
dc_parser_set_data (dc_parser_t *parser, const unsigned char *data, unsigned int size)
{
	if (parser == NULL)
		return DC_STATUS_UNSUPPORTED;

	if (parser->vtable->set_data == NULL)
		return DC_STATUS_UNSUPPORTED;

	parser->data = data;
	parser->size = size;

	return parser->vtable->set_data (parser, data, size);
}


dc_status_t
dc_parser_get_datetime (dc_parser_t *parser, dc_datetime_t *datetime)
{
	if (parser == NULL)
		return DC_STATUS_UNSUPPORTED;

	if (parser->vtable->datetime == NULL)
		return DC_STATUS_UNSUPPORTED;

	return parser->vtable->datetime (parser, datetime);
}

dc_status_t
dc_parser_get_field (dc_parser_t *parser, dc_field_type_t type, unsigned int flags, void *value)
{
	if (parser == NULL)
		return DC_STATUS_UNSUPPORTED;

	if (parser->vtable->field == NULL)
		return DC_STATUS_UNSUPPORTED;

	return parser->vtable->field (parser, type, flags, value);
}


dc_status_t
dc_parser_samples_foreach (dc_parser_t *parser, dc_sample_callback_t callback, void *userdata)
{
	if (parser == NULL)
		return DC_STATUS_UNSUPPORTED;

	if (parser->vtable->samples_foreach == NULL)
		return DC_STATUS_UNSUPPORTED;

	return parser->vtable->samples_foreach (parser, callback, userdata);
}


dc_status_t
dc_parser_destroy (dc_parser_t *parser)
{
	dc_status_t status = DC_STATUS_SUCCESS;

	if (parser == NULL)
		return DC_STATUS_SUCCESS;

	if (parser->vtable->destroy) {
		status = parser->vtable->destroy (parser);
	}

	dc_parser_deallocate (parser);

	return status;
}


void
sample_statistics_cb (dc_sample_type_t type, dc_sample_value_t value, void *userdata)
{
	sample_statistics_t *statistics  = (sample_statistics_t *) userdata;

	switch (type) {
	case DC_SAMPLE_TIME:
		statistics->divetime = value.time;
		break;
	case DC_SAMPLE_DEPTH:
		if (statistics->maxdepth < value.depth)
			statistics->maxdepth = value.depth;
		break;
	default:
		break;
	}
}
