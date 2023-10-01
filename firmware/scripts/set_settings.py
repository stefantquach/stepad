import hid

def write_msg_to_inout(dev, msg):
    dev.write(msg)
    print("writing")
    str_in = dev.read(64)
    print("Received from HID Device:", str_in, '\n')
    # print("report_id: %x" % (str_in[0]))
    print("message_id: %d" % (str_in[0]))
    print("message_size: %d" % (str_in[1]))
    print("ACKNACK: %d" % (str_in[2]))
    print("message_body: ",  str_in[3:])


def write_switch_settings(dev, lekker_switch_id : int, threshold : int, rapid_trigger_mode : int, rapid_trigger_sens : int):
    msg = bytes([0x00, 0x02, 0x07, lekker_switch_id, threshold, rapid_trigger_mode, rapid_trigger_sens])
    write_msg_to_inout(dev, msg)    


# default is TinyUSB (0xcafe), Adafruit (0x239a), RaspberryPi (0x2e8a), Espressif (0x303a) VID
USB_VID = (0xcafe, 0x239a, 0x2e8a, 0x303a)

print("VID list: " + ", ".join('%02x' % v for v in USB_VID))

for vid in  USB_VID:
    for dict in hid.enumerate(vid):
        print(dict)
        if(dict["interface_number"] == 0):
            continue
        dev = hid.device(dict['vendor_id'], dict['product_id'])
        dev.open(dict['vendor_id'], dict['product_id'])

        if (dev):
            write_switch_settings(dev, 0, 50, 2, 20)
            write_switch_settings(dev, 1, 50, 2, 20)