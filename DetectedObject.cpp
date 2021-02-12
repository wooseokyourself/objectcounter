//
//  DetectedObject.cpp
//
//  Created by wooseokyourself on 23/07/2019.
//  Copyright © 2019 wooseokyourself. All rights reserved.
//

#include "DetectedObject.hpp"

DetectedObject::DetectedObject(int center_x, int center_y, int width, int height, int frame, int position) : center_x(center_x), center_y(center_y), frame(frame), position(position), peoplenumber(0){
    x = center_x - width/2;
    y = center_y - height/2;
    box = Rect(x, y, width, height);
}

DetectedObject::DetectedObject(Rect box, int frame, int position) : frame(frame), position(position) {
    this->box = Rect(box.x, box.y, box.width, box.height);
    /*
    reset(frame);
     원래 여기에서 reset이 이루어졌으나, 이후 매 프레임마다 fit_box, extract_box 를 수행한 후에 reset을 호출하므로 생략.
     만약 여기에서 reset을 호출하려면, 이 클래스 생성자에서 personMax를 인자로 받아와야함.*/
}

void DetectedObject::reset(int person_max, Mat& frame, const int &roi_width, const int &roi_height){
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
    
    // 사람 한 명 분의 box
    int wid = roi_width/2;
    int hei = roi_height/2;
    
    // 상자 내의 사람 수 추정하기.
    if(box.width < wid && box.height < hei){
        if(0 <= area && area <= person_max)
            peoplenumber = 1;
    }
    else if( (wid < box.width && box.width < wid*2) && (box.height < hei*2) ){
        if(person_max < area && area < person_max*2)
            peoplenumber = 2;
    }
    
    // 당최 이유를 모르겠는데 가끔 peoplenumber 가 50의 배수로 되는 경우가 있다;;
    if(peoplenumber % 50 == 0 && peoplenumber > 3){
        peoplenumber = 1;
    }
}

// not used
void DetectedObject::reset(int person_max){
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
}
