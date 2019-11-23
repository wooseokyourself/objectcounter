//
//  DetectedObject.cpp
//
//  Created by wooseokyourself on 23/07/2019.
//  Copyright © 2019 wooseokyourself. All rights reserved.
//

#include "DetectedObject.hpp"
/*
DetectedObject::DetectedObject(int x, int y, int width, int height, int frame, int position)
: box(x, y, width, height), frame(frame), position(position){
    x = box.x;
    y = box.y;
    width = box.width;
    height = box.height;
    center_x = x + width/2;
    center_y = y + height/2;
}*/

DetectedObject::DetectedObject(int center_x, int center_y, int width, int height, int frame, int position) : center_x(center_x), center_y(center_y), frame(frame), position(position){
    x = center_x - width/2;
    y = center_y - height/2;
    box = Rect(x, y, width, height);
}

void DetectedObject::reset(Mat& frame){
    x = box.x;
    y = box.y;
    width = box.width;
    height = box.height;
    center_x = x + width/2;
    center_y = y + height/2;
    area = countNonZero(Mat(frame, box));
}

void DetectedObject::save_prev_pos(Mat& frame){
    prev_position_x = x;
    prev_position_y = y;
    reset(frame);
}
