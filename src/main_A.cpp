// #include <opencv4/opencv2/opencv.hpp>
#include <opencv2/opencv.hpp>
#include <chrono>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>



#include <iostream>
#include <mutex>
#include <thread>
#include <sys/times.h>
#include <signal.h>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>

#include "names.hpp"


using namespace boost::interprocess;


// vars used to calculate CPU usage by comparing CPU time used and real time passed
tms cpuStart, cpuEnd;
time_t realStart, realEnd;

//program termination coming from process D, calculate and print CPU usage and exit
void handleSIGINT(int sig)
{
    long realTime, cpuTime;
    double percentageOfTime;
    realEnd = times(&cpuEnd);
    realTime = (long)(realEnd - realStart);
    cpuTime = (long)(cpuEnd.tms_utime - cpuStart.tms_utime);
    percentageOfTime = (double)(cpuTime) / (double)(realTime) * 100.0;
    std::cout << "Process A with PID: " << getpid() << "\t percentage of CPU time: " << percentageOfTime << "%" << std::endl;
    exit(0);
}

class FrameSender{
private:

    // upper fps limit
    int _fps;
    // 1/fps -- the minimal time (minus a delta to make the limit less strict) between consecutive frame sendings
    std::chrono::duration<double> _frameTime;
    // used for synchronization with the thread responsible for receiving new fps limit values from the UI 
    std::mutex _fpsMutex;

    

public:
    static const int DEFAULT_FPS = 30;

    FrameSender(){
        setFrameTime(DEFAULT_FPS);
    }

    std::chrono::duration<double> getFrameTime(){
        _fpsMutex.lock();
        auto rate = _frameTime;
        _fpsMutex.unlock();
        return rate;

    }

    void setFrameTime(int fps){
        std::cout << "Changing fps to: " << fps << std::endl;
        _fpsMutex.lock();
        _fps = fps;
        _frameTime = std::chrono::duration<double>(1.0/_fps);
        _fpsMutex.unlock();
    }

    int getFps(){
        _fpsMutex.lock();
        int fps = _fps;
        _fpsMutex.unlock();
        return fps;
    }

};

// responsible for receiving information about new fps limits from the UI
// meant to run in a helper thread, since the receive() method is a blocking operation
void waitForFpsChange(FrameSender & s){

    message_queue fps_mq
        (open_only       
        ,FPS_Q_NAME  
        );

    unsigned int priority;
    std::size_t recvd_size;
    int new_fps;

    while(true){
        fps_mq.receive(&new_fps, sizeof(new_fps), recvd_size, priority);
        s.setFrameTime(new_fps);
      }
}




int main(int argc, char **argv) {
    
    //here we define a signal handler and start CPU time tracking to display average CPU usage of the process at the exit
    signal(SIGINT, handleSIGINT);
    realStart = times(&cpuStart);

    cv::VideoCapture capture;
    cv::Mat frame;
    FrameSender frameSender;
    

    // some cameras require a different open value, if needed, change it in names.hpp
    if (!capture.open(CAPTURE_OPEN_VALUE))
        capture.open(CAPTURE_OPEN_VALUE_2);


    // =================================
    // INITIAL IPC OBJECTS SETUP BEGIN

    struct shm_remove
    {
        shm_remove() { shared_memory_object::remove(FRAME_SHMEM_NAME);}
        ~shm_remove() { shared_memory_object::remove(FRAME_SHMEM_NAME);}
    } shm_remover;

    
    struct shm_remove2
    {
        shm_remove2() { shared_memory_object::remove(FRAMESIZE_SHMEM);}
        ~shm_remove2() { shared_memory_object::remove(FRAMESIZE_SHMEM);}
    } shm_remover2;

    
    struct mutex_remove
    {
        mutex_remove() { named_mutex::remove(FRAME_MUTEX_NAME); }
        ~mutex_remove(){ named_mutex::remove(FRAME_MUTEX_NAME); }
    } mutex_remover;

    struct mutex_remove2
    {
        mutex_remove2() { named_mutex::remove(FRAMESIZE_MUTEX); }
        ~mutex_remove2(){ named_mutex::remove(FRAMESIZE_MUTEX); }
    } mutex_remover2;

    //mutex used to guard shared memory with frame size
    named_mutex mutexFramesize(create_only, FRAMESIZE_MUTEX);
    
    shared_memory_object framesizeInfo(create_only, FRAMESIZE_SHMEM, read_write);
    framesizeInfo.truncate(3*sizeof(long));
    mapped_region framesizeRegion(framesizeInfo, read_write);



    //read first frame to obtain information about capture, much easier than using capture.get()
    capture >> frame;

    //use obtained info to define shmem size
    //mutex used to guard frame shared memory
    named_mutex mutexFrame(create_only, FRAME_MUTEX_NAME);
    shared_memory_object frameShmem(create_only, FRAME_SHMEM_NAME, read_write);
    //in this shmem we will store frame data + timestamp of capture thus we need additional int64_t
    frameShmem.truncate(frame.cols * frame.rows * frame.channels() + sizeof(int64_t));
    mapped_region frameRegion(frameShmem, read_write);

    // INITIAL IPC OBJECTS SETUP END
    // =================================

    //send dimensions and frame type (colors palette etc.) to B and C
    mutexFramesize.lock();
    
    long tocopy[3] = {frame.rows, frame.cols, frame.type()};
    memcpy(framesizeRegion.get_address(), tocopy, sizeof(tocopy));

    mutexFramesize.unlock();
    
    //start a new thread which listens to coming FPS change
    std::thread fpsListener(waitForFpsChange, std::ref(frameSender));
    
    std::cout << "FPS: " << frameSender.getFps() << std::endl << "dimensions: " << frame.cols << "x" << frame.rows << std::endl;

    

    if (capture.isOpened()){

		std::cout << "Video capture started" << std::endl;

        std::chrono::milliseconds delta(5);                    // leeway for the check if frame came within the frameTime allowed
        auto prev = std::chrono::system_clock::from_time_t(0); // time the last frame was processed
        int64_t imageCaptureTime;

		while (true)
		{
            capture >> frame;
            imageCaptureTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

             if (frame.empty()){
                std::cerr << "Error: The image received from the camera is empty" << std::endl;
				break;
            }

            // measure time since the last frame was processed
            auto time_elapsed = std::chrono::high_resolution_clock::now() - prev;

            // if last frame was processed long ago enough to keep up with the fps limit (minus delta) we can process this frame
            // otherwise this frame is skipped and we collect the next one   
            if (time_elapsed >= (frameSender.getFrameTime() - delta)){
                // since this frame was chosen for processing, the time measurement of last frame processed starts now
                prev = std::chrono::high_resolution_clock::now();


                // synchronize access and put the frame with its capture's timestamp into shared memory
                mutexFrame.lock();
                memcpy(frameRegion.get_address(), &imageCaptureTime, sizeof(int64_t));
                memcpy((unsigned char*)(frameRegion.get_address()) + sizeof(int64_t), frame.data, frame.cols * frame.rows * frame.channels());
                mutexFrame.unlock();
            }
	    }
    }
	else{
		std::cerr << "Error: Could not Open Camera";
    }

    capture.release();
    
    fpsListener.join();
    return 0;

}