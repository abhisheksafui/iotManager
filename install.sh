#!/bin/bash

#install dependencies first
#sudo apt-get update


sudo dpkg -L cmake > /dev/null
if [ $? -eq 1 ]
then
  echo "Cmake not installed.. installing"
  sudo apt-get install cmake
  [ $? -eq 0 ] || (echo "Error installing cmake" && exit 1) 
  echo "Installed cmake"
else
  echo "Cmake already installed.."
fi

tar -xvf safuiIotManager.tar 
[ $? -eq 0 ] || (echo "Error in extracting safuiIotManager.tar" && exit 1)

echo "Extracted package.."
cd safuiIotManager
[ -d build ] && rm -rf build && echo "Deleted build dir"
mkdir build 
cd build
cmake ..
make && sudo make install
sudo update-rc.d iotgateway defaults
sudo update-rc.d iotgateway enable
sudo update-rc.d iotscheduler defaults
sudo update-rc.d iotscheduler enable

sudo /etc/init.d/iotgateway start
sudo /etc/init.d/iotscheduler start
