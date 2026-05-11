/**
 * \file        load_configurations_rule_engine.c
 * \date        2022-09-27
 * \author      (BB) Borjan Bozhinovski <borjan.bozhinovski@seavus.com>
 * \brief       Implementation of Rule Engine's rules loading functionality.
 *
 * For more details please refer to the header file.
 *
 * \li          2022-09-27  (BB) Initial implementation
 *
 * \copyright   Dometic Group
 *              This source file and the information contained in it are
 *              confidential and proprietary to Dometic Group
 *              The reproduction or disclosure, in whole or in part,
 *              to anyone outside of Dometic Group without the written
 *              approval of a Dometic Group officer under a Non-Disclosure
 *              Agreement is expressly prohibited.
 *
 *              All rights reserved
 */

#include "load_configurations_rule_engine.h"
#include "rule_engine_def.h"

#if !defined(LOAD_CONFIGURATION_USE_JSON_CJSON) && !defined(LOAD_CONFIGURATION_USE_JSON_JSMN)
// default to CJSON
#define LOAD_CONFIGURATION_USE_JSON_CJSON
#endif
#if defined(LOAD_CONFIGURATION_USE_JSON_CJSON) && defined(LOAD_CONFIGURATION_USE_JSON_JSMN)
#error Cannot have both LOAD_CONFIGURATION_USE_JSON_CJSON and LOAD_CONFIGURATION_USE_JSON_JSMN defined
#endif

#ifdef LOAD_CONFIGURATION_USE_JSON_CJSON
#include "cJSON.h"
#else
#include "jsmn.h"
#endif
#include "esp_heap_caps.h"
#include <sys/stat.h>

// #define LOAD_CONFIGURATION_PRINT_JSON // uncomment to print all rules
#ifdef LOAD_CONFIGURATION_USE_JSON_JSMN
static int jsoneq(const char *json, jsmntok_t *tok, const char *s)
{
    if ((tok->type == JSMN_STRING) && ((int)strlen(s) == tok->end - tok->start) &&
        (strncmp(json + tok->start, s, tok->end - tok->start) == 0))
    {
        return 0;
    }
    return -1;
}
#endif
int load_rule_engine_configuration(rule_engine_inst_t *const p_rule_engine_inst, const struct load_configurations__configuration *config)
{
    int ret_value = 0;

    switch (config->configuration_location_type)
    {
    case LOAD_CONFIGURATION__LOCATION_ROM:
    {
        for (uint32_t n_rule = 0; n_rule < config->static_config->configuration_size; ++n_rule)
        {
            const struct rule_engine__rule *rule = &(((struct rule_engine__rule *)config->static_config->configuration)[n_rule]);
            if (rule)
            {
                rule_engine__add_specification(p_rule_engine_inst, rule);
            }
        }

        break;
    }
    case LOAD_CONFIGURATION__LOCATION_FILE_SYSTEM:
    {
        // Load json file from file system. Filename is the config
        struct stat st;
        if (stat(config->static_config->configuration, &st) != 0)
        {
            LOG(W, "Could not find configuration file: %s", (char *)config->static_config->configuration);
            ret_value = -1;
        }
        else
        {
            LOG(I, "Trying to load configuration file: %s (%ld bytes)", (char *)config->static_config->configuration, st.st_size);
            FILE *f = fopen((char *)config->static_config->configuration, "r");
            if (f == NULL)
            {
                LOG(W, "Could not open configuration file: %s", (char *)config->static_config->configuration);
                ret_value = -1;
            }
            else
            {
#ifdef LOAD_CONFIGURATION_USE_JSON_CJSON
                cJSON *root = NULL;
                char *buffer = heap_caps_malloc_prefer(st.st_size + 4, 2, MALLOC_CAP_DEFAULT | MALLOC_CAP_SPIRAM, MALLOC_CAP_DEFAULT | MALLOC_CAP_INTERNAL);
                assert(buffer != NULL);
                size_t read_bytes = fread(buffer, 1, st.st_size, f);
                LOG(D, "Read %d bytes", read_bytes);
                fclose(f);
                root = cJSON_ParseWithLength(buffer, read_bytes);
                // LOG(I,"After cJSON_Parse Heap left: %d, stack left: %d", esp_get_free_heap_size(), uxTaskGetStackHighWaterMark(NULL));
                if (root != NULL)
                {
                    cJSON *ruleArrayObj = cJSON_GetObjectItemCaseSensitive(root, "rules");
                    if (ruleArrayObj != NULL)
                    {
                        // "rule" item must be an array of "objects"
                        if (cJSON_IsArray(ruleArrayObj))
                        {
                            int num_childs = cJSON_GetArraySize(ruleArrayObj);
                            int active_child = 0;
                            while (num_childs > 0)
                            {
                                cJSON *childObj = cJSON_GetArrayItem(ruleArrayObj, 0);
#if defined(LOAD_CONFIGURATION_PRINT_JSON)
                                char *temp = cJSON_Print(childObj);
                                LOG(D, "%s", temp);
                                cJSON_free(temp);
#endif

                                struct rule_engine__rule rule;
                                // Get "name"
                                cJSON *nameObj = cJSON_GetObjectItemCaseSensitive(childObj, "name");
                                memset(rule.name, '\0', sizeof(rule.name));
                                strncpy(rule.name, nameObj->valuestring, RULE_ENGINE__RULE_NAME_LEN - 1);
                                // Get "rule"
                                cJSON *ruleObj = cJSON_GetObjectItemCaseSensitive(childObj, "rule");
                                rule.rule = ruleObj->valuestring;
                                rule.size = strlen(ruleObj->valuestring);
                                rule_engine__add_specification(p_rule_engine_inst, &rule);
                                // LOG(I,"After add spec (%d) Heap left: %d, stack left: %d", active_child, esp_get_free_heap_size(), uxTaskGetStackHighWaterMark(NULL));
                                cJSON_DeleteItemFromArray(ruleArrayObj, 0);
                                num_childs = cJSON_GetArraySize(ruleArrayObj);
                                // LOG(I,"After delete array item (left: %d) Heap left: %d", num_childs, esp_get_free_heap_size());
                                active_child++;
                            }
                        }
                        else
                        {
                            LOG(D, "Error json format: \"rules\" is not an cJSON_IsArray");
                            ret_value = -1;
                        }
                    }
                    else
                    {
                        LOG(D, "Error json format: Could not find \"rules\" item");
                        ret_value = -1;
                    }
                    cJSON_Delete(root);
                }
                else
                {
                    LOG(W, "Error parsing json: %p (%p)", cJSON_GetErrorPtr(), buffer);
                    ret_value = -1;
                }
                heap_caps_free(buffer);
#else
                int i;
                int r;
                int n;

                jsmn_parser p;
                jsmntok_t *t;

                char *buffer = heap_caps_malloc_prefer(st.st_size + 4, 2, MALLOC_CAP_DEFAULT | MALLOC_CAP_SPIRAM, MALLOC_CAP_DEFAULT | MALLOC_CAP_INTERNAL);
                assert(buffer != NULL);
                size_t read_bytes = fread(buffer, 1, st.st_size, f);

                LOG(D, "Read %d bytes", read_bytes);
                fclose(f);
                // LOG(I,"After fclose() Heap left: %d, stack left: %d", esp_get_free_heap_size(), uxTaskGetStackHighWaterMark(NULL));

                jsmn_init(&p);
                n = jsmn_parse(&p, buffer, read_bytes, NULL, 0);
                if (n <= 0)
                {
                    LOG(E, "Failed to parse JSON: %d", n);
                    heap_caps_free(buffer);
                    return -1;
                }
                LOG(D, "Need to allocate %d tokens", n);
                jsmn_init(&p);
                t = heap_caps_malloc_prefer(sizeof(jsmntok_t) * n, 2, MALLOC_CAP_DEFAULT | MALLOC_CAP_SPIRAM, MALLOC_CAP_DEFAULT | MALLOC_CAP_INTERNAL);
                r = jsmn_parse(&p, buffer, read_bytes, t, n);
                if (r < 0)
                {
                    LOG(E, "Failed to parse JSON: %d", r);
                    heap_caps_free(buffer);
                    heap_caps_free(t);
                    return -1;
                }

                /* Assume the top-level element is an object */
                if (r < 1 || t[0].type != JSMN_OBJECT)
                {
                    LOG(E, "Object expected");
                    heap_caps_free(buffer);
                    heap_caps_free(t);
                    return -1;
                }
                LOG(D, "Num parsed json tokens: %d", r);

                /* Root object must be "rules" */
                if ((jsoneq(buffer, &t[1], "rules") == 0) && (t[1].type == JSMN_STRING))
                {
                    int j;
                    if (t[2].type != JSMN_ARRAY)
                    {
                        heap_caps_free(buffer);
                        heap_caps_free(t);
                        return -1; /* We expect "rules" to be an array of objects */
                    }
                    i = 3;
                    // For each rule expect objects
                    for (j = 0; j < t[2].size; j++)
                    {
                        if (i >= r)
                        {
                            LOG(E, "Parsing failed!");
                            ret_value = -1;
                            break;
                        }
                        if (t[i].type == JSMN_OBJECT)
                        {
                            struct rule_engine__rule rule;
                            memset(rule.name, '\0', sizeof(rule.name));
                            rule.size = 0;
#if defined(LOAD_CONFIGURATION_PRINT_JSON)
                            LOG I, ("%d [%d (%d) - %d %d] %.*s", i, t[i].type, t[i].size, t[i].start, t[i].end, t[i].end - t[i].start, buffer + t[i].start);
#endif
                            // size in a object means number of tokens
                            int object_size = t[i].size;
                            for (int k = 0; k < object_size; k++)
                            {
                                if ((jsoneq(buffer, &t[i + 1], "name") == 0) && (t[i + 1].type == JSMN_STRING))
                                {
                                    // Get "name"
                                    strncpy(rule.name, buffer + t[i + 2].start, MIN(RULE_ENGINE__RULE_NAME_LEN - 1, t[i + 2].end - t[i + 2].start));
                                    i += 2;
                                }
                                else if ((jsoneq(buffer, &t[i + 1], "rule") == 0) && (t[i + 1].type == JSMN_STRING))
                                {
                                    // Get "rule"
                                    rule.rule = buffer + t[i + 2].start;
                                    rule.size = t[i + 2].end - t[i + 2].start;
                                    i += 2;
                                }
                                else if ((jsoneq(buffer, &t[i + 1], "comment") == 0) && (t[i + 1].type == JSMN_STRING))
                                {
                                    i += 2;
                                }
                                else
                                {
                                    LOG(E, "Unexpected string %.*s", t[i + 1].end - t[i + 1].start, buffer + t[i + 1].start);
                                    ret_value = -1;
                                    break;
                                }
                            }
                            i++;
                            rule_engine__add_specification(p_rule_engine_inst, &rule);
                            LOG(I, "After add spec Heap left: %d, stack left: %d", esp_get_free_heap_size(), uxTaskGetStackHighWaterMark(NULL));
                        }
                    }
                }
                else
                {
                    ret_value = -1;
                }
                heap_caps_free(buffer);
                heap_caps_free(t);
#endif
            }
        }
        break;
    }
    default:
        ret_value = -1;
        break;
    }

    return ret_value;
}
