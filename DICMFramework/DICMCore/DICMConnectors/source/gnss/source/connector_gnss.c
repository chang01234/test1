/*! \file connector_gnss.c
	\brief GNSS connector
*/

#include "configuration.h"

#ifdef CONNECTOR_GNSS

#include "connector_gnss.h"
#include "gnss_l86.h"
#include <string.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "ddm2_parameter_list.h"
#include "ddm_wrapper.h"

static int initialize_connector_gnss(void);
static void connector_gnss_task(uint16_t elapsed);
static void gnss_update_callback(const gnss_l86_message_t *message);
static void set_str(ddmw_item_t *item, const char *str);
static void set_pos_str(ddmw_item_t *item, const char *pos_str, const char *dir_str);
#ifdef CONNECTOR_GNSS_SET_BTS0
static void set_pos_i32(ddmw_item_t *item, const char *pos_str, const char *dir_str);
#endif

typedef struct {
	ddmw_t ddm_wrapper;

	struct {
		ddmw_item_t avl;	// Available
		ddmw_item_t lat;	// Latitude
		ddmw_item_t lon;	// Longitude
		ddmw_item_t hdop;	// Horizontal dilution of precision
		ddmw_item_t alt;	// Altitude
		ddmw_item_t fix;	// Fix status
		ddmw_item_t cog;	// Course over ground
		ddmw_item_t spkn;	// Speed
		ddmw_item_t date;	// Date
		ddmw_item_t numsat;	// Number of satellites
#ifdef CONNECTOR_GNSS_SET_BTS0
		ddmw_item_t bts0avl; // Available
		ddmw_item_t bts0lat; // Latitude
		ddmw_item_t bts0lng; // Longitude
#endif
	} ddm_param;
} connector_data_t;

static EXT_RAM_ATTR connector_data_t connector_data;

CONNECTOR connector_gnss=
{
	.name="GNSS connector",
	.initialize = initialize_connector_gnss,
	.task_run   = connector_gnss_task
};


static int initialize_connector_gnss(void)
{
#ifdef CONNECTOR_GNSS_SET_BTS0
	int cfg;
#endif
	memset(&connector_data, 0, sizeof(connector_data));

	ddmw_init(&connector_data.ddm_wrapper, &connector_gnss);
	int instance = ddmw_register(&connector_data.ddm_wrapper, GNSS0AVL);
	ddmw_add_i32(&connector_data.ddm_wrapper, &connector_data.ddm_param.avl,    GNSS0AVL,    instance, 1);
	ddmw_add_str(&connector_data.ddm_wrapper, &connector_data.ddm_param.lat,    GNSS0LAT,    instance, "");
	ddmw_add_str(&connector_data.ddm_wrapper, &connector_data.ddm_param.lon,    GNSS0LON,    instance, "");
	ddmw_add_str(&connector_data.ddm_wrapper, &connector_data.ddm_param.hdop,   GNSS0HDOP,   instance, "");
	ddmw_add_str(&connector_data.ddm_wrapper, &connector_data.ddm_param.alt,    GNSS0ALT,    instance, "");
	ddmw_add_str(&connector_data.ddm_wrapper, &connector_data.ddm_param.fix,    GNSS0FIX,    instance, "");
	ddmw_add_str(&connector_data.ddm_wrapper, &connector_data.ddm_param.cog,    GNSS0COG,    instance, "");
	ddmw_add_str(&connector_data.ddm_wrapper, &connector_data.ddm_param.spkn,   GNSS0SPKN,   instance, "");
	ddmw_add_str(&connector_data.ddm_wrapper, &connector_data.ddm_param.date,   GNSS0DATE,   instance, "");
	ddmw_add_str(&connector_data.ddm_wrapper, &connector_data.ddm_param.numsat, GNSS0NUMSAT, instance, "");
#ifdef CONNECTOR_GNSS_SET_BTS0
	cfg = read_multibroker_cfg();
	if ((cfg & CFG_MB_STANDALONE) == CFG_MB_STANDALONE)
	{
		instance = ddmw_register(&connector_data.ddm_wrapper, BTS0AVL);
		ddmw_add_i32(&connector_data.ddm_wrapper, &connector_data.ddm_param.bts0avl, BTS0AVL, instance, 1);
	}
	else
	{
		instance = 0;
	}
	ddmw_add_i32(&connector_data.ddm_wrapper, &connector_data.ddm_param.bts0lat, BTS0LAT, instance, 0);
	ddmw_add_i32(&connector_data.ddm_wrapper, &connector_data.ddm_param.bts0lng, BTS0LNG, instance, 0);
#endif

	gnss_l86_init(gnss_update_callback);

	return 1;
}

/**********************************************************
 * Function:    connector_gnss_task
 * Description: Run connector task
 *********************************************************/
static void connector_gnss_task(uint16_t elapsed)
{
	ddmw_process(&connector_data.ddm_wrapper);
}

static void gnss_update_callback(const gnss_l86_message_t *message)
{
	switch (message->type)
	{
		case GNSS_RMC:
			set_str(&connector_data.ddm_param.cog, message->rmc.cog);
			set_str(&connector_data.ddm_param.spkn, message->rmc.speed);
			set_str(&connector_data.ddm_param.date, message->rmc.date);
			break;

		case GNSS_GGA:
			set_pos_str(&connector_data.ddm_param.lat, message->gga.latitude, message->gga.latitude_ns);
			set_pos_str(&connector_data.ddm_param.lon, message->gga.longitude, message->gga.longitude_ew);
			set_str(&connector_data.ddm_param.hdop, message->gga.hdop);
			set_str(&connector_data.ddm_param.alt, message->gga.altitude);
			set_str(&connector_data.ddm_param.numsat, message->gga.number_of_sv);
#ifdef CONNECTOR_GNSS_SET_BTS0
			set_pos_i32(&connector_data.ddm_param.bts0lat, message->gga.latitude, message->gga.latitude_ns);
			set_pos_i32(&connector_data.ddm_param.bts0lng, message->gga.longitude, message->gga.longitude_ew);
#endif
			break;

		case GNSS_GSA:
			set_str(&connector_data.ddm_param.fix, message->gsa.fix_status);
			break;

		default:
			break;
	}
}

static void set_str(ddmw_item_t *item, const char *str)
{
	if (str != NULL)
	{
		ddmw_set_str(item, str);
	}
}

static void set_pos_str(ddmw_item_t *item, const char *pos_str, const char *dir_str)
{
	char buf[16];
	double pos = 0.0;
	double dec_part;
	int int_part;

	if ((pos_str == NULL) || (dir_str == NULL))
	{
		return;
	}

	if (strlen(pos_str) == 0)
	{
		return;
	}

	// L86 outputs position in degrees and decimal minutes format (dddmm.mmmm). Converts it to decimal degrees.

	pos = atof(pos_str) / 100.0;

	int_part = (int)pos;
	dec_part = pos - int_part;
	dec_part /= 60.0;
	dec_part *= 100.0;

	pos = int_part + dec_part;

	if ((strcmp("N", dir_str) == 0) || (strcmp("E", dir_str) == 0))
	{
		snprintf(buf, sizeof(buf), "%.6f", pos);
		ddmw_set_str(item, buf);
	}
	else if ((strcmp("S", dir_str) == 0) || (strcmp("W", dir_str) == 0))
	{
		snprintf(buf, sizeof(buf), "%.6f", -pos);
		ddmw_set_str(item, buf);
	}
	else
	{
		ddmw_set_str(item, "");
	}
}
#ifdef CONNECTOR_GNSS_SET_BTS0
static void set_pos_i32(ddmw_item_t *item, const char *pos_str, const char *dir_str)
{
	double pos = 0.0;
	double dec_part;
	int int_part;

	if ((pos_str == NULL) || (dir_str == NULL))
	{
		return;
	}

	if (strlen(pos_str) == 0)
	{
		return;
	}

	// L86 outputs position in degrees and decimal minutes format (dddmm.mmmm). Converts it to decimal degrees.

	pos = atof(pos_str) / 100.0;

	int_part = (int)pos;
	dec_part = pos - int_part;
	dec_part /= 60.0;
	dec_part *= 100.0;

	pos = int_part + dec_part;

	if ((strcmp("N", dir_str) == 0) || (strcmp("E", dir_str) == 0))
	{
		ddmw_set_i32(item, (int32_t)(pos * 10000000.0 + 0.5));
	}
	else if ((strcmp("S", dir_str) == 0) || (strcmp("W", dir_str) == 0))
	{
		ddmw_set_i32(item, -(int32_t)(pos * 10000000.0 + 0.5));
	}
}
#endif

#endif
