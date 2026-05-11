/**
 * \brief DICM Unique ID generator, uses the ESP Random Generator library
 */

#include "uid_generator.h"

uint64_t dicm_generate_uid(void)
{
	uint64_t uid = 0;

	uint32_t uid_pt1 = esp_random();
	uint32_t uid_pt2 = esp_random();
	
	uid = ((uint64_t)uid_pt1 << 32) | (uint64_t)uid_pt2;

	return uid;
}

int dicm_generate_uid_str(char* hex_uid, size_t hex_uid_size)
{
	uint64_t uid = 0;
	int ret = 0;

	TRUE_CHECK_RETURNX(-1, hex_uid_size == DICM_UID_STR_LEN);

	uid = dicm_generate_uid();

	ret = snprintf(hex_uid, hex_uid_size, "%016llx", uid);

	return ret;
}

int dicm_generate_uid_key_str(char* hex_uid, size_t hex_uid_size)
{
	uint64_t uid = 0;
	int ret = 0;

	TRUE_CHECK_RETURNX(-1, hex_uid_size == DICM_UID_KEY_STR_LEN)

	uid = dicm_generate_uid();
	uint64_t uid_masked = uid & 0x00FFFFFFFFFFFFFF;
	
	ret = snprintf(hex_uid, hex_uid_size, "%015llx", uid_masked);

	return ret;
}


