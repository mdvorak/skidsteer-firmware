#include <string.h>
#include <uni.h>
#include <parser/uni_hid_parser_wii.h>
#include "skid.h"

#define constrain(amt, low, high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))

// Custom "instance"
typedef struct my_platform_instance_s {
    uni_gamepad_seat_t gamepad_seat;  // which "seat" is being used
} my_platform_instance_t;

// Declarations
static void trigger_event_on_gamepad(uni_hid_device_t *d);

static my_platform_instance_t *get_my_platform_instance(uni_hid_device_t *d);

//
// Platform Overrides
//
static void my_platform_init(int argc, const char **argv) {
    ARG_UNUSED(argc);
    ARG_UNUSED(argv);

    // Configure WiiMote
    wii_force_flags(WII_FLAGS_NATIVE);

    logi("custom: init()\n");
}

static void my_platform_on_init_complete(void) {
    logi("custom: on_init_complete()\n");

    // Initial values
    skid_motor_set(&SKID_MOTOR_LEFT, 0);
    skid_motor_set(&SKID_MOTOR_RIGHT, 0);
    skid_motor_set(&SKID_MOTOR_ARM, SKID_MOTOR_HOLD);

    skid_servo_set(&SKID_SERVO_BUCKET, 90);
    skid_servo_set(&SKID_SERVO_AUX, 170);

    // Listen
    uni_bt_enable_new_connections_unsafe(true);
}

static void my_platform_on_device_connected(uni_hid_device_t *d) {
    logi("custom: device connected: %p\n", d);
    // Only one controller allowed
    uni_bt_enable_new_connections_unsafe(false);
}

static void my_platform_on_device_disconnected(uni_hid_device_t *d) {
    logi("custom: device disconnected: %p\n", d);
    uni_bt_enable_new_connections_unsafe(true);
}

static uni_error_t my_platform_on_device_ready(uni_hid_device_t *d) {
    logi("custom: device ready: %p\n", d);
    my_platform_instance_t *ins = get_my_platform_instance(d);
    ins->gamepad_seat = GAMEPAD_SEAT_A;

    trigger_event_on_gamepad(d);
    return UNI_ERROR_SUCCESS;
}

static const int AXIS_MAX = 360;
static const float SERVO_STEP = 1;
static float bucket_movement = 0;
static float aux_movement = 0;

static void my_platform_on_gamepad(const uni_gamepad_t *gp) {
    // Tracks
    if (gp->dpad & DPAD_UP) {
        skid_motor_set(&SKID_MOTOR_LEFT, 1);
        skid_motor_set(&SKID_MOTOR_RIGHT, 1);
    } else if (gp->dpad & DPAD_DOWN) {
        skid_motor_set(&SKID_MOTOR_LEFT, -1);
        skid_motor_set(&SKID_MOTOR_RIGHT, -1);
    } else if (gp->dpad & DPAD_LEFT) {
        skid_motor_set(&SKID_MOTOR_LEFT, -1);
        skid_motor_set(&SKID_MOTOR_RIGHT, 1);
    } else if (gp->dpad & DPAD_RIGHT) {
        skid_motor_set(&SKID_MOTOR_LEFT, 1);
        skid_motor_set(&SKID_MOTOR_RIGHT, -1);
    } else {
        int32_t axisX = constrain(gp->axis_x, -AXIS_MAX, AXIS_MAX);
        int32_t axisY = constrain(gp->axis_y, -AXIS_MAX, AXIS_MAX);

        double leftMotor = (double) constrain(-axisY + axisX, -AXIS_MAX, AXIS_MAX) / (double) AXIS_MAX;
        double rightMotor = (double) constrain(-axisY - axisX, -AXIS_MAX, AXIS_MAX) / (double) AXIS_MAX;

        skid_motor_set(&SKID_MOTOR_LEFT, leftMotor);
        skid_motor_set(&SKID_MOTOR_RIGHT, rightMotor);
    }

    // Arm
    if (gp->buttons & BUTTON_A) {
        skid_motor_set(&SKID_MOTOR_ARM, 1);
    } else if (gp->buttons & BUTTON_B) {
        skid_motor_set(&SKID_MOTOR_ARM, -1);
    } else {
        skid_motor_set(&SKID_MOTOR_ARM, SKID_MOTOR_HOLD);
    }

    // Bucket
    if (gp->buttons & BUTTON_X || gp->buttons & BUTTON_TRIGGER_L) {
        bucket_movement = SERVO_STEP;
    } else if (gp->buttons & BUTTON_Y || gp->buttons & BUTTON_SHOULDER_L) {
        bucket_movement = -SERVO_STEP;
    } else {
        bucket_movement = 0;
    }

    // Aux
    if (gp->misc_buttons & MISC_BUTTON_START) {
        aux_movement = SERVO_STEP;
    } else if (gp->buttons & MISC_BUTTON_SELECT) {
        aux_movement = -SERVO_STEP;
    } else {
        aux_movement = 0;
    }
}

static void my_platform_on_controller_data(uni_hid_device_t *d, uni_controller_t *ctl) {
    switch (ctl->klass) {
        case UNI_CONTROLLER_CLASS_GAMEPAD:
            my_platform_on_gamepad(&ctl->gamepad);
        default:
            break;
    }
}

static void my_platform_on_oob_event(uni_platform_oob_event_t event, void *data) {
    switch (event) {
        case UNI_PLATFORM_OOB_GAMEPAD_SYSTEM_BUTTON: {
            uni_hid_device_t *d = data;

            if (d == NULL) {
                loge("ERROR: my_platform_on_oob_event: Invalid NULL device\n");
                return;
            }
            logi("custom: on_device_oob_event(): %d\n", event);

            my_platform_instance_t *ins = get_my_platform_instance(d);
            ins->gamepad_seat = ins->gamepad_seat == GAMEPAD_SEAT_A ? GAMEPAD_SEAT_B : GAMEPAD_SEAT_A;

            trigger_event_on_gamepad(d);
            break;
        }

        case UNI_PLATFORM_OOB_BLUETOOTH_ENABLED:
            logi("custom: Bluetooth enabled: %d\n", (bool) (data));
            break;

        default:
            logi("my_platform_on_oob_event: unsupported event: 0x%04x\n", event);
            break;
    }
}

//
// Helpers
//
static my_platform_instance_t *get_my_platform_instance(uni_hid_device_t *d) {
    return (my_platform_instance_t *) &d->platform_data[0];
}

static void trigger_event_on_gamepad(uni_hid_device_t *d) {
    my_platform_instance_t *ins = get_my_platform_instance(d);

    if (d->report_parser.play_dual_rumble != NULL) {
        d->report_parser.play_dual_rumble(d, 0 /* delayed start ms */, 150 /* duration ms */, 128 /* weak magnitude */,
                                          40 /* strong magnitude */);
    }

    if (d->report_parser.set_player_leds != NULL) {
        d->report_parser.set_player_leds(d, ins->gamepad_seat);
    }

    if (d->report_parser.set_lightbar_color != NULL) {
        uint8_t red = (ins->gamepad_seat & 0x01) ? 0xff : 0;
        uint8_t green = (ins->gamepad_seat & 0x02) ? 0xff : 0;
        uint8_t blue = (ins->gamepad_seat & 0x04) ? 0xff : 0;
        d->report_parser.set_lightbar_color(d, red, green, blue);
    }
}

//
// Entry Point
//
struct uni_platform *get_my_platform(void) {
    static struct uni_platform plat = {
            .name = "custom",
            .init = my_platform_init,
            .on_init_complete = my_platform_on_init_complete,
            .on_device_connected = my_platform_on_device_connected,
            .on_device_disconnected = my_platform_on_device_disconnected,
            .on_device_ready = my_platform_on_device_ready,
            .on_oob_event = my_platform_on_oob_event,
            .on_controller_data = my_platform_on_controller_data,
            .get_property = NULL,
    };

    return &plat;
}
