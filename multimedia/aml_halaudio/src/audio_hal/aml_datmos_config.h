#ifndef _AML_DATMOS_CONFIG_H_
#define _AML_DATMOS_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_PARAM_COUNT 64
#define VALUE_BUF_SIZE  256
/*
 *@brief reset datmos default options
 * input params:
 *          none
 *
 * return value:
 *          void *: an pointer for datmos defautl config
 */
void *datmos_default_options();

/*
 *@brief get datmos current options
 * input params:
 *          none
 *
 * return value:
 *          void *: an pointer for datmos current config
 */
void *get_datmos_current_options();

/*
 *@brief add datmos options
 * input params:
 *          void *opt_handle: an pointer for datmos current config
            const char *key: config parameter key
            const char *value: config parameter value
 *
 * return value:
 *          none
 */
void add_datmos_option(void *opt_handle, const char *key, const char *value);

/*
 *@brief delete datmos options
 * input params:
 *          void *opt_handle: an pointer for datmos current config
            const char *key: config parameter key
 *
 * return value:
 *          none
 */
void delete_datmos_option(void *opt_handle, const char *key);

/*
 *@brief get datmos config
 * input params:
 *          void *opt_handle: an pointer for datmos current config
            char **init_argv: to store the argv of datmos
            int *init_argc: to store the argc of datmos
 *
 * return value:
 *          0: success
            others: fail
 */
bool get_datmos_config(void *opt_handle, char **init_argv, int *init_argc);
#ifdef __cplusplus
}
#endif

#endif//end of _AML_DATMOS_CONFIG_H_