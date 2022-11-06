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

/**
 * Global vars
 */
fitsfile *fptr =NULL;
int exist = 0;
int status = 0, ii, jj;
int fpixel = 1;
int naxis = 3, nElements, exposure;
long naxes[2];// = {3600,200, 720000}; //200 filas(eje y) 400 columnas(eje x)
float array_img[3600][200]; //naxes[0]naxes[y] (axis x ,axis y)

int create(char fileFitsName[])
{   

    /*Checks if the new file exist*/
    fits_file_exists(fileFitsName, &exist, &status);
    if(exist == 1)
    {
        
        printf("create | File already exists. Overwritting fits file.\n");
        if(fits_open_file(&fptr, fileFitsName, READWRITE, &status))
        {
            perror("create | Was not possible to open the file");
            return -1;
        }
        
        if(fits_delete_file(fptr,&status))
        {
            perror("create | Was not possible to delete the file");
            return -1;
        }
        if(fits_create_file(&fptr, fileFitsName, &status))
        {
            perror("create | Was not possible to create the file");
            return -1;
        }
        
       /*create the primary array image (16-bit short integer pixels*/
        if(fits_create_img(fptr, FLOAT_IMG, naxis, naxes, &status))
        {
            perror("create | Was not possible to create the image");
            return -1;
        }
        printf("create | File overwrriten: %s\n", fileFitsName);
        return 0;
    }
    else //if not exist
    {
        if(fits_create_file(&fptr, fileFitsName, &status))
        {
            perror("create | Was not possible to create the file");
            return -1;
        }
        

        /*create the primary array image (16-bit short integer pixels*/
        if(fits_create_img(fptr, FLOAT_IMG, naxis, naxes, &status))
        {
            perror("create | Was not possible to create the image");
            return -1;
        }
        printf("create | File created: %s\n", fileFitsName);
        return 0;
    }
}

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
    printf("updateHeadersFitsFile | Header fits file updated\n");
}

int insertData(float* samples)
{
    int nRounds = 0;
/*Initialize the values in the image with a linear ramp function*/
    printf("insertData | Inserting data...\n");
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
        perror("insertData | Was not possible to write data into the image");
        return -1;
    }
    return 0;
}

void closeFits()
{
    /*Close and report any error*/
    fits_close_file(fptr, &status);
    fits_report_error(stderr, status);
    
    printf("closeFits | fits file closed\n");
}

int generateFitsFile(char fileFitsName[], float*samples)
{   
   //Creation
    printf("generateFitsFile | Filename: %s\n",fileFitsName);
    printf("generateFitsFile | Dimensions:[%ld][%ld]\n",naxes[0],naxes[1]);

   if (create(fileFitsName) == -1)
   {
    return -1;
   }

   //Update
   updateHeadersFitsFile();
   
   //Insert info
   printf("generateFitsFile | Inserting data into img\n");
   if(insertData(samples) == -1)
   {
    printf("generateFitsFile | There was an error\n");
    return -1;
   }
   printf("generateFitsFile | Finished insertion\n");

   //CloseFile
   closeFits();
   return 0;
}
