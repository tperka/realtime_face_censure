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



tms cpuStart, cpuEnd;
time_t realStart, realEnd;

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

    int _fps;
    bool _fpsChanged = false;
    std::chrono::duration<double> _frameRate;
    std::mutex _fpsMutex;

    

public:
    static const int DEFAULT_FPS = 30;

    FrameSender(){
        setFrameRate(DEFAULT_FPS);
    }

    std::chrono::duration<double> getFrameRate(){
        _fpsMutex.lock();
        auto rate = _frameRate;
        _fpsMutex.unlock();
        return rate;

    }

    void setFrameRate(int fps){
        std::cout << "Changing fps to: " << fps << std::endl;
        _fpsMutex.lock();
        _fpsChanged = true;
        _fps = fps;
        _frameRate = std::chrono::duration<double>(1.0/_fps);
        _fpsMutex.unlock();
    }

    bool isFpsChanged(){
        _fpsMutex.lock();
        bool return_val = _fpsChanged;
        _fpsChanged = false;
        _fpsMutex.unlock();
        return return_val;
    }

    int getFps(){
        _fpsMutex.lock();
        int fps = _fps;
        _fpsMutex.unlock();
        return fps;
    }

};


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
        s.setFrameRate(new_fps);
      }
}




int main(int argc, char **argv) {
    
    signal(SIGINT, handleSIGINT);
    realStart = times(&cpuStart);

    cv::VideoCapture capture;
    cv::Mat frame;
    FrameSender frameSender;
    

    capture.open(CAPTURE_OPEN_VALUE);

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

    named_mutex mutexFrame(create_only, FRAME_MUTEX_NAME);
    named_mutex mutexFramesize(create_only, FRAMESIZE_MUTEX);
    
    capture >> frame;

    

    shared_memory_object segment(create_only, FRAME_SHMEM_NAME, read_write);
    segment.truncate(frame.cols * frame.rows * frame.channels() + sizeof(int64_t) + sizeof(int));
    mapped_region region(segment, read_write);



    mutexFramesize.lock();
    
    shared_memory_object framesizeInfo(create_only, FRAMESIZE_SHMEM, read_write);
    framesizeInfo.truncate(3*sizeof(long));
    mapped_region framesizeRegion(framesizeInfo, read_write);
    long tocopy[3] = {frame.rows, frame.cols, frame.channels()};
    memcpy(framesizeRegion.get_address(), tocopy, sizeof(tocopy));

    mutexFramesize.unlock();
    
    
    std::thread fpsListener(waitForFpsChange, std::ref(frameSender));
    
    std::cout << "FPS: " << frameSender.getFps() << std::endl << "dimensions: " << frame.cols << "x" << frame.rows << std::endl;

    

    if (capture.isOpened()){

		std::cout << "Video capture started" << std::endl;
        auto start = std::chrono::high_resolution_clock::now();
        std::chrono::milliseconds delta(5);
        auto prev = std::chrono::system_clock::from_time_t(0);


        int64_t imageCaptureTime;
        unsigned int frameCounter = 0;
		while (true)
		{
            capture >> frame;
            imageCaptureTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

             if (frame.empty()){
                std::cout << "frame empty" << std::endl;
				break;
            }

            auto time_elapsed = std::chrono::high_resolution_clock::now() - prev;

            if (time_elapsed >= (frameSender.getFrameRate() - delta)){
                prev = std::chrono::high_resolution_clock::now();

                mutexFrame.lock();
                memcpy(region.get_address(), &frameCounter, sizeof(unsigned int));
                memcpy(region.get_address() + sizeof(int), &imageCaptureTime, sizeof(int64_t));
                memcpy(region.get_address() + sizeof(int64_t) + sizeof(int), frame.data, frame.cols * frame.rows * frame.channels());
                mutexFrame.unlock();
                ++frameCounter;
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