# -*- coding: utf-8 -*-
"""
Created on Mon Sep 05 12:02:42 2016
Lecture 3.1/9 How to plot single spectrum
@author: cmonstei
"""

from astropy.io import fits

# -----------------------------------------------------------------------------

files = 'hackRFOne_UAH_20221215_14h_47m.fit'

hdu = fits.open(files)
print(hdu.info())  # FIT-file structure
print(hdu[0].header)
print(hdu[1].header)
print("Datamax", hdu[0].header["DATAMAX"])
print("Datamin", hdu[0].header["DATAMIN"])
print("BSCALE", hdu[0].header["BSCALE"])
print("BZERO", hdu[0].header["BZERO"])
hdu.close()

"""
data = hdu[0].data
# freqs = hdu[1].data['Frequency'][0] # extract frequency axis
# time  = hdu[1].data['Time'][0] # extract time axis
hdu.close()
# -----------------------------------------------------------------------------

plt.figure(figsize=(12, 8))
plt.imshow(data, aspect='auto', cmap='jet')
plt.tick_params(labelsize=14)
plt.xlabel('Sample of ' + files, fontsize=15)
plt.ylabel('Observed frequency [channel number]', fontsize=15)
plt.title('FIT file presentation as original raw data, full size', fontsize=15)
plt.savefig('sdrdata.png')
plt.show()
# -----------------------------------------------------------------------------
"""