
echo "...Downloading dependencies..."
wget -O cfitsio_latest.tar.gz http://heasarc.gsfc.nasa.gov/FTP/software/fitsio/c/cfitsio_latest.tar.gz

tar xvf cfitsio_latest.tar.gz
rm cfitsio_latest.tar.gz
cd cfitsio-*

echo "...Installing dependencies..."
./configure
make 
make install

./configure --prefix=/usr1/local

echo "...Dependencies successfully installed..."
