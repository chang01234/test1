#ifndef RULE_ENGINE_DEF_H
#define RULE_ENGINE_DEF_H

#include "rule_engine_exp_eval.h"

#define RULE_ENGINE__IS_TRIGGER_PARAM(mask, position) (((mask) & (1 << (position))))

#define RULE_ENGINE__SET_MASK(mask, position)           (((mask) |= (1 << (position))))
#define RULE_ENGINE__CLEAR_MASK(mask)                   ((mask) &= ~(0xFFFFFFFF))
#define RULE_ENGINE__HIGHEST_VAR_POSITION_IN_MASK(mask) ((CHAR_BIT * sizeof(uint32_t)) - __builtin_clz(mask))

#define RULE_ENGINE__EXPRESSION_TYPE_INSTANCE_RESOLUTION_INV      1
#define RULE_ENGINE__EXPRESSION_TYPE_INSTANCE_RESOLUTION_DYN_INST 2
#define RULE_ENGINE__EXPRESSION_TYPE_STANDARD                     3

#define RULE_ENGINE__MASK_CONSTRAINT_OR  0
#define RULE_ENGINE__MASK_CONSTRAINT_AND 1

#define RULE_ENGINE__VAR_TYPE_LOCAL         0
#define RULE_ENGINE__VAR_TYPE_GLOBAL        1
#define RULE_ENGINE__VAR_TYPE_DDM2          2
#define RULE_ENGINE__VAR_TYPE_DDM2_DYNAMIC  3
#define RULE_ENGINE__VAR_TYPE_DDM2_INSTANCE 4

#define RULE_ENGINE__VAR_TYPE_UNRESOLVED  0
#define RULE_ENGINE__VAR_TYPE_RESOLVED    1
#define RULE_ENGINE__VAR_TYPE_UNAPLICABLE 2

#define RULE_ENGINE__VAR_TYPE_ID_UNRESOLVED  7  // max value for 3 bits
#define RULE_ENGINE__VAR_TYTE_ID_UNAPLICABLE RULE_ENGINE__VAR_TYPE_ID_UNRESOLVED

#define RULE_ENGINE__VAR_TYPE_DDM2_CREATE_SUBSCRIBED   0  //!< \~ DDM2 variable is created as subscribed
#define RULE_ENGINE__VAR_TYPE_DDM2_CREATE_OWNED        1  //!< \~ DDM2 variable is created as owned
#define RULE_ENGINE__VAR_TYPE_DDM2_CREATE_UNAPPLICABLE 2  //!< \~ DDM2 variable is not applicable(GLOBAL and LOCAL variables)

#define RULE_ENGINE__VAR_VALUE_INIT 0

#define RULE_ENGINE__TIMER_TRIGG_EVENT 0

#define RULE_ENGINE__ABSOLUTE_TIMER 0
#define RULE_ENGINE__RELATIVE_TIMER 1

#define RULE_ENGINE__TIMER_OFF     0  //!< \~ Timer off
#define RULE_ENGINE__TIMER_ACTIVE  1  //!< \~ Timer on
#define RULE_ENGINE__TIMER_SET_ON  2  //!< \~ Timer ready to be turned on
#define RULE_ENGINE__TIMER_SET_OFF 3  //!< \~ Timer ready to be turned off

#if defined(CONNECTOR_SYSTEM)
#define RULE_ENGINE__NUM_USER_FUNCS 13
#else
#define RULE_ENGINE__NUM_USER_FUNCS 12
#endif

#define RULE_ENGINE__INV_PTR           0
#define RULE_ENGINE__OK                1
#define RULE_ENGINE__PARAM_NOT_EXIST   -1
#define RULE_ENGINE__SUBS_DEPTH        -2
#define RULE_ENGINE__RULES_DEPTH       -3
#define RULE_ENGINE__INV_RULE_FORMAT   -4
#define RULE_ENGINE__INV_MASK          -5
#define RULE_ENGINE__TIMER_NOT_CREATED -6
#define RULE_ENGINE__INV_FUNC_CONFIG   -7
#define RULE_ENGINE__INV_DYN_VAR_NAME  -8
#define RULE_ENGINE__INV_VAR_TYPE      -9
#define RULE_ENGINE__INV_RULE_TYPE     -10

typedef struct rule_engine__expression_type
{
    int type;
    struct expr *func;
} rule_engine__expression_type_t;

typedef struct rule_engine_func_context
{
    rule_engine_inst_t *p_rule_engine_inst;  //!< \~ Pointer to rule engine instance
} rule_engine_func_context_t;

//! \~ Rule Engine timers
typedef struct rule_engine__timer
{
    TimerHandle_t timer_handle;
    StaticTimer_t timer_buffer;

    uint8_t type;                                              //!< \~ Absolute/Relative timer
    uint8_t state;                                             //!< \~ State of timer
    uint32_t time_ticks;                                       //!< \~ Time in ticks
    uint32_t rule_id;                                          //!< \~ Rule id
    struct expr_var *instance;                                 //!< \~ Timer instance bound to global variable
    void (*timer_callback)(struct rule_engine__timer *timer);  //~< \~ Callback
    const rule_engine_inst_t *p_rule_engine_inst;
} rule_engine__timer_t;

//! \~ Functions used during rules execution
struct rule_engine__functions
{
    struct expr_func *funcs;  //!< \~ Statically defined array of rules functions
};

//! \~ Rule's execution members
struct rule_engine__expression
{
    char *rule_string;                    //!< \~ Rule definition
    struct expr *rule;                    //!< \~ Compiled expression from rule_string
    struct expr_var_list vars;            //!< \~ Variable list used in rule expression
    rule_engine__expression_type_t type;  //!< \~ Rule expression type
};

//! \~ Sensitivity list
struct rule_engine__sensitivity_list
{
    uint32_t parameter;      /*!< \~ Variable id. If variable type is RULE_ENGINE__VAR_TYPE_DDM2
                                 variable id is set from ddm2_parameter_list. If variable type is
                                 RULE_ENGINE__VAR_TYPE_GLOBAL or RULE_ENGINE__VAR_TYPE_LOCAL,
                                 variable id is generated using varible's name.
                             */
    uint8_t updated : 1;     /*!< \~ True when ddm2 parameter is published or when rule variable is
                                 updated during execution of the rules.
                             */
    uint8_t initialized : 1; /*!< \~ True when ddm2 parameter is initialized in the sensitivity list
                                 definition of the rule.
                             */

    struct expr_var *variable;  //!< \~ Reference to variables used in expr_var_list
};

//! \~ Trigger section
struct rule_engine__trigger_section
{
    uint32_t mask;      //!< \~ Determine which variables are used to trigger a rule
    uint8_t mask_type;  /*!< \~ Mask type. It can bese set to AND(all variables to be updated)
                            or OR(any of the variables to be updated)
                        */
    uint32_t n_params;  //!< \~ Number of variables used to trigger a rule
    struct rule_engine__sensitivity_list variables[RULE_ENGINE__MAX_NUMBER_OF_VARS_PER_RULE];
};

//! \~ Rule specification
struct rule_engine__specification
{
#ifdef RULE_ENGINE__PRINT_RULE_NAME_ENABLED
    char name[RULE_ENGINE__VAR_NAME_LEN];  //!< \~ Rule name
#else
    char *name;  //!< \~ Rule name pointer
#endif
    uint32_t id;    //!< \~ Rule id
    bool execute;   //!< \~ True if rule has been triggered and executed
    bool resolved;  //!< \~ True if rule has been resolved i.e. all dynamic variables(if any) have been resolved

    struct rule_engine__expression expr;          //!< \~ Rule's execution members
    struct rule_engine__trigger_section trigger;  //!< \~ Trigger section
    const rule_engine_inst_t *p_rule_engine_inst;
};

/*! \~ Rule definition structure.
    Used for storing predefined rules in the rule engine
*/
struct rule_engine__rule
{
    char name[RULE_ENGINE__RULE_NAME_LEN];  //!< \~ Rule's name
    char *rule;                             //!< \~ Rule string consisted of the sensitivity list and rule definition
    int size;                               //!< \~ Rule string length
};

//! \~ Sensitivity list parser. Stack allocated
struct rule_engine__sensitivity_list_parser
{
    uint32_t mask;         //!< \~ Determine which variables are used to trigger a rule
    uint8_t mask_type;     /*!< \~ Mask type. It can bese set to AND(all variables to be updated)
                               or OR(any of the variables to be updated)
                           */
    uint32_t n_variables;  //!< \~ Number of parsed variables
    struct _variables
    {
        uint32_t id;                   //!< \~ Variable id
        char *name;                    //!< \~ Variable name
        uint8_t type : 3;              //!< \~ Variable type
        uint8_t type_id : 4;           //!< \~ Variable type id
        uint8_t type_is_resolved : 1;  //!< \~ True if variable type is resolved
        uint8_t ddm_create : 2;        //!< \~ Request ddm instance in case of ddm type
        int32_t value;                 //!< \~ Variable value
        bool initialized;              //!< \~ True if variable is initialized in sensitivity list string
    } variables[RULE_ENGINE__MAX_NUMBER_OF_VARS_PER_RULE];
};

/*! \brief DDM2 Rule Engine add specification

    It will add the specification in preallocated sorted containter
    and subscribe the connector to the parameters listed in the
    @ref rule_engine__sensitivity_list

    \param       wrapper           DDM2 item object
    \param       rule              Definition of the rule

    \return      1 - specification successfully added
    \return      0 - specification not added
 */
int rule_engine__add_specification(rule_engine_inst_t *const p_rule_engine_inst, const struct rule_engine__rule *const rule);

/*! \brief DDM2 Rule Engine evaluate rule

    Itterates over the subscription list of updated parameters
    and evaluates the corresponding rules if the updated parameter
    exist in the specificaion's @ref rule_engine__sensitivity_list
*/
void rule_engine__evaluate_rules(rule_engine_inst_t *const p_rule_engine_inst);

/*! \brief DDM2 Rule Engine evaluate ddm2 parameters

    Itterates over the subscription list of updated parameters
    and verifies if any ddm2 parameter has been updated, so that
    it can trigger execution of @ref rule_engine__evaluate_rules()

    \return     true - if any ddm2 parameter in the subscription list has been updated
    \return     false - if none of the ddm2 paramters in the subscription list has been updated
*/
bool rule_engine__evaluate_ddm2_parameters(rule_engine_inst_t *const p_rule_engine_inst);

/*! \brief DDM2 Rule Engine evaluate timers

    Updates the trigger section parameter of the rules for which
    the timer has fired. If the rule's trigger section parameter
    is the timer instance itself(propagated trough \a data),
    @ref rule_engine__evaluate_rules() will be triggered and the
    rule will be executed.

    \param      data - data provided by @ref rule_engine_timer_callback() function.
                       It contains the timer instance variable.
    \param      data_size - size of \a data

    \pre        Parameter \a data must be a non-NULL pointer.
    \pre        Parameter \a data_size must be different than 0.

    \return     true - if any rule's trigger section has been updated
    \return     false - if none of the rules trigger section has been updated
*/
bool rule_engine__evaluate_timers(rule_engine_inst_t *const p_rule_engine_inst, const void *const data, uint32_t data_size);

#endif  // RULE_ENGINE_DEF_H
