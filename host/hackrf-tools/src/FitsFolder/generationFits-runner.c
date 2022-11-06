#include "fitsio.h"
#include "time.h"
#include <string.h>
#include "generatationFits.h"

/**
 * Global vars
 */
fitsfile *fptr =NULL;
int exist = 0;
int status = 0, ii, jj;
long fpixel = 1, naxis = 2, nElements, exposure;
long naxes[2] = {160,100}; //200 filas(eje y) 400 columnas(eje x)
short array_img[100][160];
char filename[];

int create()
{   
    /*Checks if the new file exist*/
    fits_file_exists(filename, &exist, &status);
    if(exist == 1)
    {
        
        printf("File already exists. Overwritting fits file.\n");
        if(fits_open_file(&fptr, filename, READWRITE, &status))
        {
            perror("Was not possible to open the file");
            return -1;
        }
        
        if(fits_delete_file(fptr,&status))
        {
            perror("Was not possible to delete the file");
            return -1;
        }
        if(fits_create_file(&fptr, filename, &status))
        {
            perror("Was not possible to create the file");
            return -1;
        }
       /*create the primary array image (16-bit short integer pixels*/
        if(fits_create_img(fptr, SHORT_IMG, naxis, naxes, &status))
        {
            perror("Was not possible to create the image");
            return -1;
        }
        printf("File overwrriten: %s\n", filename);
        return 0;
    }
    else //if not exist
    {
        if(fits_create_file(&fptr, filename, &status))
        {
            perror("Was not possible to create the file");
            return -1;
        }
    
        /*create the primary array image (16-bit short integer pixels*/
        if(fits_create_img(fptr, SHORT_IMG, naxis, naxes, &status))
        {
            perror("Was not possible to create the image");
            return -1;
        }
        printf("File created: %s\n", filename);
        return 0;
    }
}

void updateHeadersFitsFile()
{
    /*Write a keyword; must pass the ADDRESS of the value*/
    long exposure = 1500.;
    time_t now = time(0);
    char *time_str = ctime(&now);
    time_str[strlen(time_str)-1] = '\0';
    long stepX = 1, stepY = 20;
    fits_update_key(fptr,TSTRING,"DATE", time_str,"Time of observation" ,&status); //Date observation
    fits_update_key(fptr,TSTRING, "OBJECT", "Space", "Object name", &status ); //Object Description
    fits_update_key(fptr,TSTRING, "ORIGIN", "TFM Alejandro", "Property", &status ); //Organization name
    fits_update_key(fptr,TSTRING, "TELESCOP","Radio", "Type of intrument", &status); //Instrument type
    fits_update_key(fptr,TSTRING, "INSTRUME","HackRF One", "Name of the instrument", &status); //Instrument type
    fits_update_key(fptr,TSTRING, "CTYPE2", "Amp(dB)", "Power (dB)", &status ); //Title axis 1
    fits_update_key(fptr,TSTRING, "CTYPE1", "Frequency[MHz]", "MHz", &status ); //Title axis 2
    fits_update_key(fptr,TLONG, "CDELT1", &stepX, "Steps Frequency", &status); //Steps axis X
    fits_update_key(fptr,TLONG, "CDELT2", &stepY, "Steps samples", &status); //Steps axis Y

    fits_update_key(fptr,TLONG,"EXPOSURE", &exposure, "Total Exposure Time", &status);
}

int insertData()
{
/*Initialize the values in the image with a linear ramp function*/
    for(jj=0; jj<naxes[1]; jj++)
    {
        for(ii =0; ii < naxes[0]; ii++)
            {
                array_img[jj][ii] =  jj + ii;
            }
            
            
    }
           
    nElements = naxes[0] * naxes[1];

    /*Write the array of integers of the image*/
    if(fits_write_img(fptr, TSHORT, fpixel, nElements, array_img[0], &status))
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

int generateFitsFile(char fileFitsName[])
{
    strcpy(filename, fileFitsName);
    
   //Creation
   if (create() == -1)
   {
    return -1;
   }

   //Update
   updateHeadersFitsFile();
   
   //Insert info
   if(insertData() == -1)
   {
    return -1;
   }
   
   //CloseFile
   closeFits();
   return 0;
}

void something(){
    printf("hola");
}

int main(){
    char file[] = "sample.fits";
    generateFitsFile(file);
    return 0;
}