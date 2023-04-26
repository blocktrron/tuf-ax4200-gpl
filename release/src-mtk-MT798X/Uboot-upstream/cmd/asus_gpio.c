/******************************************************************************
* Filename : gpio.c
* This part is used to control LED and detect button-press
******************************************************************************/

#include <common.h>
#include <command.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/gpio.h>
#include <gpio.h>

#define LED_ON 1
#define LED_OFF 0

/* LED/BTN definitions */
static struct gpio_s {
	char		*name;
	unsigned int	gpio_nr;		/* GPIO# */
	unsigned int	dir;			/* direction. 0: output; 1: input */
	unsigned int	is_led;			/* 0: NOT LED; 1: is LED */
	unsigned int	def_onoff_highlow;	/* default value of LEDs, or default high/low of output pin */
	unsigned int	active_low;		/* LED low active if non-zero */
} gpio_tbl[GPIO_IDX_MAX] = {
#if defined(PANTHERA) || defined(PANTHERB) // GPIO9,GPIO10
	[WPS_BTN] =		{	/* GPIO10, Low  active, input  */
		.name = "WPS button",
		.gpio_nr = 10,
		.dir = 1,
		.is_led = 0,
		.active_low = 1,
	},
	[RST_BTN] =		{	/* GPIO9, Low  active, input  */
		.name = "Reset button",
		.gpio_nr = 9,
		.dir = 1,
		.is_led = 0,
		.active_low = 1,
	},
#if defined(PANTHERA)
	[PWR_LED] = 		{	/* GPIO21, Low active, output, default on, PWM0, PWM device must be disabled if you want to use it in firmware. */
		.name = "Power LED",
		.gpio_nr = 21,
		.dir = 0,
		.is_led = 1,
		.active_low = 0,
		.def_onoff_highlow = 1,
	},
#endif	/* PANTHERA */
#elif defined(CHEETAH)
	[WPS_BTN] =		{	/* GPIO0, Low  active, input  */
		.name = "WPS button",
		.gpio_nr = 0,
		.dir = 1,
		.is_led = 0,
		.active_low = 1,
	},
	[RST_BTN] =		{	/* GPIO1, Low  active, input  */
		.name = "Reset button",
		.gpio_nr = 1,
		.dir = 1,
		.is_led = 0,
		.active_low = 1,
	},
#elif defined(TUFAX4200) || defined(TUFAX6000)
	[WPS_BTN] =		{	/* GPIO10, Low  active, input  */
		.name = "WPS button",
		.gpio_nr = 10,
		.dir = 1,
		.is_led = 0,
		.active_low = 1,
	},
	[RST_BTN] =		{	/* GPIO9, Low  active, input  */
		.name = "Reset button",
		.gpio_nr = 9,
		.dir = 1,
		.is_led = 0,
		.active_low = 1,
	},
	[PWR_LED] = 		{	/* GPIO11, High active, output, default on */
		.name = "Power LED",
		.gpio_nr = 11,
		.dir = 0,
		.is_led = 1,
		.active_low = 0,
		.def_onoff_highlow = 1,
	},
	[WAN_RED_LED] = 	{	/* GPIO12, Low active, output */
		.name = "WAN Red LED",
		.gpio_nr = 12,
		.dir = 0,
		.is_led = 1,
		.active_low = 1,
		.def_onoff_highlow = 0,
	},
	[WIFI_2G_LED] = 	{	/* GPIO1, High active, output */
		.name = "2G LED",
		.gpio_nr = 1,
		.dir = 0,
		.is_led = 1,
		.active_low = 0,
		.def_onoff_highlow = 0,
	},
	[WIFI_5G_LED] = 	{	/* GPIO2, High active, output */
		.name = "5G LED",
		.gpio_nr = 2,
		.dir = 0,
		.is_led = 1,
		.active_low = 0,
		.def_onoff_highlow = 0,
	},
#if defined(TUFAX6000) // RGB LED
	[WIFI_B_LED] =	{	/* GPIO#25, High active, output */
		.name = "Blue LED",
		.gpio_nr = 20,
		.dir = 0,
		.is_led = 1,
		.active_low = 0,
		.def_onoff_highlow = 1,
	},
	[WIFI_G_LED] =	{
		.name = "Green LED",
		.gpio_nr = 22,
		.dir = 0,
		.is_led = 1,
		.active_low = 0,
		.def_onoff_highlow = 0,
	},
	[WIFI_R_LED] =	{
		.name = "Red LED",
		.gpio_nr = 21,
		.dir = 0,
		.is_led = 1,
		.active_low = 0,
		.def_onoff_highlow = 0,
	},
#endif
#elif defined(RTAX59U)
	[RST_BTN] =		{	/* GPIO 10, Low  active, input  */
		.name = "Reset button",
		.gpio_nr = 10,
		.dir = 1,
		.is_led = 0,
		.active_low = 1,
	},
	[WPS_BTN] =		{	/* GPIO 9, Low  active, input  */
		.name = "WPS button",
		.gpio_nr = 9,
		.dir = 1,
		.is_led = 0,
		.active_low = 1,
	},
	[WIFI_G_LED] =	{		/* GPIO 11, Low active, output */
		.name = "Green LED",
		.gpio_nr = 11,
		.dir = 0,
		.is_led = 1,
		.active_low = 1,
		.def_onoff_highlow = 1, /* boot time LED */
	},
	[WIFI_R_LED] =	{		/* GPIO 12, Low active, output */
		.name = "Red LED",
		.gpio_nr = 12,
		.dir = 0,
		.is_led = 1,
		.active_low = 1,
	},
	[WIFI_B_LED] =	{		/* GPIO 13, Low active, output */
		.name = "Blue LED",
		.gpio_nr = 13,
		.dir = 0,
		.is_led = 1,
		.active_low = 1,
	},
	[WIFI_W_LED] =	{		/* GPIO 14, Low active, output */
		.name = "White LED",
		.gpio_nr = 14,
		.dir = 0,
		.is_led = 1,
		.active_low = 1,
	},
#elif 0 /* example */
	[WPS_BTN] =		{	/* GPIO34, Low  active, input  */
		.name = "WPS button",
		.gpio_nr = 34,
		.dir = 1,
		.is_led = 0,
		.active_low = 1,
	},
	[MALIBU_RESET] = 	{	/* GPIO#37, Low active, output */
		.name = "MALIBU_RESET",	/* QCA8075 */
		.gpio_nr = 37,
		.dir = 0,
		.is_led = 0,
		.active_low = 1,
	},
	[PWR_LED] = 		{	/* GPIO#21, High active, output */
		.name = "Power LED",
		.gpio_nr = 21,
		.dir = 0,
		.is_led = 1,
		.active_low = 0,
	},
	[WIFI_2G_LED] = 	{	/* GPIO#18, High active, output */
		.name = "2G LED",
		.gpio_nr = 18,
		.dir = 0,
		.is_led = 1,
		.active_low = 0,
	},
	[WIFI_5G_LED] = 	{	/* GPIO#19, High active, output */
		.name = "5G LED",
		.gpio_nr = 19,
		.dir = 0,
		.is_led = 1,
		.active_low = 0,
	},
	[WAN_LED] = 	{		/* GPIO#47, High active, output */
		.name = "WAN WHITE LED",
		.gpio_nr = 47,
		.dir = 0,
		.is_led = 1,
		.active_low = 0,
	},
	[LAN_LED] = 		{	/* GPIO#35, High active, output */
		.name = "LAN LED",
		.gpio_nr = 35,
		.dir = 0,
		.is_led = 1,
		.active_low = 0,
	},
#endif
};

/* Get real GPIO# of gpio_idx
 * @return:
 *  NULL:	GPIO# not found
 *  otherwise:	pointer to GPIO PIN's data
 */
static struct gpio_s *get_gpio_def(enum gpio_idx_e gpio_idx)
{
	struct gpio_s *g;

	if (gpio_idx < 0 || gpio_idx >= GPIO_IDX_MAX) {
		printf("%s: Invalid GPIO index %d/%d\n", __func__, gpio_idx, GPIO_IDX_MAX);
		return NULL;
	}

	g = &gpio_tbl[gpio_idx];
	if (!g->name)
		return NULL;
	return g;
}

/* Whether gpio_idx is active low or not
 * @return:	1:	active low
 * 		0:	active high
 */
static unsigned int get_gpio_active_low(enum gpio_idx_e gpio_idx)
{
	struct gpio_s *g = get_gpio_def(gpio_idx);

	if (!g) {
		debug("%s: gpio_idx %d not found.\n", __func__, gpio_idx);
		return -1;
	}

	return !!(g->active_low);
}

/* Set GPIO# as GPIO PIN and direction.
 * @gpio_nr:	GPIO#
 * @dir:	GPIO direction
 * 		0: output
 * 		1: input.
 */
static void set_gpio_dir(enum gpio_idx_e gpio_idx, int dir, int out_val)
{
	struct gpio_s *g = get_gpio_def(gpio_idx);

	if (!g) {
		debug("%s: gpio_idx %d not found.\n", __func__, gpio_idx);
		return;
	}

	if (!dir) {
		gpio_direction_output(g->gpio_nr, out_val);
	} else {
		gpio_direction_input(g->gpio_nr);
	}
}

/* Set raw value to GPIO#
 * @gpio_nr:	GPIO#
 * @val:	GPIO direction
 * 		0: low-level voltage
 * 		1: high-level voltage
 */
static void set_gpio_pin(enum gpio_idx_e gpio_idx, int val)
{
	struct gpio_s *g = get_gpio_def(gpio_idx);

	if (!g) {
		debug("%s: gpio_idx %d not found.\n", __func__, gpio_idx);
		return;
	}

	gpio_set_value(g->gpio_nr, !!val);
}

/* Read raw value of GPIO#
 * @gpio_nr:	GPIO#
 * @return:
 * 		0: low-level voltage
 * 		1: high-level voltage
 */
static int get_gpio_pin(enum gpio_idx_e gpio_idx)
{
	struct gpio_s *g = get_gpio_def(gpio_idx);

	if (!g) {
		debug("%s: gpio_idx %d not found.\n", __func__, gpio_idx);
		return 0;
	}

	return !!gpio_get_value(g->gpio_nr);
}

/* Check button status. (high/low active is handled in this function)
 * @return:	1: key is pressed
 * 		0: key is not pressed
 */
static int check_button(enum gpio_idx_e gpio_idx)
{
	struct gpio_s *g = get_gpio_def(gpio_idx);

	if (!g) {
		debug("%s: gpio_idx %d not found.\n", __func__, gpio_idx);
		return 0;
	}

	return !!(get_gpio_pin(gpio_idx) ^ get_gpio_active_low(gpio_idx));
}

extern void gpy211_force_onoff(int phy, int onoff);
extern void mt7531_force_onoff(int onoff);

/* ON/OFF a LED that is controlled by a hardware component instead of GPIO pin of SoC.
 * @gpio_idx:
 * @onoff:	0: OFF, otherwise: ON
 * @return:
 * 	0:	@gpio_idx is not handled
 *  otherwise:	@gpio_idx is handled
 */
static int hwctl_led_onoff(enum gpio_idx_e gpio_idx, int onoff)
{
	int ret = 0;
#if defined(TUFAX4200) || defined(TUFAX6000) || defined(PANTHERA)
	if (gpio_idx == WAN_LED) {
		gpy211_force_onoff(6, onoff);
		ret = 1;
	} else if (gpio_idx == LAN_LED) {
		mt7531_force_onoff(onoff);
		gpy211_force_onoff(5, onoff);
		ret = 1;
	}
#endif

	return ret;
}

/* Check button status. (high/low active is handled in this function)
 * @onoff:	1: Turn on LED
 * 		0: Turn off LED
 */
void led_onoff(enum gpio_idx_e gpio_idx, int onoff)
{
#if defined(ETJ)
	switch (gpio_idx) {
		case WIFI_W_LED: return;
		case WIFI_R_LED: //pwm0
				pwm_onoff(0, onoff);
				break;
		case WIFI_G_LED: //pwm1
				pwm_onoff(1, onoff);
				break;
		case WIFI_B_LED: //pwm2
				pwm_onoff(2, onoff);
				break;
		default:
				break;
	}
#endif
	set_gpio_pin(gpio_idx, onoff ^ get_gpio_active_low(gpio_idx));
}

void asus_gpio_init(void)
{
	int i;
	int on;

	printf("ASUS %s gpio/led init\n", model);

	for (i = 0; i < GPIO_IDX_MAX; i++) {
		if (!gpio_tbl[i].name)
			continue;
		if (gpio_request(gpio_tbl[i].gpio_nr, "boot") != 0) {
			printf("Request gpio[%d] ERR!\n", gpio_tbl[i].gpio_nr);
			continue;
		}
		if (gpio_tbl[i].dir) // input
			set_gpio_dir(i, 1, 0); // init input
		else { // output
			if (gpio_tbl[i].is_led) {
				on = 1;
				if (i == WAN_RED_LED || i == WAN2_RED_LED) /* turn on all LEDs, except WANx RED LED */
					on = 0;
#if defined(TUFAX6000) // RGB LED
				if (i == WIFI_G_LED || i == WIFI_R_LED ) // initial BLUE same as hardware state
					on = 0;
#elif defined(RTAX59U) // RGB LED
				if (i == WIFI_B_LED || i == WIFI_R_LED || i == WIFI_W_LED) // only turn on WIFI_G_LED
					on = 0;
#endif
				set_gpio_dir(i, 0, on ^ !!(gpio_tbl[i].active_low)); // init led 
			}
			else
				set_gpio_dir(i, 0, gpio_tbl[i].def_onoff_highlow); // init output
		}
	}

	// turn LEDs to its default state after 300ms
	udelay(300 * 1000UL);
	for (i = 0; i < GPIO_IDX_MAX; i++) {
		if (!gpio_tbl[i].name || gpio_tbl[i].dir)
			continue;
		led_onoff(i, gpio_tbl[i].def_onoff_highlow);
	}
}

unsigned long DETECT(void)
{
	int key = 0;

	if (check_button(RST_BTN)) {
		key = 1;
		printf("reset button pressed!\n");
	}
	return key;
}

unsigned long DETECT_WPS(void)
{
	int key = 0;

	if (check_button(WPS_BTN)) {
		key = 1;
		printf("wps button pressed!\n");
	}
	return key;
}

void power_led_on(void)
{
	led_onoff(PWR_LED, 1);
}

void power_led_off(void)
{
	led_onoff(PWR_LED, 0);
}

void asus_blue_led_on(void)
{
	led_onoff(WIFI_B_LED, LED_ON);
	led_onoff(WIFI_G_LED, LED_OFF);
	led_onoff(WIFI_R_LED, LED_OFF);
	led_onoff(WIFI_W_LED, LED_OFF);
}

void asus_green_led_on(void)
{
	led_onoff(WIFI_B_LED, LED_OFF);
	led_onoff(WIFI_G_LED, LED_ON);
	led_onoff(WIFI_R_LED, LED_OFF);
	led_onoff(WIFI_W_LED, LED_OFF);
}

void asus_green_led_off(void)
{
	led_onoff(WIFI_G_LED, LED_OFF);
}

void asus_red_led_on(void)
{
	led_onoff(WIFI_B_LED, LED_OFF);
	led_onoff(WIFI_G_LED, LED_OFF);
	led_onoff(WIFI_R_LED, LED_ON);
	led_onoff(WIFI_W_LED, LED_OFF);
}

void asus_red_led_off(void)
{
	led_onoff(WIFI_R_LED, LED_OFF);
}

void asus_purple_led_on(void)
{
	led_onoff(WIFI_B_LED, LED_ON);
	led_onoff(WIFI_G_LED, LED_OFF);
	led_onoff(WIFI_R_LED, LED_ON);
	led_onoff(WIFI_W_LED, LED_OFF);
}

void asus_yellow_led_on(void)
{
	led_onoff(WIFI_B_LED, LED_OFF);
	led_onoff(WIFI_G_LED, LED_ON);
	led_onoff(WIFI_R_LED, LED_ON);
	led_onoff(WIFI_W_LED, LED_OFF);
}

void asus_rgb_led_off(void)
{
	led_onoff(WIFI_B_LED, LED_OFF);
	led_onoff(WIFI_G_LED, LED_OFF);
	led_onoff(WIFI_R_LED, LED_OFF);
	led_onoff(WIFI_W_LED, LED_OFF);
}

/* Turn on model-specific LEDs */
void leds_on(void)
{
	led_onoff(PWR_LED, 1);
	led_onoff(WAN_LED, 1);
	led_onoff(WAN2_LED, 1);
	led_onoff(LAN_LED, 1);

#if defined(TUFAX4200) || defined(TUFAX6000) || defined(PANTHERA)
	hwctl_led_onoff(LAN_LED, 1);
	hwctl_led_onoff(WAN_LED, 1);
#endif

	/* Don't turn on below LEDs in accordance with PM's request. */
	led_onoff(PWR_RED_LED, 0);
	wan_red_led_off();
	led_onoff(USB_LED, 0);
	led_onoff(USB3_LED, 0);
	led_onoff(WIFI_2G_LED, 0);
	led_onoff(WIFI_5G_LED, 0);
	led_onoff(WPS_LED, 0);
	led_onoff(FAIL_OVER_LED, 0);
}

/* Turn off model-specific LEDs */
void leds_off(void)
{
	led_onoff(PWR_LED, 0);
	led_onoff(WAN_LED, 0);
	led_onoff(WAN2_LED, 0);
	led_onoff(LAN_LED, 0);
	wan_red_led_off();

#if defined(TUFAX4200) || defined(TUFAX6000) || defined(PANTHERA)
	hwctl_led_onoff(LAN_LED, 0);
	hwctl_led_onoff(WAN_LED, 0);
#endif

	led_onoff(PWR_RED_LED, 0);
	led_onoff(USB_LED, 0);
	led_onoff(USB3_LED, 0);
	led_onoff(WIFI_2G_LED, 0);
	led_onoff(WIFI_5G_LED, 0);
	led_onoff(WPS_LED, 0);
	led_onoff(FAIL_OVER_LED, 0);

#if defined(PLAX56_XP4)
	led_onoff(WIFI_B_LED, LED_OFF);
	led_onoff(WIFI_R_LED, LED_OFF);
	led_onoff(WIFI_W_LED, LED_OFF);
#endif
}

/* Turn on all model-specific LEDs */
void all_leds_on(void)
{
	int i;

	for (i = 0; i < GPIO_IDX_MAX; i++) {
		if (!gpio_tbl[i].name || !gpio_tbl[i].is_led)
			continue;
		led_onoff(i, 1);
	}

	/* WAN RED LED share same position with WAN BLUE LED. Turn on WAN BLUE LED only*/
	wan_red_led_off();
}

/* Turn off all model-specific LEDs */
void all_leds_off(void)
{
	int i;

	for (i = 0; i < GPIO_IDX_MAX; i++) {
		if (!gpio_tbl[i].name || !gpio_tbl[i].is_led)
			continue;
		led_onoff(i, 0);
	}

	wan_red_led_off();
}

#if defined(ALL_LED_OFF)
void enable_all_leds(void)
{
}

void disable_all_leds(void)
{
}
#endif

#if defined(DEBUG_LED_GPIO)
int do_test_gpio (cmd_tbl_t * cmdtp, int flag, int argc, char * const argv[])
{
	struct gpio_s *g = NULL;
	int i, j, stop, old = 0, new = 0, status;
	unsigned int gpio_idx = GPIO_IDX_MAX;

	if (argc >= 2) {
		gpio_idx = simple_strtoul(argv[1], 0, 10);
		g = get_gpio_def(gpio_idx);
	}
	if (!g) {
		printf("%8s %20s %5s %9s %10s \n", "gpio_idx", "name", "gpio#", "direction", "active low");
		for (i = 0; i < GPIO_IDX_MAX; ++i) {
			if (!(g = get_gpio_def(i)))
				continue;
			if (!g->dir && !g->is_led)
				continue;
			printf("%8d %20s %5d %9s %10s \n", i, g->name, g->gpio_nr,
				(!g->dir)?"output":"input", (g->active_low)? "yes":"no");
		}
		return 1;
	}

	printf("%s: GPIO index %d GPIO#%d direction %s active_low %s\n",
		g->name, gpio_idx, g->gpio_nr,
		(!g->dir)?"output":"input", (g->active_low)? "yes":"no");
	printf("Press any key to stop testing ...\n");
	if (!g->dir) {
		/* output */
		for (i = 0, stop = 0; !stop; ++i) {
			printf("%s: %s\n", g->name, (i&1)? "ON":"OFF");
			led_onoff(gpio_idx, i & 1);
			for (j = 0, stop = 0; !stop && j < 40; ++j) {
				udelay(100000);
				if (tstc())
					stop = 1;
			}
		}
	} else {
		/* input */
		for (i = 0, stop = 0; !stop; ++i) {
			new = get_gpio_pin(gpio_idx);
			status = check_button(gpio_idx);
			if (!i || old != new) {
				printf("%s: %d [%s]\n", g->name, new, status? "pressed":"not pressed");
				old = new;
			}
			for (j = 0, stop = 0; !stop && j < 10; ++j) {
				udelay(5000);
				if (tstc())
					stop = 1;
			}
		}
	}

	return 0;
}

U_BOOT_CMD(
    test_gpio, 2, 0, do_test_gpio,
    "test_gpio - Test GPIO.\n",
    "test_gpio [<gpio_idx>] - Test GPIO PIN.\n"
    "                <gpio_idx> is the index of GPIO table.\n"
    "                If gpio_idx is invalid or is not specified,\n"
    "                GPIO table is printed.\n"
);


int do_ledon(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	leds_on();

	return 0;
}

int do_ledoff(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	leds_off();

	return 0;
}

U_BOOT_CMD(
    ledon, 1, 1, do_ledon,
	"ledon\t -set led on\n",
	NULL
);

U_BOOT_CMD(
    ledoff, 1, 1, do_ledoff,
	"ledoff\t -set led off\n",
	NULL
);

#if defined(ALL_LED_OFF)
int do_all_ledon(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	enable_all_leds();

	return 0;
}

int do_all_ledoff(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	disable_all_leds();

	return 0;
}

U_BOOT_CMD(
    all_ledon, 1, 1, do_all_ledon,
	"all_ledon\t -set all_led on\n",
	NULL
);

U_BOOT_CMD(
    all_ledoff, 1, 1, do_all_ledoff,
	"all_ledoff\t -set all_led off\n",
	NULL
);
#endif

#endif	/* DEBUG_LED_GPIO */
