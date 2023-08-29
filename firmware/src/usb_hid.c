#include "usb_hid.h"
#include "parameters.h"
#include "switch.h"
#include "keymap.h"
#include "keycodes.h"
#include <stdbool.h>
#include <stdint.h>
#include "tusb.h"
#include "usb_descriptors.h"


/**
 * 
*/
void send_keyboard_report()
{
    int i;
    // bool key_press_to_send;
    uint8_t keycodes[6] = { 0 }; // 6 is the number of keycodes sent in 6 key rollover keyboard reports
    uint8_t modifier;
    uint16_t keycode;
    int keycode_index = 0;

    // skip if hid is not ready yet
    if ( !tud_hid_ready() ) return;

    // key_press_to_send = false;
    modifier = 0;
    for(i = 0; i < TOTAL_NUM_SWITCH; ++i)
    {
        if(switch_pressed[i])
        {
            keycode = get_keycode(i);
            if(IS_KC_BASIC(keycode))
            {
                // checking for modifier keys
                if(keycode >= KC_LEFT_CTRL && keycode <= KC_RIGHT_GUI)
                {
                    modifier |= keycode & 0xF;
                }
                else
                {
                    keycodes[keycode_index] = keycode;
                    ++keycode_index;
                }
            }
        }
    }

    // TODO wake up
    // // Remote wakeups
    // if ( tud_suspended() && btn )
    // {
    //     // Wake up host if we are in suspend mode
    //     // and REMOTE_WAKEUP feature is enabled by host
    //     tud_remote_wakeup();
    // }

    // TODO figure out a way to prevent sending multiple zero keyboard reports
    tud_hid_keyboard_report(REPORT_ID_KEYBOARD, modifier, keycodes);
}
