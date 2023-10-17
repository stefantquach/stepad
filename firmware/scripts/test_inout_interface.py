import hid

# default is TinyUSB (0xcafe), Adafruit (0x239a), RaspberryPi (0x2e8a), Espressif (0x303a) VID
USB_VID = (0xcafe, 0x239a, 0x2e8a, 0x303a)

print("VID list: " + ", ".join('%02x' % v for v in USB_VID))

for vid in  USB_VID:
    for dict in hid.enumerate(vid):
        print(dict)
        if(dict["interface_number"] == 1):
            dev = hid.device(dict['vendor_id'], dict['product_id'])
            dev.open(dict['vendor_id'], dict['product_id'])
            if dev:
                msg = bytes([0x00, 0x01]) # Get version
                # msg = bytes([0x00, 0x03, 0x03, 0x1]) # get switch settings
                # msg = bytes([0x00, 0x02, 0x06, 0x0, 0x1F, 0]) # set switch settings
                # msg = bytes([0x00, 0x05, 0x03, 0x3]) # get switch keymap
                # msg = bytes([0x00, 0x04, 0x07, 0x3, 0x00, 0x00, 0x22, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01]) # set switch keymap
                # msg = bytes([0x00, 0x06])

                # Set switch 1 to z
                # set swtich 2 to x
                
                dev.write(msg)
                print("writing")
                str_in = dev.read(64)
                print("Received from HID Device:", str_in, '\n')
                # print("report_id: %x" % (str_in[0]))
                print("message_id: %d" % (str_in[0]))
                print("message_size: %d" % (str_in[1]))
                print("ACKNACK: %d" % (str_in[2]))
                print("message_body: ",  str_in[3:])
                
                exit()
