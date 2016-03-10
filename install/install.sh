#!/bin/bash
# Make sure only root can run our script
if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 1>&2
   echo "sudo sh install.sh" 1>&2	
   exit 1
fi


echo "Hi, this will install the SALSA python3 scripts on you system"
echo "it is a three step process."
echo "1) Install all required packages"
echo "2) Download and install the quickwire i2c python3 lib"
echo "3) Download the SALSA scripts"
echo "4) Optionally run a demo script"
echo "Hit ENTER to start"
read input_variable


locationOfScript=$(dirname "$(readlink -e "$0")")


echo "==================================="
echo "======= STEP 1 ===================="
echo "==================================="

apt-get update
apt-get install python3-dev git python3-setuptools -y

echo "==================================="
echo "======= STEP 2 ===================="
echo "==================================="

cd /tmp/
git clone https://github.com/quick2wire/quick2wire-python-api.git
cd quick2wire-python-api
python3 setup.py install

echo "==================================="
echo "======= STEP 3 ===================="
echo "==================================="

cd $locationOfScript
git clone https://github.com/KoljaWindeler/CHIP_SALSA.git
cd $locationOfScript'/CHIP_SALSA/python/'

echo "==================================="
echo "========= Done ===================="
echo "==================================="
echo " "
echo "Do you want me to run the demo script, demo.py?"

select yn in "Yes" "No"; do
    case $yn in
        Yes ) python3 demo.py; break;;
        No ) exit;;
    esac
done

echo "==================================="
echo "======== Thats it, thanks ========="
echo "==================================="


