import logging as logger

from astropy.io import fits
import numpy as np

from io import open

import os
import sys
import datetime as dt

error_code = "ERROR"
success_code = "OK"
hdul = None
min_value = None
max_value = None
fits_name = None


def create_image():
    """
    Creates a fits image as primary extension with the dimensions of nChannels and triggeringTimes

    @return: Result of the function was succesfull or not (OK | ERROR)
    """

    global hdul
    global max_value
    global min_value

    # input data (samples ordered) of SDR as byte
    logger.info("generationFits | createImage() | Creation of fits image")
    logger.info("generationFits | createImage() | Reading power samples from SDR")

    power_sample = read_power_samples()
    if power_sample == error_code:
        logger.error("generationFits | create_image() | Error at reading power samples")
        return error_code

    # Before inserting data into img save max and min values of power samples for headers
    max_value = max(power_sample)
    min_value = min(power_sample)

    # Reverse array to start from max freq to min freq. Samples must be reversed too
    power_sample.reverse()

    # Insert data into image es primary HDU in fit file
    image_data = np.ones((n_channels, triggering_times), dtype='u1')
    image_data = insert_data_image(image_data, power_sample)

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


def read_header_data():
    """
    Read header extra info (dates and times) from header_times.txt

    @return: Header data as an array.
             If the file cannot be opened, return "ERROR"
    """

    header_data = []

    logger.info("generationFits | read_header_data() | Reading headers extra data")

    header_file = open("header_times.txt", "r")
    if header_file is None:
        logger.error("generationFits | read_header_data() | Error at reading header file")
        return error_code

    header_not_formated = header_file.readlines()
    for header in header_not_formated:
        header_data.append(header.replace("\n", ""))

    header_file.close()

    logger.info("generationFits | read_header_data() | Execution Success")
    return header_data


def update_headers_image():
    """
    Update headers image to adapt to fit standar header. Some headers are extract from header_times.txt

    @return: Result of the function was succesfull or not (OK | ERROR)
    """

    global hdul
    global max_value
    global min_value
    bscale = 1.
    bzero = 0.
    header_data = read_header_data()
    if header_data == "ERROR":
        logger.error("generationFits | update_headers_image() | Error at reading header file")
        return error_code

    logger.info("generationFits 1 UpdateHeadersImage() | Updating headers of fits image")
    len_headers = len(hdul[0].header)

    # Update headers
    hdul[0].header.append(("DATE", header_data[0].replace("/", "-"), "Time of observation"))
    hdul[0].header.append(("CONTENT", sys.argv[9], "Title"))

    hdul[0].header.append(("INSTRUME", "HACKRF One", "Name of the instrument"))
    hdul[0].header.append(("OBJECT", sys.argv[8], "Object name"))

    hdul[0].header.append(("DATE-OBS", header_data[0], "Date observation starts"))
    hdul[0].header.append(("TIME-OBS", header_data[1], "Time observation starts"))
    hdul[0].header.append(("DATE-END", header_data[2], "Date Observation ends"))
    hdul[0].header.append(("TIME-END", header_data[3], "Time observation ends"))

    hdul[0].header.append(("BZERO", bzero, "Scaling offset"))
    hdul[0].header.append(("BSCALE", bscale, "Scaling factor"))

    hdul[0].header.append(("BUNIT", "digits", "Z - axis title"))

    hdul[0].header.append(("DATAMAX", max_value, "Max pixel data"))
    hdul[0].header.append(("DATAMIN", min_value, "Min pixel data"))

    hdul[0].header.append(("CRVAL1", header_data[4], "Value on axis 1 [sec of day]"))
    hdul[0].header.append(("CRPIX1", 0, "Reference pixel of axis 1"))
    hdul[0].header.append(("CTYPE1", "TIME [UT]", "Title of axis 1"))
    hdul[0].header.append(("CDELT1", 0.25, "Step between first and second element in x-axis"))

    hdul[0].header.append(("CRVAL2", min(read_frequencies()), "Value on axis 2 "))
    hdul[0].header.append(("CRPIX2", 0, "Reference pixel of axis 2"))
    hdul[0].header.append(("CTYPE2", "Frequency [MHz]", "Title of axis 2"))
    hdul[0].header.append(("CDELT2", -1, "Step samples"))

    hdul[0].header.append(("OBS_LAT", sys.argv[3], "Observatory latitude in degree"))
    hdul[0].header.append(("OBS_LAC", sys.argv[4], "Observatory latitude code {N, S}"))
    hdul[0].header.append(("OBS_LON", sys.argv[5], "Observatory longitude in degree"))
    hdul[0].header.append(("OBS_LOC", sys.argv[6], " Observatory longitude code {E, W}"))
    hdul[0].header.append(("OBS_ALT", sys.argv[7], "Observatory altitude in meter"))

    if len_headers == len(hdul[0].header):
        return error_code

    logger.info("generationFits | UpdateHeadersImage() | Execution Success")
    return success_code


def create_binary_table():
    """
    Create a binary table formed by 2 columns (frequencies and times).
    This data is read from frequencies.txt and times.txt

    @return: Result of the function was successful or not (OK | ERROR)
    """

    global hdul

    frequencies = np.arange(n_channels)
    frequencies = read_frequencies()
    if frequencies == "ERROR":
        logger.error("generationFits | create_binary_table() | Error at reading frequency file")
        return error_code

    times = np.arange(triggering_times)
    times = read_times()
    if times == "ERROR":
        logger.error("generationFits | create_binary_table() | Error at reading times file")
        return error_code

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
    """
    Generate the name of the fit file in a dynamic way
    @return: name of the fits
    """

    global hdul
    global fits_name

    extension = ".fit"
    date_obs = hdul[0].header["DATE-OBS"].replace("/", "") + "_"
    start_date = hdul[0].header["TIME-OBS"]
    logger.info("Start date" + start_date)
    format_date = start_date[:3].replace(":", "") + start_date[3:6].replace(":", "") + start_date[6:8]

    fits_name = sys.argv[1] + "_" + date_obs + format_date + "_" + sys.argv[2] + extension

    logger.info("generationFits | generate_dynamic_name() | File generated with name: " + fits_name)
    return fits_name


def read_power_samples():
    """
    Read power samples from samples.txt which is the output of executing hackrf_sweep

    return: If OK: Return the power samples read from samples.txt as an array and reformated.
            If there is an error: Return "ERROR
    """

    logger.info("generationFits | read_power_samples() | Reading samples as output of SDR")

    sample_formated = []

    sample_file = open("samples.txt", "r")
    if sample_file is None:
        return error_code

    samples_not_formated = sample_file.readlines()
    for sample in samples_not_formated:
        sample_formated.append(int(sample.replace("\n", "")))

    sample_file.close()

    logger.info("generationFits | read_power_samples() | Execution Success")
    return sample_formated


def insert_data_image(image_data, power_sample):
    """
    Insert data to the image taking it from the previous ones read

    @param image_data: Initial data of img (which is 2 full arrays of ones
    @param power_sample: Power samples read
    @return: image data created
    """

    i = 0  # iterator of samples

    j = 0  # Aux iterator
    z = 0  # Aux iterator
    y = 0  # Aux iterator
    image_data_2 = np.ones((n_channels, triggering_times), dtype='u1')

    logger.info("generationFits | insert_data_image() | Inserting data into image")

    for times in range(triggering_times):
        for frequencies in range(n_channels):
            image_data_2[frequencies][times] = np.array(power_sample[i]).astype("u1")
            i += 1

    # Values should be entered in ordered so there is a need of regroup
    for element in range(n_channels * triggering_times):
        if element % n_channels == 0 and element != 0:
            j += 1
            z = 0
        power_sample[element] = image_data_2[z][triggering_times - j - 1]
        z += 1

    i = 0  # restart iterator
    for times in range(triggering_times):
        for frequencies in range(n_channels):
            image_data[frequencies][times] = np.array(power_sample[i]).astype("u1")
            i += 1

    logger.info("generationFits | insert_data_image() | Execution Success")
    return image_data


def read_frequencies():
    """
    Read frequencies from frequencies.txt which is the output of executing hackrf_sweep

    return: If OK: Return the frequencies read from frequencies.txt as an array and reformated.
            If there is an error: Return "ERROR
    """

    logger.info("generationFits | read_frequencies() | Reading frequencies as output of SDR")
    frequency_formated = []

    freq_file = open("frequencies.txt", "r")
    if freq_file is None:
        return error_code

    frequency_not_formated = freq_file.readlines()
    for freq in frequency_not_formated:
        frequency_formated.append(float(freq.replace("\n", "")))

    freq_file.close()

    logger.info("generationFits | read_frequencies() | Execution Success")
    return frequency_formated


def read_times():
    """
    Read times from times.txt which is the output of executing hackrf_sweep

    return: If OK: Return the times read from times.txt as an array and reformated.
            If there is an error: Return "ERROR
    """

    logger.info("generationFits | read_times() | Reading frequencies as output of SDR")
    times_formated = []

    time_file = open("times.txt", "r")
    if time_file is None:
        return error_code

    times_not_formated = time_file.readlines()
    for times in times_not_formated:
        times_formated.append(float(times.replace("\n", "")))

    time_file.close()
    logger.info("generationFits | read_times() | Execution Success")
    return times_formated


def print_fits_info():
    """
    Test function to check data

    @return: None
    """

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
    """
    Main functions which creates the fits file formed by an image and a binary table wich dimensions 3600x200

    @param n_channels: Nº of frequencies
    @param triggering_times: Nº of shots
    @return: Result of the function was successful or not (OK | ERROR)
    """

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
    logger.info(dt.datetime.now())
    logger.shutdown()

    # Rename fits.log with the name of the data
    old_name = r"fits.log"
    new_name = fits_name.replace(".fit", "_python_logs.txt")

    # Renaming the file
    os.rename(old_name, new_name)
    
    """
    # print fits data to debug
    print_fits_info()
    """