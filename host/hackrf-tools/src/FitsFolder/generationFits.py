from astropy.io import fits
from astropy.io.fits import Header
import numpy as np
import pandas as pd


def generateFits(n_channels, triggering_times):
    # input data (samples ordered) of SDR as byte
    image_data = np.ones((n_channels, triggering_times), dtype='i1')

    # Create PrimaryHDU to encapsulate the data
    image = fits.PrimaryHDU(data=image_data)

    # Create an HDUList to contain the newly created PrimaryHDU and write to a new file
    hdul = fits.HDUList([image])

    # Update headers
    hdul[0].header.append(("DATE", "2022-12-15", "Time of observation"))
    hdul[0].header.append(("CONTENT", "2022/12/15 Radio Flux density, HackRF-One (Spain)", "Title"))

    hdul[0].header.append(("INSTRUME", "HACKRF One", "Name of the instrument"))
    hdul[0].header.append(("OBJECT", "Space", "Object name"))

    hdul[0].header.append(("DATE-OBS", "2022/12/15", "Date observation starts"))
    hdul[0].header.append(("TIME-OBS", "14:47:32.897", "Time observation starts"))
    hdul[0].header.append(("DATE-END", "2022/12/15", "Date Observation ends"))
    hdul[0].header.append(("TIME-END", "15:02:43", "Time observation ends"))

    hdul[0].header.append(("BZERO", 0., "Scaling offset"))
    hdul[0].header.append(("BSCALE", 1., "Scaling factor"))
    hdul[0].header.append(("BUNIT", "digits", "Z - axis title"))

    hdul[0].header.append(("DATAMAX", -5, "Max pixel data"))
    hdul[0].header.append(("DATAMIN", -255, "Min pixel data"))

    hdul[0].header.append(("CRVAL1", 53252, "Value on axis 1 at the reference pixel [sec of day]"))
    hdul[0].header.append(("CRPIX1", 0, "Reference pixel of axis 1"))
    hdul[0].header.append(("CTYPE1", "TIME [UT]", "Title of axis 1"))
    hdul[0].header.append(("CDELT1", 0.25, "Step between first and second element in x-axis"))

    hdul[0].header.append(("CRVAL2", 45, "Value on axis 2 at the reference pixel"))
    hdul[0].header.append(("CRPIX2", 0, "Reference pixel of axis 2"))
    hdul[0].header.append(("CTYPE2", "Frequency [MHz]", "Title of axis 2"))
    hdul[0].header.append(("CDELT2", -1, "Step samples"))

    hdul[0].header.append(("OBS_LAT", 40.5131623, "Observatory latitude in degree"))
    hdul[0].header.append(("OBS_LAC", "N", "Observatory latitude code {N, S}"))
    hdul[0].header.append(("OBS_LON", -3.3527243, "Observatory longitude in degree"))
    hdul[0].header.append(("OBS_LOC", "W", " Observatory longitude code {E, W}"))
    hdul[0].header.append(("OBS_ALT", 594., "Observatory altitude in meter"))

    # Create binary table
    c1 = fits.Column(name="Frequency", array=np.array([np.arange(triggering_times)]), format='3600D8.3')
    c2 = fits.Column(name="Times", array=np.array([np.arange(n_channels)]), format='200D8.3')
    binary_table = fits.BinTableHDU.from_columns([c1, c2])
    hdul.append(binary_table)

    # Create the fits file
    hdul.writeto("Example.fits", overwrite=True)

    # FIT-file structure
    print("Fits file Structure\n", hdul.info())

    print("Image data\n", hdul[0].data)     # sdr data
    print("Headers image data\n", hdul[0].header)   # oK except values needed to take from funcs

    print("Binary table data", hdul[1].data)    # times and frequency datas
    print("Headers bin table", hdul[1].header)  # ok


if __name__ == "__main__":
    triggering_times = 3600
    n_channels = 200
    generateFits(n_channels, triggering_times)
