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
int naxis = 2, nElements, exposure;
long naxes[2];// = {160,100}; //200 filas(eje y) 400 columnas(eje x)
uint8_t array_img[3600][200]; //naxes[0]naxes[y] (axis x ,axis y)

/**Extern var**/
extern uint32_t rounds;

int create(char fileFitsName[])
{   

    /*Checks if the new file exist*/
    fits_file_exists(fileFitsName, &exist, &status);
    if(exist == 1)
    {
        
        printf("File already exists. Overwritting fits file.\n");
        if(fits_open_file(&fptr, fileFitsName, READWRITE, &status))
        {
            perror("Was not possible to open the file");
            return -1;
        }
        
        if(fits_delete_file(fptr,&status))
        {
            perror("Was not possible to delete the file");
            return -1;
        }
        if(fits_create_file(&fptr, fileFitsName, &status))
        {
            perror("Was not possible to create the file");
            return -1;
        }
        
       /*create the primary array image (16-bit short integer pixels*/
        if(fits_create_img(fptr, BYTE_IMG, naxis, naxes, &status))
        {
            perror("Was not possible to create the image");
            return -1;
        }
        printf("File overwrriten: %s\n", fileFitsName);
        return 0;
    }
    else //if not exist
    {
        if(fits_create_file(&fptr, fileFitsName, &status))
        {
            perror("Was not possible to create the file");
            return -1;
        }
        

        /*create the primary array image (16-bit short integer pixels*/
        if(fits_create_img(fptr, BYTE_IMG, naxis, naxes, &status))
        {
            perror("Was not possible to create the image");
            return -1;
        }
        printf("File created: %s\n", fileFitsName);
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
}

int insertData(uint8_t* samples)
{
    int nRounds = 0;
/*Initialize the values in the image with a linear ramp function*/
    printf("Inserting data...\n");
    //naxes[1] = 200
    //naxes[0] = 3600
    nElements = naxes[0]*naxes[1];
    while(nRounds < rounds)
    {
        for(ii= 0; ii< naxes[1]; ii++ )
        {        
            for (jj=nRounds*naxes[0]/rounds; jj< (1+nRounds)*naxes[0]/rounds; jj++)
            {
                //There are 4 rounds, so 4 values are needed to be set at iteration
                array_img[jj][ii] = (uint8_t)(-samples[jj]);
            }
            
        }    
        nRounds++;
    }

/*    for(ii= 0; ii< naxes[1]; ii++ )
    {        
        for (jj=0; jj< naxes[0]; jj++)
        {
            array_img[ii][jj] = samples[];
        }
    }*/
    /*Write the array of integers of the image*/
    if(fits_write_img(fptr, TBYTE, fpixel, nElements, array_img[0], &status))
    {
        perror("Was not possible to write data into the image");
        return -1;
    }
    return 0;
}

void closeFits()
{
    /*Close and report any error*/
    fits_close_file(fptr, &status);
    fits_report_error(stderr, status);
}

int generateFitsFile(char fileFitsName[], uint8_t*samples)
{   
   //Creation
    printf("Filename: %s\n",fileFitsName);
    printf("Dimensions:[%ld][%ld]\n",naxes[0],naxes[1]);

   if (create(fileFitsName) == -1)
   {
    return -1;
   }

   //Update
   updateHeadersFitsFile();
   
   //Insert info
   printf("Inserting data into img\n");
   if(insertData(samples) == -1)
   {
    printf("There was an error\n");
    return -1;
   }
   printf("Finished insertion\n");

   //CloseFile
   closeFits();
   return 0;
}
