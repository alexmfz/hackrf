/**
 * @file generationFits.c
 * @author Alejandro
 * @brief 
 * @version 0.1
 * @date 2022-11-05
 * 
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
#define TRIGGERING_TIMES (3600) //3600

/*** Global Variables***/
fitsfile *fptr =NULL;
float *frequencyDatas; // Frecuency Datas of the sweeping
char timeDatas[TRIGGERING_TIMES][60] = {""}; // Time Datas of the sweeping | 3600 dates
float* samples; // Array of float samples where dbs measures will be saved

int exist = 0;
int status = 0, ii, jj;
int fpixel = 1;
int naxis = 2, nElements, exposure;
long naxes[2];// = {3600,200, 720000}; //200 filas(eje y) 400 columnas(eje x)
float array_img[200][TRIGGERING_TIMES]; //naxes[0]naxes[y] (axis x ,axis y)

extern struct tm timeFirstSweeping;
/********************/

/**
 * @brief  Creates fits file
 * @note   
 * @param  fileFitsName[]: Name of the fits file
 * @retval Result of the function was succesfull or not (EXIT_SUCCESS | EXIT_FAILURE) 
 */
int create(char fileFitsName[])
{   
    /*Checks if the new file exist*/
    fits_file_exists(fileFitsName, &exist, &status);
    if(exist == 1)
    {
        printf("generationFits | create() | File already exists. Overwritting fits file\n");
        if(fits_open_file(&fptr, fileFitsName, READWRITE, &status))
        {
            fprintf(stderr, "generationFits | create() | Was not possible to open the file\n");
            return EXIT_FAILURE;
        }
        
        if(fits_delete_file(fptr,&status))
        {
            fprintf(stderr,"generationFits | create() | Was not possible to delete the file\n");
            return EXIT_FAILURE;
        }
        if(fits_create_file(&fptr, fileFitsName, &status))
        {
            fprintf(stderr, "generationFits | create() | Was not possible to create the file\n");
            return EXIT_FAILURE;
        }
        
       /*create the primary array image (16-bit short integer pixels*/
        if(fits_create_img(fptr, FLOAT_IMG, naxis, naxes, &status))
        {
            fprintf(stderr, "generationFits | create() | Was not possible to create the image\n");
            return EXIT_FAILURE;
        }

        printf("generationFits | create() | Execution Sucess. File overwrriten: %s\n", fileFitsName);
        return EXIT_SUCCESS;
    }
    else //if not exist
    {
        if(fits_create_file(&fptr, fileFitsName, &status))
        {
            fprintf(stderr, "generationFits | create() | Was not possible to create the file\n");
            return EXIT_FAILURE;
        }

        /*create the primary array image (16-bit short integer pixels*/
        if(fits_create_img(fptr, FLOAT_IMG, naxis, naxes, &status))
        {
            fprintf(stderr, "generationFits | create() | Was not possible to create the image\n");
            return EXIT_FAILURE;
        }

        printf("generationFits | create() | Execution Sucess. File created: %s with dimensions [%ld]x[%ld]\n", fileFitsName, naxes[0], naxes[1]);
        return EXIT_SUCCESS;
    }
}

/**
 * @brief  Get the min value of the array of powers
 * @note   
 * @param  samples: power sample 
 * @retval Min value of the power samples
 */
long minData(float * samples)
{
    int  i = 0;
    long minimum = (long)samples[i];

    for (i = 0; i< nElements; i++)
    {
        if (minimum > (long)samples[i])
        {
            minimum = (long)samples[i];
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
long maxData(float * samples)
{
    int  i = 0;
    long maximum = (long)samples[i];
    for (i = 0; i< nElements; i++)
    {
        if (maximum < (long)samples[i])
        {
            maximum = (long)samples[i];
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
    printf("Sec of day: %ld\n", sec_of_day);
    return sec_of_day;
}

/**
 * @brief  Update header values of the fits file
 * @note   
 * @retval None
 */
void updateHeadersFitsFile(char* startDate, char *timeStart, char *endDate, char *timeEnd, uint32_t freq_min, struct tm timeFirstSweeping)
{
    printf("generationFits | updateHeadersFitsFile | Updating fits headers to format\n");
    
    long bzero = 0., bscale = 1.;
    long dataMax = maxData(samples), dataMin= minData(samples);
    long crVal1 = getSecondsOfDayOfFirstSweeping(timeFirstSweeping), crPix1 = 0; //0.25   // Time axis value must be sec of a day (where it starts the sweeping)
    float stepX = 0.25;
    long crVal2 = (long)freq_min, crPix2 = 0, stepY = 1; // Frequencies axis value must start at freq_min 

    fits_update_key(fptr, TSTRING,"DATE", startDate, "Time of observation" ,&status); // Date observation

    fits_update_key(fptr, TSTRING, "CONTENT", "Sweeping hackRF Operation", "Title", &status);

    fits_update_key(fptr, TSTRING, "ORIGIN", "University of Alcala de Henares", "Organization name", &status); // Organization name

    fits_update_key(fptr, TSTRING, "TELESCOP","SDR", "Type of intrument", &status); // Instrument type
    fits_update_key(fptr, TSTRING, "INSTRUME","HACKRF One", "Name of the instrument", &status); // Instrument name

    fits_update_key(fptr, TSTRING, "OBJECT", "Space", "Object name", &status); // Object Description

    fits_update_key(fptr, TSTRING, "DATE-OBS", startDate, "Date observation starts", &status); // Date observation starts
    fits_update_key(fptr, TSTRING, "TIME-OBS", timeStart, "Time observation starts", &status); // Time observation starts
    fits_update_key(fptr, TSTRING, "DATE-END", endDate, "Date observation ends", &status); // Date observation ends
    fits_update_key(fptr, TSTRING, "TIME-END", timeEnd, "Time observation starts", &status); // Time observation ends

    fits_update_key(fptr, TLONG, "BZERO", &bzero, "Scaling offset", &status); // Scaling offset
    fits_update_key(fptr, TLONG, "BSCALE", &bscale, "Scaling factor", &status);  // Scaling factor
    
    fits_update_key(fptr, TSTRING, "BUNIT", "digits", "Z - axis title", &status); // z- axis title

    fits_update_key(fptr, TLONG, "DATAMAX", &dataMax, "Max pixel data", &status);
    fits_update_key(fptr, TLONG, "DATAMIN", &dataMin, "Min pixel data", &status);

    fits_update_key(fptr, TLONG, "CRVAL1", &crVal1, "Value on axis 1 at the reference pixel [sec of a day]", &status);  // Value on axis 1 at reference pixel [sec of a day]
    fits_update_key(fptr, TLONG, "CRPIX1", &crPix1, "Reference pixel of axis 1", &status); // Reference pixel of axis 1
    fits_update_key(fptr, TSTRING, "CTYPE1", "Time [UT]", "Title of axis 1", &status ); // Title of axis 1
    fits_update_key(fptr, TFLOAT, "CDELT1", &stepX, "Step between first and second element in x-axis", &status); // Step between first and second element in x-axis (0.25 s)

    fits_update_key(fptr, TLONG, "CRVAL2", &crVal2, "Value on axis 2 at the reference pixel", &status);  // Value on axis 2 at the reference pixel
    fits_update_key(fptr, TLONG, "CRPIX2", &crPix2, "reference Pixel 2 ", &status); 
    fits_update_key(fptr, TSTRING, "CTYPE2", "Frequency [MHz]", "Title of axis 2", &status ); // Title of axis 2
    fits_update_key(fptr, TLONG, "CDELT2", &stepY, "Steps samples", &status); // Steps axis Y

    printf("generationFits | updateHeadersFitsFile | Execution Sucess\n");
}

/**
 * @brief  Insert data into fits file
 * @note  
 * @param  samples: Data to be inserted into fits file from callback function
 * @retval Result of the function was succesfull or not (EXIT_SUCCESS | EXIT_FAILURE) 
 */
int insertData(float* samples)
{
    /*Initialize the values in the image with a linear ramp function*/
    printf("generationFits | insertDta() | Inserting data...\n");
    int id = 0;
    for(ii= 0; ii< naxes[0]; ii++ ) 
    {        
        for (jj=0; jj< naxes[1]; jj++) 
        {
            array_img[jj][ii] = samples[id];
            id++;
        }
        
    }
    
    printf("generationFits | insertData() | Inserting data finished. Creating fits image...\n");

    /*Write the array of integers of the image*/
    if(fits_write_img(fptr, TFLOAT, fpixel, nElements, array_img[0], &status))
    {
        fprintf(stderr, "generationFits | insertData() | Was not possible to write data into the image");
        return EXIT_FAILURE;
    }

    printf("generationFits | insertData() | Execution Sucess\n");
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
    
    printf("generationFits | closeFits() | Execution Sucess\n");
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
    printf("generationFits | saveFrequencies() | Start saving index data frequencies in ranges to generate fits file\n");
    
    frequencyDatas = (float*)calloc(n_ranges,sizeof(float));
    for(i = 0; i< n_ranges; i++)
    {
        frequencyDatas[i] = actualFrequency;
        actualFrequency += step_value_between_ranges;
    }
    
    if (frequencyDatas == NULL && frequencyDatas[n_ranges] != freq_max) 
    {
        fprintf(stderr, "generationFits | saveFrequencies() | Was not possible to save frequencies.\n");
        return EXIT_FAILURE; 
    }
    
    printf("generationFits | saveFrequencies() | Execution Sucess.\n");
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
        printf("generationFits | saveTimes() | Start saving index data to generate fits file\n");
    }
    
    // Saving step
    strcpy(timeDatas[i], sweepingTime);

    // Checks
    if (strcmp(timeDatas[i], "") == 0)
    {
        fprintf(stderr, "generationFits | saveTimes() | Was not possible to save timing data\n");
        return EXIT_FAILURE;
    }

    if (i == triggeringTimes-1)
    {
        printf("generationFits | saveTimes() | Execution Sucess\n");
    }

    return EXIT_SUCCESS;
}

/**
 * @brief  Save the samples from the sweeping to insert into fits file has data
 * @note   TODO: Wont be used by the moment
 * @param  i: Actual iteration 
 * @param  powerSample: parameter of power sample from sweeping
 * @retval Result of the function was succesfull or not (EXIT_SUCCESS | EXIT_FAILURE) 
 */
int saveSamples(int i, float powerSample, int nElements)
{
    if (i == 0)
    {
        printf("generationFits | saveSamples() | Start saving power sample data to generate fits file\n");
        samples = (float*)calloc(nElements,sizeof(float));
        samples[i] = powerSample;
    }

    else
    {
        samples[i] = powerSample;
    }
    
    if (samples == NULL)
    {
        fprintf(stderr, "generationFits | saveSamples() | Was not possible to save power samples\n");
        return EXIT_FAILURE;
    }

    if (i == nElements-1)
    {
        printf("generationFits | saveSamples() | Execution Sucess\n");
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
    printf("generationFits | checkSavedData() | Start checking saved times data to generate fits file\n");
    
    // Check timing data
    for (i = 0; i < TRIGGERING_TIMES; i++)
    {
        if (strcmp(timeDatas[i], "") == 0)
        {
            fprintf(stderr, "generationFits | checkSavedData() | Was not possible to save timing data\n");
            return EXIT_FAILURE;
        }
    }

    printf("generationFits | checkSavedData() | Start checking saved power sample data to generate fits file\n");
    // Check power sample data
    for (i = 0; i < nElements; i++)
    {
        if (samples[i] == 0)
        {
            fprintf(stderr, "Data not saved correctly\n");
            return EXIT_FAILURE;
        }
    }

    printf("generationFits | checkSavedData() | Execution Sucess\n");
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
int generateFitsFile(char fileFitsName[], float*samples, uint32_t freq_min, uint32_t freq_max, float step_value, char startDate[], char timeStart[], char endDate[], char timeEnd[], struct tm timeFirstSweeping)
{   
    nElements = naxes[0]*naxes[1];

    printf("generationFits | generateFitsFile() | Start generating fits file\n");
    
    //Creation
    if (create(fileFitsName) == EXIT_FAILURE) { return EXIT_FAILURE; }

    //Update
    updateHeadersFitsFile(startDate, timeStart, endDate, timeEnd, freq_min, timeFirstSweeping);
   
    //Insert info
    if(insertData(samples) == EXIT_FAILURE)  { return EXIT_FAILURE;  }
    

    //CloseFile
    closeFits();

    printf("generationFits | generateFitsFile() | Execution Sucess\n");
    return EXIT_SUCCESS;
}
