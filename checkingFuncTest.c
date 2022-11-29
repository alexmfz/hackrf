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
float *example;

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
        fprintf(stderr, "generationFits | saveTimes() | Was not possible to save timing data\n");
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
        fprintf(stderr, "generationFits | saveSamples() | Was not possible to save power samples\n");
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

/**TEST FITS**/
fitsfile *fptr =NULL;
int exist = 0;
int status = 0, ii, jj;
int fpixel = 1;
int naxis = 2, nElements, exposure;
long naxes[2];// = {3600,200, 720000}; //200 filas(eje y) 400 columnas(eje x)

float array_img[200][200]; //naxes[0]naxes[y] (axis x ,axis y)
float* samples; // Array of float samples where dbs measures will be saved


long minData(float * samples)
{
    int  i = 0;
    long minimum = (long)samples[0];

    for (i = 0; i< nElements; i++)
    {
        if (minimum > (long)samples[i])
        {
            minimum = (long)samples[i];
        }
    }

    return minimum;
}

long maxData(float * samples)
{
    int  i = 0;
    long maximum = (long)samples[0];

    for (i = 0; i< nElements; i++)
    {
        if (maximum < (long)samples[i])
        {
            maximum = (long)samples[i];
        }
    }

    return maximum;
}

long getSecondsOfDayOfFirstSweeping(struct tm timeFirstSweeping)
{
    long sec_of_day = timeFirstSweeping.tm_hour*3600 + timeFirstSweeping.tm_min*60 + timeFirstSweeping.tm_sec;
    printf("Sec of day: %ld\n", sec_of_day);
    return sec_of_day;
}

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

void updateHeadersFitsFile(char* startDate, char *timeStart, char *endDate, char *timeEnd, uint32_t freq_min, struct tm timeFirstSweeping)
{
    printf("generationFits | updateHeadersFitsFile | Updating fits headers to format\n");

    long bzero = 0., bscale = 1.;
    long dataMax = maxData(samples), dataMin= minData(samples);
    printf("Min Value: %ld\n", dataMin);
    printf("Max Value: %ld\n", dataMax);

    long crVal1 = getSecondsOfDayOfFirstSweeping(timeFirstSweeping), crPix1 = 0, stepX = 1; // crVal1 Start axis x value 0 [sec of a day]
    
    long crVal2 = 45, crPix2 = 0, stepY = 1.; // crVal2 Start axis y value 0

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

    fits_update_key(fptr, TLONG, "CRVAL2", &crVal2, "Value on axis 2 at the reference pixel", &status);  // Value on axis 2 at the reference pixel
    fits_update_key(fptr, TLONG, "CRPIX2", &crPix2, "reference Pixel 2 ", &status); 
    fits_update_key(fptr, TSTRING, "CTYPE2", "Frequency [MHz]", "Title of axis 2", &status ); // Title of axis 2
    fits_update_key(fptr, TLONG, "CDELT2", &stepY, "Steps samples", &status); // Steps axis Y

    printf("generationFits | updateHeadersFitsFile | Execution Sucess\n");
}

int insertData(float* samples)
{
    /*Initialize the values in the image with a linear ramp function*/
    printf("generationFits | insertData() | Inserting data...\n");
    //naxes[1] = 200
    //naxes[0] = 3600
    nElements = naxes[0]*naxes[1];
    
    for(ii= 0; ii< naxes[1]; ii++ )
    {        
        for (jj=0; jj< naxes[0]; jj++)
        {
            array_img[jj][ii] = samples[jj];
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

void closeFits()
{
    /*Close and report any error*/
    fits_close_file(fptr, &status);
    fits_report_error(stderr, status);
    
    printf("generationFits | closeFits() | Execution Sucess\n");
}

int generateFitsFile_test(char fileFitsName[], float*samples, char startDate[], char timeStart[], char endDate[], char timeEnd[], uint32_t freq_min, struct tm timeFirstSweeping)
{   
    printf("generationFits | generateFitsFile() | Start generating fits file\n");
    //save frequency data | Timing data should before calling this function
    //if (saveFrequencies(freq_min, freq_max, step_value) == EXIT_FAILURE) { return EXIT_FAILURE; } 

    //Creation
    if (create(fileFitsName) == EXIT_FAILURE) { return EXIT_FAILURE; }

    //Update
    updateHeadersFitsFile(startDate, timeStart, endDate, timeEnd, 45, timeFirstSweeping);
   
    //Insert info
    if(insertData(samples) == EXIT_FAILURE)  { return EXIT_FAILURE;  }
    

    //CloseFile
    closeFits();

    printf("generationFits | generateFitsFile() | Execution Sucess\n");
    return EXIT_SUCCESS;
}


int main(int argc, char** argv) 
{
    int testData = 0;

    /*fprintf(stderr, "timerNewConceptTest()\n");
    timerNewConceptTest(); // Is in a while(1), discomment to test*/
    if (testData ==1)
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

    else
    {
        fprintf(stderr, "testGenenerationFitsFile\n");
        char pathFits[] = "testGenenerationFitsFile.fits"; // File name of fits file
        int i = 0;
        naxes[0] = 200;
        naxes[1] = 200;
        nElements = naxes[0] * naxes[1];
        
        time_t now = time(NULL);
        struct tm localTimeFirst = *localtime(&now);
	    
        char startDate[70];
	    char timeStart[70];
	
	    char endDate[70];
	    char timeEnd[70];

	    strftime(startDate, sizeof startDate,"%Y-%m-%d", &localTimeFirst);
	    strftime(timeStart, sizeof timeStart, "%Y-%m-%d %H:%M:%S", &localTimeFirst);

        samples = (float*)calloc(nElements,sizeof(float));
        for(i = 0; i< nElements; i++)
        {
            samples[i] = -i + 0.1;
        }

        time_t after = time(NULL);
        struct tm localTimeLast = *localtime(&now);
        localTimeLast = *localtime(&after);
        strftime(endDate, sizeof endDate,"%Y-%m-%d", &localTimeLast);
	    strftime(timeEnd, sizeof timeEnd, "%Y-%m-%d %H:%M:%S", &localTimeLast);
        generateFitsFile_test(pathFits,samples, startDate, timeStart, endDate, timeEnd, 45, localTimeFirst);
    }
    return 0;
}