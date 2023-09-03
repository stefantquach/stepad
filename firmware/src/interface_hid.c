#include "interface_hid.h"
#include "settings.h"
#include "usb_descriptors.h"
#include "tusb.h"
#include "parameters.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * This is a custom messaging interface in order for the host PC to set
 * settings on the keyboard.
 * 
 * The system leverages the generic in/out HID interface for low-rate data. Due to
 * USB limitations, the maximum message size is 64 bytes.
 * 
 * Each message contains a header describing the following
 *  - message ID.
 *  - Size field.
 * 
 * Following the header is simply the data payload of variable size, ranging from 0 to 62 bytes.
 * Each message ID is intended to describe a specific function.
 * 
 * After the keyboard receives a command, it is supposed to reply with an ACK/NACK to indicate whether
 * or not the command succeeded.
*/
#define MSG_MAX_SIZE CFG_TUD_HID_EP_BUFSIZE

// Enumeration for the different message IDs
enum message_id
{
    MSG_ACK_NACK, // Message ID zero for ack/nack and response messages
    GET_VERSION,
    SET_KEY_SETTINGS,
    GET_KEY_SETTINGS,
    SET_KEYMAP,
    GET_KEYMAP,

    NUM_MESSAGE_IDS
};

enum message_error_code
{
    MSG_ACK_SUCCESS,
    MSG_NACK_INVALID_SIZE,
    MSG_NACK_INVALID_SWITCH_ID,
};

// Header struct of the message
typedef struct message_struct
{
    uint8_t message_id;
    uint8_t message_size;
} message_struct_t;


// Forward declarations
void send_ack_nack(int8_t error_code, uint8_t* data_buf, uint16_t bufsize);

/**
 * Main call to parse the incoming message from the PC.
*/
void parse_message(const uint8_t* buf, uint16_t bufsize)
{
    message_struct_t* message_header = (message_struct_t*)buf;
    const uint8_t* message_body = &buf[sizeof(message_struct_t)];
    if(message_header->message_size >= MSG_MAX_SIZE)
    {
        // return a NACK
        send_ack_nack(MSG_NACK_INVALID_SIZE, NULL, 0);
        return;
    }

    uint8_t reply_buf[MSG_MAX_SIZE] = {0};
    switch(message_header->message_id)
    {
        case MSG_ACK_NACK:
            // Keyboard should not send ACK/NACK. Its a response
            // Do nothing if we receive this message
            break;

        case GET_VERSION:
        {
            // this message has no payload. Just reply with the version number
            reply_buf[0] = 1; // TODO fill in BUILD version
            reply_buf[1] = 0; // TODO fill in MINOR version
            reply_buf[2] = 1; // TODO fill in MAJOR version
            send_ack_nack(MSG_ACK_SUCCESS, reply_buf, 3);
            break;
        }

        case SET_KEY_SETTINGS:
        {
            // 0: lekker switch_id
            // 1: new threshold value
            // 2: new rapid trigger setting
            uint8_t switch_id  = message_body[0];
            uint8_t new_threshold = message_body[1];
            bool new_trigger = message_body[2];
            if(switch_id >= NUM_LEKKER_SWITCH)
            {
                // send a NACK
                send_ack_nack(MSG_NACK_INVALID_SWITCH_ID, NULL, 0);
            }
            else
            {
                // set settings
                settings.switch_config[switch_id].threshold = new_threshold;
                settings.switch_config[switch_id].rapid_trigger = new_trigger;
                // Send and ACK in return
                send_ack_nack(MSG_ACK_SUCCESS, NULL, 0);
            }
            break;
        }

        case GET_KEY_SETTINGS:
        {
            // incoming message body is just the lekker switch ID
            uint8_t switch_id = message_body[0];
            if(switch_id >= NUM_LEKKER_SWITCH)
            {
                // send a NACK
                send_ack_nack(MSG_NACK_INVALID_SWITCH_ID, NULL, 0);
            }
            else
            {
                // create reply
                reply_buf[0] = switch_id;
                reply_buf[1] = settings.switch_config[switch_id].threshold;
                reply_buf[2] = settings.switch_config[switch_id].rapid_trigger;
                // Send ACK
                send_ack_nack(MSG_ACK_SUCCESS, reply_buf, 3);
            }
            break;
        }

        case SET_KEYMAP:
        {
            // 0: Switch ID of ANY switch
            // 1: padding (zero)
            // 2 to 2*MAX LAYERS + 2: keymap for the given key for all layers 
            uint8_t switch_id = message_body[0];
            if(switch_id >= TOTAL_NUM_SWITCH)
            {
                // send a NACK
                send_ack_nack(MSG_NACK_INVALID_SWITCH_ID, NULL, 0);
            }
            else
            {
                uint16_t* u16_message_buf = (uint16_t*)&message_body[2];
                for(int i=0; i < MAX_KEY_LAYERS; ++i)
                {
                    settings.keymap[i][switch_id] = u16_message_buf[i];
                }
                // Send and ACK in return
                send_ack_nack(MSG_ACK_SUCCESS, NULL, 0);
            }
            break;
        }

        case GET_KEYMAP:
        {
            // 0: Switch ID of ANY switch
            uint8_t switch_id = message_body[0];
            if(switch_id >= TOTAL_NUM_SWITCH)
            {
                // send a NACK
                send_ack_nack(MSG_NACK_INVALID_SWITCH_ID, NULL, 0);
            }
            else
            {
                reply_buf[0] = switch_id;
                uint16_t* u16_reply_buf = (uint16_t*)&reply_buf[2];
                // construct response message
                for(int i=0; i<MAX_KEY_LAYERS; ++i)
                {
                    u16_reply_buf[i] = settings.keymap[i][switch_id];
                }
                send_ack_nack(MSG_ACK_SUCCESS, reply_buf, 2 + MAX_KEY_LAYERS*2);
            }
            break;
        }
            
        default:
            // Unrecognized message_id. NACK
            break;
    }
    return;
}


/**
 * Send an ACK/NACK message. The format of the body of the messaage is as follows:
 *  - (int8_t) Error code (0 indicates success)
 *  - variably sized response data.
 * 
 * Data size in bytes
*/
void send_ack_nack(int8_t error_code, uint8_t* data_buf, uint16_t bufsize)
{
    uint8_t msg_buf[MSG_MAX_SIZE] = {0};
    msg_buf[0] = MSG_ACK_NACK;
    msg_buf[1] = sizeof(message_struct_t) + 1 + bufsize;
    msg_buf[2] = error_code;

    // copy data into buffer if there is data to copy
    if(data_buf && bufsize > 0)
    {
        memcpy(&msg_buf[3], data_buf, bufsize);
    }

    // Send the message. Size is data size + header + error code
    // tud_hid_n_report(ITF_NUM_HID_LR, 0, msg_buf, bufsize+sizeof(message_struct_t)+1);
    tud_hid_n_report(ITF_NUM_HID_LR, 0, msg_buf, 64);
}


// ---------------------------------- TinyUSB Callbacks --------------------------------------
// Unused callbacks
void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report, uint16_t len)
{
}

/**
 * Invoked when received GET_REPORT control request or
 * output data on IN endpoint ( Report ID = 0, Type = 0 )
 * 
 * We use this call back to process incoming messages from the PC to change the settings on the board.
*/
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
  // TODO not Implemented
  (void) instance;
  (void) report_id;
  (void) report_type;
  (void) buffer;
  (void) reqlen;

  return 0;
}

/**
 * Invoked when received SET_REPORT control request or
 * received data on OUT endpoint ( Report ID = 0, Type = 0 )
 * 
 * We use this call back to process incoming messages from the PC to change the settings on the board.
*/
void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
    // anything on itf == 0 should be ignored
    // echo back anything we received from host
    if(itf == ITF_NUM_HID_LR)
    {
        // printf("Received set_report on itf 1\n");
        // for(int i=0; i<bufsize; ++i)
        // {
        //     printf("%x ", buffer[i]);
        // }
        // printf("\n");
        // tud_hid_n_report(ITF_NUM_HID_LR, 0, buffer, bufsize);
        parse_message(buffer, bufsize);
    }
}
