#include "names.hpp"

#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>

#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>

#include <cstring>
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
    std::cout << "Process B with PID: " << getpid() << "\t percentage of CPU time: " << percentageOfTime << "%" << std::endl;
    exit(0);
}


class FaceDetector {

private:
    //requires prototxt file and pretrained Caffe model(with weights)
    cv::dnn::Net detection_network;

    int image_width;
    int image_height;
    int image_scale;
    //mean values network was trained with
    cv::Scalar mean_val;
    //confidence (default 0.5)
    float confidence_threshold;


public:
    //detect faces in image
    explicit FaceDetector();

    //return list of detected faces
    std::vector<int> detected_face(const cv::Mat &frame) ;
};


FaceDetector::FaceDetector() :
        confidence_threshold(0.5),
        image_width(300),
        image_height(300),
        image_scale(1.0),
        mean_val({104., 177.0, 123.0}) {
    //detection network read from files which are in ./assets/ directory, pretrained and written by 
    detection_network = cv::dnn::readNetFromCaffe(FACE_DETECTION_CONFIGURATION, FACE_DETECTION_WEIGHTS);
    if (detection_network.empty()) {
        std::cerr<<"ERROR: could not read network";
    }


}

std::vector<int> FaceDetector::detected_face(const cv::Mat &frame) {
    cv::Mat input_blob = cv::dnn::blobFromImage(frame, image_scale, cv::Size(image_width, image_height), mean_val, false, false);

    detection_network.setInput(input_blob, "data");
    cv::Mat detection = detection_network.forward("detection_out");
    cv::Mat detection_matrix(detection.size[2], detection.size[3], CV_32F, detection.ptr<float>());

    std::vector<int> faces;

    for (int i = 0; i < detection_matrix.rows; i++) {
        float confidence = detection_matrix.at<float>(i, 2);

        if (confidence > confidence_threshold) {
            //left bottom pixel
            int x1 = static_cast<int>(detection_matrix.at<float>(i, 3) * frame.cols);
            int y1 = static_cast<int>(detection_matrix.at<float>(i, 4) * frame.rows);

            //right top pixel
            int x2 = static_cast<int>(detection_matrix.at<float>(i, 5) * frame.cols);
            int y2 = static_cast<int>(detection_matrix.at<float>(i, 6) * frame.rows);
            
            //we need to store primitive values needed to construct a cv::Rect so we can put them into shmem
            //since objects of custom class in shmem cause multiple problems

            //we store x,y of left bottom pixel, width and height
            faces.push_back(x1);
            faces.push_back(y1);
            faces.push_back(x2 - x1);
            faces.push_back(y2 - y1);

        }

    }
    //insert size of array as first element so process C can read it
    faces.insert(faces.begin(), faces.size());
    return faces;

}




int main () {
    //here we define a signal handler and start CPU time tracking to display average CPU usage of the process at the exit
    signal(SIGINT, handleSIGINT);
    realStart = times(&cpuStart);
    
    // =================================
    // INITIAL IPC OBJECTS SETUP BEGIN

    //removers make sure that IPC resource does get removed and we will not have errors creating a new ones

    //syncing with process C
    struct q_remover{
        q_remover(){ boost::interprocess::message_queue::remove(BC_SYNC_Q_NAME); }
        ~q_remover(){ boost::interprocess::message_queue::remove(BC_SYNC_Q_NAME); }
    } remover;

    boost::interprocess::message_queue bc_mq
         (boost::interprocess::create_only               //only create
         ,BC_SYNC_Q_NAME    //name
         ,500                        //max message number
         ,sizeof(char)               //max message size
         );



    // save faces:
    struct shm_remove
    {
        shm_remove() { shared_memory_object::remove(FACES_SHMEM_NAME);}
        ~shm_remove() { shared_memory_object::remove(FACES_SHMEM_NAME);}
    } shm_remover;
    shared_memory_object facesShmem(create_only, FACES_SHMEM_NAME, read_write);
    facesShmem.truncate(1024*16);
    mapped_region facesRegion(facesShmem, read_write);



    struct mutex_remove
    {
        mutex_remove() { named_mutex::remove(FACES_MUTEX_NAME); }
        ~mutex_remove(){ named_mutex::remove(FACES_MUTEX_NAME); }
    } BCmutex_remover;
    named_mutex mutexFaces(create_only, FACES_MUTEX_NAME);


    //get image
    shared_memory_object segmentFrame(open_only, FRAME_SHMEM_NAME, read_only);
    mapped_region regionFrame(segmentFrame, read_only);
    named_mutex mutexFrame(open_only, FRAME_MUTEX_NAME);


    
    named_mutex mutexFramesize(open_only, FRAMESIZE_MUTEX);

    shared_memory_object framesizeInfo(open_only, FRAMESIZE_SHMEM, read_only);
    mapped_region framesizeRegion(framesizeInfo, read_only);
    // INITIAL IPC OBJECTS SETUP END
    // =================================

    long framesize[3];
   
    mutexFramesize.lock();
    memcpy(framesize, framesizeRegion.get_address(), sizeof(framesize));
    mutexFramesize.unlock();

    int64_t imageProcessedTime, imageCaptureTime;


    FaceDetector face_detector;
    
    //this value is ignored, but needed to communicate via message q
    char whatever = 0;

    while(true) {
  
        
        mutexFrame.lock();
        memcpy(&imageCaptureTime, regionFrame.get_address(), sizeof(int64_t));
        cv::Mat img(framesize[0], framesize[1],
                            framesize[2],
                            (unsigned char*)(regionFrame.get_address()) + sizeof(int64_t),
                            cv::Mat::AUTO_STEP);
        mutexFrame.unlock();

 
        imageProcessedTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        std::vector<int> result = face_detector.detected_face(img);
        

        //operate on array to copy to shmem
        int facesArray[result.size()];
        std::copy(result.begin(), result.end(), facesArray);
 
        mutexFaces.lock();

        //copy all found faces into region
        memcpy(facesRegion.get_address(), facesArray, sizeof(facesArray));


        //if synchro with C is enabled (it's recommended) than the C will wait until B processes the frame and put detected faces
        //into shmem, which happens here
        if(SYNC_BC)
            bc_mq.send(&whatever, sizeof(whatever), 0);

        mutexFaces.unlock();

    }
    return 0;
}