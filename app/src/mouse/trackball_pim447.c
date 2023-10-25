/*
 * Copyright (c) 2021 Cedric VINCENT - original code
 * Copyright (c) 2022 voidyourwarranty@mailbox.org - extension & modification
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <zmk/sensors.h>
#include <zmk/hid.h>
#include <zmk/endpoints.h>
#include <zmk/trackball_pim447.h>

LOG_MODULE_REGISTER(PIM447, CONFIG_SENSOR_LOG_LEVEL);
//LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define MOVE_X_FACTOR  DT_PROP(DT_INST(0, pimoroni_trackball_pim447), move_factor_x)
#define MOVE_Y_FACTOR  DT_PROP(DT_INST(0, pimoroni_trackball_pim447), move_factor_y)
#define MOVE_X_INVERT  DT_PROP(DT_INST(0, pimoroni_trackball_pim447), invert_move_x)
#define MOVE_Y_INVERT  DT_PROP(DT_INST(0, pimoroni_trackball_pim447), invert_move_y)
#define MOVE_X_INERTIA DT_PROP(DT_INST(0, pimoroni_trackball_pim447), move_inertia_x)
#define MOVE_Y_INERTIA DT_PROP(DT_INST(0, pimoroni_trackball_pim447), move_inertia_y)
#define FACTOR_X  (MOVE_X_FACTOR * (MOVE_X_INVERT ? -1 : 1))
#define FACTOR_Y  (MOVE_Y_FACTOR * (MOVE_Y_INVERT ? -1 : 1))

#define SCROLL_X_INVERT   DT_PROP(DT_INST(0, pimoroni_trackball_pim447), invert_scroll_x)
#define SCROLL_Y_INVERT   DT_PROP(DT_INST(0, pimoroni_trackball_pim447), invert_scroll_y)
#define SCROLL_X_DIVISOR  DT_PROP(DT_INST(0, pimoroni_trackball_pim447), scroll_divisor_x)
#define SCROLL_Y_DIVISOR  DT_PROP(DT_INST(0, pimoroni_trackball_pim447), scroll_divisor_y)
#define DIVISOR_X  (SCROLL_X_DIVISOR * (SCROLL_X_INVERT ? -1 : 1))
#define DIVISOR_Y  (SCROLL_Y_DIVISOR * (SCROLL_Y_INVERT ?  1 : -1))

#define SWAP_AXES      DT_PROP(DT_INST(0, pimoroni_trackball_pim447), swap_axes)
#define POLL_INTERVAL  DT_PROP(DT_INST(0, pimoroni_trackball_pim447), poll_interval)

#define BUTTON    DT_PROP(DT_INST(0, pimoroni_trackball_pim447), button)
#define NORM      DT_PROP(DT_INST(0, pimoroni_trackball_pim447), norm)
#define EXACTNESS DT_PROP(DT_INST(0, pimoroni_trackball_pim447), exactness)
#define MAX_ACCEL DT_PROP(DT_INST(0, pimoroni_trackball_pim447), max_accel)

static int mode = DT_PROP(DT_INST(0, pimoroni_trackball_pim447), mode);

#define ABS(x) ((x<0)?(-x):(x))

void zmk_trackball_pim447_set_mode(int new_mode)
{
    switch (new_mode) {
        case PIM447_MOVE:
        case PIM447_SCROLL:
            mode = new_mode;
            break;

       case PIM447_TOGGLE:
            mode = mode == PIM447_MOVE
                   ? PIM447_SCROLL
                   : PIM447_MOVE;
            break;

       default:
            break;
    }
}

/*
 * Given some <delta> that is reported from the track ball, depending on the currently stored motion <stored_dx>,
 * <stored_dy>, implement acceleration by increasing <delta> depending on <stored_dx> and <stored_dy>.
 */

static int32_t acceleration ( int32_t stored_dx, int32_t stored_dy, int32_t delta ) {

  int32_t square;

  /*
   * Acceleration depends on the 2d-distance stored. Here <square> is the square of the Euclidean norm by default. In
   * order to enhance diagonal motion, the maximum norm can also be used.
   */

  if (NORM == PIM447_NORM_MAX) {
    square = (ABS(stored_dx) + ABS(stored_dy))*(ABS(stored_dx) + ABS(stored_dy));
  } else { // NORM == PIM447_NORM_EUCLID
    square = stored_dx*stored_dx + stored_dy*stored_dy;
  }

  /*
   * The absolute value, integer division and plus one make sure that a small range of <square> values are not
   * accelerated.
   */

  int32_t accelerated = (ABS(square-1)/EXACTNESS+1)*EXACTNESS*delta/100;

  /*
   * Finally, the accelerated motion is capped at some maximum value.
   */

  if (ABS(accelerated) > ABS(8*MAX_ACCEL*3*delta/10000))
    return (8*MAX_ACCEL*3*delta/10000);
  else
    return (accelerated);
}

static void thread_code(void *p1, void *p2, void *p3)
{
    const struct device *dev;
    int result;

    /* PIM447 trackball initialization. */

    const char *label = DT_PROP(DT_INST(0, pimoroni_trackball_pim447),label);
    dev = device_get_binding(label);
    if (dev == NULL) {
        LOG_ERR("Cannot get TRACKBALL_PIM447 device");
        return;
    }

    /* Event loop. */

    bool button_press_sent   = false;
    bool button_release_sent = false;

    /*
     * In order to implement acceleration and inertia of the pointer in mouse-move mode, only a part of the x,y
     * difference that has been received from the track ball is immediately reported via HID. The remainder is stored in
     * the following registers as 'yet to be reported'.
     */

    int32_t stored_dx = 0;
    int32_t stored_dy = 0;

    while (true) {
        struct sensor_value pos_dx, pos_dy, pos_dz;
        bool send_report = false;
        int clear = PIM447_NONE;

        result = sensor_sample_fetch(dev);
        if (result < 0) {
            LOG_ERR("Failed to fetch TRACKBALL_PIM447 sample");
            return;
        }

        result = sensor_channel_get(dev, SENSOR_CHAN_POS_DX, &pos_dx);
        if (result < 0) {
            LOG_ERR("Failed to get TRACKBALL_PIM447 pos_dx channel value");
            return;
        }

        result = sensor_channel_get(dev, SENSOR_CHAN_POS_DY, &pos_dy);
        if (result < 0) {
            LOG_ERR("Failed to get TRACKBALL_PIM447 pos_dy channel value");
            return;
        }

        result = sensor_channel_get(dev, SENSOR_CHAN_POS_DZ, &pos_dz);
        if (result < 0) {
            LOG_ERR("Failed to get TRACKBALL_PIM447 pos_dz channel value");
            return;
        }

        if (pos_dx.val1 != 0 || pos_dy.val1 != 0) {
            if (SWAP_AXES) {
                int32_t tmp = pos_dx.val1;
                pos_dx.val1 = pos_dy.val1;
                pos_dy.val1 = tmp;
            }
	}

	switch (mode) {
	default:
	case PIM447_MOVE:

	  {
	    /*
	     * Acceleration is implemented by re-scaling the current x,y difference reported from the sensors depending
	     * on the most recent motion in (stored_dx,stored_dy).
	     */

	    int add_dx = acceleration (stored_dx,stored_dy,pos_dx.val1*FACTOR_X);
	    int add_dy = acceleration (stored_dx,stored_dy,pos_dy.val1*FACTOR_Y);

	    stored_dx += add_dx;
	    stored_dy += add_dy;

	    if ((stored_dx != 0) || (stored_dy != 0)) {

	      /*
	       * Inertia is implemented by sending only a part of the accelerated x,y difference and keeping the
	       * remainder in the (store_dx,store_dy) registers.
	       */

	      int keep_dx = MOVE_X_INERTIA*stored_dx/100;
	      int keep_dy = MOVE_Y_INERTIA*stored_dy/100;

	      int send_dx = stored_dx-keep_dx;
	      int send_dy = stored_dy-keep_dy;

	      zmk_hid_mouse_movement_set (send_dx,send_dy);

	      stored_dx = keep_dx;
	      stored_dy = keep_dy;

	      send_report = true;
	      clear = PIM447_MOVE;
	    }
	  }

	  break;

	case PIM447_SCROLL:

	  {
	    int dx = pos_dx.val1 / DIVISOR_X;
	    int dy = pos_dy.val1 / DIVISOR_Y;

	    zmk_hid_mouse_scroll_set (dx,dy);

	    send_report = true;
	    clear = PIM447_MOVE;
	  }

	  break;
	}

        if (pos_dz.val1 == 0x80 && button_press_sent == false) {
            zmk_hid_mouse_button_press(BUTTON);
            button_press_sent   = true;
            button_release_sent = false;
            send_report = true;
        } else if (pos_dz.val1 == 0x01 && button_release_sent == false) {
            zmk_hid_mouse_button_release(BUTTON);
            button_press_sent   = false;
            button_release_sent = true;
            send_report = true;
        }

        if (send_report) {
            zmk_endpoints_send_mouse_report();

            switch (clear) {
                case PIM447_MOVE: zmk_hid_mouse_movement_set(0, 0); break;
                case PIM447_SCROLL: zmk_hid_mouse_scroll_set(0, 0); break;
                default: break;
            }
        }

        k_sleep (K_MSEC (POLL_INTERVAL));
    }
}

#define STACK_SIZE 1024

static K_THREAD_STACK_DEFINE(thread_stack, STACK_SIZE);
static struct k_thread thread;

int zmk_trackball_pim447_init()
{
    k_thread_create(&thread, thread_stack, STACK_SIZE, thread_code,
                    NULL, NULL, NULL, K_PRIO_PREEMPT(8), 0, K_NO_WAIT);
    return 0;
}

SYS_INIT(zmk_trackball_pim447_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
