echo "...Downloading dependencies..."

sudo apt-get install libfftw3-dev
sudo apt-get install libusb-1.0
sudo apt-get install cmake
sudo apt-get install make
sudo apt-get install gcc
sudo apt-get install hackrf
sudo apt install zlib1g
sudo apt install zlib1g-dev

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


echo "...Dependencies successfully installed..."
