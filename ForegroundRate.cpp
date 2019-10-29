//
//  ForegroundRate.cpp
//  prev_src
//
//  Created by ChoiWooSeok on 2019/10/28.
//  Copyright © 2019 ChoiWooSeok. All rights reserved.
//

#include "ForegroundRate.hpp"
#include <cmath>

bool checkObjsRate (Mat& binaryFrame, const vector<DetectedObject> Objects, const int& roi_width, const int& roi_height){
    ROINum = Objects.size();
    int ObjectsNumber_min;
    int ObjectsNumber_max;
    int P_min = round (roi_width * 8/10) * round (roi_height * 8/10);
    int P = roi_width * roi_height;
    int frame = binaryFrame.rows * binaryFrame.cols;
    int white = 0;
    
    for (int y = 0; y < binaryFrame.rows; ++y){
        for (int x = 0; x < binaryFrame.cols; ++x){
            if (binaryFrame.at<uchar> (y, x) == 255)
                white++;
                
        }
    }
    expectedNum_min = round (P_min / frame);
    expectedNum_max = round (P / frame);
    
    if (expectedNum_min <= ROINum && ROINum <= expectedNum_max){ // 정상
        
    }
    else if (expectedNum_min > ROINum){ // 사람의 foreground가 더 많이 잡혀야함
        
    }
    else if (ROINum > expectedNum_max){ // ROI에 잡히지 않은 사람이 있음
        
    }
}
