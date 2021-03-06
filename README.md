This project contains a basic SDR Receiver which feeds JAERO with audio from an R820T2 RTL2832U device like the RTL SDR V3. Rather than feeding JAERO with audio via audio cables, this implementation makes use of ZeroMQ as the interface between the SDR and JAERO. This provides a more stable data flow which is significanty less prone to CRC errors in JAERO. 

In addition, the aim of the project is to run at low CPU usage without any bells or whistles. The GUI is very simple and has only basic features to select the RTL device, start the SDR as well as enable or disable the device bias tee. The FFT is purposely slow to reduce CPU usage. You can select individual VFO's in the dropdown menu which should display the approximate spectrum shown in JAERO.

All settings are made in an ini file. A few examples ini files are included in the project. Further details on the options in the ini file are to follow but be very careful when editing the file, any mistakes may cause the program not to run correctly. 

Note that this SDR requires a JAERO instance of version 1.0.4.13 or higher which has the ZeroMQ audio source options included.

![image](https://user-images.githubusercontent.com/31091871/126459963-0726ea9d-3d03-40b8-ae90-45676c3c21b1.png)

Inmarsat AERO channels are more or less grouped in clusters of lower speed channnels (600/1200) and higher speed channels (10500/8400). The SDR takes advantage of this by grouping these in a "main" VFO which allows the whole group to be mixed down and decimated from the original sample rate down to a much lower rate that still covers all of the channels inside the group. Each channel / sub VFO is then mixed / decimated down to the required frequency and sample rate as well as USB demodulated. The resulting data is transmitted to JAERO via a ZeroMQ PUB/SUB pattern (https://zguide.zeromq.org/). This means that it is possible to connect several JAERO instances to the same SDR VFO output. It also allows you to run the SDR on a different device to where JAERO is running, provided there is network connectivity between the two. The SDR is the ZMQ Publisher and binds to the address specficied in the ini file:

zmq_address=tcp://*:6003  

Each VFO defined in the ini file should have it own Topic name as shown in the below ini file excerpt

![image](https://user-images.githubusercontent.com/31091871/126470644-0c8b4030-8096-4c58-80e9-549bec89e0db.png)

This topic name would then be matched by the settings in JAERO:

![image](https://user-images.githubusercontent.com/31091871/126470450-cf25d78e-f123-4878-8ab8-16693719cc22.png)

The easiest way to start the SDR is by creating a bat or shell script to pass the ini file name argument, i.e. on windows:

start SDRReceiver.exe -s ini/SDR_25E.ini

In order to make this work I have re-used some of Jonti's excellent code from his JDSCA project for handling the RTL callback and data handling. I also used some gnuradio project code for FIR coeffecient calculations. 

The main sample rate for the device is set as follows:

sample_rate=1536000

However this should probably not be changed. Except for the older I3 satellite at 54W all channels fit nicely with a 1.536.000 sample rate and it can be decimated down to 48Khz entirely with half band FIR filters. In order to cover the data channels and voice channels around 1546.8 for 54W it is also possible to run at 1.920.000 but obviously this takes a little more CPU and has no obvious benefit for the I4 satellites. A more narrow 288.000 sample rate is also available which might be useful on slower devices or if you are only interested in particular channels. 

The center frequency is chosen so that both the lower speed data channels and higher speed channels as well voice channels are covered:

center_frequency=1545600000

Other ini file keys:
set the tuner gain, 496 is the highest for R820T2 devices

tuner_gain=496

remove the annoying spike in the center of the spectrum
correct_dc_bias=1

when changing dongles there may be a slight freqency difference. Use this to tune ALL VFO's up or down by a number of Hz. Use positive values to tune higher, negative values to tune lower

mix_offset=0

You should be able to connect to remote RTL that is running via rtl_tcp, it will show in the device drop down when enabled

remote_rtl=127.0.0.1:1234

Also the tuner gain by index position can be selected with this key where a value of 28 is the highest gain for an RTL SDR V3

remote_rtl_gain_idx=28

The main VFO's are shown in the image above. There is typically no need to change these unless perhaps while setting up a new C Band ini file. The SDR should work for C Band as well but the FFT is quite slow so it is probabaly a good idea to determine the exact frequencies to use via other means.

The SDR can be autostarted and with the following ini keys:

auto_start=1

auto_start_tuner_idx=0

The latter key being the zero based index of the RTL device to be started as shown in the device dropdown

On some VFO you may like to low pass the audio prior to sending to jaero, i.e. for the audio channels a 10Khz audio filter could be useful to limit adjecent channel activity. Use the filter_bandwidth ini setting on the vfo to set this. A value of 0 means no filter, a value of 10000 would set a 10Khz hamming windowed low pass filter.

You may also need to adjust the gain slightly for your setup. Increase or decrease the VFO gain value until you get a green volume light in jaero 

The decoding results in jaero should be similar to what one would get from other SDR's but it becomes possible to run jaero almost free of CRC errors even at high CPU loads.


![image](https://user-images.githubusercontent.com/31091871/126675321-a9fb3c35-ec5b-4b5b-972d-512ac160d65f.png)

**Updated 17th October 2021**

Added two new ini keys to auto start based on device serial:

auto_start=1

auto_start_tuner_serial=00000001

#auto_start_tuner_idx = 

Note that the serial option will be checked first before applying the index based key if present (i.e. if you leave both enabled). if you have multiple devices with the same serial it will take the first one it finds in the list of devices. The serial number now also shows in the device list drop down.

You can now also auto start the bias t by enabling the following key/setting

auto_start_biast=1

Also added a quick radio button to disable to the main FFT. I doubt it saves any significant cpu usage but certainly some. Hopefully I will have time to do a little bit more work on this to also make C Band a little easier.


