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

/*** Global Variables***/
fitsfile *fptr =NULL;
float *frequencyDatas; // Frecuency Datas of the sweeping
char timeDatas[3600][50] = {""}; // Time Datas of the sweeping | 3600 dates
float* samples; // Array of float samples where dbs measures will be saved

int exist = 0;
int status = 0, ii, jj;
int fpixel = 1;
int naxis = 3, nElements, exposure;
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

        printf("generationFits | create() | Execution Sucess. File created: %s\n", fileFitsName);
        return EXIT_SUCCESS;
    }
}

/**
 * @brief  Update header values of the fits file
 * @note   
 * @retval None
 */
void updateHeadersFitsFile()
{
    /*Write a keyword; must pass the ADDRESS of the value*/
    time_t now = time(0);
    char *time_str = ctime(&now);
    time_str[strlen(time_str)-1] = '\0';
    long stepX = 0.25, stepY = -1, dataMax = UINT8_MAX, dataMin= 0, crPix1 = 0, crPix2 = 0,crVal1 = 3599., crVal2 = 200, bzero = 0., bscale = 1.;
    fits_update_key(fptr,TSTRING,"DATE", time_str,"Time of observation" ,&status); //Date observation
    fits_update_key(fptr,TSTRING, "OBJECT", "Space", "Object name", &status ); //Object Description
    fits_update_key(fptr,TLONG, "DATAMAX", &dataMax, "Max pixel data", &status);
    fits_update_key(fptr,TLONG, "DATAMIN", &dataMin, "Min pixel data", &status);
    fits_update_key(fptr,TSTRING, "ORIGIN", "TFM Alejandro", "Property", &status ); //Organization name
    fits_update_key(fptr,TSTRING, "TELESCOP","Radio", "Type of intrument", &status); //Instrument type
    fits_update_key(fptr,TSTRING, "INSTRUME","HackRF One", "Name of the instrument", &status); //Instrument type
    fits_update_key(fptr,TSTRING, "CTYPE1", "Frequency[MHz]", "MHz", &status ); //Title axis 1
    fits_update_key(fptr,TSTRING, "CTYPE2", "Power[dB]", "Power (dB)", &status ); //Title axis 2
  /*  fits_update_key(fptr,TLONG, "CDELT1", &stepX, "Steps Frequency", &status); //Steps axis X
    fits_update_key(fptr,TLONG, "CDELT2", &stepY, "Steps samples", &status); //Steps axis Y
    fits_update_key(fptr,TLONG, "CRPIX1", &crPix1, "reference Pixel 1", &status); 
    fits_update_key(fptr,TLONG, "CRPIX2", &crPix2, "reference Pixel 2 ", &status); 
    fits_update_key(fptr,TLONG, "CRVAL1", &crVal1, "Value reference Pixel 1", &status); 
    fits_update_key(fptr,TLONG, "CRVAL2", &crVal2, "Value reference Pixel 2 ", &status); 
    fits_update_key(fptr,TLONG, "BZERO", &bzero, "Scaling offset", &status); 
    fits_update_key(fptr,TLONG, "BSCALE", &bscale, "Scaling factor", &status); 
*/
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
 * @note   
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
 * @note   
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
 * @brief Invokes all the functionality to have a correct fits file 
 * @note   
 * @param  fileFitsName[]: name of the fits file
 * @param  samples: Input data of the fits file
 * @param  freq_min: Min frequency
 * @param  freq_max: Max frequency
 * @param  step_value: step value between frequencies
 * @retval Result of the function was succesfull or not (EXIT_SUCCESS | EXIT_FAILURE) 
 */
int generateFitsFile(char fileFitsName[], float*samples, uint32_t freq_min, uint32_t freq_max, float step_value)
{   
    //save frequency data | Timing data should before calling this function
    if (saveFrequencies(freq_min, freq_max, step_value) == EXIT_FAILURE) { return EXIT_FAILURE; } 

    //Creation
    printf("generationFits | generateFitsFile() | Generating Filename: %s\n",fileFitsName);
    printf("generationFits | generateFitsFile() | Dimensions of Filename:[%ld][%ld]\n",naxes[0],naxes[1]);
    
    if (create(fileFitsName) == EXIT_FAILURE) { return EXIT_FAILURE; }

    //Update
    updateHeadersFitsFile();
   
    //Insert info
    if(insertData(samples) == EXIT_FAILURE)  { return EXIT_FAILURE;  }
    

    //CloseFile
    closeFits();
    return EXIT_SUCCESS;
}
