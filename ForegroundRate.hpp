//
//  ForegroundRate.hpp
//  prev_src
//
//  Created by ChoiWooSeok on 2019/10/28.
//  Copyright © 2019 ChoiWooSeok. All rights reserved.
//

#ifndef ForegroundRate_hpp
#define ForegroundRate_hpp

#include "FrameHandler.hpp"

bool checkObjsRate (Mat& binaryFrame, const vector<DetectedObject> Objects, const int& roi_width, const int& roi_height);


#endif /* ForegroundRate_hpp */
