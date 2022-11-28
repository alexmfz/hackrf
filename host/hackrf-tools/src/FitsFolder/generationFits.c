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

#define TRIGGERING_TIMES (5)

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
float array_img[3600][200]; //naxes[0]naxes[y] (axis x ,axis y)

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
        printf("generationFits | create() | File already exists. Overwritting fits file.\n");
        if(fits_open_file(&fptr, fileFitsName, READWRITE, &status))
        {
            fprintf(stderr, "generationFits | create() | Was not possible to open the file");
            return EXIT_FAILURE;
        }
        
        if(fits_delete_file(fptr,&status))
        {
            fprintf(stderr,"generationFits | create() | Was not possible to delete the file");
            return EXIT_FAILURE;
        }
        if(fits_create_file(&fptr, fileFitsName, &status))
        {
            fprintf(stderr, "generationFits | create() | Was not possible to create the file");
            return EXIT_FAILURE;
        }
        
       /*create the primary array image (16-bit short integer pixels*/
        if(fits_create_img(fptr, FLOAT_IMG, naxis, naxes, &status))
        {
            fprintf(stderr, "generationFits | create() | Was not possible to create the image");
            return EXIT_FAILURE;
        }
        printf("generationFits | create() | Execution Sucess. File overwrriten: %s\n", fileFitsName);
        return EXIT_SUCCESS;
    }
    else //if not exist
    {
        if(fits_create_file(&fptr, fileFitsName, &status))
        {
            fprintf(stderr, "generationFits | create() | Was not possible to create the file");
            return EXIT_FAILURE;
        }

        /*create the primary array image (16-bit short integer pixels*/
        if(fits_create_img(fptr, FLOAT_IMG, naxis, naxes, &status))
        {
            fprintf(stderr, "generationFits | create() | Was not possible to create the image");
            return EXIT_FAILURE;
        }

        printf("generationFits | create() | Execution Sucess. File created: %s with dimensions [%ld]x[%ld]\n", fileFitsName, naxes[0], naxes[1]);
        return EXIT_SUCCESS;
    }
}

/**
 * @brief  Update header values of the fits file
 * @note   
 * @retval None
 */
void updateHeadersFitsFile(char* startDate, char *timeStart, char *endDate, char *timeEnd)
{
    printf("generationFits | updateHeadersFitsFile | Updating fits headers to format\n");

    long bzero = 0., bscale = 1.;
    long dataMax = 0, dataMin= -200;
    long crVal1 = 900., crPix1 = 0, stepX = 0.25;
    long crVal2 = 200., crPix2 = 0, stepY = -1;

    fits_update_key(fptr, TSTRING,"DATE", startDate, "Time of observation" ,&status); // Date observation

    fits_update_key(fptr, TSTRING, "CONTENT", "Sweeping hackRF Operation", "Title", &status);

    fits_update_key(fptr, TSTRING, "ORIGIN", "University of Alcala de Henares", "Organization name", &status); // Organization name

    fits_update_key(fptr, TSTRING, "TELESCOP","SDR", "Type of intrument", &status); // Instrument type
    fits_update_key(fptr, TSTRING, "INSTRUME","HACKRF One", "Name of the instrument", &status); // Instrument name

    fits_update_key(fptr, TSTRING, "OBJECT", "Space", "Object name", &status); // Object Description

    fits_update_key(fptr, TSTRING, "DATE-OBS", startDate, "Date observation starts", &status); // Date observation starts
    fits_update_key(fptr, TSTRING, "TIME-OBS", timeStart, "Time observation starts", &status); // Time observation starts
    fits_update_key(fptr, TSTRING, "DATE-OBS", endDate, "Time observation starts", &status); // Date observation ends
    fits_update_key(fptr, TSTRING, "TIME-END", timeEnd, "Time observation starts", &status); // Time observation ends

    fits_update_key(fptr, TLONG, "BZERO", &bzero, "Scaling offset", &status); // Scaling offset
    fits_update_key(fptr, TLONG, "BSCALE", &bscale, "Scaling factor", &status);  // Scaling factor
    
    fits_update_key(fptr, TSTRING, "BUNIT", "digits", "Z - axis title", &status); // z- axis title

    fits_update_key(fptr, TLONG, "DATAMAX", &dataMax, "Max pixel data", &status);
    fits_update_key(fptr, TLONG, "DATAMIN", &dataMin, "Min pixel data", &status);

    fits_update_key(fptr, TLONG, "CRVAL1", &crVal1, "Value on axis 1 at the reference pixel [sec of a day]", &status);  // Value on axis 1 at reference pixel [sec of a day]
    fits_update_key(fptr, TLONG, "CRPIX1", &crPix1, "Reference pixel of axis 1", &status); // Reference pixel of axis 1
    fits_update_key(fptr, TSTRING, "CTYPE1", "Time [UT]", "Title of axis 1", &status ); // Title of axis 1
    fits_update_key(fptr, TLONG, "CDELT1", &stepX, "Step between first and second element in x-axis", &status); // Step between first and second element in x-axis (0.25 s)

// TODO: CHECK CRVAL2 && STEP Y
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
    printf("generationFits | insertData() | Inserting data...\n");
    //naxes[1] = 200
    //naxes[0] = 3600
    nElements = naxes[0]*naxes[1];
    {
        for(ii= 0; ii< naxes[1]; ii++ )
        {        
            for (jj=naxes[0]; jj< naxes[0]; jj++)
            {
                array_img[jj][ii] = (uint8_t)(-samples[jj]);
            }
            
        }    
    }

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
 * @brief  Save the frequencies from the sweeping to insert into fits file has headers data
 * @note   
 * @param  freq_min: Min freq of the sweeping
 * @param  freq_max: Max freq of the sweeping
 * @param  step_value: value between frequencies
 * @retval Result of the function was succesfull or not (EXIT_SUCCESS | EXIT_FAILURE) 
 */
int saveFrequencies(uint32_t freq_min, uint32_t freq_max, float step_value)
{   
    int i = 0;
    int numberFrequencies = naxes[1];
    
    printf("generationFits | saveFrequencies() | Start saving index data to generate fits file\n");
    
    frequencyDatas = (float*)calloc(numberFrequencies,sizeof(float));
    for(i = 0; i< numberFrequencies; i++)
    {
        frequencyDatas[i] = freq_min + step_value;
    }
    
    if (frequencyDatas == NULL && frequencyDatas[numberFrequencies] != freq_max) 
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
 * @brief  Freqs are not in ordered so sample values must be reorganizated
 * @note   TODO: still not developed
 * @retval None
 */
void associateFreqsToSample()
{
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
int generateFitsFile(char fileFitsName[], float*samples, uint32_t freq_min, uint32_t freq_max, float step_value, char startDate[], char timeStart[], char endDate[], char timeEnd[])
{   
    printf("generationFits | generateFitsFile() | Start generating fits file\n");
    //save frequency data | Timing data should before calling this function
    if (saveFrequencies(freq_min, freq_max, step_value) == EXIT_FAILURE) { return EXIT_FAILURE; } 

    //Creation
    if (create(fileFitsName) == EXIT_FAILURE) { return EXIT_FAILURE; }

    //Update
    updateHeadersFitsFile(startDate, timeStart, endDate, timeEnd);
   
    //Insert info
    if(insertData(samples) == EXIT_FAILURE)  { return EXIT_FAILURE;  }
    

    //CloseFile
    closeFits();

    printf("generationFits | generateFitsFile() | Execution Sucess\n");
    return EXIT_SUCCESS;
}
