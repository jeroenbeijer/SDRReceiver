This project contains a basic SDR Receiver which feeds JAERO with audio from an R820T2 RTL2832U device like the RTL SDR V3. Rather than feeding JAERO with audio via audio cables, this implementation makes use of ZeroMQ as the interface between the SDR and JAERO. This provides a more stable data flow which is significanty less prone to CRC errors in JAERO. 

In addition, the aim of the project is to run at low CPU usage without any bells or whistles. The GUI is very simple and has only basic features to select the RTL device, start the SDR as well as enable or disable the device bias tee. The FFT is purposely slow to reduce CPU usage. You can select individual VFO's in the dropdown menu which should display the approximate spectrum shown in JAERO.

All settings are made in an ini file. A few examples ini files are included in the project. Further details on the options in the ini file are to follow but be very careful when editing the file, any mistakes may cause the program not to run correctly. 

Note that this SDR requires a modified JAERO, see the forked JAERO project. Hopefully this will be merged into Jonti's version and releases.

![image](https://user-images.githubusercontent.com/31091871/126459963-0726ea9d-3d03-40b8-ae90-45676c3c21b1.png)

Inmarsat AERO channels are more or less grouped in clusters of lower speed channnels (600/1200) and higher speed channels (10500/8400). The SDR takes advantage of this by grouping these in a "main" VFO which allows the whole group to be mixed down and decimated from the original sample rate down to a much lower rate that still covers all of the channels inside the group. Each channel / sub VFO is then mixed / decimated down to the required frequency and sample rate as well as USB demodulated. The resulting data is transmitted to JAERO via a ZeroMQ PUB/SUB pattern (https://zguide.zeromq.org/). This means that it is possible to connect several JAERO instances to the same SDR VFO output. It also allows you to run the SDR on a different device to where JAERO is running, provided there is network connectivity between the two. The SDR is the ZMQ Publisher and binds to the address specficied in the ini file:

zmq_address=tcp://*:6003  

Each VFO defined in the ini file should have it own Topic name as shown in the below ini file excerpt

[vfos]
size=33
1\frequency=1545010000
1\gain=10
1\data_rate=600
1\filter_audio=0

2\frequency=1545029850
2\gain=10
2\data_rate=600
2\filter_audio=0
2\topic=VFO02

This topic name would then be matched by the settings in JAERO:

![image](https://user-images.githubusercontent.com/31091871/126470450-cf25d78e-f123-4878-8ab8-16693719cc22.png)

