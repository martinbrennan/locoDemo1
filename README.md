ESP Based Sonos Controller


. ~/espm/esp-idf/export.sh

idf.py set-target esp32s2

idf.py menuconfig

idf.py build

idf.py -p /dev/ttyUSB1 flash

idf.py monitor	# <ctrl>+] to exit

in ~/esp32/esp-idf ./install.sh esp32s3		# then export (above)


To set up the build environment I did this (for 5.1 - I subsequently switched to master (latest))


Follow instructions here
https://docs.espressif.com/projects/esp-idf/en/v5.1/esp32/get-started/linux-macos-setup.html 


in espm/esp-idf
./install.sh esp32s3


Then in each terminal where I want to use esp-idf
. ~/espm/esp-idf/export.sh


Then in espm
git clone https://github.com/martinbrennan/spot.git


Then in espm/sot...
idf.py set target esp32s3


idf.py menuconfig
idf.py build

idf.py -p /dev/ttyUSB1 flash monitor


I needed to use this to get mdns

idf.py add-dependency espressif/mdns

/********************************************************
 3rd September fresh environment build 
*********************************************************/

mkdir espl
cd espl
git clone --recursive https://github.com/espressif/esp-idf.git

in espl/esp-idf
./install.sh esp32s3

Then in each terminal where I want to use esp-idf
. ~/espl/esp-idf/export.sh

Then in espl
-

in espl
git clone --recursive https://github.com/espressif/esp-adf.git

cd esp-adf

./install.sh
. ./export.sh

idf.py set-target esp32s3
idf.py menuconfig.
idf.py build

Error in miniz_zip.c caused by GCC-13
idf-py menuconfig
	/ GCC13 ... disable warnings

idf.py -p /dev/ttyUSB1 flash monitor

E (15313) AUDIO_THREAD: Not found right xTaskCreateRestrictedPinnedToCore.

Need to patch these files

espl/esp-idf/components/freertos/esp_additions/freertos_tasks_c_additions.h
espl/esp-idf/components/freertos/esp_additions/include/freertos/idf_additions.h


[This worked but they have upgraded instructions if necessary in future]

Please enter IDF-PATH with "cd $IDF_PATH" and apply the IDF patch with "git apply $ADF_PATH/idf_patches/idf_v5.4_freertos.patch" first



/********************************************************
 13th September fresh terminal
*********************************************************/

cd espm
./esp-idf/install.sh esp32s3
. ./esp-idf/export.sh
./esp-adf/install.sh
. ./esp-adf/export.sh
export ADF_PATH="/home/martin/espm/esp-adf"
cd spot
idf.py -p /dev/ttyUSB1 flash monitor




