//
//  FrameHandler.cpp
//
//  Created by wooseokyourself on 23/07/2019.
//  Copyright © 2019 wooseokyourself. All rights reserved.
//

#include "FrameHandler.hpp"
#include <limits.h>

#define THOLD_BIN 100   // for threshold of Binarization.
#define THOLD_HISTORY 100   // for MOG2 

/**
 * 이 값은 calibration에서 마우스이벤트로 초기화되고,
 * 그 값을 그대로 MakeBox에서 roi_width, roi_height 를 결정하는데에 쓰인다.
 */
int boxpt1x, boxpt1y, boxpt2x, boxpt2y;

int mouseEventCount;

void callback_setBox(int event, int x, int y, int flags, void *userdata){
    if(event == EVENT_LBUTTONUP){
        if(mouseEventCount == 0){
            cout << "Mouse Event " << mouseEventCount + 1 << endl;
            boxpt1x = x;
            boxpt1y = y;
        }
        else if(mouseEventCount == 1){
            cout << "Mouse Event " << mouseEventCount + 1 << endl;
            boxpt2x = x;
            boxpt2y = y;
        }
        mouseEventCount++;
    }
}

/**
 *  기존에는 binarization = 170에 7개의 시퀀스를 세분화하여
 * i 는 subtract = 0 이고 i+1 혹은 i-1 이 subtract > 0 인 경우의 var[i] 를 varThreshold 값으로 리턴되게 하였으나,
 * 이렇게 할 경우 foreground가 출력되고 안되고의 경계가 위태위태해서 binarization = 240 으로 올렸다.
 * >> 여기 binarization 을 올리지 말고, varThreshold 값을 더 올리는게 맞을 것 같다.
 */
int calib(string videopath, int f, int min, int max, int& person_max){
    cout << "calib func (" << min << ", " << max << ")" << endl;
    if (min < 1)
        min = 1;
    
    Rect box = Rect(Point(boxpt1x, boxpt1y), Point(boxpt2x, boxpt2y));
    int itv = round((max - min) / 7);
    
    int var[7];
    if(max - min >= 7){ // var = {min, min+itv, min+2*itv, min+3*itv, min+4*itv, min+5*itv, max};
        for(int i=0; i<7; i++){
            if(i!=6)
                var[i] = min+(itv*i);
            else
                var[i] = max;
        }
    }
    else{ // var = {min, min+1, min+2, min+3, ... }
        for(int i=0; i<7; i++){
            var[i] = min+i;
        }
    }
    
    int inbox[7];
    int subtract[7];
    int high = 0;
    int secondhigh = 0;
    int low = INT_MAX;
    int highidx = -1;
    int secondhighidx = -1;
    int lowidx = -1;
    bool morethanzero[7];
    
    int sumDiffVars = 0; // var[i] - var[i-1] 의 합 (i=0~6)
    int sumDiffSubtracts = 0; // subtract[i-1] - subtract[i] 의 합 (i=0~6)
    
    int nowf;

    // inbox, subtract 계산
    for(int i=0; i<7; i++){
        Mat *frame = new Mat;
        Mat *fg_mask_mog2 = new Mat;
        Ptr<BackgroundSubtractor> *ptr_mog2 = new Ptr<BackgroundSubtractor>;
        VideoCapture *inputVideo = new VideoCapture;
        namedWindow("processing...");
        inputVideo->open(videopath);
        Size size = Size((int)inputVideo->get(CAP_PROP_FRAME_WIDTH), (int)inputVideo->get(CAP_PROP_FRAME_HEIGHT));
        *ptr_mog2 = createBackgroundSubtractorMOG2(THOLD_HISTORY, var[i], true);
        nowf = 0;
        
        cout << "idx: " << i << "| var: " << var[i] << ", ";
        for(;;){
            nowf++;
            if(!inputVideo->read(*frame)){
                cout << "error. 왜 동영상이 벌써끊겨?" << endl;
                exit(-1);
            }
            (*ptr_mog2)->apply(*frame, *fg_mask_mog2);
            
            // blur 처리
            blur(*fg_mask_mog2, *fg_mask_mog2, cv::Size(15, 15), cv::Point(-1, -1));
            // Binarization threshold = 100 로 고정
            threshold(*fg_mask_mog2, *fg_mask_mog2, THOLD_BIN, 255, THRESH_BINARY);
            
            
            imshow("processing...", *fg_mask_mog2);
            if(nowf == f){
                inbox[i] = countNonZero(Mat(*fg_mask_mog2, box));
                cout << inbox[i] << " | " ;
                if(inbox[i] > high){
                    secondhigh = high;
                    secondhighidx = highidx;
                    high = inbox[i];
                    highidx = i;
                }
                else if(inbox[i] < low){
                    low = inbox[i];
                    lowidx = i;
                }
                subtract[i] = countNonZero(*fg_mask_mog2) - inbox[i];
                cout << subtract[i] << endl;
                if(subtract[i] > 0)
                    morethanzero[i] = true;
                else
                    morethanzero[i] = false;
                
                // subtract[왼쪽] > 0 && subtract[오른쪽] = 0 이라면 여기에서 바로 그 두개를 가지고 재귀호출
                if(i>0 && morethanzero[i-1] && !morethanzero[i]){
                    // person_max 갱신
                    if(person_max == 0){
                        person_max = (inbox[i-1] + inbox[i]) / 2;
                    }
                    else{
                        person_max = (person_max + ((inbox[i-1] + inbox[i]) / 2)) / 2;
                    }
                    
                    delete frame;
                    delete fg_mask_mog2;
                    delete ptr_mog2;
                    delete inputVideo;
                    destroyWindow("processing...");
                    
                    if(var[i] - var[i-1] == 1) // 종료조건
                        return var[i];
                    else
                        return calib(videopath, f, var[i-1], var[i], person_max);
                }
                break;
            }
        }
        delete frame;
        delete fg_mask_mog2;
        delete ptr_mog2;
        delete inputVideo;
        destroyWindow("processing...");
        
        if(i != 0){
            sumDiffVars += (var[i] - var[i-1]); // var[i] 는 i와 비례하기 때문에 가능.
            sumDiffSubtracts += abs(subtract[i] - subtract[i-1]); // subtract[i] 는 i와 반비례하기 때문에 가능
        }
    }
    
    /*
     모든 원소가 subtract>0 인 경우.
     0~6 인덱스의 subtract 는 반비례관계이다.
     
     6개 시퀀스에 대해, (var[i+1] - var[i]) 의 평균을 aver(diff(var)) 이라고 하자. (=V)
     또한, (subtract[i+1] - subtract[i]) 의 평균을 aver(diff(subtract)) 라고 하자. (=S)
     현재 모든 시퀀스는 0이 아니므로 다음 구간을 재탐색해야 하는데, subtract[6] 에 비해 S가 굉장히
     작은 숫자라면, var[7] 혹은 var[8]에 대응되는 var값 등 var[6]과 가까운 시퀀스는 충분히
     다음 호출의 input으로 넣지 말고 건너뛰어도 된다고 추정할 수 있다.
     이 때, 얼만큼을 건너뛸것인가를 결정하기 위한 알고리즘이 지금 설명하는 알고리즘이다.
     (실상황에서도 항상 그렇지만, subtract[0] > subtract[6] != 0 이라고 가정하겠다.)
     현재 우리가 알고자 하는 것은 다음 input의 min, max 값이다.
     현재 가장 subtract 값이 낮은 6번째의 시퀀스에서, subtract = 0 이 되는 첫 번째의 6+k 번째 시퀀스를 구하기 위해 식을 세운다.
     
        k = subtract[6] / S
     
     만약 var[6] = 21 이라면 (현재 호출의 max),
       var[6+k] = 21 + V*k 이다.
     구간을 더욱 정교하게 잡기 위해, 다음 호출의 min 을 k-1, max를 k+1 로 한다고 하면,
     
        var[6+(k-1)] = 21 + V * (k-1)
        var[6+(k+1)] = 21 + V * (k+1)
     
     즉 다음 호출은 calib( (var[6] + V*(k-1)), (var[6] + V*(k+1)) ) 이 된다.
     */
    if(subtract[0] > 0 && subtract[6] > 0){
        int V = round(sumDiffVars/7);
        int S = round(sumDiffSubtracts/7);
        int k = round(subtract[6] / S);
        if(subtract[0] > subtract[6]){
            // return calib(videopath, f, var[6], (max-min)+var[6], person_max);
            return calib(videopath, f, (var[6] + (V*(k-1))), (var[6] + (V*(k+1))), person_max);
        }
        else if(subtract[0] < subtract[6]){
            cout << "뭔가 이상한데용? 어떻게 subtract[6]이 subtract[0]보다 더 클 수 있죠?" << endl;
            exit(-1);
        }
        else{
            /*
             subtract[0] = subtract[6], 일반적으로 bounding box(boxpt1, boxpt2)를 너무 작게 설정해서,
             아무리 varThreshold를 높여도 subtract에 일정 값이 항상 검출되어서 이런 일이 벌어진다.
             */
            cout << "Bounding Box가 너무 좁습니다! 더 넓게 설정하십씨오!" << endl;
            exit(-1);
        }
    }
    
    /*
     모든 원소가 subtract=0 인 경우.
     각 그래프의 x축은 0~6의 index이며, y축은 inbox의 값을 의미.
     사실 볼록그래프, 혹은 오목그래프일 수는 없다. varThreshold와 inbox 값은 선형관계이기 때문이다.
     */
    if(lowidx == 0 && highidx == 6){
        if(max-min <= 7){
            return var[highidx];
        }
        else{
            return calib(videopath, f, max, max+(max-min)/2, person_max);
        }
    }
    else{
        cout << "이건 예정에 없던 그래프인데용?" << endl;
        exit(-1);
    }
}

int calib_init(string videopath, int& person_max){
    /*
     맥에서 비디오 저장이 안되어서 일단은 원본동영상에서 정지하였을 때의 프레임수를 넘겨서,
     calib에서는 원본동영상과 프레임수로 언제 MOG2 계산을 할 지를 결정하는 식으로 구현함.
     변수 f가 바로 그 프레임을 나타냄.
     */
    VideoCapture inputVideo;
    VideoWriter outputVideo;
    Mat frame;
    Mat image;
    inputVideo.open(videopath);
    Size size = Size((int)inputVideo.get(CAP_PROP_FRAME_WIDTH), (int)inputVideo.get(CAP_PROP_FRAME_HEIGHT));
    cout << "Size = " << size << endl;
    int fourcc = VideoWriter::fourcc('m', 'p', '4', 'v');
    double fps = inputVideo.get(CAP_PROP_FPS); // 여기에서 fps가 지금 추출될 수 있는지는 모름
    bool isColor = true;
    outputVideo.open("ref.mov", fourcc, 25, size, isColor);
    namedWindow("optimization");
    
    int f = 0;
    
    for(;;){
        f++;
        
        if(waitKey(20) == 'p'){
            frame.copyTo(image);
            break;
        }
        
        if (!inputVideo.read(frame)){
            cout << "video is over" << endl;
            break;
        }
        outputVideo.write(frame);
        imshow("optimization", frame);
    }
    inputVideo.release();
    outputVideo.release();
    frame.release();
    destroyWindow("optimization");
    namedWindow("image");
    setMouseCallback("image", callback_setBox, NULL);
    imshow("image", image);
    waitKey(0);
    image.release();
    destroyWindow("image");
    cout << "set pt1(" << boxpt1x << "," << boxpt1y << ")" << endl;
    cout << "set pt2(" << boxpt2x << "," << boxpt2y << ")" << endl;
    
    // calib 재귀 초귀 inputs: 1(min), 50(max)
    return calib(videopath, f, 1, 500, person_max);
}

FrameHandler::FrameHandler(string videopath) : totalframe(0), time_start(0), time_end(0), max_width_temp(0), recursive_temp1(0), recursive_temp2(0), recursive_temp3(0), counter(0), person_max(0)
{
    int capture_type;
    cout << "Capture type(1: video input / 0: Cam) : "; cin >> capture_type;
    
    // MOG2 파라미터
    int varThreshold = calib_init(videopath, person_max);
    cout << "varThreshold: " << varThreshold << endl;
    cout << "person_max   : " << person_max << endl;
    
    if(capture_type == 1)
        capture.open(videopath);
    else if(capture_type == 0)
        capture.open(-1);
    else{
        cout << "Wrong input." << endl;
        exit(1);
    }
    
    // 몇 프레임마다 detect를 시행할 것인지를 결정하는 변수 지정
    double fps = capture.get(CAP_PROP_FPS); // 현재 영상의 fps
    cout << "FPS: " << fps << endl;
    thold_detect_time = fps/7;
    cout << "Detecting per " << thold_detect_time << " frames." << endl;
    ptr_mog2 = createBackgroundSubtractorMOG2(THOLD_HISTORY, varThreshold, true);
    namedWindow("Frame");
    namedWindow("FG Mask MOG 2");
    
    capture >> frame;
    
    // ROI size 지정
    roi_width = (boxpt2x - boxpt1x) * 2;
    roi_height = (boxpt2y - boxpt1y) * 2;
    if(roi_width > frame.cols)
        roi_width = frame.cols;
    if(roi_height > frame.rows)
        roi_height = frame.rows;
    if(roi_width <= 0 || roi_height <= 0){
        cout << "왜 roi 가 지정이 안됐어??" << endl;
        exit(-1);
    }
    cout << "roi_width : " << roi_width << endl;
    cout << "roi_height: " << roi_height << endl;
    
    // 흰색blob 탐색시 사용될 최소기준값 지정
    thold_object_width = round(roi_width / 50);
    thold_object_height = round(roi_height / 50);
    cout << "obj_width : " << thold_object_width << endl;
    cout << "obj_height: " << thold_object_height << endl;
    
    cout << "Video : " << frame.cols << " X " << frame.rows << endl;
    
    // this ratio need to be changed when other video loaded -> round(frame.cols / thold_detect_cols)
    ratio = round(frame.cols / thold_detect_cols);
    
    upperline = frame.rows*2/10;
    midline = frame.rows*5/10;
    belowline = frame.rows*8/10;
    
    cout << "Which side is inside? Up(0), Down(1)" << endl;
    cin >> inside;
    if(inside == upper_area)
        cout << "UP" << endl;
    else if(inside == below_area)
        cout << "DOWN" << endl;
    else{
        cout << "Wrong input. Program exit." << endl;
        exit(1);
    }
    
    cout << "SYSTEM : Program will reboot when 'binarization' is 0" << endl;
    cout << "SYSTEM : Press 'p' to stop/play the video." << endl;
    cout << "SYSTEM : Press 'q' to quit." << endl;
    
    ptr_mog2->apply(frame, fg_mask_mog2);
    
    cout << frame.cols << " " << frame.rows << endl;
    cout << fg_mask_mog2.cols << " " << fg_mask_mog2.rows << endl;
    
    upper_1, upper_2, upper_3, below_3, below_2, below_1 = nullptr;
}

FrameHandler::~FrameHandler(){
    if(frame.isContinuous() && fg_mask_mog2.isContinuous()){
        frame.release();
        fg_mask_mog2.release();
        destroyAllWindows();
    }
    objects.clear();
}

bool FrameHandler::play(){
    while(1){
        time_start = getTickCount();
        
        if(waitKey(20) == 'p'){
            while(waitKey(1) != 'p');
        }
        if(waitKey(1) == 'q'){
            break;
        }
        capture >> frame;
        if(frame.empty()){
            cout << "Unable to read next frame." << endl;
            cout << "Exiting..." << endl;
            exit(EXIT_FAILURE);
        }
        
        ptr_mog2->apply(frame, fg_mask_mog2);
        set_mask();
        
        check_endpoint();
        
        if(totalframe % thold_detect_time == 0){
            detection();
        }
        tracking_and_counting();
        
        
        paint_line();
        
        imshow("Frame", frame);
        imshow("FG Mask MOG 2", fg_mask_mog2);
        
        totalframe++;
        if(totalframe == 2000)
            totalframe = 0;
        
        time_end = getTickCount();
        
        if(totalframe % 50 == 0){ // THIS IS FOR DEBUGGING
            // use this code when you wanna get elapsed time in a frame.
            if(totalframe % thold_detect_time == 0){
                cout << "(DETECT RUN? YES) " << (time_end - time_start) / getTickFrequency() << endl;
            }
            else{
                cout << "(DETECT RUN? NO )" <<(time_end - time_start) / getTickFrequency() << endl;
            }
        }
        
    } // total while
    return true; // exit the program
}



// UTILITIES

void FrameHandler::set_mask(){
    // Blur the foreground mask to reduce the effect of noise and false positives
    blur(fg_mask_mog2, fg_mask_mog2, cv::Size(15, 15), cv::Point(-1, -1));
    
    // Remove the shadow parts and the noise
    threshold(fg_mask_mog2, fg_mask_mog2, THOLD_BIN, 255, cv::THRESH_BINARY);
    
    upper_1 = fg_mask_mog2.ptr<uchar>(upperline - thold_object_height);
    upper_2 = fg_mask_mog2.ptr<uchar>(upperline);
    upper_3 = fg_mask_mog2.ptr<uchar>(upperline + thold_object_height);
    
    below_3 = fg_mask_mog2.ptr<uchar>(belowline - thold_object_height);
    below_2 = fg_mask_mog2.ptr<uchar>(belowline);
    below_1 = fg_mask_mog2.ptr<uchar>(belowline + thold_object_height);
}

void FrameHandler::check_endpoint(){
    int k = 0;
    while(true){
        if(k >= objects.size())
            break;
        
        // 중앙선을 넘어서 화면 밖으로 사라지는 box 제거
        if(totalframe - objects[k].frame > thold_detect_time*10){
            // This "if" checks the difference between "object's frame" from "total frame".
            // this difference helps to prevent not removing the box which created just now.
            
            if(objects[k].center_y <= upperline /*Down-to-Top*/ || objects[k].center_y >= belowline  /*Top-to-Down*/
               || objects[k].y == 0 || objects[k].y + objects[k].height == frame.rows){
                if(objects.size() != 0)
                    swap(objects[k], objects.back());
                objects.pop_back();
                continue;
                
            }   
        }
        
        /* // 움직임이 없는 Box 제거
        if(abs(objects[k].prev_position_y - objects[k].y) < 10 && abs(objects[k].prev_position_x - objects[k].x) < 10){
            swap(objects[k], objects.back());
            objects.pop_back();
            continue;
        }
         */
        
        // 센터가 겹치는 box 중 하나 제거
        for(int i=0; i<objects.size(); i++){
            if(k == i)
                continue;
            if(abs(objects[k].center_x - objects[i].center_x) < roi_width*8/10 && abs(objects[k].center_y - objects[i].center_y) < roi_height*8/10){
                swap(objects[k], objects.back());
                objects.pop_back();
            }
        }
        
// (이하는 일시적인 조명 등으로 box가 잘못생성된 것들을 제거)
        fit_box(objects[k]);
        extract_box(objects[k]);
        objects[k].reset(person_max, fg_mask_mog2, roi_width, roi_height);
        
        // 너비 밑 높이가 비정상적으로 큰 box 제거
        if(objects[k].width > roi_width*2 || objects[k].height > roi_height*2){
            swap(objects[k], objects.back());
            objects.pop_back();
            continue;
        }
        
        // foreground가 아예 없는 box 제거
        if(objects[k].area == 0){
            swap(objects[k], objects.back());
            objects.pop_back();
            continue;
        }
        k++;
    }
}

void FrameHandler::detection(){
    int x = 0;
    /*
     The two "while" checks "x(from '0' to 'thold_detect_cols')"
     whether (x , upperline +- thold_object_column) pixel is white.
     The first checks only on upperline, and the second checks only in belowline.
     The "for"s in each "while" checks if detected object(boxes) is on the "x".
     If it does, then "x" jumps the box.
     (In this "for", if the "x" is in below situation, move "x" to the next pixel from boxes[i].x + boxes[i].width)
     
                          ooooooooooooooooooooooooooooooooo
                          o                               o
     (DETECTLINE)---------o'x'(<-before pos)--------------o'x'(<-after pos)----------(DETECTLINE)
                          o                               o
                          o                               o
                          ..                             ..
                    "boxes[i].x"           "boxes[i].x + boxes[i].width"
    */
    
    while(true){ // for upper line
        
        for(int i=0; i<objects.size(); i++){
            objects[i].save_prev_pos(fg_mask_mog2);
            if(objects[i].center_y <= upperline + thold_object_height){
                if((objects[i].x <= x * ratio) && (x * ratio <= objects[i].x + objects[i].width)){
                    x += int(objects[i].width/ratio);
                }
            }
        }
        if(x > thold_detect_cols){
            break;
        }
        else{
            detect_upperline(x);
            x++;
        }
    }
    
    x = 0;
    while(true){ // for below line
        for(int i=0; i<objects.size(); i++){
            if(objects[i].center_y >= belowline - thold_object_height){
                if((objects[i].x <= x * ratio) && (x * ratio <= objects[i].x + objects[i].width)){
                    x += int(objects[i].width/ratio);
                }
            }
        }
        if(x > thold_detect_cols){
            break;
        }
        else{
            detect_belowline(x);
            x++;
        }
    }
    // waitKey(0);
}

void FrameHandler::detect_upperline(int x){
    if(upper_3[x * ratio] == 255){
        if(upper_2[x * ratio] == 255){
            if(upper_1[x * ratio] == 255){
                // time_start = getTickCount();
                max_width_temp = 0;
                recursive_temp1 = recursive_ruler_x(upper_1, x * ratio, thold_detect_cols);
                recursive_temp2 = recursive_ruler_x(upper_2, x * ratio, thold_detect_cols);
                recursive_temp3 = recursive_ruler_x(upper_3, x * ratio, thold_detect_cols);
                if(recursive_temp2 >= recursive_temp1 && recursive_temp2 >= recursive_temp3){
                    max_width_temp = max( max(recursive_temp1, recursive_temp2), recursive_temp3 );
                    if(thold_object_width <= max_width_temp){
                        // This "if" checks whether detected object is big enough ; standard is "thold_object_width"
                        make_box((x*ratio) + (max_width_temp/2), upperline);
                        // upperline is y cordinate of upper_2 pixels
                    }
                    x += int(max_width_temp/ratio); // After checking the object, we don't need to check the overlapping pixels, so jump.
                }
                else{
                    x += int(max_width_temp/ratio);
                    return;
                }
            }
        }
    }
}

void FrameHandler::detect_belowline(int x){
    if(below_3[x * ratio] == 255){
        if(below_2[x * ratio] == 255){
            if(below_1[x * ratio] == 255){
                // time_start = getTickCount();
                max_width_temp = 0;
                recursive_temp1 = recursive_ruler_x(below_1, x * ratio, thold_detect_cols);
                recursive_temp2 = recursive_ruler_x(below_2, x * ratio, thold_detect_cols);
                recursive_temp3 = recursive_ruler_x(below_3, x * ratio, thold_detect_cols);
                max_width_temp = max( max(recursive_temp1, recursive_temp2), recursive_temp3 );
                if(recursive_temp2 >= recursive_temp1 && recursive_temp2 >= recursive_temp3){
                    max_width_temp = max( max(recursive_temp1, recursive_temp2), recursive_temp3 );
                    if(thold_object_width <= max_width_temp){
                        // This "if" checks whether detected object is big enough ; standard is "thold_object_width"
                        make_box((x*ratio) + (max_width_temp/2), belowline);
                        // upperline is y cordinate of upper_2 pixels
                    }
                    x += int(max_width_temp/ratio); // After checking the object, we don't need to check the overlapping pixels, so jump.
                }
                else{
                    x += int(max_width_temp/ratio);
                    return;
                }
            }
        }
    }
}

void FrameHandler::tracking_and_counting(){
    string area;
    if(objects.size() != 0){ // this "if" condition is necessary because 'roi_hist' is empty when program starts.
        // cvtColor(frame, hsv, COLOR_BGR2HSV); // for HSV in meanShift
        for(int i=0; i<objects.size(); i++){
            // time_start = getTickCount();
            
            meanShift(fg_mask_mog2, objects[i].box, TermCriteria(TermCriteria::EPS | TermCriteria::COUNT, 10, 1));
            rectangle(frame, objects[i].box, Scalar(0, 255, 0), 3);
            objects[i].reset(person_max, fg_mask_mog2, roi_width, roi_height);
            
            area = to_string(objects[i].area) + ", NUMBER: " + to_string(objects[i].peoplenumber);
            
            
            rectangle(frame, Point(objects[i].x, objects[i].y), Point(objects[i].x + 50, objects[i].y + 25), Scalar(255,255,255), -1);
            putText(frame, area, Point(objects[i].x, objects[i].y), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 0, 0));
            
            
            
            if( (objects[i].position == upper_area) && (objects[i].center_y > midline) ){
                objects[i].position = below_area;
                if(inside == upper_area)
                    counter -= objects[i].peoplenumber;
                else
                    counter += objects[i].peoplenumber;
                cout << "\t >> IN : " << objects[i].peoplenumber << "명, area: " << objects[i].area << " | " << person_max <<endl;
            }
            if( (objects[i].position == below_area) && (objects[i].center_y < midline) ){
                objects[i].position = upper_area;
                if(inside == upper_area)
                    counter += objects[i].peoplenumber;
                else
                    counter -= objects[i].peoplenumber;
                cout << "\t >> OUT: " << objects[i].peoplenumber << "명, area: " << objects[i].area << " | " << person_max <<endl;
            }
            // time_end = getTickCount();
            // cout << "tracking_and_counting() time : " << (time_end - time_start) / getTickFrequency() << endl;
        }
    }
}

void FrameHandler::paint_line(){
    // VISUAL DETECT LINE ; UPPER
    line(frame, Point(0, upperline - thold_object_height), Point(frame.cols, upperline - thold_object_height), Scalar(255, 0, 0));
    line(frame, Point(0, upperline),                       Point(frame.cols, upperline),                       Scalar(255, 0, 0)); // UPPER LINE
    line(frame, Point(0, upperline + thold_object_height), Point(frame.cols, upperline + thold_object_height), Scalar(255, 0, 0));
    
    // VISUAL DETECT LINE ; MIDDLE
    line(frame, Point(0, midline),                         Point(frame.cols, midline),                         Scalar(0, 0, 255));
    
    // VISUAL DETECT LINE ; BELOW
    line(frame, Point(0, belowline + thold_object_height), Point(frame.cols, belowline + thold_object_height), Scalar(255, 0, 0));
    line(frame, Point(0, belowline),                       Point(frame.cols, belowline),                       Scalar(255, 0, 0)); // BELOW LINE
    line(frame, Point(0, belowline - thold_object_height), Point(frame.cols, belowline - thold_object_height), Scalar(255, 0, 0));
    
    
    /* frame count
    stringstream ss;
    rectangle(frame, Point(10, 2), Point(100,20), Scalar(255,255,255), -1);
    ss << capture.get(CAP_PROP_POS_FRAMES);
    string frameNumberString = ss.str();
    putText(frame, frameNumberString.c_str(), Point(15, 15), FONT_HERSHEY_SIMPLEX, 0.5 , Scalar(0,0,0));
    */
    
    rectangle(frame, Point(10, 20), Point(200, 40), Scalar(255, 255, 255), -1);
    string dude = "COUNTER : " + to_string(counter);
    putText(frame, dude, Point(15, 35), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 0, 0));
    
    /* // box
    rectangle(fg_mask_mog2, Point(10, 40), Point( 10 + thold_object_width, 40 + thold_object_height*2 ), Scalar(255,255,255), 2);
    rectangle(frame, Point(10, 40), Point( 10 + thold_object_width, 40 + thold_object_height*2 ), Scalar(255,255,255), 2);
    */
    
    // rectangle(frame, Point(10, 40), Point( 10 + roi_width, 40 + roi_height), Scalar(255, 0, 0), 2);
}

int FrameHandler::recursive_ruler_x(uchar* ptr, int start, const int& interval){
    // @ptr - points a pixel
    // @start - the real x cordinate of the the first input of ptr
    if(ptr[start] < 255){
        return interval;
    }
    else{
        return recursive_ruler_x(ptr, start + interval, interval) + interval;
    }
}

void FrameHandler::make_box(int center_x, int center_y){
    /* This "if" checks whether this box is detected previous box's object(by frame).
    If does, return (not making a new box). */
    
    if(objects.size() > 0){
        objects.back().reset(person_max, fg_mask_mog2, roi_width, roi_height);
        if(abs(objects.back().center_y - center_y) < roi_height/2 && abs(objects.back().center_x - center_x) < roi_width ){
            return;
        }
    }
    
    int x = center_x - roi_width/2;
    int y = center_y - roi_height/2;
    int width = roi_width;
    int height = roi_height;
        
    // 생성하고자 하는 box가 frame 을 벗어나는지 조사.
    if(x < 0)
        x = 0;
    if( (x + width) > frame.cols)
        width -= ((x + width) - frame.cols);
    if(y < 0)
        y = 0;
    if( (y + height) > frame.rows)
        height -= ((y + height) - frame.rows);
    
    // 조명변화로 인해 foreground가 비정상적으로 많이 배출된 것인지 조사.
    if(x > frame.cols || y > frame.rows){
        cout << "Abnormal foreground was detected!" << endl;
        return;
    }
    if( (x + width) <= 0 || (y + height) <= 0 ){ // 이 조건은 위에서 box가 frame 을 벗어나는지를 조사하는 과정이 있기에 성립되는거임.
        cout << "Abnormal foreground was detected!" << endl;
        return;
    }
    
    Rect box(x, y, width, height);
    
    if(center_y < frame.rows/2){
        objects.push_back( DetectedObject(box, totalframe, upper_area) );
    }
    else{
        objects.push_back( DetectedObject(box, totalframe, below_area) );
    }
    DetectedObject *BoundingBox = &(objects.back());
    fit_box(*BoundingBox);
    extract_box(*BoundingBox);
    BoundingBox->reset(person_max, fg_mask_mog2, roi_width, roi_height);
}

void FrameHandler::fit_box(DetectedObject &roi){
    Rect& box = roi.box;
    
    // 세로 줄 조정
    bool flagA = false;
    bool flagB = false;
    while(!flagA || !flagB){
        // 줄이고자 하는 줄이 프레임 밖을 벗어났을 경우, 그 줄을 프레임 안으로 지정하고 줄이기 수행
        if(box.x < 0){
            box.x = 0;}
        if(box.x >= frame.cols){
            box.x = frame.cols; break;}
        if(box.width <= 0){
            box.width = 1; break;}
        if(box.width >= frame.cols)
            box.width = frame.cols;
        if(box.x + box.width >= frame.cols)
            box.width = (frame.cols - box.x) - 1;
        if(box.y < 0){
            box.y = 0; break;}
        if(box.y >= frame.rows){
            box.y = frame.rows; break;}
        if(box.height <= 0){
            box.height = 1; break;}
        if(box.height >= frame.rows)
            box.height = frame.rows;
        if(box.y + box.height >= frame.rows)
            box.height = (frame.rows - box.y) - 1;

        
        // flag = false 이면 땡겨야 한다.
        flagA = flagB = false;
        for(int i=box.y; i<box.y + box.height; i++){
            if(is_tracked(&roi, box.x, i))
                break;
            if(!flagA){
                if(fg_mask_mog2.at<uchar>(i, box.x) == 255){ // 좌측 픽셀 검사
                    flagA = true;
                }
            }
            if(!flagB){
                if(fg_mask_mog2.at<uchar>(i, box.x + box.width) == 255){ // 우측 픽셀 검사
                    flagB = true;
                }
            }
            if(flagA && flagB)
                break;
        }
        if(box.width <= 0)
            break;
        if(!flagA){ // 좌측 줄 땡기기
            box.x += 1;
            box.width -= 1;
        }
        if(!flagB){ // 우측 줄 땡기기
            box.width -= 1;
        }
    }
    
    // 가로 줄 조정
    flagA = flagB = false;
    while(!flagA || !flagB){
        // 줄이고자 하는 줄이 프레임 밖을 벗어났을 경우, 그 줄을 프레임 안으로 지정하고 줄이기 수행
        if(box.x < 0){
            box.x = 0; break;}
        if(box.x >= frame.cols){
            box.x = frame.cols; break;}
        if(box.width <= 0){
            box.width = 1; break;}
        if(box.width >= frame.cols)
            box.width = frame.cols;
        if(box.x + box.width >= frame.cols)
            box.width = (frame.cols - box.x) - 1;
        if(box.y < 0){
            box.y = 0; break;}
        if(box.y >= frame.rows){
            box.y = frame.rows; break;}
        if(box.height <= 0){
            box.height = 1; break;}
        if(box.height >= frame.rows)
            box.height = frame.rows;
        if(box.y + box.height >= frame.rows)
            box.height = (frame.rows - box.y) - 1;
        
        // flag = true 가 한번이라도 있으면 땡기지 않아도 된다.
        flagA = flagB = false;
        for(int i=box.x; i<box.x + box.width; i++){
            if(is_tracked(&roi, box.x, i))
                break;
            if(!flagA){
                if(fg_mask_mog2.at<uchar>(box.y, i) == 255){ // 상단 픽셀 검사
                    flagA = true;
                }
            }
            if(!flagB){
                if(fg_mask_mog2.at<uchar>(box.y + box.height, i) == 255){ // 하단 픽셀 검사
                    flagB = true;
                }
            }
            if(flagA && flagB)
                break;
        }
        if(box.height <= 0)
            break;
        if(!flagA){ // 상단 줄 땡기기
            box.y += 1;
            box.height -= 1;
        }
        if(!flagB){ // 하단 줄 땡기기
            box.height -= 1;
        }
    }
}

void FrameHandler::extract_box(DetectedObject &roi){
    Rect& box = roi.box;
    
    // 세로 줄 조정
    /* work flag는, 늘리고자 하는 줄이 프레임 밖을 벗어날 경우 수행을 멈추기 위함이다.
     while문에서 (box.x < 0 || box.x + box.width > frame.cols) 로 실행조건을 하지 않는 이유는,
     box.x <= 0 이라고 하더라도 box.width는 더 늘릴 수 있는 상황이 있을 수도 있기 때문이다.
     그러므로 대신 workA, workB 플래그를 이용한다. */
    bool workA = true; // for x ; 좌측 세로줄
    bool workB = true; // for width ; 우측 세로줄
    
    bool flagA = true;
    bool flagB = true;
    while((workA || workB) && (flagA || flagB)){
        // 줄이고자 하는 줄이 프레임 밖을 벗어났을 경우, 그 줄을 프레임 안으로 지정하고 줄이기 수행
        if(box.x <= 0){
            box.x = 0; workA=false;
        }
        if(box.x >= frame.cols){
            box.x = frame.cols; break;
        }
        if(box.width <= 0){ // 넓이의 경우 1로 조정하면 그 이후 extract를 진행할 수 있음.
            box.width = 1; break;
        }
        if(box.width >= frame.cols){
            box.width = frame.cols; workB=false;
        }
        if(box.x + box.width >= frame.cols){
            box.width -= box.x + box.width - frame.cols; workB=false;
        }
        if(box.y <= 0)
            box.y = 0;
        if(box.y >= frame.rows)
            box.y = frame.rows - 1;
        if(box.height <= 0){
            box.height = 1;
        }
        if(box.height >= frame.rows)
            box.height = frame.rows;
        if(box.y + box.height >= frame.rows)
            break;
        
        flagA = flagB = false;
        // flag = true 이면 늘려야 한다.
        for(int i=box.y; i<box.y + box.height; i++){
            if(is_tracked(&roi, box.x, i))
                break;
            if(workA && !flagA){
                if(fg_mask_mog2.at<uchar>(i, box.x) == 255){ // 좌측 픽셀 검사
                    flagA = true;
                }
            }
            if(workB && !flagB){
                if(fg_mask_mog2.at<uchar>(i, box.x + box.width) == 255){ // 우측 픽셀 검사
                    flagB = true;
                }
            }
            if(flagA && flagB)
                break;
        }
        if(box.width <= 1)
            break;
        if(workA && flagA){ // 좌측 줄 늘리기
            box.x -= 1;
            box.width += 1;
        }
        if(workB && flagB){ // 우측 줄 늘리기
            box.width += 1;
        }

    }
    
    // 가로 줄 조정
    workA = workB = true;
    flagA = flagB = true;
    while((workA || workB) && (flagA || flagB)){
        // 줄이고자 하는 줄이 프레임 밖을 벗어났을 경우, 그 줄을 프레임 안으로 지정하고 줄이기 수행
        if(box.x < 0)
            box.x = 0;
        if(box.x >= frame.cols)
            box.x = frame.cols - 1;
        if(box.width <= 0)
            box.width = 1;
        if(box.width >= frame.cols)
            box.width = frame.cols;
        if(box.x + box.width >= frame.cols)
            break;
        if(box.y <= 0){
            box.y = 0; workA=false;
        }
        if(box.y >= frame.rows){
            box.y = frame.rows; break;
        }
        if(box.height <= 0){
            box.height = 1; break;
        }
        if(box.height >= frame.rows){
            box.height = frame.rows; workB=false;
        }
        if(box.y + box.height >= frame.rows){
            box.height -= box.y + box.height - frame.rows; workB=false;
        }
        
        flagA = flagB = false;
        for(int i=box.x; i<box.x + box.width; i++){
            if(is_tracked(&roi, box.x, i))
                break;
            if(workA && !flagA){
                if(fg_mask_mog2.at<uchar>(box.y, i) == 255){ // 상단 픽셀 검사
                    flagA = true;
                }
            }
            if(workB && !flagB){
                if(fg_mask_mog2.at<uchar>(box.y + box.height, i) == 255){ // 하단 픽셀 검사
                    flagB = true;
                }
            }
            if(flagA && flagB)
                break;
        }
        if(box.height <= 1)
            break;
        if(workA && flagA){ // 상단 줄 늘리기
            box.y -= 1;
            box.height += 1;
        }
        if(workB && flagB){ // 하단 줄 늘리기
            box.height += 1;
        }
    }
}

bool FrameHandler::is_tracked(DetectedObject *except, int x, int y){
    bool flag = false;
    DetectedObject *roi = NULL;
    for(int i=0; i<objects.size(); i++){
        if(except != &objects[i]){
            roi = &objects[i];
            if(roi->x <= x && x <= roi->x + roi->width){
                if(roi->y <= y && y <= roi->y + roi->height){
                    flag = true;
                    break;
                }
            }
        }
    }
    return flag;
}

//good Wooseok!
//Thank you!!
//Have a nice day!!!