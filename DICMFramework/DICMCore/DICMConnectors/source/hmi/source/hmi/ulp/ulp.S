/* ULP source
 * 
 * This code handles ADC readings of butons attached to ADC,
 * and provide raw data to upper layers.
 * 
 * Also it handles logic for one rotary encoder, provides a flag of either 
 * a rotation was detected and its direction.
 *   Note: Ckick flags must be reset after reading in upprer layers
 *   (click_cw, click_ccw).
 */

/* ULP assembly files are passed through C preprocessor first, so include directives
   and C macros may be used in these files
 */
#include "soc/rtc_cntl_reg.h"
#include "soc/rtc_io_reg.h"
#include "soc/soc_ulp.h"
#include "soc/sens_reg.h"


	/* ADC1 channel 0, GPIO36 */
	.set adc_channel_button, 0
	/* ADC1 channel 6, GPIO34 */
	.set adc_channel_ntc, 6

	/* Configure the number of ADC samples to average on each measurement.
	   For convenience, make it a power of 2. */
	.set adc_oversampling_factor_log, 2
	.set adc_oversampling_factor, (1 << adc_oversampling_factor_log)

	/* Define variables, which go into .bss section (zero-initialized data) */
	.bss

	/* Counter of measurements done */
	.global sample_counter
sample_counter:
	.long 0

	.global button_middle
button_middle:
	.long 0

	.global adc_last_result
adc_last_result:
	.long 0

	.global adc_ntc
adc_ntc:
	.long 0

	.global p_status_old
p_status_old:
	.long 0

	.global p_cw_flag
p_cw_flag:
	.long 0

	.global p_ccw_flag
p_ccw_flag:
	.long 0

	.global p_cw_counter
p_cw_counter:
	.long 0

	.global p_ccw_counter
p_ccw_counter:
	.long 0

	.global click_cw
click_cw:
	.long 0

	.global click_ccw
click_ccw:
	.long 0

	.global adc_button_threshold
adc_button_threshold:
	.long 0

	.global adc_status
adc_status:
	.long 0

	.global lcd_suspended
lcd_suspended:
	.long 0

	/* Code goes into .text section */
	.text
	.global entry
entry:
	/* skip turning on the OP LED if lcd_suspended == HMI_SLEEP */
	move r3, lcd_suspended /* get address of lcd_suspended in r3 */
	ld r2, r3, 0           /* load the value of lcd_suspended */
	sub r2, r2, 1          /* check if lcd_suspended is 1(HMI_SLEEP) */
	jump start_adc, eq     /* if it is 1(HMI_SLEEP) skip to ADC measurements */

	/* turn on OP LED */
	WRITE_RTC_REG(RTC_IO_TOUCH_PAD5_REG, RTC_IO_TOUCH_PAD5_HOLD_S, 1, 0)
	WRITE_RTC_REG(RTC_GPIO_OUT_REG,RTC_GPIO_OUT_DATA_S+15,1,1)

start_adc:
	/* read the RTC pins and store it into global variables, RTC_GPIO_10 = GPIO_NUM_4
	   this is done for HW that supports digital middle button*/
	READ_RTC_REG(RTC_GPIO_IN_REG, RTC_GPIO_IN_NEXT_S + 10, 1)
	/* Store RTC_GPIO_10 state to button_middle */
	move r1, button_middle
	st r0, r1, 0

	/* increment sample counter */
	move r3, sample_counter
	ld r2, r3, 0
	add r2, r2, 1
	st r2, r3, 0

	/* do measurements using ADC */
	/* r0 and r2 will be used as accumulator */
	move r0, 0
	move r2, 0
	/* initialize the loop counter */
	stage_rst

	WRITE_RTC_REG(SENS_SAR_MEAS_WAIT2_REG, SENS_FORCE_XPD_SAR_S, 2, SENS_FORCE_XPD_SAR_PU) /* Force enable SAR ADC power, see sar_periph_ctrl_power_enable() */
measure:
	/* measure and add value to accumulator */
	adc r1, 0, adc_channel_button + 1
	add r0, r0, r1
	adc r1, 0, adc_channel_ntc + 1
	add r2, r2, r1
	/* increment loop counter and check exit condition */
	stage_inc 1
	jumps measure, adc_oversampling_factor, lt

	WRITE_RTC_REG(SENS_SAR_MEAS_WAIT2_REG, SENS_FORCE_XPD_SAR_S, 2, SENS_FORCE_XPD_SAR_PD) /* Force disable SAR ADC power, see sar_periph_ctrl_power_disable() */

	/* divide accumulator by adc_oversampling_factor.
	Since it is chosen as a power of two, use right shift */
	rsh r0, r0, adc_oversampling_factor_log
	rsh r2, r2, adc_oversampling_factor_log
	/* averaged value is now in r0; store it into adc_last_result */
	move r3, adc_last_result
	st r0, r3, 0
	/* averaged value is now in r2; store it into adc_ntc */
	move r3, adc_ntc
	st r2, r3, 0

/* Compare ADC value with buttons thresholds */
/* NOTE: order of comparison matters, the higher threshold should be compared first */

simplified_adc_detection:
	move r3, adc_button_threshold /* get address of adc_button_threshold in r3 */
	ld r3, r3, 0                  /* load the value of adc_button_threshold */
	sub r3, r0, r3                /* adc_button_threshold - adc_value */
	jump set_adc_status, ov       /* if adc is below adc_button_threshold wake up */
	jump reset_adc_status         /* else reset adc_status and check if knob is rotated */

reset_adc_status:
	move r3, adc_status  /* get address of adc_status in r3 */
	move r2, 0           /* assign const value to r2 */
	st r2, r3, 0         /* set adc_status value - false*/
	jump rot_check       /* check if knob is rotated */

set_adc_status:
	move r3, adc_status  /* get address of adc_status in r3 */
	move r2, 1           /* assign const value to r2 */
	st r2, r3, 0         /* set adc_status value - true */
	jump wake_up         /* button press detected wakeup */

/* Rortary logic */
init_flag:
	and r2, r1, 1          /* r1 = GPIO status, get first bit value*/
	move r3, p_cw_flag     /* get address of p_cw_flag */
	st r2, r3, 0           /* store p_cw_flag */
	rsh r2, r1, 1          /* r1 = GPIO status, get second bit value */
	and r2, r2, 1          /* remove upper bits */
	move r3, p_ccw_flag    /* get address of p_ccw_flag */
	st r2, r3, 0           /* store p_ccw_flag */

	/* this is to trigger further click detection at the beginning of rotation */
	move r3, p_cw_flag     /* get address of p_cw_flag */
	ld r3, r3, 0           /* get value of p_cw_flag */
	sub r3, r3, 1          /* check if p_cw_flag is 1 */
	jump set_flag_cw, eq   /* if it is 1 initialize p_cw_flag with 1 */
	move r3, p_ccw_flag    /* get address of p_ccw_flag */
	ld r3, r3, 0           /* get value of p_ccw_flag */
	sub r3, r3, 1          /* check if p_ccw_flag is 1 */
	jump set_flag_ccw, eq  /* if it is 1 initialize p_ccw_flag with 1 */
	halt                   /* should not be reached */

/* reset all flags, counters and old values */
zero_position:
	move r2, 0
	move r3, p_cw_flag
	st r2, r3, 0
	move r3, p_ccw_flag
	st r2, r3, 0
	move r3, p_cw_counter
	st r2, r3, 0
	move r3, p_ccw_counter
	st r2, r3, 0
	move r3, p_status_old
	st r1, r3, 0
	halt

rot_check:
	/* GPIO_12 = RTC_GPIO_15 = TOUCH5 */
	/* Set the RTC_GPIO10 output HIGH to signal that ULP is now up */
	wait(4400) /* Calculated for 4.3k load resistance and 100nF filtering capacitor on optical sensor output */

/* read the RTC pins and store it into global variables ; just read the pin register once and store it into R1 */
	READ_RTC_REG(RTC_GPIO_IN_REG, RTC_GPIO_IN_NEXT_S, 16)

	move r1, r0      /* r1 hold RTC_GPIO status register */
/* GET GPIO */
	rsh r1, r1, 5    /* shift to get RTC_GPIO_5 and RTC_GPIO_6 bits */
	and r1, r1, 3    /* remove upper bits */

	/* turn off OP LED */
	WRITE_RTC_REG(RTC_GPIO_OUT_REG,RTC_GPIO_OUT_DATA_S+15,1,0)
	WRITE_RTC_REG(RTC_IO_TOUCH_PAD5_REG, RTC_IO_TOUCH_PAD5_HOLD_S, 1, 1)

/* Init ccw flag to 0x1 nad cw ot 0x0*/
	sub r0, r1, 1
	jump zero_position, ov	
	move r3, p_ccw_flag    /* get p_ccw_flag address */
	ld r3, r3, 0           /* get p_ccw_flag value in r3 */
	move r2, p_cw_flag     /* get p_cw_flag address */
	ld r2, r2, 0           /* get p_cw_flag value in r2 */
	add r3, r2, r3         /* p_ccw_flag + p_cw_flag */
	sub r3, r3, 1          /* check if any flag is 1 */
	jump init_flag, ov     /* if not jump to init_flag */

end_init_flag:

/* GET BIT FLAG */
check_cw_change:
	move r3, p_ccw_flag   /* get p_ccw_flag address */
	ld r3, r3, 0          /* get p_ccw_flag value in r3 */
	//and r3, r3, 1       /* remove upper bits */
	sub r3, r3, 1         /* check if p_ccw_flag is "0" */
	jump check_ccw_change, ov /*if true skip cw_flag check*/

	move r3, p_status_old /* get previous GPIO status */
	ld r3, r3, 0          /* load p_status_old value to r3 */
	and r2, r1, 1         /* mask unnecesary bits for current status */
	add r3, r3, r2        /* add old and new value of GPIO status register */
	and r3, r3, 1         /* mask unnecesary bits */
	sub r2, r3, 1         /* check for state change */
	jump set_flag_cw, eq  /* change detected, save state */
	halt

set_flag_cw:
	move r2, 1            /* set a constant */
	move r3, p_cw_flag    /* get p_cw_flag address */
	st r2, r3, 0          /* set p_cw_flag to 1 */
	move r2, 0            /* set a constant */
	move r3, p_ccw_flag   /* reset ccw flag */
	st r2, r3, 0          /* set p_ccw_flag to 0 */
	jump check_for_status_change
 
check_ccw_change:
	move r3, p_cw_flag    /* get p_cw_flag address */
	ld r3, r3, 0          /* get p_cw_flag value in r3 */
	and r3, r3, 1         /* remove upper bits */
	sub r3, r3, 1         /* check if p_cw_flag is "0" */
	jump check_cw_change, ov

	move r3, p_status_old /* get previous GPIO status */
	ld r3, r3, 0          /* load p_status_old value to r3 */
	rsh r3, r3, 1         /* right shift to get second bit */
	rsh r2, r1, 1         /* right shift to get second bit */
	add r3, r3, r2        /* add old and new value of GPIO status register */
	and r3, r3, 1         /* mask unnecesary bits */
	sub r2, r3, 1         /* check for state change */
	jump set_flag_ccw, eq /* change detected, save state */
	halt

set_flag_ccw:
	move r2, 1             /* set a constant */
	move r3, p_ccw_flag    /* get p_ccw_flag address */
	st r2, r3, 0           /* set p_ccw_flag to 1 */
	move r2, 0             /* set a constant */
	move r3, p_cw_flag     /* get p_cw_flag address */
	st r2, r3, 0           /* reset p_cw_flag to 0 */
	jump check_for_status_change/* flag was changed proceed with checking */

skip_lookup_table:
	halt

check_for_status_change:
/* COMPARE */
	move r3, p_status_old /* get in r3 address of p_status_old */
	ld r3, r3, 0          /* get in r3 value of p_status_old */
	sub r0, r3, r1        /* check if old and new values are equal */
	jump skip_lookup_table, eq /* if equal stop the program */

	/* check what value is p_status_old and jump to respective branch */
	sub r0, r3 , 0
	jump status_0, eq  /* if p_status_old is 0 */
	sub r0, r3 , 1
	jump status_1, eq  /* if p_status_old is 1 */
	sub r0, r3 , 2
	jump status_2, eq  /* if p_status_old is 2 */
	sub r0, r3 , 3
	jump status_3, eq  /* if p_status_old is 3 */
	halt               /* wrong value, stop the program */

/* check the sequence, old and new GPIO statuses
  * if after 0 current value is 1 or 2 jump to increment respective counter
  * else wrong sequence, reset counters
*/
status_0:
	sub r0, r1, 2
	jump status_cw_inc, eq
	sub r0, r1, 1
	jump status_ccw_inc, eq
	jump reset_counters

/*
  * check the sequence, old and new GPIO statuses
  * if after 1 current value is 0 or 3 jump to increment respective counter
  * else wrong sequence, reset counters
*/
status_1:
	sub r0, r1, 0
	jump status_cw_inc, eq
	sub r0, r1, 3
	jump status_ccw_inc, eq
	jump reset_counters

/*
  * check the sequence, old and new GPIO statuses
  * if after 2 current value is 3 or 0 jump to increment respective counter
  * else wrong sequence, reset counters
*/
status_2:
	sub r0, r1, 3
	jump status_cw_inc, eq
	sub r0, r1, 0
	jump status_ccw_inc, eq
	jump reset_counters

/*
  * check the sequence, old and new GPIO statuses
  * if after 2 current value is 3 or 0 jump to increment respective counter
  * else wrong sequence, reset counters
*/
status_3:
	sub r0, r1, 1
	jump status_cw_inc, eq
	sub r0, r1, 2
	jump status_ccw_inc, eq
	jump reset_counters

/* increment satus cw event counter */
status_cw_inc:
	move r3, p_cw_counter
	ld r2, r3, 0
	add r2, r2, 1
	st r2, r3, 0
	jump next_step_cw

/* increment satus ccw event counter */
status_ccw_inc:
	move r3, p_ccw_counter
	ld r2, r3, 0
	add r2, r2, 1
	st r2, r3, 0
	jump next_step_ccw

/* reset counters */
reset_counters:
	move r3, p_cw_counter
	move r2, 0
	st r2, r3, 0
	move r3, p_ccw_counter
	move r2, 0
	st r2, r3, 0
	jump save_as_old_status

/* save current gpio status in p_status_old */
save_as_old_status:
	move r3, p_status_old
	st r1, r3, 0
	halt

/* if event counter reached a threshold a click was detected */
next_step_cw:
	move r3, p_status_old
	st r1, r3, 0
	sub r3, r2, 3
	jump click_detected_cw, eq
	halt /* if not stop the program */

/* if event counter reached a threshold a click was detected */
next_step_ccw:
	move r3, p_status_old
	st r1, r3, 0
	sub r3, r2, 3
	jump click_detected_ccw, eq
	halt /* if not stop the program */

/* reset p_cw_counter and store click_cw flag */
click_detected_cw:
	move r3, p_cw_counter
	move r2, 0
	st r2, r3, 0
	move r3, click_cw
	move r2, 1
	st r2, r3, 0
	jump wake_up

/* reset p_ccw_counter and store click_ccw flag */
click_detected_ccw:
	move r3, p_ccw_counter
	move r2, 0
	st r2, r3, 0
	move r3, click_ccw
	move r2, 1
	st r2, r3, 0
	jump wake_up

/* end the program */
	.global exit	
exit:
	halt
	.global wake_up

wake_up:

	/* Check if the system can be woken up */
	READ_RTC_FIELD(RTC_CNTL_LOW_POWER_ST_REG, RTC_CNTL_RDY_FOR_WAKEUP)
	and r0, r0, 1
	jump exit, eq
	/* Wake up the SoC, end program */
	wake
	halt