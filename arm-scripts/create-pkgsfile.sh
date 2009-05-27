#!/bin/sh

# This will create a tar file that contains all the armel packages already extracted.

# Cleanup
if [ -e pkgs-output.txt ]; then
	rm pkgs-output.txt
fi

# Cleanup
if [ -e pkgs.tar.bz2 ]; then
	rm pkgs.tar.bz2
fi

# Extract all deb packages into a temporary directory.
# Keep the output log, incase something went wrong.
cd pkgs ; mkdir tmp

echo "Collecting all package data... please wait"
for i in `ls *.deb`
do
	dpkg-deb -x $i tmp/ >> ../pkgs-output.txt 2>&1
done
echo "Please check pkgs-output.txt for any errors that may have been encountered!"

echo "Creating tar file... please wait"
cd tmp
tar cjf ../../armel-pkgs.tar.bz2 ./
cd ../
rm -r tmp
echo "Done! Output: armel-pkgs.tar.bz2"
