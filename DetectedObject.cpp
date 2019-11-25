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

DetectedObject::DetectedObject(Rect box, int frame, int position) : frame(frame), position(position) {
    this->box = Rect(box.x, box.y, box.width, box.height);
    reset();
}

void DetectedObject::reset(Mat& frame){
    if(box.x < 0) box.x = 0;
    if(box.width <= 0 ) box.width = 1;
    if(box.width > frame.cols) box.width = frame.cols;
    if(box.x + box.width > frame.cols) box.width -= (box.x + box.width) - frame.cols;
    if(box.y < 0) box.y = 0;
    if(box.height <= 0) box.height = 1;
    if(box.height > frame.rows) box.height = frame.rows;
    if(box.y + box.height > frame.rows) box.height -= (box.y + box.height) - frame.rows;
    x = box.x;
    y = box.y;
    width = box.width;
    height = box.height;
    center_x = x + width/2;
    center_y = y + height/2;
    area = countNonZero(Mat(frame, box));
}

void DetectedObject::reset(){
    x = box.x;
    y = box.y;
    width = box.width;
    height = box.height;
    center_x = x + width/2;
    center_y = y + height/2;
}

void DetectedObject::save_prev_pos(Mat& frame){
    prev_position_x = x;
    prev_position_y = y;
    reset(frame);
}
