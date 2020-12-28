# Realtime Face Censure

## General info
System for real-time censuring. It takes a live video feed from a webcam, detects faces in it, then displays the video with all of the faces blurred out. The system is divided into 4 processes which communicate using boost::interprocess. The console UI offers an option to change the blur mode and frame rate, as well as the CPU scheduling options of each process.


### Prerequisites
- OpenCV 4.5.0
- Boost C++ 1.75.0
- Linux OS

### Compilation and running
First cd to the main directory of the project (the one containing CMakeLists.txt)

> cd _path_ 

Then generate a makefile with cmake:

> cmake .

Run the makefile:

> make	

Now you can run the program. Sudo is required to use some of the special scheduling options, it also helps with some boost-ipc errors.

> sudo ./D.out


### Features
- gets images captured from the camera
- face recognition based on OpenCV DNN module
- two censure modes
- diffrent schedulers
- set CPU affinity of each process

	
