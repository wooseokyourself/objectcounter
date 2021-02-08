//
//  FrameHandler.hpp
//
//  Created by wooseokyourself on 23/07/2019.
//  Copyright © 2019 wooseokyourself. All rights reserved.
//

#ifndef FrameHandler_hpp
#define FrameHandler_hpp

#include <opencv2/opencv.hpp>
#include <vector>
#include <iostream>
#include <algorithm>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include "DetectedObject.hpp"

#define thold_detect_cols 50

using namespace cv;
using namespace std;

class FrameHandler{
private:
    bool MAKEBOXFUNC_DIVIDING_TOKEN;
  
private:
    int thold_detect_time;
    
    int person_max;    /* (box < person_max) means 1명, (box > person_max) means 2명 */
    int counter;
    int inside; // UP(0) or DOWN(1)

    VideoCapture capture;
    Mat frame;
    Mat fg_mask_mog2;
    Ptr<BackgroundSubtractor> ptr_mog2;

    int ratio;
    int thold_binarization;

    int upperline;
    int midline;
    int belowline;

    uchar* upper_1;
    uchar* upper_2;
    uchar* upper_3;

    uchar* below_3;
    uchar* below_2;
    uchar* below_1;

    int totalframe;

    vector<DetectedObject> objects;

    uint64 time_start;
    uint64 time_end;

private:
    int roi_width; /* min: 1, max: frame.cols/2 */
    int roi_height; /* min: 1, max: upperline*2 */
    double roi_width_rate_temp;
    double roi_height_rate_temp;

    int thold_object_width; /* min: 1, max: frame.cols/2 */
    int thold_object_height; /* min: 1, max: upperline*2 */
    double thold_object_width_rate_temp;
    double thold_object_height_rate_temp;

private: // Used only for methods
    int max_width_temp;
    int recursive_temp1;
    int recursive_temp2;
    int recursive_temp3;

public:
    FrameHandler(string videopath);
    ~FrameHandler();
    bool play();

protected:
    void set_mask();
    void check_endpoint();
    void detection();
    void detect_upperline(int x);
    void detect_belowline(int x);
    void tracking_and_counting();
    void paint_line();

protected:
    int recursive_ruler_x(uchar* ptr, int start, const int& interval);
    void make_box(int center_x, int center_y);
    void fit_box(DetectedObject& roi); /* Make roi to fit for its blob */
    void extract_box(DetectedObject& roi); /* Extract the box if white blobs exists right beside the barrier of the box. */
    bool is_tracked(DetectedObject *except, int x, int y); /* True if (x,y) is in boundary of some objects which excepts '*except'. */
};

#endif /* FrameHandler_hpp */
