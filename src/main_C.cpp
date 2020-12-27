// This process is responsible for receiving frames from the camera sent by process A,
// receiving coordinates of all faces in the picture from process B, applying censure to them
// and drawing the resulting picture onto the screen.
// This process can receive requests to change the censure mode from process D, which is responsible for the UI.

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
             //increasing sigma X val strengthen blur
             cv::GaussianBlur(image(r), image(r), cv::Size(0,0), 11);
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

    //here we define a signal handler and start CPU time tracking to display average CPU usage of the process at the exit
    signal(SIGINT, handleSIGINT);
    realStart = times(&cpuStart);

    BlurDrawer drawer;
    //thread which listens for censure mode's change
    std::thread mode_listener(wait_for_mode_change, std::ref(drawer));
 
    std::vector<cv::Mat> framesBuffer;
    std::vector<unsigned int> frameNumber;


    // =================================
    // INITIAL IPC OBJECTS SETUP BEGIN

    //removers make sure that IPC resource does get removed and we will not have errors creating a new ones


    message_queue bc_mq
         (open_only               //only create
         ,BC_SYNC_Q_NAME    //name
         );

    //opening faces shmem
    shared_memory_object facesShmem(open_only, FACES_SHMEM_NAME, read_only);
    named_mutex mutexBC(boost::interprocess::open_only, FACES_MUTEX_NAME);
    mapped_region facesRegion(facesShmem, read_only);
    
    //opening frame shmem

    shared_memory_object frameShmem(open_only, FRAME_SHMEM_NAME, read_only);
    mapped_region regionFrame(frameShmem, read_only);
    named_mutex mutexFrame(open_only, FRAME_MUTEX_NAME);

    //opening framesize shmem

    named_mutex mutexFramesize(open_only, FRAMESIZE_MUTEX);
    shared_memory_object framesizeInfo(open_only, FRAMESIZE_SHMEM, read_only);
    mapped_region framesizeRegion(framesizeInfo, read_only);
    long framesize[3];
   
    //read framesize
    mutexFramesize.lock();
    memcpy(framesize, framesizeRegion.get_address(), sizeof(framesize));
    mutexFramesize.unlock();

    int64_t imageProcessedTime, imageCaptureTime;

    //those vars are used only for receiving info from queue and will not be actively used
    unsigned int priority;
    size_t recvd_size;
    char whatever;

    //file to save frames processing times to
    std::ofstream timeFile;
    if(SAVE_PROCESSING_TIME) {
        timeFile.open("perf_test/times.txt", std::ios_base::out);
    }


    while(true) {

        //create frame based on data in shared memory
        mutexFrame.lock();
        memcpy(&imageCaptureTime, regionFrame.get_address(), sizeof(int64_t));
        cv::Mat img(framesize[0], framesize[1],
                            framesize[2],
                            (unsigned char*)(regionFrame.get_address()) + sizeof(int64_t),
                            cv::Mat::AUTO_STEP);

        mutexFrame.unlock();
        
        //if synchro with B is enabled (it's recommended) than the C will wait until B processes the frame and put detected faces
        //into shmem
        if(SYNC_BC)
            bc_mq.receive(&whatever, sizeof(whatever), recvd_size, priority);

        mutexBC.lock();

        //first element put into array is size of this array and then we have 4 int values for every face which
        //define rectangle containing detected face
        int sizeOfArray;
        memcpy(&sizeOfArray, facesRegion.get_address(), sizeof(int));

        //n of faces read, create and copy array from shmem
        int faces[sizeOfArray+1];

        memcpy(faces, facesRegion.get_address(), sizeof(faces));
        mutexBC.unlock();
        
        // for every face construct a rectangle and put it into vector used then to draw
        std::vector<cv::Rect> list;
        for(int i = 1; i <= sizeOfArray; i += 4) {

            cv::Rect temp(faces[i] , faces[i+1], faces[i+2], faces[i+3]);
            list.push_back(temp);
        }



        drawer.set_draw_info(img, list);

        cv::Mat image = drawer.draw();

        
       
        //display the frame with censure
        cv::imshow("Real-Time Face Censure", image);

        imageProcessedTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        
        if(SAVE_PROCESSING_TIME) {        
            timeFile << imageProcessedTime - imageCaptureTime << std::endl;
            timeFile.flush();
        }

        //needed but ignored, without it the window will disappear
        char c = (char)cv::waitKey(1);
        
    }

    mode_listener.join();

    return 0;
}
