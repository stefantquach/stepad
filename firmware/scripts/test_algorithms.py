import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from scipy import signal

SWITCH_HYSTERESIS_COUNTS = 3

df = pd.read_csv("scripts/data/data_log.csv")

t = np.array(df["index"]) * 0.001 # Millisecond accuracy log
travel = np.array(df["sw0 travel"])

def normal_mode(data, threshold):
    pressed = []
    a = False
    for val in data:
        if a:
            a = val > (threshold - SWITCH_HYSTERESIS_COUNTS)
        else:
            a = val > (threshold + SWITCH_HYSTERESIS_COUNTS)
        # last_value = a
        pressed.append(a)

    return pressed

def rapid_trigger(data, threshold):
    # Iterate over the data to simulate actual software implementation
    pressed = []
    velocity_lpf = []
    velocity = []
    accel = []
    last_value = 0

    # Velocity lpf
    fc = 100
    alpha = np.exp(-2*np.pi*fc*0.001)
    # alpha = 0.93910137 # for 200Hz LPF

    fc2 = 10
    alpha2 = np.exp(-2*np.pi*fc2*0.001)

    debounce_limit = 4

    a = False
    lpf_diff = 0
    lpf_diff2 = 0
    last_lpf_diff = 0
    debounce_count = 0
    for val in data:
        # Continuously calculate diff
        diff = val - last_value
        lpf_diff = lpf_diff*alpha + diff*(1-alpha)
        # lpf_diff = diff
        # lpf_diff = np.fix(lpf_diff)
        # lpf_diff = np.fix(lpf_diff/2)*2

        # # Just first derivative
        # if val > threshold + SWITCH_HYSTERESIS_COUNTS:
        #     # Once crossing the threshold, check if its the first time, after which set the switch state to pressed.
        #     if last_value <= threshold + SWITCH_HYSTERESIS_COUNTS:
        #         a = True
        #     else:
        #         # After the first value just apply normal rapid trigger
        #         if (not a and lpf_diff > 2):
        #             a = True
        #         if (a and lpf_diff < -2):
        #             a = False
        
        # # First derivative with Second derivative
        # diff2 = lpf_diff - last_lpf_diff
        # lpf_diff2 = lpf_diff2*alpha2 + diff2*(1-alpha2)
        # if val > threshold + SWITCH_HYSTERESIS_COUNTS:
        #     # Once crossing the threshold, check if its the first time, after which set the switch state to pressed.
        #     if last_value <= threshold + SWITCH_HYSTERESIS_COUNTS:
        #         a = True
        #     else:
        #         # After the first value just apply normal rapid trigger
        #         if (not a and lpf_diff > 1 and lpf_diff2 > 0):
        #             a = True
        #         if (a and lpf_diff < -1 and lpf_diff2 < 0):
        #             a = False

        # Standard debouncing
        if val > threshold + SWITCH_HYSTERESIS_COUNTS:
            # Once crossing the threshold, check if its the first time, after which set the switch state to pressed.
            if last_value <= threshold + SWITCH_HYSTERESIS_COUNTS:
                a = True

            debounce_state = a
            if diff > 0:
                debounce_state = True
            if diff < 0:
                debounce_state = False 

            if a != debounce_state:
                debounce_count += 1
                if debounce_count >= debounce_limit:
                    a = debounce_state
                    debounce_count = 0
            else:
                debounce_count = 0

            

        if val < threshold - SWITCH_HYSTERESIS_COUNTS:
            a = False

        pressed.append(a)
        velocity.append(diff)
        velocity_lpf.append(lpf_diff)
        accel.append(lpf_diff2)
        last_value = val
        last_lpf_diff = lpf_diff

    return pressed, (velocity_lpf, velocity, accel)

    # Test stuff offline
    # diff = data[1:] - data[:-1]
    # diff = np.pad(diff, [1,0], mode='constant')
    # diff[0] = 10
    # return diff > 0, diff

    # fc = 250 # 250 Hz
    # fs = 1000
    # num, dem = signal.iirdesign(fc, fc+1 , 1, 60, analog=False, ftype='ellip', fs=fs)
    # tf = signal.TransferFunction(num, dem, dt=1/fs)

    # print(tf)
    # w, mag, phase = signal.dbode(tf)
    # plt.figure()
    # plt.plot(w/(2*np.pi), mag)
    # _, lpf_diff = signal.dlsim(tf, diff)

    # print(lpf_diff.shape)
    # lpf_diff = lpf_diff.reshape(1,len(t))[0]
    # lpf_diff = diff
    # lpf_diff = np.fix(lpf_diff/6)*6
    # return lpf_diff > 0, lpf_diff, diff

    # d_travel = np.gradient(data)

    # fc = 200
    # tf = signal.cont2discrete(([2*np.pi*fc], [1, 2*np.pi*fc]), dt=1/1000)
    # print(tf)
    # w, mag, phase = signal.dbode(tf)
    # plt.figure()
    # plt.plot(w/(2*np.pi), mag)

    # _, lpf_d_travel = signal.dlsim(tf, d_travel)
    # lpf_d_travel = lpf_d_travel.reshape(1,len(t))[0]
    # print(lpf_d_travel)
    # lpf_d_travel =  np.fix(lpf_d_travel/2)*2

    # return lpf_d_travel > 0, lpf_d_travel, d_travel

def rapid_trigger2(data, threshold, continuous=False):
    pressed = []
    active_rt = []
    curr_mm = []

    a = False
    curr_max_min = 0
    rapid_trigger_active = False

    deactivate_thresh = int(0.1*255/4) if continuous else (threshold - SWITCH_HYSTERESIS_COUNTS)

    for val in data:
        if val > threshold + SWITCH_HYSTERESIS_COUNTS and rapid_trigger_active == False:
            # First time passing the threshold set the switch to pressed
            curr_max_min = val
            a = True
            rapid_trigger_active = True

        if val < deactivate_thresh and rapid_trigger_active == True:
            a = False
            rapid_trigger_active = False

        # Only do rapid trigger when below threshold
        if rapid_trigger_active:
            # behavior depends on if the switch is pressed
            if a:
                # Switch is pressed, wait for counts to decrease for unpress. curr_max_min is a MAX
                curr_max_min = val if val > curr_max_min else curr_max_min
                if curr_max_min - val > int(0.1*255/4):
                    a = False
                    curr_max_min = val # curr_max_min becomes a MIN
            else:
                # Switch is released, wait for counts to increase for unpress. curr_max_min is a MIN
                curr_max_min = val if val < curr_max_min else curr_max_min
                if val - curr_max_min > int(0.1*255/4):
                    a = True
                    curr_max_min = val

        pressed.append(a)
        active_rt.append(rapid_trigger_active)
        curr_mm.append(curr_max_min)

    return pressed, active_rt, curr_mm
   
    

threshold = 100

# pressed = np.array(normal_mode(travel, threshold))
# pressed, extras = rapid_trigger(travel, threshold)
pressed, rt_active, mm = rapid_trigger2(travel, threshold, continuous=False)
# lpf_diff, diff, diff2 = extras
pressed = np.array(pressed)
rt_active = np.array(rt_active)
mm = np.array(mm)
# print(diff)

factor = -1

ax = plt.subplot(1,1,1)
ax.plot(t, factor*travel, 'x-')
# ax.plot(t, factor*mm)
ax.fill_between(t, 0, 1, where=pressed, alpha=0.4, transform=ax.get_xaxis_transform(), color='red')
ax.fill_between(t, 0, 1, where=rt_active, alpha=0.4, transform=ax.get_xaxis_transform(), color='green')
ax.hlines(factor*threshold, t[0], t[-1], color='black')
ax.hlines(factor*(threshold-SWITCH_HYSTERESIS_COUNTS), t[0], t[-1], color='orange', alpha=0.5)
ax.hlines(factor*(threshold+SWITCH_HYSTERESIS_COUNTS), t[0], t[-1], color='orange', alpha=0.5)
plt.grid(True, which='both')

# ax2 = plt.subplot(3,1,2, sharex=ax)
# plt.plot(t, lpf_diff, '-', label="lpf_diff")
# plt.plot(t, diff, '-', alpha=0.4, label="diff")
# ax2.hlines(0, t[0], t[-1], color='black')
# ax2.fill_between(t, 0, 1, where=pressed, alpha=0.4, transform=ax2.get_xaxis_transform(), color='red')
# plt.legend()
# plt.grid(True, which='both')

# ax3 = plt.subplot(3,1,3, sharex=ax)
# plt.plot(t, diff2, '-', label="accel")
# ax3.hlines(0, t[0], t[-1], color='black')
# ax3.fill_between(t, 0, 1, where=pressed, alpha=0.4, transform=ax3.get_xaxis_transform(), color='red')
# plt.grid(True, which='both')
# plt.legend()

plt.show()