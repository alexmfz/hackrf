echo "...Downloading dependencies..."
sudo apt-get --yes update
sudo apt-get --yes install libfftw3-dev
sudo apt-get --yes install libusb-1.0
sudo apt-get --yes install cmake
sudo apt-get --yes install make
sudo apt-get --yes install gcc
sudo apt-get --yes install hackrf
sudo apt --yes install zlib1g
sudo apt --yes install zlib1g-dev
sudo apt-get --yes install default-jdk
sudo apt-get --yes install default-jre
sudo apt --yes autoremove

wget -O cfitsio-3.47.tar.gz https://heasarc.gsfc.nasa.gov/FTP/software/fitsio/c/cfitsio-3.47.tar.gz

tar xvf cfitsio-3.47.tar.gz
rm cfitsio-3.47.tar.gz
cd cfitsio-3.47

echo "...Installing dependencies..."
sudo ./configure --prefix=/usr/local
sudo make
sudo make install
sudo make clean
sudo ldconfig
sudo pip3 install astropy

base=$(pwd)
sudo cp $base/host/build/libhackrf/src/libhackrf.so.0 /lib/arm-linux/gnueabihf/libhackrf.so.0

echo "...Dependencies successfully installed..."
