/**
 * \brief DICM Unique ID generator, uses the ESP Random Generator library
 */

#ifndef UID_GENERATOR_H_
#define UID_GENERATOR_H_

#include "esp_random.h"
#include "configuration.h"

#define DICM_UID_STR_LEN (16+1)
#define DICM_UID_KEY_STR_LEN (15+1)

uint64_t dicm_generate_uid(void);
int dicm_generate_uid_str(char* hex_uid, size_t hex_uid_size);
int dicm_generate_uid_key_str(char* hex_uid, size_t hex_uid_size);
#endif /* UID_GENERATOR_H_ */
