import logging as logger

from astropy.io import fits
import numpy as np

from io import open
import pickle

error_code = "ERROR"
success_code = "OK"
hdul = None


def create_image():
    global hdul

    # input data (samples ordered) of SDR as byte
    logger.info("generationFits | createImage() | Creation of fits image")
    logger.info("generationFits | createImage() | Reading power samples from SDR")

    power_sample = read_power_samples()
    if power_sample == error_code:
        logger.error("generationFits | create_image() | Error at reading power samples")

    image_data = np.ones((n_channels, triggering_times), dtype='i1')

   # image_data = insert_data_image(image_data, power_sample)

    if image_data is None:
        logger.error("generationFits | createImage() | Was not possible to read data")
        return error_code

    # Create PrimaryHDU to encapsulate the data
    image = fits.PrimaryHDU(data=image_data)
    if image is None:
        logger.error("generationFits | createImage() | Was not possible to create the image")
        return error_code

    # Create an HDUList to contain the newly created PrimaryHDU and write to a new file

    hdul = fits.HDUList([image])
    if hdul is None:
        logger.error("generationFits | createImage() | Was not possible to create the Primary HDU")

    logger.info("generationFits | createImage() | Execution Success")
    return success_code


def update_headers_image():
    global hdul

    logger.info("generationFits 1 UpdateHeadersImage() | Updating headers of fits image")
    len_headers = len(hdul[0].header)

    # Update headers
    hdul[0].header.append(("DATE", "2022-12-15", "Time of observation"))
    hdul[0].header.append(("CONTENT", "2022/12/15 Radio Flux density, HackRF-One (Spain)", "Title"))

    hdul[0].header.append(("INSTRUME", "HACKRF One", "Name of the instrument"))
    hdul[0].header.append(("OBJECT", "Space", "Object name"))

    hdul[0].header.append(("DATE-OBS", "2022/12/15", "Date observation starts"))
    hdul[0].header.append(("TIME-OBS", "14:47:32.897", "Time observation starts"))
    hdul[0].header.append(("DATE-END", "2022/12/15", "Date Observation ends"))
    hdul[0].header.append(("TIME-END", "15:02:43", "Time observation ends"))

    # hdul[0].header."BZERO", 0, "Scaling offset"))
    # hdul[0].header["BSCALE"] = 1.
    hdul[0].header.append(("BUNIT", "digits", "Z - axis title"))

    hdul[0].header.append(("DATAMAX", -5, "Max pixel data"))
    hdul[0].header.append(("DATAMIN", -255, "Min pixel data"))

    hdul[0].header.append(("CRVAL1", 53252, "Value on axis 1 [sec of day]"))
    hdul[0].header.append(("CRPIX1", 0, "Reference pixel of axis 1"))
    hdul[0].header.append(("CTYPE1", "TIME [UT]", "Title of axis 1"))
    hdul[0].header.append(("CDELT1", 0.25, "Step between first and second element in x-axis"))

    hdul[0].header.append(("CRVAL2", 45, "Value on axis 2 "))
    hdul[0].header.append(("CRPIX2", 0, "Reference pixel of axis 2"))
    hdul[0].header.append(("CTYPE2", "Frequency [MHz]", "Title of axis 2"))
    hdul[0].header.append(("CDELT2", -1, "Step samples"))

    hdul[0].header.append(("OBS_LAT", 40.5131623, "Observatory latitude in degree"))
    hdul[0].header.append(("OBS_LAC", "N", "Observatory latitude code {N, S}"))
    hdul[0].header.append(("OBS_LON", -3.3527243, "Observatory longitude in degree"))
    hdul[0].header.append(("OBS_LOC", "W", " Observatory longitude code {E, W}"))
    hdul[0].header.append(("OBS_ALT", 594., "Observatory altitude in meter"))

    if len_headers == len(hdul[0].header):
        return error_code

    logger.info("generationFits | UpdateHeadersImage() | Execution Success")
    return success_code


def create_binary_table():
    global hdul

    frequencies = np.arange(n_channels)
    frequencies = read_frequencies()

    times = np.arange(triggering_times)
    times = read_times()

    # Create binary table
    logger.info("generationFits | createBinaryTable() | Creating binary table of dimensions 1x2")
    c1 = fits.Column(name="Time", array=np.array([times]), format='3600D8.3')
    c2 = fits.Column(name="Frequency", array=np.array([frequencies]), format='200D8.3')
    binary_table = fits.BinTableHDU.from_columns([c1, c2])

    hdul.append(binary_table)

    if binary_table is None or len(hdul) == 1:
        logger.error("generationFits | createBinaryTable() | Was not possible to create binary table")
        return error_code

    logger.info("generationFits | createBinaryTable() | Execution Success")
    return success_code


def generate_dynamic_name():
    global hdul

    extension = ".fit"
    date_obs = hdul[0].header["DATE-OBS"].replace("/", "") + "_"
    start_date = hdul[0].header["TIME-OBS"]
    format_date = start_date[:3].replace(":", "h_") + start_date[3:6].replace(":", "m")

    fits_name = "hackRFOne_UAH_" + date_obs + format_date + extension

    logger.info("generationFits | generate_dynamic_name() | File generated with name: " + fits_name)
    return fits_name


def read_power_samples():
    logger.info("generationFits | read_power_samples() | Reading samples as output of SDR")

    sample_formated = []
    fichero = open("data_test.txt", "r")
    if fichero is None:
        return error_code

    samples_not_formated = fichero.readlines()
    for sample in samples_not_formated:
        sample_formated.append(int(sample.replace("\n", "")))

    fichero.close()

    logger.info("generationFits | read_power_samples() | Execution Success")
    return sample_formated


def insert_data_image(image_data, power_sample):
    logger.info("generationFits | insert_data_image() | Inserting data into image")

    i = 0
    for times in range(triggering_times):
        for frequencies in range(n_channels):
            image_data[times][frequencies] = power_sample[i]
            i += 1

    logger.info("generationFits | insert_data_image() | Execution Success")
    return image_data


def read_frequencies():
    logger.info("generationFits | read_frequencies() | Reading frequencies as output of SDR")
    frequency_formated = []

    fichero = open("freq_test.txt", "r")
    if fichero is None:
        return error_code

    frequency_not_formated = fichero.readlines()
    for freq in frequency_not_formated:
        frequency_formated.append(int(freq.replace("\n", "")))

    fichero.close()

    logger.info("generationFits | read_frequencies() | Execution Success")
    return frequency_formated


def read_times():
    logger.info("generationFits | read_times() | Reading frequencies as output of SDR")
    times_formated = []

    fichero = open("times_test.txt", "r")
    if fichero is None:
        return error_code

    times_not_formated = fichero.readlines()
    for times in times_not_formated:
        times_formated.append(float(times.replace("\n", "")))

    fichero.close()
    logger.info("generationFits | read_times() | Execution Success")
    return times_formated


def print_fits_info():
    global hdul
    # FIT-file structure
    print(hdul.info())

    logger.info("----------Image data----------")
    logger.info(hdul[0].data)  # sdr data

    logger.info("----------Headers image data----------")
    logger.info(hdul[0].header)

    logger.info("----------Binary table data----------")
    logger.info(hdul[1].data)  # times and frequency datas

    logger.info("----------Headers binary table data----------")
    logger.info("Headers bin table", hdul[1].header)  # ok


def generate_fits(n_channels, triggering_times):
    global hdul

    logger.info("generationFits | Initializing generation of fits file through python")
    logger.info("generationsFits | Dimensions Fits file: " + str(triggering_times) + "x" + str(n_channels))

    # Create image from data received as output of SDR as a .txt
    if create_image() != success_code or hdul is None:
        return error_code

    # Update headers of image
    if update_headers_image() != success_code:
        return error_code

    # Create binary table with frequency and times columns
    if create_binary_table() != success_code:
        return error_code

    # Create the fits file
    hdul.writeto(generate_dynamic_name(), overwrite=True)

    logger.info("generationFits | generateFits() | Execution Success")
    return success_code


if __name__ == "__main__":
    triggering_times = 3600
    n_channels = 200

    logger.basicConfig(filename='fits.log', filemode='w', level=logger.INFO)

    if generate_fits(n_channels, triggering_times) != success_code:
        logger.info("generationFits | " + error_code)

    logger.info("generationFits | Execution Success")

    # print fits data to debug
    print_fits_info()
