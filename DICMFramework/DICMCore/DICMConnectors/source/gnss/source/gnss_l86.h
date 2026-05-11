#ifndef GNSS_L86_H_
#define GNSS_L86_H_

#include "configuration.h"

typedef enum {
	GNSS_RMC = 0,
	GNSS_VTG,
	GNSS_GGA,
	GNSS_GSA,
	GNSS_GSV,
	GNSS_GLL,
	GNSS_GPTXT,
	GNSS_PMTK,
} gnss_l86_message_type_t;

typedef struct {
	char *utc_time;
	char *data_valid;
	char *latitude;
	char *latitude_ns;
	char *longitude;
	char *longitude_ew;
	char *speed;
	char *cog;
	char *date;
	char *positioning_mode;
	char *nav_status;
} gnss_l86_message_rmc_t;

typedef struct {
	char *utc_time;
	char *latitude;
	char *latitude_ns;
	char *longitude;
	char *longitude_ew;
	char *fix_status;
	char *number_of_sv;
	char *hdop;
	char *altitude;
	char *geoid_height;
	char *dgps_age;
	char *dgps_station_id;
} gnss_l86_message_gga_t;

typedef struct {
	char *mode;
	char *fix_status;
	char *satellite[12];
	char *pdop;
	char *hdop;
	char *vdop;
	char *gnss_system_id;
} gnss_l86_message_gsa_t;

typedef struct {
	gnss_l86_message_type_t type;
	union {
		gnss_l86_message_rmc_t rmc;
		gnss_l86_message_gga_t gga;
		gnss_l86_message_gsa_t gsa;
	};
} gnss_l86_message_t;

typedef void (*gnss_l86_message_callback_t)(const gnss_l86_message_t *msg);

void gnss_l86_init(gnss_l86_message_callback_t callback);
int gnss_l86_reset(int level);
int gnss_l86_get_fix_interval(int *interval_ms);
int gnss_l86_set_fix_interval(int interval_ms);
int gnss_l86_power_on(void);
int gnss_l86_power_alwayslocate(void);
int gnss_l86_power_save(void);

#endif /* GNSS_L86_H_ */
