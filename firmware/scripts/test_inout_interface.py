import hid

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
        if dev:
            msg = bytes([0x00, 0x01])
            dev.write(msg)
            print("writing")
            str_in = dev.read(64)
            print("Received from HID Device:", str_in, '\n')
            exit()
                