import numpy as np
import matplotlib.pyplot as plt

log = open('log_files/refresh_rate.txt')
dataRaw1 = log.readlines()
log.close()
log = open('log_files/refresh_rate.txt')
dataRaw2 = log.readlines()
log.close()
log = open('log_files/refresh_rate.txt')
dataRaw3 = log.readlines()
log.close()
log = open('log_files/refresh_rate.txt')
dataRaw4 = log.readlines()
log.close()

length = len(dataRaw1)
frames = list(range(length))

fig = plt.figure(1)

dataHz1 = np.empty([length], dtype=np.float)
dataHz2 = np.empty([length], dtype=np.float)
dataHz3 = np.empty([length], dtype=np.float)
dataHz4 = np.empty([length], dtype=np.float)

for i in frames:
    dataHz1[i] = float(dataRaw1[i])
    dataHz2[i] = float(dataRaw2[i])
    dataHz3[i] = float(dataRaw3[i])
    dataHz4[i] = float(dataRaw4[i])

plt.plot(frames, dataHz1, 'g', label='New implementation, max-P')
plt.plot(frames, dataHz2, 'bo', label='New implementation, max-Q')
plt.plot(frames, dataHz3, 'mx', label='Old implementation, max-P')
plt.plot(frames, dataHz4, 'c-', label='Old implementation, max-Q')

plt.title("Performance of ElasticFusion on NVidia Jetson TX2\nRunning Dataset \"SOMETHING\"")
plt.xlabel("Frame number")
plt.ylabel("Refresh rate (Hz)")
#plt.savefig('testing.png')
#plt.minorticks_on()
#plt.legend(loc='lower right', ncol=1)
#plt.grid(True, which='both')

plt.show()
