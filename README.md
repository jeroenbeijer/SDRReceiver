This project contains a basic SDR Receiver which feeds JAERO with audio from an R820T2 RTL2832U device like the RTL SDR V3. Rather than feeding JAERO with audio via audio cables, this implementation makes use of ZeroMQ as the interface between the SDR and JAERO. This provides a more stable data flow which is significanty less probe to CRC errors in JAERO. 

In addition, the aim of the project is to run at low CPU usage without any bells or whistles. The GUI is very simple and has only basic features to select the RTL device, start the SDR as well as enable or disable the device bias tee. The FFT is purposely slow to reduce CPU usage. You can select individual VFO's in the dropdown menu which should display the approximate spectrum shown in JAERO.


![image](https://user-images.githubusercontent.com/31091871/126459963-0726ea9d-3d03-40b8-ae90-45676c3c21b1.png)


