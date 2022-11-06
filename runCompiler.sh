
#Remove previous files .c.o, .o
cd /home/manolo/hackrf/host/build/hackrf-tools/src
rm hackrf_sweep

cd /home/manolo/hackrf/host/build/hackrf-tools/src/CMakeFiles/hackrf_sweep.dir
rm generationFits.c.o 
rm functions.c.o

echo "Files succesfully removed"

#Compile
cd /home/manolo/hackrf/host/hackrf-tools/src/FitsFolder/
gcc -c generationFits.c -o generationFits.c.o -I usr/local/cfitsio-4.1.0 -lcfitsio
cp -R generationFits.c.o /home/manolo/hackrf/host/build/hackrf-tools/src/CMakeFiles/hackrf_sweep.dir
echo "Fits functions sucessfully compiled"

cd /home/manolo/hackrf/host/hackrf-tools/src/Functions/
gcc -c functions.c -o functions.c.o
cp -R functions.c.o /home/manolo/hackrf/host/build/hackrf-tools/src/CMakeFiles/hackrf_sweep.dir
echo "Extern functions sucessfully compiled"

cd /home/manolo/hackrf/host/build/hackrf-tools/src/

make
echo "All files were succesfully compiled"

cp -R /home/manolo/hackrf/host/build/hackrf-tools/src/hackrf_sweep /home/manolo/hackrf/host/hackrf-tools/src/FitsFolder/

echo "Ready to exec"

