#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include "fitsio.h"
#include <sys/types.h>
#include <inttypes.h>

// -I/usr/local/src/cfitsio-4.1.0 -lcfitsio to compile CFITSIO

#define INTERVAL 25e4 //us => 25e4 us = 250 ms

struct itimerval timer;    
struct timeval preTriggering;
struct timeval postTriggering;

int timerFlag = 0;
char timeDatas[3600][50] = {""}; // Time Datas of the sweeping | 3600 dates
float *samples;
float * samplesOrdered;

float *example;
int *flagsOrder;

/**TEST FITS**/
fitsfile *fptr = NULL; // pointer to table with X and Y cols
fitsfile *histptr = NULL; // pointer to output FITS Image
fitsfile *histptr2 = NULL;

int exist = 0;
int status = 0, ii, jj;
int fpixel = 1;
int naxis = 2, nElements, exposure;
long naxes[2];// = {3600,200, 720000}; //200 filas(eje y) 400 columnas(eje x
uint8_t array_img[200][3600];//float array_img[200][200]; //naxes[0]naxes[y] (axis x ,axis y)
float *frequencies;
float *times;

/**Test order**/
int orderValue = 1; 

void timerHandler(int sig)
{
    timerFlag = 1;
} 

void setTimerParams()
{
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = INTERVAL;
    timer.it_interval = timer.it_value;

    printf("timer | setTimerParams | Value interval: %ld us\n", timer.it_interval.tv_usec);
    printf("timer | setTimerParams | Success\n");
}

void execTimer()
{
 if(signal(SIGALRM, timerHandler) == SIG_ERR)
    {
        fprintf(stderr, "timer | hackRFTrigger | Unable to catch alarm signal");
        return ;
    }
   
    if(setitimer(ITIMER_REAL, &timer, 0) == -1)
    {
        fprintf(stderr, "timer | hackRFTrigger | Error calling timer");
        return ;
    }
    
}

static float TimevalDiff(const struct timeval *a, const struct timeval *b) {
   return (a->tv_sec - b->tv_sec) + 1e-6f * (a->tv_usec - b->tv_usec);
}

void timerNewConceptTest()
{
    int initTimer = 0, iteration = 0;
    setTimerParams();

    gettimeofday(&preTriggering, NULL);
    execTimer();
    while(1)
    {
        if (timerFlag == 1)
        {
            fprintf(stderr, "Iteration %d: %f s\n", iteration, TimevalDiff(&postTriggering, &preTriggering));
            gettimeofday(&postTriggering, NULL);
            timerFlag = 0;
            iteration++;
            execTimer();
        }
    }

}

int saveTimesTest(int i, int triggeringTimes, char* sweepingTime)
{
    if ( i == 0)
    {
        printf("generationFits | saveTimes() | Start saving index data to generate fits file\n");
    }
    
     // Saving step
    strcpy(timeDatas[i], sweepingTime);

    // Checks
    if (strcmp(timeDatas[i], "") == 0)
    {
        fprintf(stderr, "generationFits | saveTimes() | Was not possible to allocate timing data\n");
        return -1;
    }

    if ( i == triggeringTimes-1)
    {
        printf("generationFits | saveTimes() | Execution Sucess\n");
    }

   
    return 0;
}

void saveTimesLoopTest(char *sweepingTime)
{
    int i = 0; 
    int triggeringTimes = 3600;
    char assignation[50];

    gettimeofday(&preTriggering, NULL);
    for (i = 0; i < triggeringTimes; i++)
        if (saveTimesTest( i, triggeringTimes, sweepingTime) == -1)
        {
            fprintf(stderr, "Exiting\n");
            return;
        }
    gettimeofday(&postTriggering, NULL);
    fprintf(stderr, "Time with checking: %f\n", TimevalDiff(&postTriggering, &preTriggering));

   /*printf("Printing results:\n");
    for (i = 0; i < triggeringTimes; i++)
        printf("Result %i: %s\n", i, timeDatas[i]);
*/
    /**Time with just assignation**/
    gettimeofday(&preTriggering, NULL);
    for(i = 0; i< triggeringTimes; i++)
    {
        strcpy(assignation, sweepingTime);
    }
    gettimeofday(&postTriggering, NULL);
    fprintf(stderr, "Time without checking: %f", TimevalDiff(&postTriggering, &preTriggering));
}

int saveSamplesTest(int i, float powerSample, int nElements)
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
        fprintf(stderr, "generationFits | saveSamples() | Was not possible to allocate power samples\n");
        return -1;
    }

    if (i == nElements-1)
    {
        printf("generationFits | saveSamples() | Execution Sucess\n");
    }

    return 0;
}

void saveSamplesLoopTest()
{
    int i = 0, j = 0; 
    int triggeringTimes = 10;

    for (i = 0; i < triggeringTimes; i++)
    {
        for (j = i*(50/triggeringTimes); j < (50/triggeringTimes)*(i+1); j++)
        {
            saveSamplesTest(j, j, 50);
        }
    }
    printf("\n");

    for (i = 0; i < triggeringTimes; i++)
    {
        for (j = i*(50/triggeringTimes); j < (50/triggeringTimes)*(i+1); j++)
        {
            printf("Value[%d]: %f\n", j, samples[j]);
        }
    }
     

}

void stringTest()
{
    char texto[100], texto2[100], texto3[100], texto4[100], texto5[100], texto6[100]={""};
    long int numero = 212;
    
    fprintf(stderr, "Prueba con numero con 0 delante: %ld\n", numero);
    sprintf(texto, "%04ld", numero);
    sprintf(texto2, "%04ld", numero);
    printf("Texto1: %s\n",texto);
    printf("Texto2: %s\n",texto2);
    strncat(texto2, texto, 2);
    printf("%s\n",texto2);


    fprintf(stderr, "Prueba con mismo numero pero sin 0 delante\n");
    int numero2 = 2012;

    sprintf(texto3, "%d", numero2);
    sprintf(texto4, "%d", numero2);
    printf("Texto3: %s\n",texto3);
    printf("Texto4: %s\n",texto4);
    strncat(texto4, texto3, 2);
    printf("%s\n",texto4);


    fprintf(stderr, "Prueba con cadena con 0 delante\n");
    strcpy(texto5, "02012");
    strncat(texto6, "02012",3);
    printf("Texto 5: %s\n",texto5);
    printf("Texto6: %s\n",texto6);



}

void format_headerFitsTest()
{
    time_t now = time(0);
    char *time_str = ctime(&now);
    time_str[strlen(time_str)-1] = '\0';
    printf("%s", time_str);
}

float minData(float * samples)
{
    int  i = 0;
    float minimum = samples[0];

    for (i = 0; i< nElements; i++)
    {
        if (minimum > samples[i])
        {
            minimum = samples[i];
        }
    }

    return minimum;
}

float maxData(float * samples)
{
    int  i = 0;
    long maximum = samples[0];

    for (i = 0; i< nElements; i++)
    {
        if (maximum < samples[i])
        {
            maximum = samples[i];
        }
    }

    return maximum;
}

double getSecondsOfDayOfFirstSweeping(struct tm timeFirstSweeping)
{
    double sec_of_day = (double)timeFirstSweeping.tm_hour*3600 + (double)timeFirstSweeping.tm_min*60 + (double)timeFirstSweeping.tm_sec;
    return sec_of_day;
}

int createFile(char fileFitsName[], char histogram[])
{   
    /*Checks if the new file exist*/
    fits_file_exists(fileFitsName, &exist, &status);
    if(exist == 1)
    {
        printf("generationFits | createFile() | File already exists. Overwritting fits file.\n");
        if(fits_open_file(&fptr, fileFitsName, READWRITE, &status))
        {
            fprintf(stderr, "generationFits | createFile() | Was not possible to open the file");
            return EXIT_FAILURE;
        }

        if(fits_open_file(&histptr, histogram, READWRITE, &status))
        {
            fprintf(stderr, "generationFits | createFile() | Was not possible to open the file");
            return EXIT_FAILURE;
        }
        
        
        if(fits_delete_file(fptr, &status))
        {
            fprintf(stderr,"generationFits | createFile() | Was not possible to delete the file\n");
            return EXIT_FAILURE;
        }

        if(fits_delete_file(histptr, &status))
        {
            fprintf(stderr,"generationFits | createFile() | Was not possible to delete the file\n");
            return EXIT_FAILURE;
        }


        if(fits_create_file(&fptr, fileFitsName, &status))
        {
            fprintf(stderr, "generationFits | createFile() | Was not possible to create the file\n");
            return EXIT_FAILURE;
        }

         if(fits_create_file(&histptr, histogram, &status))
        {
            fprintf(stderr, "generationFits | createFile() | Was not possible to create the file\n");
            return EXIT_FAILURE;
        }

        printf("generationFits | createFile() | Execution Sucess. File overwrriten: %s\n", fileFitsName);
        return EXIT_SUCCESS;
    }

    else //if not exist
    {
        if(fits_create_file(&fptr, fileFitsName, &status))
        {
            fprintf(stderr, "generationFits | createFile() | Was not possible to create the file\n");
            return EXIT_FAILURE;
        }
        
        if(fits_create_file(&histptr, histogram, &status))
        {
            fprintf(stderr, "generationFits | createFile() | Was not possible to create the file\n");
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }
}

int createImage()
{
    /*create the primary array image (float) pixels*/
    if(fits_create_img(fptr, BYTE_IMG, naxis, naxes, &status))
    {
        fprintf(stderr, "generationFits | createImage() | Was not possible to create the image");
        return EXIT_FAILURE;
    }
        
    printf("generationFits | createImage() | Execution Sucess. Image created with dimensions [%ld]x[%ld]\n", naxes[0], naxes[1]);
    return EXIT_SUCCESS;
       
}

void updateHeadersFitsFileImage(struct tm localTimeFirst, struct tm localTimeLast, uint32_t freq_min)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    char user_buf[70]; //msconds
    char date[70]; // Time of observation
   	strftime(date, sizeof date,"%Y-%m-%d", &localTimeFirst);

	char date_obs[70]; //Date observation starts
    strftime(date_obs, sizeof date_obs, "%Y/%m/%d", &localTimeFirst);

    char time_obs[70]; // Time observation starts
    strftime(time_obs, sizeof time_obs, "%H:%M:%S", &localTimeFirst); 
    strcat(time_obs, ".");
    sprintf(user_buf, "%03d", (int)tv.tv_usec);
    strncat(time_obs, user_buf, 3);

	char date_end[70]; // Date observation ends
    strftime(date_end, sizeof date_end, "%Y/%m/%d", &localTimeLast); 

	char time_end[70]; // time observation ends
	strftime(time_end, sizeof time_end, "%H:%M:%S", &localTimeLast); 

    char content[70];
    strftime(content, sizeof content, "%Y/%m/%d", &localTimeFirst);
    strcat(content, " Radio Flux density, HackRF-One (Spain)");

    printf("generationFits | updateHeadersFitsFile | Updating fits headers to format\n");

    double bzero = 0., bscale = 1.;
    float dataMax = maxData(samples), dataMin= minData(samples);
    double exposure = 1500. ;
    double crVal1 = getSecondsOfDayOfFirstSweeping(localTimeFirst);
    long crPix1 = 0;
    double stepX = 0.25; // crVal1 Start axis x value 0 [sec of a day]
    
    double crVal2 = 45.;
    long crPix2 = 0;
    double stepY = -1.; // crVal2 Start axis y value 0
    
    double obs_lat = 40.5131623, obs_lon = -3.3527243, obs_alt = 594.; // Coordinates
    long pwm_val = -10;

    fits_update_key(fptr, TDOUBLE, "EXPOSURE", &exposure, "EXPOSURE", &status);
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

    fits_update_key(fptr, TFLOAT, "DATAMAX", &dataMax, "Max pixel data", &status);
    fits_update_key(fptr, TFLOAT, "DATAMIN", &dataMin, "Min pixel data", &status);

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

    fits_update_key(fptr, TSTRING, "FRQFILE", "FRQ02545.CFG", "Name of frequency file", &status);
    fits_update_key(fptr, TLONG, "PWM_VAL", &pwm_val, "PWM value to control tuner gain", &status);

    printf("generationFits | updateHeadersFitsFile | Execution Sucess\n");
}

int insertDataImage(float* samples)
{
    /*Initialize the values in the image with a linear ramp function*/
    printf("generationFits | insertDataImage() | Inserting data...\n");
    int id = 0;
    
    for(ii= 0; ii< naxes[0]; ii++ )
    {        
        for (jj=0; jj< naxes[1]; jj++)
        {
            array_img[jj][ii] = (uint8_t)samples[id];
            id++;
        }
        
    }
        
      /*Write the array of integers of the image*/
    if(fits_write_img(fptr, TBYTE, fpixel, nElements, array_img[0], &status))
    {
        fprintf(stderr, "generationFits | insertData() | Was not possible to write data into the image");
        return EXIT_FAILURE;
    }

    printf("generationFits | insertData() | Execution Sucess\n");
    return EXIT_SUCCESS;
}

int createBinTable()
{
    long nRows = 0, nCols = 2;
   
    char *ttype[] = {"TIME", "FREQUENCY"}; // Name of columns

    char *tform[] = {"3600D8.3", "200D8.3"}; // Data type of each column

    char *tunit[] = {"SECONDS", "MHz"}; // Physical unit of each column

    char *extname = {"BINARY TABLE"}; // Name of the table

    printf("generationFits | createBinTable() | Creating Binary table\n");

    /*create the binary table*/
  /*  if(fits_create_tbl(fptr, BINARY_TBL, nRows, nCols, ttype, tform, tunit, NULL, &status))
    {
        fprintf(stderr, "generationFits | createBinTable() | Was not possible to create the binary table\n");
        return EXIT_FAILURE;
    }
*/
    if(fits_create_tbl(histptr, BINARY_TBL, nRows, nCols, ttype, tform, tunit, NULL, &status))
    {
        fprintf(stderr, "generationFits | createBinTable() | Was not possible to create the binary table\n");
        return EXIT_FAILURE;
    }

    printf("generationFits | createBinTable() | Execution Sucess\n");
    return EXIT_SUCCESS;
}

void updateHeadersFitsFileBinTable()
{
    double tzero = 0., tscale = 1.;

    fits_update_key(fptr, TDOUBLE, "TSCALE1", &tscale, "Scaling factor", &status);  // Scaling factor
    fits_update_key(fptr, TDOUBLE, "TZERO1", &tzero, "Scaling offset", &status); // Scaling offset

    fits_update_key(fptr, TDOUBLE, "TSCALE2", &tscale, "Scaling factor", &status);  // Scaling factor
    fits_update_key(fptr, TDOUBLE, "TZERO2", &tzero, "Scaling offset", &status); // Scaling offset


    fits_update_key(histptr, TDOUBLE, "TSCALE1", &tscale, "Scaling factor", &status);  // Scaling factor
    fits_update_key(histptr, TDOUBLE, "TZERO1", &tzero, "Scaling offset", &status); // Scaling offset

    fits_update_key(histptr, TDOUBLE, "TSCALE2", &tscale, "Scaling factor", &status);  // Scaling factor
    fits_update_key(histptr, TDOUBLE, "TZERO2", &tzero, "Scaling offset", &status); // Scaling offset

    printf("generationFits | updateHeadersFitsFileBinTable() | Execution Sucess\n");
}

int insertDataBinTable_aux(float *times, float* frequencies)
{
    long tlmin = 0.25, tlmax = 900;
    long freqmin = 45, freqmax = 245;
    long nRows = 1, nCols = 2;
    double tzero = 0., tscale = 1.;
   
    char *ttype[] = {"TIME", "FREQUENCY"}; // Name of columns

    char *tform[] = {"3600D8.3", "200D8.3"}; // Data type of each column

    char *tunit[] = {"SECONDS", "MHz"}; // Physical unit of each column

    char *extname = {"BINARY TABLE"}; // Name of the table

    printf("generationFits | createBinTable() | Creating Binary table\n");

    /*create the binary table*/
    if(fits_create_tbl(fptr, BINARY_TBL, nRows, nCols, ttype, tform, tunit, NULL, &status))
    {
        fprintf(stderr, "generationFits | createBinTable() | Was not possible to create the binary table\n");
        return EXIT_FAILURE;
    }

    fits_update_key(fptr, TDOUBLE, "TSCALE1", &tscale, "Scaling factor", &status);  // Scaling factor
    fits_update_key(fptr, TDOUBLE, "TZERO1", &tzero, "Scaling offset", &status); // Scaling offset

    fits_update_key(fptr, TDOUBLE, "TSCALE2", &tscale, "Scaling factor", &status);  // Scaling factor
    fits_update_key(fptr, TDOUBLE, "TZERO2", &tzero, "Scaling offset", &status); // Scaling offset
    fits_update_key(fptr, TLONG, "TLMIN1", &tlmin, "Value", &status);
    fits_update_key(fptr, TLONG, "TLMAX1", &tlmax, "Value", &status);
    fits_update_key(fptr, TLONG, "TLMIN1", &tlmin, "Value", &status);
    fits_update_key(fptr, TLONG, "TLMAX1", &tlmax, "Value", &status);
    fits_update_key(fptr, TLONG, "TLMIN2", &freqmin, "Value", &status);
    fits_update_key(fptr, TLONG, "TLMAX2", &freqmax, "Value", &status);

    if (times == NULL || frequencies == NULL)
    {
        fprintf(stderr, "generationFits | insertDataBinTable() | Inserting data into the binary table failed\n");
        return EXIT_FAILURE;
    }   


    if (fits_write_col(fptr, TFLOAT, 1, 1, 1, naxes[0], times, &status)) 
    {
        fprintf(stderr, "generationFits | insertDataBinTable() | Inserting time datas into col failed\n");
        return EXIT_FAILURE;
    }

    if (fits_write_col(fptr, TFLOAT, 2, 1, 1, naxes[1], frequencies, &status)) 
    {
        fprintf(stderr, "generationFits | insertDataBinTable() | Inserting frequency datas into col failed\n");
        return EXIT_FAILURE;
    }
}

int insertDataBinTable(float* times, float* frequencies)
{
    printf("generationFits | insertDataBinTable() | Inserting data into the binary table\n");
    
    if (times == NULL || frequencies == NULL)
    {
        fprintf(stderr, "generationFits | insertDataBinTable() | Inserting data into the binary table failed\n");
        return EXIT_FAILURE;
    }   

/*
    if (fits_write_col(fptr, TFLOAT, 1, 1, 1, naxes[0], times, &status)) 
    {
        fprintf(stderr, "generationFits | insertDataBinTable() | Inserting time datas into col failed\n");
        return EXIT_FAILURE;
    }

    if (fits_write_col(fptr, TFLOAT, 2, 1, 1, naxes[1], frequencies, &status)) 
    {
        fprintf(stderr, "generationFits | insertDataBinTable() | Inserting frequency datas into col failed\n");
        return EXIT_FAILURE;
    }*/

      if (fits_write_col(histptr, TFLOAT, 1, 1, 1, naxes[0], times, &status)) 
    {
        fprintf(stderr, "generationFits | insertDataBinTable() | Inserting time datas into col failed\n");
        return EXIT_FAILURE;
    }

    if (fits_write_col(histptr, TFLOAT, 2, 1, 1, naxes[1], frequencies, &status)) 
    {
        fprintf(stderr, "generationFits | insertDataBinTable() | Inserting frequency datas into col failed\n");
        return EXIT_FAILURE;
    }

    printf("generationFits | insertDataBinTable() | Execution Success\n");
    return EXIT_SUCCESS;
}

void closeFits(fitsfile* fits)
{
    /*Close and report any error*/
    fits_close_file(fits, &status);
    fits_report_error(stderr, status);
    printf("generationFits | closeFits() | Execution Sucess\n");
}

int associateImageBinTable()
{
    float amin[2];
    amin[0] = 0.25; // Min value for time axis
    amin[1] = 45; // Min value for freq axis

    float amax[2];
    amax[0] = 900; // Max value for time axis
    amax[1] = 244; // Max value for freq axis

    int colnum[2];
    colnum[0] = 400;
    colnum[1] = 200;

    float binSize[2];
    binSize[0] = 400;
    binSize[1] = 400;

    char outfile[] = "example.fit";
    char colname[4][71] = {"TIME", "FREQUENCY"};
    char colname_aux[4][71] = {"TSCALE1", "TZERO1", "TSCALE2", "TZERO2"};

    printf("generationFits | associateImageBinTable() | Associating image to bin table to create histogram\n");
    if( fptr == NULL)
    {
        printf("generationFits | associateImageBinTable() | NULL\n");
    }
 
    
    /*if (ffhist2(&histptr, outfile, TFLOAT, naxis, colname, 
                (double*)amin, (double*)amax,
                (double*)binSize, 
                colname_aux, 
                colname_aux, 
                colname_aux, 
                0, 
                colname_aux[0], 
                0, 
                NULL, 
                &status
              ))*/
  //  if (fits_calc_binning(histptr, naxis, colname, (double*)amin, (double*)amax, (double*)binSize, colname_aux, colname_aux, colname_aux, &naxis, naxes, amin, amax, binSize, &status))
    if (fits_make_hist(histptr,
                    fptr,
                    FLOAT_IMG,
                    naxis, 
                    naxes, 
                    &naxis,
                    amin,  
                    amax, 
                    binSize,
                    FLOATNULLVALUE,
                    0, 
                    0, 
                    NULL,
                    &status
                    ))
    /*if (fits_calc_binning(histptr,
                        naxis, 
                        colname, 
                        NULL, NULL, binSize,
                        NULL, NULL, NULL,
                        &naxis, naxes,
                        amin, amax,
                        binSize,
                        &status
                        ))*/
    {
        fits_report_error(stderr, status);

        fprintf(stderr, "generationFits | associateImageBinTable() | Association failed. Status: %d\n", status);
        return EXIT_FAILURE;
    }
    printf("generationFits | associateImageBinTable() | Execution Success\n");
    return EXIT_SUCCESS;
}

int generateFitsFile_test(char histogram[], char fileFitsName[], float*samples, struct tm localTimeFirst, struct tm localTimeLast, uint32_t freq_min)
{   
    int typeCode = 0, hdutype = 0;
    long repeat = 0, width = 0;
    printf("generationFits | generateFitsFile() | Start generating fits file\n");

    // Creation of file for image
    if (createFile(fileFitsName, histogram) == EXIT_FAILURE) { return EXIT_FAILURE; }

    // Creation of image
    if (createImage() == EXIT_FAILURE) { return EXIT_FAILURE; }

    // Update headers of image
    updateHeadersFitsFileImage(localTimeFirst, localTimeLast, 45);

    // Insert data of image
    if(insertDataImage(samples) == EXIT_FAILURE) { return EXIT_FAILURE; }

    // Creation of binary table
    if(createBinTable() == EXIT_FAILURE) { return EXIT_FAILURE; }

    // Update headers of binary table
    updateHeadersFitsFileBinTable();

    // Insert data of binary table
    if(insertDataBinTable(times, frequencies) == EXIT_FAILURE) { return EXIT_FAILURE; }
    //if(insertDataBinTable_aux(times, frequencies) == EXIT_FAILURE) { return EXIT_FAILURE; }

    // Associate image to the bin table to create histograms
  //  if (associateImageBinTable() == EXIT_FAILURE) { return EXIT_FAILURE; }

    if(insertDataBinTable_aux(times, frequencies) == EXIT_FAILURE) { return EXIT_FAILURE; }
    // CloseFile (with the pointer of image)

    /*f (fits_read_tdim(fptr, 2, 2, &naxis, naxes, &status))
    {
                fits_report_error(stderr, status);

    }*/

    
    closeFits(fptr);
    closeFits(histptr);


    printf("generationFits | generateFitsFile() | Execution Sucess\n");
    return EXIT_SUCCESS;
}

int checkOrderFreqs_test(float *list_frequencies, int *real_order_frequencies, uint32_t freq_min, int nRanges) // WIll be invoke at hackrf_sweep callback
{
    int i = 0;
    
  //  printf("hackrf_sweep | checkOrder() | Ordering insertion of frequencies\n");
    if (list_frequencies == NULL || real_order_frequencies == NULL)
    {
        fprintf(stderr, "hackrf_sweep | checkOrder() | Not reserved memory correctly\n");
        return EXIT_FAILURE;
    }

    for (i = 0; i < nRanges; i++)
    {
        if (list_frequencies[i] == freq_min)
        {
            real_order_frequencies[i] = orderValue;
            orderValue++;
            break;
        }
    }
    
    printf("hackrf_sweep | checkOrder() | Execution Success\n");
    return EXIT_SUCCESS;
}

void printOrder(int *real_order_frequencies)
{
    int i = 0;
    for(i = 0; i < 40; i++)
    {
        printf("frequency_order[%d]: %d - samples[%d...%d] - Frequency: %d\n", i, real_order_frequencies[i], (real_order_frequencies[i]-1)*5, (real_order_frequencies[i]-1)*5-1 + 5, 45 +(real_order_frequencies[i]-1)*5);
    }
}

/**
 * @brief  Reorganize power samples into the correct position
 * @note   
 * @param  ordered_frecuency_position: Correct order of frequency
 * @param  real_order_frequency_position: Real order of frequency at sweeping
 * @param  nRanges: Nº of ranges of frequencies 
 * @param  samples: power samples at sweeping
 * @param  samplesOrdered: power samples organized after sweeping
 * @param  nElements: nElements of total sweepings
 * @param  valuesPerFreq: values per frequency (fftSize/4)
 * @param  nChannels: number of channels 
 * @retval Result of the function was succesfull or not (EXIT_SUCCESS | EXIT_FAILURE) 
 */
int reorganizeSamples_test(int ordered_frecuency_position, int real_order_frequency_position, int nRanges, float* samples, float* samplesOrdered, int nElements, int valuesPerFreq, int nChannels)
{
    int j = 0, z = 0, i = 0;
    int samplesPerSweeping = valuesPerFreq*nRanges; // 200
    int actualElementIndex = 0; //Actual element index
    float sampleToSave; //sample to backup
    printf("hackrf_sweep | reorganizeSamples() | Reorganizing Samples;\n");

    if (samplesOrdered == NULL || samples == NULL)
    {
        fprintf(stderr, "hackrf_sweep | reorganizeSamples() | Was not possible to allocate memory\n");
        return EXIT_FAILURE;
    }

    printf("ordered_frequency_position: %d \t real_order_frequency_position:%d\n", ordered_frecuency_position, real_order_frequency_position);

    // Initial positions at first iteration
    j = (ordered_frecuency_position-1)*valuesPerFreq;  // Index of ordered samples
    i = j + valuesPerFreq; // actual Index
    z = (real_order_frequency_position-1)*valuesPerFreq; // Index of disordered samples

    printf("j = %d\t i = %d\t z = %d\n", j, i ,z);
    while (actualElementIndex < 1000 ) // 1000 => nElements
    {
        while(j < i) // valuesPerFreq = fftSize/4
        {
            sampleToSave = samples[j]; // samples[i] at this moment are in incorrect order
            samplesOrdered[j] = samples[z]; // samples[z] are the ones we want to move to the correct order
            samplesOrdered[z] = sampleToSave; // Recover of samples saves as backup
           /* printf("j = %d\t z = %d\t i = %d\t sampleToSave: %lf\t samples[%d]: %lf\t samplesOrdered[%d]: %lf\t samplesOrdered[%d]: %lf\n", 
                j, z, i,
                sampleToSave,
                z, samples[z],
                j, samplesOrdered[j],
                z, samplesOrdered[z]);
            printf("Press 'Enter' to continue....");
            while(getchar()!='\n');*/
            j++;
            z++;         
        }

        actualElementIndex += samplesPerSweeping; // nRanges of frequencies * valuesPerfrequency index
        j += samplesPerSweeping-valuesPerFreq;
        z += samplesPerSweeping-valuesPerFreq;
        i += samplesPerSweeping;
        /* printf("Finished iteration\n actualElementIndex: %d\t j = %d \t i = %d\t z = %d\n Press 'Enter' to continue....", actualElementIndex, j, i , z);
            while(getchar()!='\n');*/
        printf("\n");
    }

    flagsOrder[real_order_frequency_position-1] = 1;
    flagsOrder[ordered_frecuency_position-1] = 1;
    

    printf("hackrf_sweep | reorganizeSamples() | Execution Success;\n");
    return EXIT_SUCCESS;
}

void generateDinamicNameTest()
{
    //Format name: hackRFOne_UAH_DDMMYYYY_HH_MM.fits
    time_t timeNow;
    struct tm basename;

    char fileName[50];
    char name[50];
    char suffix[50] = ".fits";
    char preffix[50] = "hackRFOne_UAH_";

    timeNow = time(NULL);
    localtime_r(&timeNow, &basename);
    strftime(name, sizeof(name), "%d%m%Y_%Hh_%Mm", &basename);
    
    strcat(preffix, strcat(name, suffix));
    strcpy(fileName, preffix);

    printf("%s\n", fileName);
}

void anotherTimingTest()
{
        struct tm info1;
        struct tm info2;
      	time_t beginning = time(NULL);

       	char timeStart[70];
        char timeEnd[70];


        localtime_r(&beginning, &info1);
        
        sleep(2);
        time_t end = time(NULL);
        localtime_r(&end, &info2);

	    strftime(timeStart, sizeof timeStart, "%Y-%m-%d %H:%M:%S", &info1);
        strftime(timeEnd, sizeof timeStart, "%Y-%m-%d %H:%M:%S", &info2);
        printf("%s\n", timeStart);
        printf("%s\n", timeEnd);
}

int startExecutionTest()
{
    time_t tStart;
    struct tm tmStart;
    char startString[50];
    tStart = time(NULL);
    localtime_r(&tStart, &tmStart);
    
    tmStart.tm_min = tmStart.tm_min + 15 - tmStart.tm_min%15;
    tmStart.tm_sec = 0;
    strftime(startString, sizeof(startString), "%H:%M:%S", &tmStart);
    
    tStart = time(NULL);
    localtime_r(&tStart, &tmStart);
    
    printf("functions| startExecution() | Execution will start at %s\n", startString);

    while(tmStart.tm_min %15 != 0 || tmStart.tm_sec != 0 )
    {
        tStart = time(NULL);
        localtime_r(&tStart, &tmStart);
    }

    strftime(startString, sizeof(startString), "%H:%M:%S", &tmStart);
    printf("functions| startExecution() | Execution Started\n");
}

void calculateTimes(time_t* t_timeStartConfig, struct tm* tm_timeStartConfig, struct timeval* timeValStartConfig)
{
        gettimeofday(timeValStartConfig, NULL);
	    *t_timeStartConfig = time(NULL);
	    localtime_r(t_timeStartConfig, tm_timeStartConfig);
}

int generateFitsFile_withBinTable_test()
{

    int i = 0;
    uint32_t freq_min = 45;
    float previousTime = 0;
    float previousFreq = 0;
    float previousValue = 0;

    fprintf(stderr, "testGenenerationFitsFile\n");
    char pathFits[] = "testGenenerationFitsFile.fit"; // File name of fits file
    char histogram[] = "histogram.fit";
    
    time_t now = time(NULL);
    struct tm localTimeFirst;
	localtime_r(&now, &localTimeFirst);

   
    previousValue = 0.01;

    samples = (float*)calloc(nElements,sizeof(float));
    for(i = 0; i< nElements; i++)
    {
        samples[i] = previousValue;
        previousValue += 0.01; 
    }

    times = (float*)calloc(naxes[0], sizeof(float));
    frequencies = (float*)calloc(naxes[1], sizeof(float));
    for (i = 0; i< naxes[0]; i++)
    {
        if (i == 0)
        {
            times[i] = 0.25;
            previousTime = times[i];
        }
        
        else
        {
            previousTime += 0.25;
            times[i] = previousTime;
        }
    }

    for (i = 0; i< naxes[1]; i++)
    {
        if (i == 0)
        {
            frequencies[i] = (float)freq_min;
            previousFreq = frequencies[i];
        }

        else
        {
            previousFreq += 1;
            frequencies[i] = previousFreq;
        }
    }

    time_t after = time(NULL);
    struct tm localTimeLast;
    
    localtime_r(&after, &localTimeLast);

    generateFitsFile_test(histogram, pathFits, samples, localTimeFirst, localTimeLast, freq_min);
}


int main(int argc, char** argv) 
{
    int testData = 7;
    int i = 0;
    int nChannels = 200;
    int stepValue = 5;
    int nRanges = nChannels/stepValue;
    int valuesPerFreq = 5;
    float* list_frequencies;
    uint32_t freq_min = 45;
    int * real_order_frequencies;
    naxes[0] = 200;
    naxes[1] = 200;
    nElements = naxes[0] * naxes[1];

    /*fprintf(stderr, "timerNewConceptTest()\n");
    timerNewConceptTest(); // Is in a while(1), discomment to test*/
    if (testData ==0)
    {
        gettimeofday(&preTriggering, NULL);
        fprintf(stderr, "saveTimesLoopTest()\n");
        saveTimesLoopTest("Sweeping Time");
        printf("\n");
        printf("Press 'Enter' to continue....");
        while(getchar()!='\n');

        saveTimesLoopTest("");
        printf("\n");
        printf("Press 'Enter' to continue....");
        while(getchar()!='\n');

        gettimeofday(&preTriggering, NULL);
        saveSamplesLoopTest();
        gettimeofday(&postTriggering, NULL);
        printf("Time of saving samples: %f s\n", TimevalDiff(&postTriggering, &preTriggering));
        printf("\n");
        printf("Press 'Enter' to continue....");
        while(getchar()!='\n');

        stringTest();
        printf("\n");
        printf("Press 'Enter' to continue....");
        while(getchar()!='\n');

        format_headerFitsTest();
        printf("\n");
        printf("Press 'Enter' to continue....");
        while(getchar()!='\n');
    }

    else if (testData == 1)
    {
        fprintf(stderr, "testGenenerationFitsFile\n");
        char pathFits[] = "testGenenerationFitsFile.fits"; // File name of fits file
        char histogram[] = "histogram.fits";
        
        time_t now = time(NULL);
        struct tm localTimeFirst;
        localtime_r(&now, &localTimeFirst);

        samples = (float*)calloc(nElements,sizeof(float));
        for(i = 0; i< nElements; i++)
        {
            samples[i] = -i + 0.1;
        }

        time_t after = time(NULL);
        struct tm localTimeLast;
        localtime_r(&after, &localTimeLast);
      
        generateFitsFile_test(histogram, pathFits, samples, localTimeFirst, localTimeLast, freq_min);
    }  
    
    else if (testData == 2)// For ordered samples
    {
        fprintf(stderr, "Execution of ordered samples logic\n");

        float actual_freq = (float)freq_min;
        
        list_frequencies = (float*)calloc(nRanges,sizeof(float)); // from 45MHz to 245MHz
        real_order_frequencies = (int*) calloc(nRanges, sizeof(int)); // positions of frecuencies
        flagsOrder = (int*)calloc(nRanges, sizeof(int));

        for (i = 0; i< nRanges; i++)
        {
            list_frequencies[i] = actual_freq;
            actual_freq+=stepValue;
            flagsOrder[i] = 0;
        }

        if (checkOrderFreqs_test(list_frequencies, real_order_frequencies, freq_min, nRanges) == EXIT_FAILURE) { return EXIT_FAILURE; } //45 => ok
        
        freq_min+=10;
        if (checkOrderFreqs_test(list_frequencies, real_order_frequencies, freq_min, nRanges) == EXIT_FAILURE) { return EXIT_FAILURE; } //55 => wrong
        
        freq_min-=5; 
        if (checkOrderFreqs_test(list_frequencies, real_order_frequencies, freq_min, nRanges) == EXIT_FAILURE) { return EXIT_FAILURE; } //50 => wrong
        
        freq_min+=10;
        if (checkOrderFreqs_test(list_frequencies, real_order_frequencies, freq_min, nRanges) == EXIT_FAILURE) { return EXIT_FAILURE; } //60 => ok

        freq_min+=5;
        if (checkOrderFreqs_test(list_frequencies, real_order_frequencies, freq_min, nRanges) == EXIT_FAILURE) { return EXIT_FAILURE; } //65 => ok        

        freq_min+=10;
        if (checkOrderFreqs_test(list_frequencies, real_order_frequencies, freq_min, nRanges) == EXIT_FAILURE) { return EXIT_FAILURE; } //75 => wrong

        freq_min-=5;
        if (checkOrderFreqs_test(list_frequencies, real_order_frequencies, freq_min, nRanges) == EXIT_FAILURE) { return EXIT_FAILURE; } //70 => wrong

        freq_min+=10;
        if (checkOrderFreqs_test(list_frequencies, real_order_frequencies, freq_min, nRanges) == EXIT_FAILURE) { return EXIT_FAILURE; } //80 => ok

        freq_min+=5;
        if (checkOrderFreqs_test(list_frequencies, real_order_frequencies, freq_min, nRanges) == EXIT_FAILURE) { return EXIT_FAILURE; } //85 => ok

        freq_min+=10;
        if (checkOrderFreqs_test(list_frequencies, real_order_frequencies, freq_min, nRanges) == EXIT_FAILURE) { return EXIT_FAILURE; } //95 => wrong

        freq_min-=5;
        if (checkOrderFreqs_test(list_frequencies, real_order_frequencies, freq_min, nRanges) == EXIT_FAILURE) { return EXIT_FAILURE; } //90 => wrong

        freq_min+=10;
        if (checkOrderFreqs_test(list_frequencies, real_order_frequencies, freq_min, nRanges) == EXIT_FAILURE) { return EXIT_FAILURE; } //100 => ok

        freq_min+=5;
        if (checkOrderFreqs_test(list_frequencies, real_order_frequencies, freq_min, nRanges) == EXIT_FAILURE) { return EXIT_FAILURE; } //105 => ok
        
        freq_min+=10;
        if (checkOrderFreqs_test(list_frequencies, real_order_frequencies, freq_min, nRanges) == EXIT_FAILURE) { return EXIT_FAILURE; } //115 => wrong
        
        freq_min-=5; 
        if (checkOrderFreqs_test(list_frequencies, real_order_frequencies, freq_min, nRanges) == EXIT_FAILURE) { return EXIT_FAILURE; } //110 => wrong
        
        freq_min+=10;
        if (checkOrderFreqs_test(list_frequencies, real_order_frequencies, freq_min, nRanges) == EXIT_FAILURE) { return EXIT_FAILURE; } //120 => ok

        freq_min+=5;
        if (checkOrderFreqs_test(list_frequencies, real_order_frequencies, freq_min, nRanges) == EXIT_FAILURE) { return EXIT_FAILURE; } //125 => ok        

        freq_min+=10;
        if (checkOrderFreqs_test(list_frequencies, real_order_frequencies, freq_min, nRanges) == EXIT_FAILURE) { return EXIT_FAILURE; } //135 => wrong

        freq_min-=5;
        if (checkOrderFreqs_test(list_frequencies, real_order_frequencies, freq_min, nRanges) == EXIT_FAILURE) { return EXIT_FAILURE; } //130 => wrong

        freq_min+=10;
        if (checkOrderFreqs_test(list_frequencies, real_order_frequencies, freq_min, nRanges) == EXIT_FAILURE) { return EXIT_FAILURE; } //140 => ok

        freq_min+=5;
        if (checkOrderFreqs_test(list_frequencies, real_order_frequencies, freq_min, nRanges) == EXIT_FAILURE) { return EXIT_FAILURE; } //145 => ok

        freq_min+=10;
        if (checkOrderFreqs_test(list_frequencies, real_order_frequencies, freq_min, nRanges) == EXIT_FAILURE) { return EXIT_FAILURE; } //155 => wrong

        freq_min-=5;
        if (checkOrderFreqs_test(list_frequencies, real_order_frequencies, freq_min, nRanges) == EXIT_FAILURE) { return EXIT_FAILURE; } //150 => wrong

        freq_min+=10;
        if (checkOrderFreqs_test(list_frequencies, real_order_frequencies, freq_min, nRanges) == EXIT_FAILURE) { return EXIT_FAILURE; } //160 => ok

        freq_min+=5;
        if (checkOrderFreqs_test(list_frequencies, real_order_frequencies, freq_min, nRanges) == EXIT_FAILURE) { return EXIT_FAILURE; } //165 => ok

        freq_min+=10;
        if (checkOrderFreqs_test(list_frequencies, real_order_frequencies, freq_min, nRanges) == EXIT_FAILURE) { return EXIT_FAILURE; } //175 => wrong

        freq_min-=5;
        if (checkOrderFreqs_test(list_frequencies, real_order_frequencies, freq_min, nRanges) == EXIT_FAILURE) { return EXIT_FAILURE; } //170 => wrong

        freq_min+=10;
        if (checkOrderFreqs_test(list_frequencies, real_order_frequencies, freq_min, nRanges) == EXIT_FAILURE) { return EXIT_FAILURE; } //180 => ok

        freq_min+=5;
        if (checkOrderFreqs_test(list_frequencies, real_order_frequencies, freq_min, nRanges) == EXIT_FAILURE) { return EXIT_FAILURE; } //185 => ok

        freq_min+=10;
        if (checkOrderFreqs_test(list_frequencies, real_order_frequencies, freq_min, nRanges) == EXIT_FAILURE) { return EXIT_FAILURE; } //195 => wrong

        freq_min-=5;
        if (checkOrderFreqs_test(list_frequencies, real_order_frequencies, freq_min, nRanges) == EXIT_FAILURE) { return EXIT_FAILURE; } //190 => wrong

        freq_min+=10;
        if (checkOrderFreqs_test(list_frequencies, real_order_frequencies, freq_min, nRanges) == EXIT_FAILURE) { return EXIT_FAILURE; } //200 => ok

        freq_min+=5;
        if (checkOrderFreqs_test(list_frequencies, real_order_frequencies, freq_min, nRanges) == EXIT_FAILURE) { return EXIT_FAILURE; } //205 => ok

        freq_min+=10;
        if (checkOrderFreqs_test(list_frequencies, real_order_frequencies, freq_min, nRanges) == EXIT_FAILURE) { return EXIT_FAILURE; } //215 => wrong

        freq_min-=5;
        if (checkOrderFreqs_test(list_frequencies, real_order_frequencies, freq_min, nRanges) == EXIT_FAILURE) { return EXIT_FAILURE; } //210 => wrong

        freq_min+=10;
        if (checkOrderFreqs_test(list_frequencies, real_order_frequencies, freq_min, nRanges) == EXIT_FAILURE) { return EXIT_FAILURE; } //220 => ok

        freq_min+=5;
        if (checkOrderFreqs_test(list_frequencies, real_order_frequencies, freq_min, nRanges) == EXIT_FAILURE) { return EXIT_FAILURE; } //225 => ok
        
        freq_min+=10;
        if (checkOrderFreqs_test(list_frequencies, real_order_frequencies, freq_min, nRanges) == EXIT_FAILURE) { return EXIT_FAILURE; } //235 => wrong

        freq_min-=5;
        if (checkOrderFreqs_test(list_frequencies, real_order_frequencies, freq_min, nRanges) == EXIT_FAILURE) { return EXIT_FAILURE; } //230 => wrong

        freq_min+=10;
        if (checkOrderFreqs_test(list_frequencies, real_order_frequencies, freq_min, nRanges) == EXIT_FAILURE) { return EXIT_FAILURE; } //240 => ok

        freq_min+=5;
        if (checkOrderFreqs_test(list_frequencies, real_order_frequencies, freq_min, nRanges) == EXIT_FAILURE) { return EXIT_FAILURE; } //245 => ok

        printOrder(real_order_frequencies);

        samples = (float*)calloc(nElements, sizeof(float));
        samplesOrdered = (float*)calloc(nElements,sizeof(float)); // At the beginning will be a copy of samples

        freq_min = 45;

        for(i = 0; i< nElements; i++)
        {
            samples[i] = -i + 0.1;
            samplesOrdered[i] = samples[i];
        }
        

        /*printf("Flags before ordering:\n");
        for (i = 0; i< nRanges; i++)
        {
            printf("Flag[%d]: %d\n", i, flagsOrder[i]);
        }

        printf("\n Values before ordering:\n");
        for (i = 0; i< 1000; i++) // 5 iterations
        {
            if (i % 5 == 0 || i == 0)
            {
                printf("\n");
            }

            printf("sample[%d]: %lf\t", i, samples[i]);
        }

        printf("\n\n");
*/
        for (i = 0; i < nRanges; i++)
        {
            if ((i+1) != real_order_frequencies[i] && flagsOrder[i] == 0) //i+1 is the position that should have
            {
              //  printf("Frequency %d MHz is located at wrong position\n", freq_min + i * 5);
                if (reorganizeSamples_test(i+1, real_order_frequencies[i], nRanges, samples, samplesOrdered, nElements, valuesPerFreq, nChannels) == EXIT_FAILURE) { return EXIT_FAILURE; }
            }

            else
            {
                flagsOrder[i] = 1;
            }

        }

       
        /*printf("\nFlags after ordering:\n");
        for (i = 0; i< nRanges; i++)
        {
            printf("Flag[%d]: %d\n", i, flagsOrder[i]);
        }

        printf("\n Values after ordering:\n");
        for (i = 0; i< 1000; i++) // 5 iterations
        {
            if (i % 5 == 0 || i == 0)
            {
                printf("\n");
            }
            if (i %200 == 0)
            {
                printf("Iteration %d\n", i/200);
            }
            printf("sampleOrdered[%d]: %lf\t", i, samplesOrdered[i]);
        }

        printf("\n\n");*/

        free(list_frequencies);
        free(samplesOrdered);
        free(samples);
        free(flagsOrder);
    }

    else if (testData == 3)
    {
        anotherTimingTest();       
    }
    
    else if (testData == 4)
    {
        generateDinamicNameTest();
    }
    
    else if (testData == 5)
    {
        startExecutionTest();
    }
    
    else if (testData == 6)
    {
        time_t t_timeStartConfig, t_timeEndConfig;
        struct tm tm_timeStartConfig, tm_timeEndConfig; // Struct of time values at beginning and end of the configuration
	    struct timeval timeValStartConfig, timeValEndConfig; // Use to get duration of the configuration
	    char timeStartConfig[70], timeEndConfig[70]; // Time as date and hours

       /* gettimeofday(&timeValStartConfig, NULL);
	    t_timeStartConfig = time(NULL);
	    localtime_r(&t_timeStartConfig, &tm_timeStartConfig);
	    strftime(timeStartConfig, sizeof timeStartConfig, "%Y-%m-%d %H:%M:%S", &tm_timeStartConfig);
*/
       // printf("Time in the function: %s\n", timeStartConfig);
        calculateTimes(&t_timeStartConfig, &tm_timeStartConfig, &timeValStartConfig);
        strftime(timeStartConfig, sizeof timeStartConfig, "%Y-%m-%d %H:%M:%S", &tm_timeStartConfig);
        printf("Time out of the function: %s\t value time sec: %ld\n", timeStartConfig, timeValStartConfig.tv_sec);

    }
    
    else if (testData == 7)
    {
        naxes[0] = 3600;
        naxes[1] = 200;
        nElements = naxes[0]*naxes[1];
        generateFitsFile_withBinTable_test();
    }
    return 0;
}