
#Remove previous files .c.o, .o
cd /home/manolo/hackrf/host/build/hackrf-tools/src
rm hackrf_sweep

cd /home/manolo/hackrf/host/hackrf-tools/src/FitsFolder
rm hackrf_sweep
echo "Executable hackrf_sweep successfully removed"

cd /home/manolo/hackrf/host/build/hackrf-tools/src/CMakeFiles/hackrf_sweep.dir
rm generationFits.c.o 
rm functions.c.o
rm timer.c.o
echo "Files object successfully removed"

###Compile###
#Fits functions
cd /home/manolo/hackrf/host/hackrf-tools/src/FitsFolder/
gcc -c generationFits.c -o generationFits.c.o -I usr/local/cfitsio-4.1.0 -lcfitsio
cp -R generationFits.c.o /home/manolo/hackrf/host/build/hackrf-tools/src/CMakeFiles/hackrf_sweep.dir
echo "Fits functions successfully compiled"

#External API functions
cd /home/manolo/hackrf/host/hackrf-tools/src/Functions/
gcc -c functions.c -o functions.c.o
cp -R functions.c.o /home/manolo/hackrf/host/build/hackrf-tools/src/CMakeFiles/hackrf_sweep.dir
echo "Extern API functions successfully compiled"

#Timer functions
cd /home/manolo/hackrf/host/hackrf-tools/src/Timer/
gcc -c timer.c -o timer.c.o
cp -R timer.c.o /home/manolo/hackrf/host/build/hackrf-tools/src/CMakeFiles/hackrf_sweep.dir
echo "Extern timers functions successfully compiled"
#Main folder
cd /home/manolo/hackrf/host/build/hackrf-tools/src/

make
echo "All files were successfully compiled"

cp -R /home/manolo/hackrf/host/build/hackrf-tools/src/hackrf_sweep /home/manolo/hackrf/host/hackrf-tools/src/FitsFolder/

echo "Ready to exec"

