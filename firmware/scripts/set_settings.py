import hid
import struct

def write_msg_to_inout(dev, message_id, size, payload : bytes):
    header = bytes([0x00, message_id, size])
    dev.write(header + payload)
    print(header+payload)
    print("writing")
    str_in = dev.read(64)
    print("Received from HID Device:", str_in, '\n')
    print("message_id: %d" % (str_in[0]))
    print("message_size: %d" % (str_in[1]))
    print("ACKNACK: %d" % (str_in[2]))
    print("message_body: ",  str_in[3:])
    message_id = str_in[0]
    message_size = str_in[1]
    ACK_NACK = str_in[2]
    payload = str_in[3:]

    return (message_id, message_size, ACK_NACK, payload)



def write_switch_settings(dev, lekker_switch_id : int, threshold : int, rapid_trigger_mode : int, rapid_trigger_sens : int):
    msg = bytes([lekker_switch_id, threshold, rapid_trigger_mode, rapid_trigger_sens])
    _, _, ACK, _ = write_msg_to_inout(dev, 0x02, 0x07, msg)
    return ACK == 0



def read_switch_settings(dev, lekker_switch_id : int):
    msg = bytes([lekker_switch_id])
    _, _, ACK, response = write_msg_to_inout(dev, 0x03, 0x03, msg)
    if ACK != 0:
        return None
    
    print("reading switch settings")
    print("Switch ID: %d" % (int(response[0])))
    print("Switch threshold: %d" % (int(response[1])))
    print("Rapid trigger mode: %d" % (int(response[2])))
    print("Rapid trigger sensitivity: %d" % (int(response[3])))


def set_keymap(dev, switch_id, keymap):
    encoded = struct.pack("<{}H".format(len(keymap)), *keymap)
    msg = bytes([switch_id, 0x00]) + encoded
    _, _, ACK, response = write_msg_to_inout(dev, 0x04, 0x03 + 1 + 2*len(keymap), msg)
    if ACK != 0:
        print("NACK")
        return None


def read_keymap(dev, switch_id):
    msg = bytes([switch_id])
    _, _, ACK, response = write_msg_to_inout(dev, 0x05, 0x03, msg)
    if ACK != 0:
        return None
    
    print("reading switch settings")
    print("Switch ID: %d" % (int(response[0])))
    print("Keycodes:" + str(response[1:]))


def write_settings_to_flash(dev):
    dummy = bytes([0])
    _, _, ACK, _ = write_msg_to_inout(dev, 0x06, 0x02, dummy )
    return ACK == 0

    


# default is TinyUSB (0xcafe), Adafruit (0x239a), RaspberryPi (0x2e8a), Espressif (0x303a) VID
USB_VID = (0xcafe, 0x239a, 0x2e8a, 0x303a)

print("VID list: " + ", ".join('%02x' % v for v in USB_VID))

for vid in  USB_VID:
    for dict in hid.enumerate(vid):
        print(dict)
        if(dict["interface_number"] == 1):
            dev = hid.device(dict['vendor_id'], dict['product_id'])
            dev.open(dict['vendor_id'], dict['product_id'])

            if (dev):
                # write_switch_settings(dev, 0, 51, 2, 30)
                # write_switch_settings(dev, 1, 51, 2, 30)

                # read_switch_settings(dev, 0)
                # read_switch_settings(dev, 1)

                # set_keymap(dev, 2, [0x1D, 0x02, 0x04, 0x03])
                # set_keymap(dev, 3, [0x1B, 0x01, 0x01, 0x01])

                read_keymap(dev, 2)
                # read_keymap(dev, 3)

                write_settings_to_flash(dev)