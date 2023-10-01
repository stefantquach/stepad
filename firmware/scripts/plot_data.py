import pandas as pd
import numpy as np
import matplotlib.pyplot as plt

df = pd.read_csv("scripts/data/data_log5.csv")

t = np.array(df["index"]) * 0.001 # Millisecond accuracy log
travel = np.array(df["sw1 travel"])
pressed = np.array(df["sw1 pressed"])


factor = -1

ax = plt.subplot(1,1,1)
ax.plot(t, factor*travel, 'x-')
# ax.plot(t, factor*mm)
ax.fill_between(t, 0, 1, where=pressed, alpha=0.4, transform=ax.get_xaxis_transform(), color='red')
# ax.fill_between(t, 0, 1, where=rt_active, alpha=0.4, transform=ax.get_xaxis_transform(), color='green')
# ax.hlines(factor*threshold, t[0], t[-1], color='black')
# ax.hlines(factor*(threshold-SWITCH_HYSTERESIS_COUNTS), t[0], t[-1], color='orange', alpha=0.5)
# ax.hlines(factor*(threshold+SWITCH_HYSTERESIS_COUNTS), t[0], t[-1], color='orange', alpha=0.5)
plt.grid(True, which='both')

plt.show()