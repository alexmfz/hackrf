
echo "...Downloading dependencies..."
wget -O cfitsio.tar.gz http://heasarc.gsfc.nasa.gov/FTP/software/fitsio/c/cfitsio-4.1.0.tar.gz

tar xvf cfitsio.tar.gz
rm cfitsio.tar.gz
cd cfitsio-*

echo "...Installing dependencies..."
sudo ./configure --prefix=/usr/local/
sudo make 
sudo make install
sudo make clean
sudo ldconfig


echo "...Dependencies successfully installed..."
