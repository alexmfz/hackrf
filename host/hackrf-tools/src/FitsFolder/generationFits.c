/**
 * @file generationFits.c
 * @author Alejandro
 * @brief 
 * @version 0.1
 * @date 2022-11-05
 * @copyright Copyright (c) 2022
 * 
 */
#include "fitsio.h"
#include "time.h"
#include <string.h>
#include "generatationFits.h"
#include <sys/types.h>
#include <inttypes.h>

// -I/usr/local/src/cfitsio-4.1.0 -lcfitsio
#define TRIGGERING_TIMES (3600) //36005
#define FD_BUFFER_SIZE  (8*1024)
/*** Global Variables***/
fitsfile *fptr =NULL;
float *frequencyDataRanges; // Frecuency Datas of the sweeping in ranges
float *frequencyDatas; // Frecuency Datas of the sweeping in steps
float *timeSteps; // Time data in steps
char timeDatas[TRIGGERING_TIMES][60] = {""}; // Time Datas of the sweeping | 3600 dates
int* samples; // Array of float samples where dbs measures will be saved

int exist = 0;
int status = 0, ii, jj;
int fpixel = 1;
int naxis = 2 , exposure;
extern int nElements;
long naxes[2];// = {3600,200, 720000}; //200 filas(eje y) 400 columnas(eje x)
uint8_t array_img_aux[200][TRIGGERING_TIMES]; //naxes[0]naxes[y] (axis x ,axis y)
uint8_t array_img[200][TRIGGERING_TIMES]; //naxes[0]naxes[y] (axis x ,axis y)

extern struct timeval timeValStartSweeping;
extern struct tm timeFirstSweeping;
extern float step_value;
extern FILE* hackrfLogsFile;
extern struct tm tm_timeScheduled;

/********************/

/**
 * @brief  Get the min value of the array of powers
 * @note   
 * @param  samples: power sample 
 * @retval Min value of the power samples
 */
int minData(int * samples)
{
    int  i = 0;
    int minimum = samples[i];

    for (i = 0; i< nElements; i++)
    {
        if (minimum > samples[i])
        {
            minimum = samples[i];
        }
    }

    return minimum;
}

/**
 * @brief  Get the max value of the array of powers
 * @note   
 * @param  samples: power sample
 * @retval Max value of power samples
 */
int maxData(int * samples)
{
    int  i = 0;
    int maximum = samples[i];
    for (i = 0; i< nElements; i++)
    {
        if (maximum < samples[i])
        {
            maximum = samples[i];
        }
    }

    return maximum;
}

/**
 * @brief  Converts a date into secs of day
 * @note   
 * @param  timeFirstSweeping: Time when first sweeeping was done
 * @retval sec of day when first sweeping was done
 */
long getSecondsOfDayOfFirstSweeping(struct tm timeFirstSweeping)
{
    long sec_of_day = timeFirstSweeping.tm_hour*3600 + timeFirstSweeping.tm_min*60 + timeFirstSweeping.tm_sec;
    return sec_of_day;
}

/**
 * @brief  Creates fits file
 * @note   
 * @param  fileFitsName[]: Name of the fits file
 * @retval Result of the function was succesfull or not (EXIT_SUCCESS | EXIT_FAILURE) 
 */
int createFile(char fileFitsName[])
{   
    /*Checks if the new file exist*/
    fits_file_exists(fileFitsName, &exist, &status);
    if(exist == 1)
    {
        fprintf(hackrfLogsFile, "generationFits | createFile() | File already exists. Overwritting fits file\n");
        if(fits_open_file(&fptr, fileFitsName, READWRITE, &status))
        {
            fprintf(hackrfLogsFile, "generationFits | createFile() | Was not possible to open the file\n");
            return EXIT_FAILURE;
        }
        
        if(fits_delete_file(fptr,&status))
        {
            fprintf(hackrfLogsFile, "generationFits | createFile() | Was not possible to delete the file\n");
            return EXIT_FAILURE;
        }
        if(fits_create_file(&fptr, fileFitsName, &status))
        {
            fprintf(hackrfLogsFile, "generationFits | createFile() | Was not possible to create the file\n");
            return EXIT_FAILURE;
        }
        
        fprintf(hackrfLogsFile, "generationFits | createFile() | Execution Sucess. File overwrriten: %s\n", fileFitsName);
        return EXIT_SUCCESS;
    }
    else //if not exist
    {
        if(fits_create_file(&fptr, fileFitsName, &status))
        {
            fprintf(hackrfLogsFile, "generationFits | createFile() | Was not possible to create the file\n");
            return EXIT_FAILURE;
        }

        fprintf(hackrfLogsFile, "generationFits | createFile() | Execution Sucess\n");
        return EXIT_SUCCESS;
    }
}

/**
 * @brief  Create the fits image
 * @note   
 * @retval Result of the function was succesfull or not (EXIT_SUCCESS | EXIT_FAILURE) 
 */
int createImage()
{
    /*create the primary array image*/
    if(fits_create_img(fptr, BYTE_IMG, naxis, naxes, &status))
    {
        fprintf(hackrfLogsFile, "generationFits | createImage() | Was not possible to create the image\n");
        return EXIT_FAILURE;
    }

    fprintf(hackrfLogsFile, "generationFits | createImage() | Execution Success\n");
    return EXIT_SUCCESS;
}

/**
 * @brief  Update header values of the fits file
 * @note   
 * @retval None
 */
void updateHeadersFitsFileImage(struct tm localTimeFirst, struct tm localTimeLast, uint32_t freq_min)
{
    char user_buf[70]; //msconds
    char date[70]; // Time of observation
   	strftime(date, sizeof date,"%Y-%m-%d", &localTimeFirst);

	char date_obs[70]; //Date observation starts
    strftime(date_obs, sizeof date_obs, "%Y/%m/%d", &localTimeFirst);

    char time_obs[70]; // Time observation starts
    strftime(time_obs, sizeof time_obs, "%H:%M:%S", &localTimeFirst); 
    strcat(time_obs, ".");
    sprintf(user_buf, "%03d", (int)timeValStartSweeping.tv_usec);
    strncat(time_obs, user_buf, 3);

	char date_end[70]; // Date observation ends
    strftime(date_end, sizeof date_end, "%Y/%m/%d", &localTimeLast); 

	char time_end[70]; // time observation ends
	strftime(time_end, sizeof time_end, "%H:%M:%S", &localTimeLast); 

    char content[70];
    strftime(content, sizeof content, "%Y/%m/%d", &localTimeFirst);
    strcat(content, " Radio Flux density, HackRF-One (Spain)");

    fprintf(hackrfLogsFile, "generationFits | updateHeadersFitsFileImage() | Updating fits headers to format\n");

    double bzero = 0., bscale = 1.;
    int dataMax = maxData(samples), dataMin= minData(samples);
    double exposure = 1500. ;
    double crVal1 = getSecondsOfDayOfFirstSweeping(localTimeFirst);
    long crPix1 = 0;
    double stepX = 0.25; // crVal1 Start axis x value 0 [sec of a day]
    
    double crVal2 = 45.;
    long crPix2 = 0;
    double stepY = -1.; // crVal2 Start axis y value 0
    
    double obs_lat = 40.5131623, obs_lon = -3.3527243, obs_alt = 594.; // Coordinates
    long pwm_val = -10;

    fits_update_key(fptr, TSTRING,"DATE", date, "Time of observation" ,&status); // Date observation

    fits_update_key(fptr, TSTRING, "CONTENT", content, "Title", &status);

    fits_update_key(fptr, TSTRING, "ORIGIN", "University of Alcala de Henares", "Organization name", &status); // Organization name

    fits_update_key(fptr, TSTRING, "TELESCOP","SDR", "Type of intrument", &status); // Instrument type
    fits_update_key(fptr, TSTRING, "INSTRUME","HACKRF One", "Name of the instrument", &status); // Instrument name

    fits_update_key(fptr, TSTRING, "OBJECT", "Space", "Object name", &status); // Object Description

    fits_update_key(fptr, TSTRING, "DATE-OBS", date_obs, "Date observation starts", &status); // Date observation starts
    fits_update_key(fptr, TSTRING, "TIME-OBS", time_obs, "Time observation starts", &status); // Time observation starts
    fits_update_key(fptr, TSTRING, "DATE-END", date_end, "Time observation starts", &status); // Date observation ends
    fits_update_key(fptr, TSTRING, "TIME-END", time_end, "Time observation starts", &status); // Time observation ends

    fits_update_key(fptr, TDOUBLE, "BZERO", &bzero, "Scaling offset", &status); // Scaling offset
    fits_update_key(fptr, TDOUBLE, "BSCALE", &bscale, "Scaling factor", &status);  // Scaling factor
    
    fits_update_key(fptr, TSTRING, "BUNIT", "digits", "Z - axis title", &status); // z- axis title

    fits_update_key(fptr, TINT, "DATAMAX", &dataMax, "Max pixel data", &status);
    fits_update_key(fptr, TINT, "DATAMIN", &dataMin, "Min pixel data", &status);

    fits_update_key(fptr, TDOUBLE, "CRVAL1", &crVal1, "Value on axis 1 at the reference pixel [sec of a day]", &status);  // Value on axis 1 at reference pixel [sec of a day]
    fits_update_key(fptr, TLONG, "CRPIX1", &crPix1, "Reference pixel of axis 1", &status); // Reference pixel of axis 1
    fits_update_key(fptr, TSTRING, "CTYPE1", "Time [UT]", "Title of axis 1", &status ); // Title of axis 1
    fits_update_key(fptr, TDOUBLE, "CDELT1", &stepX, "Step between first and second element in x-axis", &status); // Step between first and second element in x-axis (0.25 s)

    fits_update_key(fptr, TDOUBLE, "CRVAL2", &crVal2, "Value on axis 2 at the reference pixel", &status);  // Value on axis 2 at the reference pixel
    fits_update_key(fptr, TLONG, "CRPIX2", &crPix2, "reference Pixel 2 ", &status); 
    fits_update_key(fptr, TSTRING, "CTYPE2", "Frequency [MHz]", "Title of axis 2", &status ); // Title of axis 2
    fits_update_key(fptr, TDOUBLE, "CDELT2", &stepY, "Steps samples", &status); // Steps axis Y

    fits_update_key(fptr, TDOUBLE, "OBS_LAT", &obs_lat, "Observatory latitude in degree", &status);
    fits_update_key(fptr, TSTRING, "OBS_LAC", "N", "Observatory latitude code {N, S}", &status);
    fits_update_key(fptr, TDOUBLE, "OBS_LON", &obs_lon, "Observatory longitude in degree", &status);
    fits_update_key(fptr, TSTRING, "OBS_LOC", "W", "Observatory longitude code {E, W}", &status);
    fits_update_key(fptr, TDOUBLE, "OBS_ALT", &obs_alt, "Observatory altitude in meter", &status);

    fits_update_key(fptr, TLONG, "PWM_VAL", &pwm_val, "PWM value to control tuner gain", &status);

    fprintf(hackrfLogsFile, "generationFits | updateHeadersFitsFile() | Execution Sucess\n");
}

/**
 * @brief  Insert data into fits file
 * @note  
 * @param  samples: Data to be inserted into fits file from callback function
 * @retval Result of the function was succesfull or not (EXIT_SUCCESS | EXIT_FAILURE) 
 */
int insertDataImage(int* samplesOrdered)
{
    /*Initialize the values in the image with a linear ramp function*/
    fprintf(hackrfLogsFile, "generationFits | insertDta() | Inserting data...\n");
    int nChannels = naxes[1];
    int id = 0; // Id sample iterator
    int offset = minData(samplesOrdered);
    int z = 0, i = 0, j = 0; // Aux id sample iterators
    // Saving data into array which is in wrong order, so we have to reorder it
    for(ii = 0; ii < naxes[0]; ii++ ) 
    {        
        for (jj = 0; jj < naxes[1]; jj++) 
        {  
            array_img_aux[jj][ii] = (uint8_t)samplesOrdered[nElements - id - 1];
            // TODO: REVIEW
            //array_img[jj][ii] = (uint8_t)(samplesOrdered[nElements-id-1]) + offset;     
            id++;
        } 
    }
  
    id = 0; // Reset iterator again

    for (i = 0; i < nElements; i++)
    {
        samples[i] = 0;
        if (i% nChannels == 0 && i != 0)
        {
            j++;
            z = 0;
        }

        samples[i] = array_img_aux[z][naxes[0] - j -1];
        z++;
    }

    for(ii = 0; ii < naxes[0]; ii++ ) 
    {        
        for (jj = 0; jj < naxes[1]; jj++) 
        {  
            array_img[jj][ii] = (uint8_t)samples[id];
            // TODO: REVIEW
            //array_img[jj][ii] = (uint8_t)(samplesOrdered[nElements-id-1]) + offset;     
            id++;
        } 
    }
    

    fprintf(hackrfLogsFile, "generationFits | insertDataImage() | Inserting data finished. Creating fits image...\n");

    /*Write the array of integers of the image*/
    if(fits_write_img(fptr, TBYTE, fpixel, nElements, array_img[0], &status))
    {
        fprintf(hackrfLogsFile, "generationFits | insertDataImage() | Was not possible to write data into the image");
        fits_report_error(hackrfLogsFile, status);
        return EXIT_FAILURE;
    }

    fprintf(hackrfLogsFile, "generationFits | insertDataImage() | Execution Sucess\n");
    return EXIT_SUCCESS;
}

/**
 * @brief  Creates a bin table
 * @note   
 * @retval Result of the function was succesfull or not (EXIT_SUCCESS | EXIT_FAILURE) 
 */
int createBinTable()
{
    long nRows = 1, nCols = 2;
   
    char *ttype[] = {"TIME", "FREQUENCY"}; // Name of columns

    char *tform[] = {"3600D8.3", "200D8.3"}; // Data type of each column

    char *tunit[] = {"SECONDS", "MHz"}; // Physical unit of each column

    char *extname = {"BINARY TABLE"}; // Name of the table

    fprintf(hackrfLogsFile, "generationFits | createBinTable() | Creating Binary table\n");

    /*create the binary table*/
    if(fits_create_tbl(fptr, BINARY_TBL, nRows, nCols, ttype, tform, tunit, NULL, &status))
    {
        fprintf(hackrfLogsFile, "generationFits | createBinTable() | Was not possible to create the binary table\n");
        return EXIT_FAILURE;
    }

    fprintf(hackrfLogsFile, "generationFits | createBinTable() | Execution Sucess\n");
    return EXIT_SUCCESS;
}

/**
 * @brief  Update headers of bin table
 * @note   
 * @retval None
 */
void updateHeadersFitsFileBinTable()
{
    double tzero = 0., tscale = 1.;

    fits_update_key(fptr, TDOUBLE, "TSCALE1", &tscale, "Scaling factor", &status);  // Scaling factor
    fits_update_key(fptr, TDOUBLE, "TZERO1", &tzero, "Scaling offset", &status); // Scaling offset

    fits_update_key(fptr, TDOUBLE, "TSCALE2", &tscale, "Scaling factor", &status);  // Scaling factor
    fits_update_key(fptr, TDOUBLE, "TZERO2", &tzero, "Scaling offset", &status); // Scaling offset

    fprintf(hackrfLogsFile, "generationFits | updateHeadersFitsFileBinTable() | Execution Sucess\n");
}

/**
 * @brief  Insert data into bin table (axis X and axis Y)
 * @note   
 * @param  times: Times saved
 * @param  frequencies: Frequencies saved
 * @retval Result of the function was succesfull or not (EXIT_SUCCESS | EXIT_FAILURE) 
 */
int insertDataBinTable(float* times, float* frequencies)
{
    fprintf(hackrfLogsFile, "generationFits | insertDataBinTable() | Inserting data into the binary table\n");
    
    if (times == NULL || frequencies == NULL)
    {
        fprintf(hackrfLogsFile, "generationFits | insertDataBinTable() | Inserting data into the binary table failed\n");
        return EXIT_FAILURE;
    }   


    if (fits_write_col(fptr, TFLOAT, 1, 1, 1, naxes[0], times, &status)) 
    {
        fprintf(hackrfLogsFile, "generationFits | insertDataBinTable() | Inserting time datas into col failed\n");
        return EXIT_FAILURE;
    }

    if (fits_write_col(fptr, TFLOAT, 2, 1, 1, naxes[1], frequencies, &status)) 
    {
        fprintf(hackrfLogsFile, "generationFits | insertDataBinTable() | Inserting frequency datas into col failed\n");
        return EXIT_FAILURE;
    }

    fprintf(hackrfLogsFile, "generationFits | insertDataBinTable() | Execution Success\n");
    return EXIT_SUCCESS;
}

/**
 * @brief  Closes fits file
 * @note   
 * @retval None
 */
void closeFits()
{
    /*Close and report any error*/
    fits_close_file(fptr, &status);
    fits_report_error(stderr, status);
    
    fprintf(hackrfLogsFile, "generationFits | closeFits() | Execution Sucess\n");
}

/**
 * @brief  Save the frequencies from the sweeping to insert into fits file has headers data in ranges of stepValues
 * @note   
 * @param  freq_min: Min freq of the sweeping
 * @param  freq_max: Max freq of the sweeping
 * @param  step_value_between_ranges: value between frequencies
 * @retval Result of the function was succesfull or not (EXIT_SUCCESS | EXIT_FAILURE) 
 */
int saveFrequencies(uint32_t freq_min, uint32_t freq_max, int n_ranges, float step_value_between_ranges)
{   
    int i = 0;
    float actualFrequency = (float)freq_min;
    //float actualFrequencyAux = (float)freq_min;
    float actualFrequencyAux = (float)freq_max - step_value;
    fprintf(hackrfLogsFile, "generationFits | saveFrequencies() | Start saving index data frequencies in ranges to generate fits file\n");
    
    frequencyDataRanges = (float*)calloc(n_ranges,sizeof(float));
    frequencyDatas = (float*)calloc(naxes[1], sizeof(float));

    for(i = 0; i< n_ranges; i++)
    {
        frequencyDataRanges[i] = actualFrequency;
        actualFrequency += step_value_between_ranges;
    }
    
    if (frequencyDataRanges == NULL && frequencyDataRanges[n_ranges] != freq_max) 
    {
        fprintf(hackrfLogsFile, "generationFits | saveFrequencies() | Was not possible to save frequencies by ranges.\n");
        return EXIT_FAILURE; 
    }
    
    fprintf(hackrfLogsFile, "generationFits | saveFrequencies() | Start saving index data frequencies by steps to generate fits file\n");
    
    for (i = 0; i< naxes[1]; i++)
    {
        frequencyDatas[i] = actualFrequencyAux;
        actualFrequencyAux -= step_value;
    }

     if (frequencyDatas == NULL && frequencyDatas[naxes[1]] != freq_max) 
    {
        fprintf(hackrfLogsFile, "generationFits | saveFrequencies() | Was not possible to save frequencies by steps.\n");
        return EXIT_FAILURE; 
    }

    fprintf(hackrfLogsFile, "generationFits | saveFrequencies() | Execution Sucess.\n");
    return EXIT_SUCCESS; 
    
}

/**
 * @brief  Save the times from the sweeping to insert into fits file has headers data
 * @note   TODO: Wont be used by the moment
 * @param  i: Actual iteration  
 * @param  triggeringTimes: Number of correct iterations
 * @param  sweepingTime: parameter of time where sweeping it happens
 * @retval Result of the function was succesfull or not (EXIT_SUCCESS | EXIT_FAILURE) 
 */
int saveTimes(int i, int triggeringTimes, char* sweepingTime)
{
    if (i == 0)
    {
        fprintf(hackrfLogsFile, "generationFits | saveTimes() | Start saving index data to generate fits file\n");
    }
    
    // Saving step
    strcpy(timeDatas[i], sweepingTime);

    // Checks
    if (strcmp(timeDatas[i], "") == 0)
    {
        fprintf(hackrfLogsFile, "generationFits | saveTimes() | Was not possible to save timing data\n");
        return EXIT_FAILURE;
    }

    if (i == triggeringTimes-1)
    {
        fprintf(hackrfLogsFile, "generationFits | saveTimes() | Execution Sucess\n");
    }

    return EXIT_SUCCESS;
}

/**
 * @brief  Save times as float
 * @note   
 * @retval Result of the function was succesfull or not (EXIT_SUCCESS | EXIT_FAILURE) 
 */
int saveTimeSteps()
{
    int i = 0;
    float previousTime = 0;

    fprintf(hackrfLogsFile, "generationFits | saveTimeSteps() | Start Saving time by steps\n");
    timeSteps = (float*)calloc(TRIGGERING_TIMES, sizeof(float));
    
    if(timeSteps == NULL)
    {
        fprintf(hackrfLogsFile, "generationFits | saveTimeSteps() | Was not possible to allocate mmeory.\n");
        return EXIT_FAILURE;
    }
    
    for (i = 0; i < TRIGGERING_TIMES; i++)
    {
        timeSteps[i] = previousTime;
        previousTime += 0.25;
    }

    fprintf(hackrfLogsFile, "generationFits | saveTimeSteps() | Execution Success\n");
    return EXIT_SUCCESS;
}

/**
 * @brief  Save the samples from the sweeping to insert into fits file has data
 * @note   TODO: Wont be used by the moment
 * @param  i: Actual iteration 
 * @param  powerSample: parameter of power sample from sweeping
 * @retval Result of the function was succesfull or not (EXIT_SUCCESS | EXIT_FAILURE) 
 */
int saveSamples(int i, int powerSample, int nElements)
{
    if (i == 0)
    {
        fprintf(hackrfLogsFile, "generationFits | saveSamples() | Start saving power sample data to generate fits file\n");
        samples = (int*)calloc(nElements,sizeof(int));
        samples[i] = powerSample;
    }

    else
    {
        samples[i] = powerSample;
    }
    
    if (samples == NULL)
    {
        fprintf(hackrfLogsFile, "generationFits | saveSamples() | Was not possible to save power samples\n");
        return EXIT_FAILURE;
    }

    if (i == nElements-1)
    {
        fprintf(hackrfLogsFile, "generationFits | saveSamples() | Execution Sucess\n");
    }

    return EXIT_SUCCESS;
}

/**
 * @brief  Checks if saved Times and power samples are correct
 * @note
 * @param nElements Number of power samples   
 * @retval Result of the function was succesfull or not (EXIT_SUCCESS | EXIT_FAILURE) 
 */
int checkSavedData(int nElements)
{
    int i = 0;

    fprintf(hackrfLogsFile, "generationFits | checkSavedData() | Start checking saved power sample data to generate fits file\n");
    // Check power sample data
    for (i = 0; i < nElements; i++)
    {
        if (samples[i] == 0)
        {
            fprintf(hackrfLogsFile, "Data not saved correctly\n");
            return EXIT_FAILURE;
        }
    }

    fprintf(hackrfLogsFile, "generationFits | checkSavedData() | Execution Sucess\n");
    return EXIT_SUCCESS;
}

/**
 * @brief Invokes all the functionality to have a correct fits file 
 * @note   
 * @param  fileFitsName[]: name of the fits file
 * @param  samples: Input data of the fits file
 * @param  freq_min: Min frequency
 * @param  freq_max: Max frequency
 * @param  step_value: step value between frequencies
 * @retval Result of the function was succesfull or not (EXIT_SUCCESS | EXIT_FAILURE) 
 */
int generateFitsFile(char fileFitsName[], int*samplesOrdered, struct tm localTimeFirst, struct tm localTimeLast, uint32_t freq_min)
{   
    nElements = naxes[0]*naxes[1];

    fprintf(hackrfLogsFile, "generationFits | generateFitsFile() | Start generating fits file\n");
    
    // Creation of file
    if (createFile(fileFitsName) == EXIT_FAILURE) { return EXIT_FAILURE; }

    // Creation of image
    if (createImage() == EXIT_FAILURE) { return EXIT_FAILURE; }

    // Update headers of image
    updateHeadersFitsFileImage(localTimeFirst, localTimeLast, freq_min);
   
    // Insert data of image
    if(insertDataImage(samplesOrdered) == EXIT_FAILURE)  { return EXIT_FAILURE;  }
    
    // Creation of binary table
    if(createBinTable() == EXIT_FAILURE) { return EXIT_FAILURE; }

    // Update headers of binary table
    updateHeadersFitsFileBinTable();

    // Insert data of binary table
    if(insertDataBinTable(timeSteps, frequencyDatas) == EXIT_FAILURE) { return EXIT_FAILURE; }

    //CloseFile
    closeFits();

    fprintf(hackrfLogsFile, "generationFits | generateFitsFile() | Execution Sucess\n");
    return EXIT_SUCCESS;
}

/**
 * @brief  Used to write hackRF into txt files to generate fits file throught python
 * @note   
 * @retval Result of the function was succesfull or not (EXIT_SUCCESS | EXIT_FAILURE) 
 */
int writeHackrfDataIntoTxtFiles(struct tm localTimeFirst, struct tm localTimeLast, int* samplesOrdered, float* frequencies, float* times)
{
    int i = 0;
    int nElements = naxes[1] * TRIGGERING_TIMES;
    FILE* samplesFile = NULL;
    FILE* frequenciesFile = NULL;
    FILE* timingFile = NULL;
    FILE* headerFile = NULL;
    
    char samplePath[] = "samples.txt";
    char frequencyPath[] = "frequencies.txt";
    char timingPath[] = "times.txt";
    char headerPath[] = "header_times.txt";

    char dateStart[70]; // Time of observation
   	strftime(dateStart, sizeof dateStart,"%Y/%m/%d", &localTimeFirst);

    char time_obs[70]; // Time observation starts
    char user_buf[70]; //msconds

    strftime(time_obs, sizeof time_obs, "%H:%M:%S", &localTimeFirst); 
    strcat(time_obs, ".");
    sprintf(user_buf, "%03d", (int)timeValStartSweeping.tv_usec);
    strncat(time_obs, user_buf, 3);

    char date_end[70]; // Date observation ends
    strftime(date_end, sizeof date_end, "%Y/%m/%d", &localTimeLast); 

	char time_end[70]; // time observation ends
	strftime(time_end, sizeof time_end, "%H:%M:%S", &localTimeLast); 


    fprintf(hackrfLogsFile, "generationFits | writeHackrfDataIntoTxtFiles() | Data will be stored into files %s , %s, %s, %s\n", samplePath, frequencyPath, timingPath, headerPath);

    if (samplePath == NULL || strstr(samplePath, ".txt")== NULL || 
        frequencyPath == NULL || strstr(frequencyPath, ".txt") == NULL ||
        timingPath == NULL || strstr(timingPath, ".txt") == NULL ||
        headerPath == NULL || strstr(headerPath, ".txt") == NULL)
        {
            samplesFile = stdout;
            frequenciesFile = stdout;
            timingFile = stdout;
            headerFile = stdout;
            
            fprintf(hackrfLogsFile, "generationFits | writeHackrfDataIntoTxtFiles() | Error extension incorrect. Should be .txt\n");
            return EXIT_FAILURE;
        }

    else
    {
        samplesFile = fopen(samplePath, "wb");        
        frequenciesFile = fopen(frequencyPath, "wb");        
        timingFile = fopen(timingPath, "wb");   
        headerFile = fopen(headerPath, "wb");     
    }

    if (samplesFile == NULL || frequenciesFile == NULL || timingFile == NULL || headerFile == NULL)
    {
        fprintf(hackrfLogsFile, "generationFits | writeHackrfDataIntoTxtFiles() | Failed to open files\n");
        return EXIT_FAILURE; 
    }

    // Set buffers to the files
    if(setvbuf(samplesFile, NULL, _IOFBF, FD_BUFFER_SIZE) != EXIT_SUCCESS || 
       setvbuf(frequenciesFile, NULL, _IOFBF, FD_BUFFER_SIZE) != EXIT_SUCCESS ||
       setvbuf(timingFile, NULL, _IOFBF, FD_BUFFER_SIZE) != EXIT_SUCCESS ||
       setvbuf(headerFile, NULL, _IOFBF, FD_BUFFER_SIZE) != EXIT_SUCCESS)
       {
            fprintf(hackrfLogsFile, "generationFits | writeHackrfDataIntoTxtFiles() | setvbuf failed\n");
            return EXIT_FAILURE;
       }

    // Write data samples
    for (i = 0; i < nElements; i++)
    {
        fprintf(samplesFile, "%d", (int)samplesOrdered[i]);
        if (i < nElements -1 )
        {
            fprintf(samplesFile, "\n");
        }
    }

    // Write data frequencies
    for (i = 0; i < naxes[1]; i++)
    {
        fprintf(frequenciesFile, "%f", frequencies[i]);
        if (i < naxes[1] -1 )
        {
            fprintf(frequenciesFile, "\n");
        }
    }

    // Write data times
    for (i = 0; i < TRIGGERING_TIMES; i++)
    {
        fprintf(timingFile, "%f", times[i]);
         if (i < TRIGGERING_TIMES -1 )
        {
            fprintf(timingFile, "\n");
        }
    }

    fprintf(headerFile, "%s\n", dateStart);
    fprintf(headerFile, "%s\n",time_obs);
    fprintf(headerFile, "%s\n", date_end);
    fprintf(headerFile, "%s\n",time_end);
    fprintf(headerFile, "%ld", getSecondsOfDayOfFirstSweeping(localTimeFirst));

    fflush(samplesFile);
    fflush(frequenciesFile);
    fflush(timingFile);
    fflush(headerFile);

    if ((samplesFile != NULL && samplesFile != stdout) &&
        (frequenciesFile != NULL && frequenciesFile != stdout) &&
        (timingFile != NULL && timingFile != stdout) &&
        (headerFile != NULL && headerFile != stdout))
        {
            fclose(samplesFile);
            fclose(frequenciesFile);
            fclose(timingFile);
            fclose(headerFile);
            fprintf(hackrfLogsFile, "generationFits | writeHackrfDataIntoTxtFiles() | Files closed\n");
        }

    fprintf(hackrfLogsFile, "generationFits | writeHackrfDataIntoTxtFiles() | Execution Success\n");
    return EXIT_SUCCESS;
}