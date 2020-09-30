/*
 * Deeplu Excursion parsing
 *
 * Copyright (C) 2020 Ryan Gardner
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

// TODO - implement this. It's LITERALLY a copy/paste of deepblue_parser.c with a find / replace on the names

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "deepsix.h"
#include "context-private.h"
#include "parser-private.h"
#include "array.h"
#include "field-cache.h"

#define C_ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))

#define MAXFIELDS 128

struct msg_desc;

typedef struct deepsix_parser_t {
    dc_parser_t base;

    dc_sample_callback_t callback;
    void *userdata;

    // 20 sec for scuba, 1 sec for freedives
    int sample_interval;

    // Common fields
    struct dc_field_cache cache;
} deepsix_parser_t;

static dc_status_t deepsix_parser_set_data (dc_parser_t *abstract, const unsigned char *data, unsigned int size);
static dc_status_t deepsix_parser_get_datetime (dc_parser_t *abstract, dc_datetime_t *datetime);
static dc_status_t deepsix_parser_get_field (dc_parser_t *abstract, dc_field_type_t type, unsigned int flags, void *value);
static dc_status_t deepsix_parser_samples_foreach (dc_parser_t *abstract, dc_sample_callback_t callback, void *userdata);

static const dc_parser_vtable_t deepsix_parser_vtable = {
        sizeof(deepsix_parser_t),
        DC_FAMILY_DEEPSIX,
        deepsix_parser_set_data, /* set_data */
        deepsix_parser_get_datetime, /* datetime */
        deepsix_parser_get_field, /* fields */
        deepsix_parser_samples_foreach, /* samples_foreach */
        NULL /* destroy */
};

dc_status_t
deepsix_parser_create (dc_parser_t **out, dc_context_t *context)
{
    deepsix_parser_t *parser = NULL;

    if (out == NULL)
        return DC_STATUS_INVALIDARGS;

    // Allocate memory.
    parser = (deepsix_parser_t *) dc_parser_allocate (context, &deepsix_parser_vtable);
    if (parser == NULL) {
        ERROR (context, "Failed to allocate memory.");
        return DC_STATUS_NOMEMORY;
    }

    *out = (dc_parser_t *) parser;

    return DC_STATUS_SUCCESS;
}

static double
pressure_to_depth(unsigned int mbar)
{
    // Specific weight of seawater (millibar to cm)
    const double specific_weight = 1.024 * 0.980665;

    // Absolute pressure, subtract surface pressure
    if (mbar < 1013)
        return 0.0;
    mbar -= 1013;
    return mbar / specific_weight / 100.0;
}

static dc_status_t
deepsix_parser_set_data (dc_parser_t *abstract, const unsigned char *data, unsigned int size)
{
    deepsix_parser_t *deepsix = (deepsix_parser_t *) abstract;
    const unsigned char *hdr = data;
    const unsigned char *profile = data + 256;
    unsigned int divetime, maxpressure;
    dc_gasmix_t gasmix = {0, };

    if (size < 256)
        return DC_STATUS_IO;

    deepsix->callback = NULL;
    deepsix->userdata = NULL;
    memset(&deepsix->cache, 0, sizeof(deepsix->cache));

    // LE16 at 0 is 'dive number'

    // LE16 at 12 is the dive time
    // It's in seconds for freedives, minutes for scuba/gauge
    divetime = hdr[12] + 256*hdr[13];

    // Byte at 2 is 'activity type' (2 = scuba, 3 = gauge, 4 = freedive)
    // Byte at 3 is O2 percentage
    switch (data[2]) {
        case 2:
            // SCUBA - divetime in minutes
            divetime *= 60;
            gasmix.oxygen = data[3] / 100.0;
            DC_ASSIGN_IDX(deepsix->cache, GASMIX, 0, gasmix);
            DC_ASSIGN_FIELD(deepsix->cache, GASMIX_COUNT, 1);
            DC_ASSIGN_FIELD(deepsix->cache, DIVEMODE, DC_DIVEMODE_OC);
            break;
        case 3:
            // GAUGE - divetime in minutes
            divetime *= 60;
            DC_ASSIGN_FIELD(deepsix->cache, DIVEMODE, DC_DIVEMODE_GAUGE);
            break;
        case 4:
            // FREEDIVE - divetime in seconds
            DC_ASSIGN_FIELD(deepsix->cache, DIVEMODE, DC_DIVEMODE_FREEDIVE);
            deepsix->sample_interval = 1;
            break;
        default:
            ERROR (abstract->context, "DeepSix: unknown activity type '%02x'", data[2]);
            break;
    }

    // Seems to be fixed at 20s for scuba, 1s for freedive
    deepsix->sample_interval = hdr[26];

    maxpressure = hdr[22] + 256*hdr[23];	// Maxpressure in millibar

    DC_ASSIGN_FIELD(deepsix->cache, DIVETIME, divetime);
    DC_ASSIGN_FIELD(deepsix->cache, MAXDEPTH, pressure_to_depth(maxpressure));

    return DC_STATUS_SUCCESS;
}

// The layout of the header in the 'data' is
//  0: LE16 dive number
//  2: dive type byte?
//  3: O2 percentage byte
//  4: unknown
//  5: unknown
//  6: LE16 year
//  8: day of month
//  9: month
// 10: minute
// 11: hour
// 12: LE16 dive time
// 14: LE16 ??
// 16: LE16 surface pressure?
// 18: LE16 ??
// 20: LE16 ??
// 22: LE16 max depth pressure
// 24: LE16 water temp
// 26: LE16 ??
// 28: LE16 ??
// 30: LE16 ??
// 32: LE16 ??
// 34: LE16 ??
static dc_status_t
deepsix_parser_get_datetime (dc_parser_t *abstract, dc_datetime_t *datetime)
{
    deepsix_parser_t *deepsix = (deepsix_parser_t *) abstract;
    const unsigned char *data = deepsix->base.data;
    int len = deepsix->base.size;

    if (len < 256)
        return DC_STATUS_IO;
    datetime->year = data[6] + (data[7] << 8);
    datetime->day = data[8];
    datetime->month = data[9];
    datetime->minute = data[10];
    datetime->hour = data[11];
    datetime->second = 0;
    datetime->timezone = DC_TIMEZONE_NONE;

    return DC_STATUS_SUCCESS;
}

static dc_status_t
deepsix_parser_get_field (dc_parser_t *abstract, dc_field_type_t type, unsigned int flags, void *value)
{
    deepsix_parser_t *deepsix = (deepsix_parser_t *) abstract;

    if (!value)
        return DC_STATUS_INVALIDARGS;

    /* This whole sequence should be standardized */
    if (!(deepsix->cache.initialized & (1 << type)))
        return DC_STATUS_UNSUPPORTED;

    switch (type) {
        case DC_FIELD_DIVETIME:
            return DC_FIELD_VALUE(deepsix->cache, value, DIVETIME);
        case DC_FIELD_MAXDEPTH:
            return DC_FIELD_VALUE(deepsix->cache, value, MAXDEPTH);
        case DC_FIELD_AVGDEPTH:
            return DC_FIELD_VALUE(deepsix->cache, value, AVGDEPTH);
        case DC_FIELD_GASMIX_COUNT:
        case DC_FIELD_TANK_COUNT:
            return DC_FIELD_VALUE(deepsix->cache, value, GASMIX_COUNT);
        case DC_FIELD_GASMIX:
            if (flags >= MAXGASES)
                return DC_STATUS_UNSUPPORTED;
            return DC_FIELD_INDEX(deepsix->cache, value, GASMIX, flags);
        case DC_FIELD_SALINITY:
            return DC_FIELD_VALUE(deepsix->cache, value, SALINITY);
        case DC_FIELD_ATMOSPHERIC:
            return DC_FIELD_VALUE(deepsix->cache, value, ATMOSPHERIC);
        case DC_FIELD_DIVEMODE:
            return DC_FIELD_VALUE(deepsix->cache, value, DIVEMODE);
        case DC_FIELD_TANK:
            return DC_STATUS_UNSUPPORTED;
        case DC_FIELD_STRING:
            return dc_field_get_string(&deepsix->cache, flags, (dc_field_string_t *)value);
        default:
            return DC_STATUS_UNSUPPORTED;
    }
    return DC_STATUS_SUCCESS;
}

static dc_status_t
deepsix_parser_samples_foreach (dc_parser_t *abstract, dc_sample_callback_t callback, void *userdata)
{
    deepsix_parser_t *deepsix = (deepsix_parser_t *) abstract;
    const unsigned char *data = deepsix->base.data;
    int len = deepsix->base.size, i;

    deepsix->callback = callback;
    deepsix->userdata = userdata;

    // Skip the header information
    if (len < 256)
        return DC_STATUS_IO;
    data += 256;
    len -= 256;

    // The rest should be samples every 20s with temperature and depth
    for (i = 0; i < len/4; i++) {
        dc_sample_value_t sample = {0};
        unsigned int temp = data[0]+256*data[1];
        unsigned int pressure = data[2]+256*data[3];

        data += 4;
        sample.time = (i+1)*deepsix->sample_interval;
        if (callback) callback (DC_SAMPLE_TIME, sample, userdata);

        sample.depth = pressure_to_depth(pressure);
        if (callback) callback (DC_SAMPLE_DEPTH, sample, userdata);

        sample.temperature = temp / 10.0;
        if (callback) callback (DC_SAMPLE_TEMPERATURE, sample, userdata);
    }

    return DC_STATUS_SUCCESS;
}
