#include "names.hpp"



#include <opencv2/opencv.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>

#include <vector>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>
#include <thread>

#include <sys/times.h>
#include <signal.h>



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
    std::cout << "Process C with PID: " << getpid() << "\t percentage of CPU time: " << percentageOfTime << "%" << std::endl;
    exit(0);
}

class BlurDrawer {
private:
    cv::Mat image;
    std::vector<cv::Rect> face_list;
    int mode;//blur mode
    std::mutex mode_mutex;

    static const int DEFAULT_MODE = 0;

public:
    BlurDrawer(cv::Mat input_image, std::vector<cv::Rect> list, int x);

    cv::Mat draw();

    int get_mode(){
        mode_mutex.lock();
        int copy = mode;
        mode_mutex.unlock();
        return copy;
    }
    void set_mode(int new_mode){
        mode_mutex.lock();
        mode = new_mode;
        mode_mutex.unlock();
    }

    void set_draw_info(cv::Mat input_image, std::vector<cv::Rect> list) {
        face_list = std::move(list);
        image = input_image.clone();

    }
    BlurDrawer(){
        mode = DEFAULT_MODE;
    }
};


BlurDrawer::BlurDrawer(cv::Mat input_image, std::vector<cv::Rect> list, int x) {

    face_list = std::move(list);
    image = input_image.clone();
    mode = x;
}

cv::Mat BlurDrawer::draw() {
    //fill face rect
     if( mode == 0) {

         cv::Scalar color(0, 0, 0);//red
         
         int frame_thickness = -1;//fill
         for (const auto &r : face_list) {

            cv::rectangle(image, r, color, frame_thickness);   

         }
         //add gaussian blur on face rect
     } else if (mode == 1) {
         for (const auto &r : face_list) {
             cv::GaussianBlur(image(r), image(r), cv::Size(0,0), 8);
         }
     }
    return image;

}


// responsible for receiving information about censure mode change from the UI
// meant to run in a helper thread, since the receive() method is a blocking operation
void wait_for_mode_change(BlurDrawer & drawer){

    message_queue mq
            (open_only       
            ,CENSURE_MODE_Q_NAME  
            );

    unsigned int priority;
    std::size_t recvd_size;
    int new_mode;

    while(true){
        mq.receive(&new_mode, sizeof(new_mode), recvd_size, priority);
        drawer.set_mode(new_mode);
      }
}



int main() {

    signal(SIGINT, handleSIGINT);
    realStart = times(&cpuStart);

    BlurDrawer drawer;
    std::thread mode_listener(wait_for_mode_change, std::ref(drawer));
 
    std::vector<cv::Mat> framesBuffer;
    std::vector<unsigned int> frameNumber;

    message_queue bc_mq
         (open_only               //only create
         ,BC_SYNC_Q_NAME    //name
         );

    //opening faces shmem
    shared_memory_object facesShmem(open_only, FACES_SHMEM_NAME, read_only);
    named_mutex mutexBC(boost::interprocess::open_only, FACES_MUTEX_NAME);
    mapped_region facesRegion(facesShmem, read_only);

    shared_memory_object frameShmem(open_only, FRAME_SHMEM_NAME, read_only);
    mapped_region regionFrame(frameShmem, read_only);
    named_mutex mutexFrame(open_only, FRAME_MUTEX_NAME);

    named_mutex mutexFramesize(open_only, FRAMESIZE_MUTEX);
    shared_memory_object framesizeInfo(open_only, FRAMESIZE_SHMEM, read_only);
    mapped_region framesizeRegion(framesizeInfo, read_only);
    long framesize[3];
   
    mutexFramesize.lock();
    memcpy(framesize, framesizeRegion.get_address(), sizeof(framesize));
    mutexFramesize.unlock();

    int64_t imageProcessedTime, imageCaptureTime;
    unsigned int priority;
    size_t recvd_size;

    char whatever;

    std::ofstream timeFile;
    if(SAVE_PROCESSING_TIME) {
        timeFile.open("perf_test/times.txt", std::ios_base::out);
    }


    while(true) {
        
        mutexFrame.lock();
        memcpy(&imageCaptureTime, regionFrame.get_address(), sizeof(int64_t));
        cv::Mat img(framesize[0], framesize[1],
                            CV_8UC3,
                            (unsigned char*)(regionFrame.get_address()) + sizeof(int64_t),
                            cv::Mat::AUTO_STEP);

        mutexFrame.unlock();
        
        if(SYNC_BC)
            bc_mq.receive(&whatever, sizeof(whatever), recvd_size, priority);

        mutexBC.lock();

        int sizeOfArray;
        memcpy(&sizeOfArray, facesRegion.get_address(), sizeof(int));

        int faces[sizeOfArray+1];

        memcpy(faces, facesRegion.get_address(), sizeof(faces));
        mutexBC.unlock();
        
        std::vector<cv::Rect> list;
        for(int i = 1; i <= sizeOfArray; i += 4) {

            cv::Rect temp(faces[i] , faces[i+1], faces[i+2], faces[i+3]);
            list.push_back(temp);
        }



        drawer.set_draw_info(img, list);

        cv::Mat image = drawer.draw();

        
       
        
        cv::imshow("Real-Time Face Censure", image);

        imageProcessedTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        
        if(SAVE_PROCESSING_TIME) {        
            timeFile << imageProcessedTime - imageCaptureTime << std::endl;
            timeFile.flush();
        }

        char c = (char)cv::waitKey(1);
        
    }

    mode_listener.join();

    return 0;
}
