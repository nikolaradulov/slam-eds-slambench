#include<Eigen/Dense>
#include<io/sensor/EventCameraSensor.h>
#include<io/sensor/CameraSensor.h>
#include<io/sensor/CameraSensorFinder.h>
#include<io/SLAMFrame.h>
#include<Eigen/Geometry>
#include<io/Event.h>
#include<iostream>
#include<SLAMBenchAPI.h>
#include<base/samples/RigidBodyState.hpp>
#include<base/samples/Event.hpp>
#include<base/Time.hpp>
/** EDS Library **/
#include "task/src/Task.hpp"

static slambench::outputs::Output *pose_out;
static slambench::io::EventCameraSensor * event_sensor = nullptr;
static slambench::io::CameraSensor * grey_sensor = nullptr;
static slambench::TimeStamp last_event_timestamp;
static slambench::TimeStamp last_grey_timestamp;
static slambench::outputs::Output *event_frame_output;
static slambench::outputs::Output *grey_frame_output;
static Eigen::Matrix<unsigned char, Eigen::Dynamic, Eigen::Dynamic> image;
static std::pair<int,int> *indices;

static slambench::TimeStamp latest_frame;
static std::vector<unsigned char> image_raw;
static std::vector<uint8_t> grey_raw;
static sb_uint2 eventInputSize;
static sb_uint2 greyInputSize;

static int begin;
static int end;
// 0 - none 
// 1 - event
// 2 - image
int frame_semaphor = 0;

static int events_per_frame;

Eigen::Matrix4f ident_matrix = Eigen::Matrix4f::Identity();

static base::samples::EventArray events_sample;
static base::samples::frame::Frame frame_sample;
static eds::Task eds_alg;
//================= [SLAMBENCH INTEGRATION] ================

// void select_frame(SLAMBenchLibraryHelper * lib){
//     // get the mutex
//     std::lock_guard<std::mutex> lock(lib->GetMutex());
//     int cur_begin_idx = lib->GetBeginIndex();
//     int cur_end_idx = lib->GetEndIndex();
//     if(cur_end_idx==lib->events_->size()-1){
//         std::cout<<lib->events_->size()<<'\n';
//         lib->SetBeginIndex(-1000);
//         // lib->begin_idx_= cur_end_idx+1;
//         lib->SetEndIndex(-1000);
//         return;
//     }
    
//     //select next frame based on time
//     lib->SetBeginIndex(cur_end_idx+1);
//     // lib->begin_idx_= cur_end_idx+1;
//     // 1000 events per frame
//     lib->SetEndIndex(cur_end_idx+events_per_frame);
//     // std::cout<<"next is "<<cur_end_idx+1<<' '<<i-1<<"\n\n";
// }

void select_frame(SLAMBenchLibraryHelper * lib){
    // get the mutex
    std::lock_guard<std::mutex> lock(lib->GetMutex());
    int cur_begin_idx = lib->GetBeginIndex();
    int cur_end_idx = lib->GetEndIndex();
    if(cur_end_idx==lib->events_->size()-1){
        std::cout<<lib->events_->size()<<'\n';
        lib->SetBeginIndex(-1000);
        // lib->begin_idx_= cur_end_idx+1;
        lib->SetEndIndex(-1000);
        return;
    }
    //select next frame based on time
    auto current_ts = (*(lib->events_))[cur_end_idx+1].ts;
    int i;
    for(i = cur_end_idx+1; i < lib->events_->size(); ++i) {
            auto delta = (*(lib->events_))[i].ts - current_ts;
            if (delta > std::chrono::milliseconds{20}) break;
    }

    lib->SetBeginIndex(cur_end_idx+1);
    // lib->begin_idx_= cur_end_idx+1;
    // 1000 events per frame
    lib->SetEndIndex(i-1);
    // std::cout<<"next is "<<cur_end_idx+1<<' '<<i-1<<"\n\n";
}

//==========================================================
bool sb_new_slam_configuration(SLAMBenchLibraryHelper * slam_settings) {
    slam_settings->event_ = true;
    return true;
}

bool sb_init_slam_system(SLAMBenchLibraryHelper * slam_settings){

    
    
    //=================== [SLAMBench setting] ==================

    // retrieve one event sensor
    event_sensor = (slambench::io::EventCameraSensor*)slam_settings->get_sensors().GetSensor(slambench::io::EventCameraSensor::kEventCameraType);
    assert(event_sensor!=nullptr && "Failed, did not find event sensor");
    
    slambench::io::CameraSensorFinder sensor_finder;
    grey_sensor = sensor_finder.FindOne(slam_settings->get_sensors(), {{"camera_type", "grey"}});
	assert(grey_sensor!=nullptr && "Failed, did not find event sensor");

    // initialise the pose output and bind it to the slam configuration output
    pose_out = new slambench::outputs::Output("Pose", slambench::values::VT_POSE, true);
    slam_settings->GetOutputManager().RegisterOutput(pose_out);

    event_frame_output = new slambench::outputs::Output("Event Frame", slambench::values::VT_FRAME);
  	event_frame_output->SetKeepOnlyMostRecent(true);
    slam_settings->GetOutputManager().RegisterOutput(event_frame_output);

    // to add output for the frame later
    grey_frame_output = new slambench::outputs::Output("Grey Frame", slambench::values::VT_FRAME);
    grey_frame_output->SetKeepOnlyMostRecent(true);
    slam_settings->GetOutputManager().RegisterOutput(grey_frame_output);
    
    

    image.resize(event_sensor->Height, event_sensor->Width);

    eventInputSize   = make_sb_uint2(event_sensor->Width, event_sensor->Height);
    image_raw.resize(event_sensor->Height*event_sensor->Width);
    // Initialize Vertices

    greyInputSize   = make_sb_uint2(grey_sensor->Width, grey_sensor->Height);
    grey_raw.resize(grey_sensor->Height * grey_sensor->Width);
    // eds cannot survive without cameras. So we make cam0 and cam 1
    // cam 0 = the rgb/ grey camera

    // ============================ [From configHook()] ==============================
    // read the foncig file for the algorithm 
    // // perhaps i can remove those and have them as runtime? 
    // // for now everything is set in the config.yaml file
   
    
    
    eds::calib::CameraInfo cam0_info;
    // cam0_info.height = grey_sensor->Height;
    // cam0_info.width = grey_sensor->Width;
    // cam0_info.out_height = grey_sensor->Height;
    // cam0_info.out_width = grey_sensor->Width;
    // try and force higher res
    cam0_info.height = 480;
    cam0_info.width = 640;
    cam0_info.out_height = 480;
    cam0_info.out_width = 640;
    for (int i =0; i<=3;i++)
        cam0_info.intrinsics.push_back(grey_sensor->Intrinsics[i]);
    switch(grey_sensor->DistortionType){
        case slambench::io::CameraSensor::Equidistant:
            cam0_info.distortion_model = "equidistant";
            break;
        default:
            cam0_info.distortion_model = "notequidistant";

    }
    for (int i =0; i<=3;i++)
        cam0_info.D.push_back(grey_sensor->Distortion[i]);
    // never comes from beamsplitter
    cam0_info.flip=false;
        
    eds::calib::CameraInfo cam1_info;
    cam1_info.height = event_sensor->Height;
    cam1_info.width = event_sensor->Width;
    cam1_info.out_height = event_sensor->Height;
    cam1_info.out_width = event_sensor->Width;
    for (int i =0; i<=3;i++)
        cam1_info.intrinsics.push_back(grey_sensor->Intrinsics[i]);
    switch(grey_sensor->DistortionType){
        case slambench::io::CameraSensor::Equidistant:
            cam1_info.distortion_model = "equidistant";
            break;
        default:
            cam1_info.distortion_model = "notequidistant";

    }
    for (int i =0; i<=3;i++)
        cam1_info.D.push_back(grey_sensor->Distortion[i]);
    // should have rotation provided but not available for us
    // init the frame samples with the right size

    frame_sample.init(cam0_info.width, cam0_info.height, 8, base::samples::frame::MODE_RGB );

    
    if(!eds_alg.configure("/home/nrad/slam-eds-slambench/src/orig/config.yaml", cam0_info, cam1_info))
        return false;
    
    eds_alg.start();

    events_per_frame = eds_alg.getEventsPerFrame();
    select_frame(slam_settings);
    return true;
}

bool sb_update_frame(SLAMBenchLibraryHelper * lib, slambench::io::SLAMFrame* s){
    
    frame_semaphor = 0;
    // std::cout<<"In update now\n";
    // we split to always have anough events whenever this gets called with an event frame
    if(s->FrameSensor==event_sensor){
        // clear previous events
        events_sample.events.clear();
        // std::cout<<"After clear "<<events_sample.events.size()<<'\n';
        last_event_timestamp=s->Timestamp;
        //outfile<<"CURRENT FRAME STAMP: "<<last_frame_timestamp<<"\n";
        indices = static_cast<std::pair<int, int> *> (s->GetData());
        for(int i = indices->first ; i<=indices->second; i++ ){
            slambench::io::Event event = (*(lib->events_))[i]; 
            // std::cout<<base::Time::fromMicroseconds(event.ts.ToMs())<<' '<<event.ts.ToMs()<<'\n';
            events_sample.events.push_back(base::samples::Event(event.x, event.y, base::Time::fromMicroseconds(event.ts.ToMs()), event.polarity));
        }
        // std::cout<<"Moved "<<events_sample.events.size()<<" events to base\n";
        // std::cout<<" Start "<<events_sample.events.begin()->ts<<' '<<events_sample.events.end()->ts<<'\n';
        frame_semaphor = 1;
        select_frame(lib);
        return true;
    }
    else {
        // handle grey frames
        if(s->FrameSensor == grey_sensor){
            memcpy(grey_raw.data(), s->GetData(), s->GetSize());
            cv::Mat grayscaleImageMat = cv::Mat(grey_sensor->Height, grey_sensor->Width, CV_8UC1, grey_raw.data());
            // std::cout<<"-----initial size: "<<grayscaleImageMat.total()*grayscaleImageMat.elemSize()<<'\n';
            cv::Mat rgbImageMat;
            cv::cvtColor(grayscaleImageMat, rgbImageMat, cv::COLOR_GRAY2BGR);
            // upscale image after setting it
            // std::cout<<"-----rgb size: "<<rgbImageMat.total()*rgbImageMat.elemSize()<<'\n';
            cv::namedWindow("Window", cv::WINDOW_NORMAL);
            // ofthen complains if not 640 x480
            cv::resize(rgbImageMat, rgbImageMat, cv::Size(640, 480));
            // Convert the cv::Mat data to a std::vector<uint8_t>
            std::vector<uint8_t> rgbImageData(rgbImageMat.data, rgbImageMat.data + rgbImageMat.total() * rgbImageMat.channels());
            memcpy(grey_raw.data(), rgbImageData.data(), rgbImageData.size());
            last_grey_timestamp = s->Timestamp;
            // set the image in a frame 
            frame_semaphor =2;
            frame_sample.setImage(rgbImageData);
            frame_sample.time =  base::Time::fromMicroseconds(last_grey_timestamp.ToMs());
            frame_sample.setAttribute("exposure_time_us", last_grey_timestamp.ToMs());
            return true;
        }
    }
    return false;
}

bool sb_process_once(SLAMBenchLibraryHelper * slam_settings){
    if(frame_semaphor==1){
        assert(frame_semaphor==1);
        eds_alg.eventsCallback(base::Time::fromMicroseconds(last_event_timestamp.ToMs()), events_sample);
    }else{
        if(frame_semaphor==2){
            // std::cout<<"Feeding event frame\n\n";
            assert(frame_semaphor==2);
            eds_alg.frameCallback(base::Time::fromMicroseconds(last_event_timestamp.ToMs()), frame_sample);
        }  
    }
   return true;
}

bool sb_update_outputs(SLAMBenchLibraryHelper * slam_settings, const slambench::TimeStamp *timestamp){
    // output the pose computed before 
    if(pose_out->IsActive()){
        std::lock_guard<FastLock> lock (slam_settings->GetOutputManager().GetLock());
        // need to figure how the psoe is represented in eds to know how to send it to slam
        // look at the output functions from Task
        // pose is Affine3d  == Transform3d type object
        // event frames have getposeMatrix function
        // i have a feeling this will not exactly work
        Eigen::Matrix4f pose_matrix4f =eds_alg.get_ef_pose().cast<float>();
        pose_out->AddPoint(*timestamp, new slambench::values::PoseValue(pose_matrix4f));

       }

    if(grey_frame_output->IsActive()) {
		std::lock_guard<FastLock> lock (slam_settings->GetOutputManager().GetLock());
        // change hard code
		grey_frame_output->AddPoint(latest_frame, new slambench::values::FrameValue(640, 480, slambench::io::pixelformat::G_I_8, (void*) &(grey_raw.at(0))));
	}

    // if(event_frame_output->IsActive()){
    //     event_frame_output->AddPoint(last_frame_timestamp, new slambench::values::EventFrameValue(eventInputSize.y, eventInputSize.x,  slam_settings->events_, indices->first, indices->second));
    // }
    return true;
}

bool sb_clean_slam_system(){
  return true;
}   