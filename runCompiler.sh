
originalPath=$(pwd)

#Remove previous files .c.o, .o
cd $originalPath/host/build/hackrf-tools/src/
rm hackrf_sweep

cd CMakeFiles/hackrf_sweep.dir
rm generationFits.c.o 
rm functions.c.o
rm timer.c.o
echo "Files object successfully removed"


cd $originalPath/host/hackrf-tools/src/FitsFolder/

rm hackrf_sweep

echo "Executable hackrf_sweep successfully removed"

###Compile###
#Fits functions
gcc -c generationFits.c -o generationFits.c.o -I usr/local/cfitsio-4.1.0 -L. -lcfitsio -lm -lz
cp -R generationFits.c.o $originalPath/host/build/hackrf-tools/src/CMakeFiles/hackrf_sweep.dir
echo "Fits functions successfully compiled"

#External API functions
cd $originalPath/host/hackrf-tools/src/Functions/
gcc -c functions.c -o functions.c.o
cp -R functions.c.o $originalPath/host/build/hackrf-tools/src/CMakeFiles/hackrf_sweep.dir
echo "Extern API functions successfully compiled"

#Timer functions
cd $originalPath/host/hackrf-tools/src/Timer/
gcc -c timer.c -o timer.c.o
cp -R timer.c.o $originalPath/host/build/hackrf-tools/src/CMakeFiles/hackrf_sweep.dir
echo "Extern timers functions successfully compiled"

#Main folder
cd $originalPath/host/build/hackrf-tools/src/

make
echo "All files were successfully compiled"

cp -R $originalPath/host/build/hackrf-tools/src/hackrf_sweep $originalPath/host/hackrf-tools/src/FitsFolder/
cp -R $originalPath/host/build/hackrf-tools/src/hackrf_info $originalPath/host/hackrf-tools/src/FitsFolder/

echo "Ready to exec"

