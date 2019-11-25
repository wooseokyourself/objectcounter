//
//  DetectedObject.hpp
//
//  Created by wooseokyourself on 23/07/2019.
//  Copyright © 2019 wooseokyourself. All rights reserved.
//

#ifndef DetectedObject_hpp
#define DetectedObject_hpp

#include <opencv2/opencv.hpp>

using namespace cv;

#define upper_area 0
#define below_area 1

class DetectedObject{
private:
    friend class FrameHandler;
    
private:
    Mat roi;
    Mat hsv_roi;
    Mat mask;
    Mat roi_hist;
    
private:
    Rect box;
    int x;
    int y;
    int width;
    int height;
    int center_x;
    int center_y;
    
    int frame;
    int position;
    
    int prev_position_x;
    int prev_position_y;
    
    int area;
    int peoplenumber;
public:
    DetectedObject(int center_x, int center_y, int width, int height, int frame, int position);
    DetectedObject(Rect box, int frame, int position);
    // width <- horizon , height <- 2*thold_object_column
    void reset(Mat& frame);
    void reset();
    void save_prev_pos(Mat& frame);
};

#endif /* DetectedObject_hpp */
