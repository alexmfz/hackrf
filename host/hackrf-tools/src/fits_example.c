#include "fitsio.h"
#include "time.h"
#include <string.h>
void  updateHeadersFitsFile(fitsfile *fptr, int status)
{
    /*Write a keyword; must pass the ADDRESS of the value*/
    long exposure = 1500.;
    time_t now = time(0);
    char *time_str = ctime(&now);
    time_str[strlen(time_str)-1] = '\0';
  
    fits_update_key(fptr,TSTRING,"DATE", time_str,"Time of observation" ,&status); //Date observation
    fits_update_key(fptr,TSTRING, "OBJECT", "Space", "Object name", &status ); //Object Description
    fits_update_key(fptr,TSTRING, "ORIGIN", "TFM Alejandro", "Property", &status ); //Organization name
    fits_update_key(fptr,TSTRING, "TELESCOP","Radio", "Type of intrument", &status); //Instrument type
    fits_update_key(fptr,TSTRING, "INSTRUME","HackRF One", "Name of the instrument", &status); //Instrument type
    fits_update_key(fptr,TSTRING, "CTYPE2", "Amp(dB)", "Power (dB)", &status ); //Title axis 1
    fits_update_key(fptr,TSTRING, "CTYPE1", "Frequency[MHz]", "MHz", &status ); //Title axis 2
    fits_update_key(fptr,TLONG,"EXPOSURE", &exposure, "Total Exposure Time", &status);
}

int main(int argc, char const *argv[])
{
    fitsfile *fptr;
    int exist = 0;
    int status = 0, ii, jj;
    long fpixel = 1, naxis = 2, nElements, exposure;
    long naxes[2] = {400,200};
    short array[200][400];
    char filename[] = "testfile_FinalHeaders.fits";

    /*Create new file*/
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
        fits_create_img(fptr, SHORT_IMG, naxis, naxes, &status);
        printf("File overwrriten: %s\n", filename);
    }
    else
    {
        if(fits_create_file(&fptr, filename, &status))
        {
            perror("Was not possible to create the file");
            return -1;
        }
        /*create the primary array image (16-bit short integer pixels*/
        fits_create_img(fptr, SHORT_IMG, naxis, naxes, &status);
        printf("File created: %s\n", filename);
    }    
    updateHeadersFitsFile(fptr, status);
    
    /*Initialize the values in the image with a linear ramp function*/
    for(jj=0; jj<naxes[1]; jj++)
    {
        for(ii =0; ii < naxes[0]; ii++)
        {
            array[jj][ii] = ii + jj;

        }
    }

    nElements = naxes[0] * naxes[1];

    /*Write the array of integers of the image*/
    fits_write_img(fptr, TSHORT, fpixel, nElements, array[0], &status);
    
    /*Close and report any error*/
    fits_close_file(fptr, &status);
    fits_report_error(stderr, status);

    return status;
}
