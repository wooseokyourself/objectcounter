//
//  FrameHandler.cpp
//
//  Created by wooseokyourself on 23/07/2019.
//  Copyright © 2019 wooseokyourself. All rights reserved.
//

#include "FrameHandler.hpp"
#include <limits.h>

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

int calib(string videopath, int f, int min, int max){
    cout << "calib func (" << min << ", " << max << ")" << endl;
    if (min < 1)
        min = 1;
    
    Rect box = Rect(Point(boxpt1x, boxpt1y), Point(boxpt2x, boxpt2y));
    int itv = round((max - min) / 7);
    int var[7] = {min, min+itv, min+2*itv, min+3*itv, min+4*itv, min+5*itv, max};
    int inbox[7];
    int subtract[7];
    int high = 0;
    int secondhigh = 0;
    int low = INT_MAX;
    int highidx = -1;
    int secondhighidx = -1;
    int lowidx = -1;
    bool morethanzero[7];
    
    int nowf;
    /* inbox, subtract 계산 */
    for(int i=0; i<7; i++){
        Mat *frame = new Mat;
        Mat *fgMaskMOG2 = new Mat;
        Ptr<BackgroundSubtractor> *pMOG = new Ptr<BackgroundSubtractor>;
        VideoCapture *inputVideo = new VideoCapture;
        
        namedWindow("processing...");
        inputVideo->open(videopath);
        Size size = Size((int)inputVideo->get(CAP_PROP_FRAME_WIDTH), (int)inputVideo->get(CAP_PROP_FRAME_HEIGHT));
        cout << "Size: " << size << endl;
        cout << "var[" << i << "]: " << var[i] << endl;
        *pMOG = createBackgroundSubtractorMOG2(500, var[i], true);
        nowf = 0;
        for(;;){
            nowf++;
            if(!inputVideo->read(*frame)){
                cout << "error. 왜 동영상이 벌써끊겨?" << endl;
                exit(-1);
            }
            (*pMOG)->apply(*frame, *fgMaskMOG2);
            
            
            /*
             blur와 binarization 은 실제 프로그램 구동시의 설정과 동일하게.
             set_Mask() 참고
             */
            /* blur 처리 */
            blur(*fgMaskMOG2, *fgMaskMOG2, cv::Size(15, 15), cv::Point(-1, -1));
            /* Binarization threshold = 175 로 고정 */
            threshold(*fgMaskMOG2, *fgMaskMOG2, 175, 255, THRESH_BINARY);
            
            
            imshow("processing...", *fgMaskMOG2);
            // if(nowf%5 == 0) waitKey(0);
            if(nowf == f){
                cout << i << " 번 째 비디오의 마지막 프레임입니다." << endl;
                inbox[i] = countNonZero(Mat(*fgMaskMOG2, box));
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
                subtract[i] = countNonZero(*fgMaskMOG2) - inbox[i];
                if(subtract[i] > 0) morethanzero[i] = true;
                else morethanzero[i] = false;
                /* debug */
                cout << "inbox[" << i << "]: " << inbox[i] << endl;
                cout << "subtr[" << i << "]: " << subtract[i] << endl;
                break;
            }
        }
        delete frame;
        delete fgMaskMOG2;
        delete pMOG;
        delete inputVideo;
        destroyWindow("processing...");
    }
    
    
    /* 종료조건 */
    if(itv < 1.9)
        return var[highidx];
    
    
// subtract > 0 인 원소가 존재한다면..
    
    /*
     subtract>0 인 원소와 subtract=0 인 원소가 서로 공존하는 경우.
     calib 호출시 subtract>0 인 값을 min 혹은 max로 그대로 넣으면 무한재귀에 빠지게 되므로
     itv/2 만큼 범위를 좁혀서 호출
     */
    for(int i=0; i<6; i++){
        if((morethanzero[i] && !morethanzero[i+1]) || (!morethanzero[i] && morethanzero[i+1])){
            if(morethanzero[i])
                return calib(videopath, f, var[i]+(var[i+1]-var[i])/2, max);
            else if(!morethanzero[i])
                return calib(videopath, f, min, var[i+1]-(var[i+1]-var[i])/2);
        }
    }
    
    /*
     모든 원소가 subtract>0 인 경우.
     (0~6 인덱스의 subtract 는 선형관계이기 때문에) 가장 subtract 값이 작은 인덱스와
     그 바깥 범위(max-min 만큼 차이나는 곳의 값) 두 개를 가지고 재귀를 실행한다.
     즉, 현재 재귀에서 탐색한 범위만큼을 더 subst가 작은 쪽으로 이동해서 탐색하는 것이다.
     */
    if(subtract[0] > 0 && subtract[6] > 0){
        if(subtract[0] > subtract[6]){
            return calib(videopath, f, var[6], (max-min)+var[6]);
        }
        else if(subtract[0] < subtract[6]){
            return calib(videopath, f, (max-min)-var[0], var[0]);
        }
        else
            return std::max(calib(videopath, f, var[6], max+var[6]), calib(videopath, f, min-var[0], var[0]));
    }
    
    /*
     모두 0일 경우;
     --> 잘못됨. 모든 원소가 subtract=0 일 경우는 밑의 코드로 바로 넘어가러 처리될 수 있음.
    else if(subtract[0] == 0 && subtract[6] == 0){
        return calib(videopath, f, std::min(var[secondhighidx], var[highidx]), std::max(var[secondhighidx], var[highidx]));
    }
     */
    
    
    
// 모든 원소가 subtract=0 이라면...
    /*
        각 그래프의 x축은 0~6의 index이며, y축은 inbox의 값을 의미.
        사실 볼록그래프, 혹은 오목그래프일 수는 없다. varThreshold와 inbox 값은 선형관계이기 때문이다.
        그럼에도 혹시 몰라서 모든 그래프의 케이스에 대응해야 하기 때문에 볼록 및 오목인 경우를 고려하였다.
     */
    
    /* inbox가 선형그래프일 경우 */
    if(lowidx == 0 && highidx == 6){ // highidx가 6이라면, 자연히 high = max 가 성립.
        cout << "선형그래프!" << endl;
        return calib(videopath, f, max, max+(max-min)/2);
    }
    else if(lowidx == 6 && highidx == 0){ // highidx가 0이라면, 자연히 high = min 이 성립.
        cout << "선형그래프!" << endl;
        return calib(videopath, f, min-(max-min)/2, min);
    }
    
    /* inbox가 볼록그래프일 경우 */
    else if(0 < highidx && highidx < 6){
        cout << "볼록그래프!" << endl;
        return calib(videopath, f, var[highidx-1], var[highidx+1]);
    }
    
    /* 오목그래프일 경우 */
    else if(0 < lowidx && lowidx < 6){
        cout << "오목그래프!" << endl;
        if(secondhighidx > -1)
            return std::max(calib(videopath, f, var[highidx]-itv, var[highidx]+itv), calib(videopath, f, var[secondhighidx]-itv, var[secondhighidx]+itv));
        else
            return calib(videopath, f, var[highidx]-itv, var[highidx]+itv);
    }
    else{
        cout << "이건 예정에 없던 그래프인데용?" << endl;
        exit(-1);
    }
}

int calib_init(string videopath){
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
    return calib(videopath, f, 1, 50);
}

FrameHandler::FrameHandler(string videopath) : totalframe(0), time_start(0), time_end(0), max_width_temp(0), recursive_temp1(0), recursive_temp2(0), recursive_temp3(0), counter(0), personMax(0),  thold_binarization(175)
{
    int capture_type;
    cout << "Capture type(1: video input / 0: Cam) : "; cin >> capture_type;
    
    varThreshold = calib_init(videopath);
    cout << "varThreshold: " << varThreshold << endl;
    
    if(capture_type == 1)
        capture.open(videopath);
    else if(capture_type == 0)
        capture.open(-1);
    else{
        cout << "Wrong input." << endl;
        exit(1);
    }
        
    pMOG = createBackgroundSubtractorMOG2(history, varThreshold, true);
    // pMOG = createBackgroundSubtractorMOG2(500, 16, false);
    
    namedWindow("Frame");
    namedWindow("FG Mask MOG 2");
    
    capture >> frame;
    
    cout << "Video : " << frame.cols << " X " << frame.rows << endl;
    
//!!!!!!!!!!!!!!!!!!!!!! this ratio need to be changed when other video loaded -> round(frame.cols / thold_detect_cols)
    //ratio = round(800/thold_detect_cols);
    ratio = round(frame.cols / thold_detect_cols);
//!!!!!!!!!!!!!!!!!!!!!!
    
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
    
    cout << "set history : "; cin >> history;
    cout << "set Obj Height(%) : "; cin >> thold_object_height_rate;
    cout << "set Obj Width(%)  : "; cin >> thold_object_width_rate;
    cout << "set ROI Height(%) : " ; cin >> roi_height_rate;
    cout << "set ROI Width(%)  : " ; cin >> roi_width_rate;
    cout << "set thold_detect_time : "; cin >> thold_detect_time;
    cout << endl;
    cout << "SYSTEM : Program will reboot when 'binarization' is 0" << endl;
    cout << "SYSTEM : Press 'p' to stop/play the video." << endl;
    cout << "SYSTEM : Press 'q' to quit." << endl;
    
    
    
    createTrackbar("White Width", "FG Mask MOG 2", &thold_object_width_rate, 100); // For controlling the minimum of detection_width.
    
    createTrackbar("White Height", "FG Mask MOG 2", &thold_object_height_rate, 100); // For controlling the minimum of detection_comlumn.
    
    createTrackbar("ROI width", "Frame", &roi_width_rate, 100);
    createTrackbar("ROI height", "Frame", &roi_height_rate, 100);
    
    // remember, upperline is (1/10 * frame.rows).
    // it means the ROI's height must not be more than upperline*2,
    // because when the ROI generated, its center_y will be on the upperline or below line.
    
    createTrackbar("Binarization", "FG Mask MOG 2", &thold_binarization, 255);
    
    
    pMOG->apply(frame, fgMaskMOG2);
    
    cout << frame.cols << " " << frame.rows << endl;
    cout << fgMaskMOG2.cols << " " << fgMaskMOG2.rows << endl;
    
    upper_1, upper_2, upper_3, below_3, below_2, below_1 = nullptr;
}

FrameHandler::~FrameHandler(){
    if(frame.isContinuous() && fgMaskMOG2.isContinuous()){
        frame.release();
        fgMaskMOG2.release();
        destroyAllWindows();
    }
    Objects.clear();
}

bool FrameHandler::Play(){
    while(1){
        time_start = getTickCount();
        
        roi_width_rate_temp = (double)roi_width_rate / 100;
        roi_height_rate_temp = (double)roi_height_rate / 100;
        
        roi_width = frame.cols/2 * roi_width_rate_temp;
        roi_height = upperline*2 * roi_height_rate_temp;
        
        thold_object_width_rate_temp = (double)thold_object_width_rate / 100;
        thold_object_height_rate_temp = (double)thold_object_height_rate / 100;
        
        thold_object_width = frame.cols/2 * thold_object_width_rate_temp;
        thold_object_height = upperline*2 * thold_object_height_rate_temp;
        
        if(waitKey(20) == 'p'){
            while(waitKey(1) != 'p');
        }
        if(waitKey(1) == 'q'){
            break;
        }
        if(thold_binarization == 0){ // reboot the program
            return false;
        }
        capture >> frame;
        if(frame.empty()){
            cout << "Unable to read next frame." << endl;
            cout << "Exiting..." << endl;
            exit(EXIT_FAILURE);
        }
        
        pMOG->apply(frame, fgMaskMOG2);
        set_Mask();
        
        /*
        stringstream ss;
        rectangle(frame, Point(10, 2), Point(100,20), Scalar(255,255,255), -1);
        ss << capture.get(CAP_PROP_POS_FRAMES);
        string frameNumberString = ss.str();
        putText(frame, frameNumberString.c_str(), Point(15, 15), FONT_HERSHEY_SIMPLEX, 0.5 , Scalar(0,0,0));
        
        rectangle(frame, Point(10, 20), Point(200, 40), Scalar(255, 255, 255), -1);
        string dude = "COUNTER : " + to_string(counter);
        putText(frame, dude, Point(15, 35), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 0, 0));
        
        rectangle(fgMaskMOG2, Point(10, 40), Point( 10 + thold_object_width, 40 + thold_object_height*2 ), Scalar(255,255,255), 2);
        rectangle(frame, Point(10, 40), Point( 10 + thold_object_width, 40 + thold_object_height*2 ), Scalar(255,255,255), 2);
        
        rectangle(frame, Point(10, 40), Point( 10 + roi_width, 40 + roi_height), Scalar(255, 0, 0), 2);
         */
        
        if(totalframe % thold_detect_time == 0){
            check_endpoint();
            detection();
        }
        tracking_and_counting();
        
        
        paint_line();
        
        imshow("Frame", frame);
        imshow("FG Mask MOG 2", fgMaskMOG2);
        
        totalframe++;
        if(totalframe == 2000)
            totalframe = 0;
        
        time_end = getTickCount();
        
        if(totalframe % 50 == 0){ // THIS IS FOR DEBUGGING
            cout << "===============================" << endl;
            cout << "varThreshold : " << varThreshold << endl;
            cout << "Binarization : " << thold_binarization << endl;
            cout << "Obj Height : " << thold_object_height_rate << "%" << endl;
            cout << "Obj Width  : " << thold_object_width_rate << "%" << endl;
            cout << "ROI HEIGHT : " << roi_height_rate << "%" << endl;
            cout << "ROI WIDTH  : " << roi_width_rate << "%" << endl;
            cout << "===============================" << endl;
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


void FrameHandler::set_Mask(){
    blur(fgMaskMOG2, fgMaskMOG2, cv::Size(15, 15), cv::Point(-1, -1));
    // Blur the foreground mask to reduce the effect of noise and false positives
    
    // dilate(fgMaskMOG2, fgMaskMOG2, Mat(), Point(-1, -1), 2, 1, 1);
    
    threshold(fgMaskMOG2, fgMaskMOG2, thold_binarization, 255, cv::THRESH_BINARY);
    // Remove the shadow parts and the noise
    
    upper_1 = fgMaskMOG2.ptr<uchar>(upperline - thold_object_height);
    upper_2 = fgMaskMOG2.ptr<uchar>(upperline);
    upper_3 = fgMaskMOG2.ptr<uchar>(upperline + thold_object_height);
    
    below_3 = fgMaskMOG2.ptr<uchar>(belowline - thold_object_height);
    below_2 = fgMaskMOG2.ptr<uchar>(belowline);
    below_1 = fgMaskMOG2.ptr<uchar>(belowline + thold_object_height);
}

void FrameHandler::check_endpoint(){
    int k = 0;
    while(true){ // Check detected boxes are in end point
        if(k >= Objects.size())
            break;
        if(totalframe - Objects[k].frame > thold_detect_time*10){
            // This "if" checks the difference between "object's frame" from "total frame".
            // this difference helps to prevent not removing the box which created just now.
            Objects[k].reset(fgMaskMOG2);
            if(Objects[k].center_y <= upperline /*Down-to-Top*/ || Objects[k].center_y >= belowline  /*Top-to-Down*/
               || Objects[k].y == 0 || Objects[k].y + Objects[k].height == frame.rows){
                if(Objects.size() != 0)
                    swap(Objects[k], Objects.back());
                Objects.pop_back();
                continue;
                
            }
            
        }
        /* 가만히 있는 box 제거 */
        /*
        if(abs(Objects[k].prev_position_y - Objects[k].y) < 10 && abs(Objects[k].prev_position_x - Objects[k].x) < 10){
            swap(Objects[k], Objects.back());
            Objects.pop_back();
            continue;
        }
         */
        
        /* 센터가 겹치는 box 중 하나 제거 */
        for(int i=0; i<Objects.size(); i++){
            if(k == i)
                continue;
            if(abs(Objects[k].center_x - Objects[i].center_x) < roi_width*8/10 && abs(Objects[k].center_y - Objects[i].center_y) < roi_height*8/10){
                swap(Objects[k], Objects.back());
                Objects.pop_back();
            }
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
        
        for(int i=0; i<Objects.size(); i++){
            Objects[i].save_prev_pos(fgMaskMOG2);
            /*
            if(Objects[i].y <= upperline + thold_object_height){
                if((Objects[i].x <= x * ratio) && (x * ratio <= Objects[i].x + Objects[i].width)){
                    x += int(Objects[i].width/ratio);
                }
            }*/
            if(Objects[i].center_y <= upperline + thold_object_height){
                if((Objects[i].x <= x * ratio) && (x * ratio <= Objects[i].x + Objects[i].width)){
                    x += int(Objects[i].width/ratio);
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
        for(int i=0; i<Objects.size(); i++){
            /*
            if(Objects[i].y + Objects[i].height >= belowline - thold_object_height){
                if((Objects[i].x <= x * ratio) && (x * ratio <= Objects[i].x + Objects[i].width)){
                    x += int(Objects[i].width/ratio);
                }
            }*/
            if(Objects[i].center_y >= belowline - thold_object_height){
                if((Objects[i].x <= x * ratio) && (x * ratio <= Objects[i].x + Objects[i].width)){
                    x += int(Objects[i].width/ratio);
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
    
    waitKey(0);
}

void FrameHandler::detect_upperline(int x){
    ////////////////////////////////////////////////////////
    //      DETECT; UPPER LINE
    ////////////////////////////////////////////////////////
    // circle(frame, Point(x * ratio, upperline + thold_object_height), 6, Scalar(0, 0, 255)); // Debug ; to see detected points.
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
                        MakeBox((x*ratio) + (max_width_temp/2), upperline);
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
    ////////////////////////////////////////////////////////
    //      DETECT; BELOW LINE
    ////////////////////////////////////////////////////////
    // circle(frame, Point(x * ratio, belowline - thold_object_height), 6, Scalar(0, 0, 255)); // Debug ; to see detected points.
    if(below_3[x * ratio] == 255){
        if(below_2[x * ratio] == 255){
            // circle(fgMaskMOG2, Point(x * ratio, belowline), 3, Scalar(0, 0, 255)); // Debug ;
            if(below_1[x * ratio] == 255){
                // time_start = getTickCount();
                max_width_temp = 0;
                // circle(fgMaskMOG2, Point(x * ratio, belowline + thold_object_height), 3, Scalar(0, 0, 255)); // Debug ;
                recursive_temp1 = recursive_ruler_x(below_1, x * ratio, thold_detect_cols);
                recursive_temp2 = recursive_ruler_x(below_2, x * ratio, thold_detect_cols);
                recursive_temp3 = recursive_ruler_x(below_3, x * ratio, thold_detect_cols);
                max_width_temp = max( max(recursive_temp1, recursive_temp2), recursive_temp3 );
                if(recursive_temp2 >= recursive_temp1 && recursive_temp2 >= recursive_temp3){
                    max_width_temp = max( max(recursive_temp1, recursive_temp2), recursive_temp3 );
                    if(thold_object_width <= max_width_temp){
                        // This "if" checks whether detected object is big enough ; standard is "thold_object_width"
                        MakeBox((x*ratio) + (max_width_temp/2), belowline);
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
    if(Objects.size() != 0){ // this "if" condition is necessary because 'roi_hist' is empty when program starts.
        // cvtColor(frame, hsv, COLOR_BGR2HSV); // for HSV in meanShift
        for(int i=0; i<Objects.size(); i++){
            // time_start = getTickCount();
            
            ////////////////////// CAM SHIFT(Visualized as red circles) ///////////////////////
            // RotatedRect tracker = CamShift(fgMaskMOG2, Objects[i].box, TermCriteria( TermCriteria::EPS | TermCriteria::COUNT, 10, 1 ));
            // boxes[i] is delivered as it's reference; CamShift renews the boxes[i]'s position.
            // ellipse( frame, tracker, Scalar(0,0,255), 3, LINE_AA ); // Draw red circle on the frame.
            ////////////////////////////////////////////////////////
            
            ////////////////////// CAM SHIFT(Visualized as rectangles) ///////////////////////
            // CamShift(fgMaskMOG2, Objects[i].box, TermCriteria( TermCriteria::EPS | TermCriteria::COUNT, 10, 1 ));
            // rectangle(frame, Objects[i].box, Scalar(0, 255, 0));
            ////////////////////////////////////////////////////////
            
            
            ////////////////////// MEAN SHIFT(Referenced from opencv docs) //////////////////////
            /*
            if(!Objects[i].roi_hist.empty()){
                calcBackProject(&hsv, 1, channels, Objects[i].roi_hist, dst, range); // for HSV in meanShift
                
                 &hsv        <- const Mat* images
                 1           <- int nimages
                 channels    <- const int* channels
                 roi_hist    <- InputArray hist
                 dst         <- OutputArray backProject
                 range       <- const float** ranges
                
                CamShift(dst, Objects[i].box, TermCriteria(TermCriteria::EPS | TermCriteria::COUNT, 10, 1));
                RotatedRect tracker = CamShift(dst, Objects[i].box, TermCriteria(TermCriteria::EPS | TermCriteria::COUNT, 10, 1));
                ellipse( frame, tracker, Scalar(0,0,255), 3, LINE_AA ); // Draw red circle on the frame.
                
                meanShift(dst, Objects[i].box, TermCriteria(TermCriteria::EPS | TermCriteria::COUNT, 10, 1));
            }
            else{
                swap(Objects[i], Objects.back());
                Objects.pop_back();
                i++;
                continue;
            }*/
            
            meanShift(fgMaskMOG2, Objects[i].box, TermCriteria(TermCriteria::EPS | TermCriteria::COUNT, 10, 1));
            rectangle(frame, Objects[i].box, Scalar(0, 255, 0), 3);
            Objects[i].reset(fgMaskMOG2);
            /////////////////////////////////////////////////////////////////////////////////////
            
            
            area = to_string(Objects[i].area);
            
            rectangle(frame, Point(Objects[i].x, Objects[i].y), Point(Objects[i].x + 50, Objects[i].y + 18), Scalar(255,255,255), -1);
            putText(frame, area, Point(Objects[i].x, Objects[i].y), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(0, 0, 0));
            
            if( (Objects[i].position == upper_area) && (Objects[i].center_y > midline) ){
                Objects[i].position = below_area;
                // cout << "\t\t\t >> DETECT OBJECT, Up to Down." << endl;
                cout << "\t >> IN" << endl;
                if(inside == upper_area)
                    counter--;
                else
                    counter++;
            }
            if( (Objects[i].position == below_area) && (Objects[i].center_y < midline) ){
                Objects[i].position = upper_area;
                // cout << "\t\t\t >> DETECT OBJECT, Down to Up." << endl;
                cout << "\t >> OUT" << endl;
                if(inside == upper_area)
                    counter++;
                else
                    counter--;
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
}

int FrameHandler::recursive_ruler_x(uchar* ptr, int start, const int& interval){
    // uchar* ptr points a pixel
    // int start means the real x cordinate of the the first input of ptr
    if(ptr[start] < 255){
        return interval;
    }
    else{
        return recursive_ruler_x(ptr, start + interval, interval) + interval;
    }
}

void FrameHandler::MakeBox(int center_x, int center_y){
    /* This "if" checks whether this box is detected previous box's object(by frame).
    If does, return (not making a new box). */
    if(Objects.size() > 0){
        Objects.back().reset(fgMaskMOG2);
        if(abs(Objects.back().center_y - center_y) < roi_height/2 && abs(Objects.back().center_x - center_x) < roi_width ){
            return;
        }
    }
    
    if((center_x + roi_width) >= frame.cols){
        center_x = frame.cols - roi_width/2;
    }
    if(center_y < frame.rows/2){
        Objects.push_back( DetectedObject(center_x, center_y, roi_width, roi_height, totalframe, upper_area) );
    }
    else{
        Objects.push_back( DetectedObject(center_x, center_y, roi_width, roi_height, totalframe, below_area) );
    }
    fitBox(Objects.back());
}

void FrameHandler::fitBox(DetectedObject &roi){
    Rect& box = roi.box;
    
    /* 세로 줄 조정 */
    bool flagA, flagB = false;
    cout << "starting x: " << box.x << endl;
    cout << " and width: " << box.width << endl;
    while(!flagA || !flagB){
        for(int i=box.y; i<box.y + box.height; i++){
            if(!flagA){
                if(fgMaskMOG2.at<uchar>(i, box.x) == 255){ /* 좌측 픽셀 검사 */
                    flagA = true;
                }
            }
            if(!flagB){
                if(fgMaskMOG2.at<uchar>(i, box.x + box.width) == 255){ /* 우측 픽셀 검사 */
                    flagB = true;
                }
            }
        }
        if(!flagA){ /* 좌측 줄 땡기기 */
            box.x += 1;
            box.width -= 1;
            // cout << " > left line: " << box.x << endl;
        }
        if(!flagB){ /* 우측 줄 땡기기 */
            box.width -= 1;
            // cout << " > right line: " << box.x + box.width << endl;
        }
        if(box.width == 1){ /* for DEBUG */
            cout << "error" << endl;
        }
    }
    
    /* 가로 줄 조정 */
    flagA, flagB = false;
    while(!flagA || !flagB){
        for(int i=box.x; i<box.x + box.width; i++){
            if(!flagA){
                if(fgMaskMOG2.at<uchar>(box.y, i) == 255){ /* 상단 픽셀 검사 */
                    flagA = true;
                }
            }
            if(!flagB){
                if(fgMaskMOG2.at<uchar>(box.y + box.height, i) == 255){ /* 하단 픽셀 검사 */
                    flagB = true;
                }
            }
        }
        if(!flagA){ /* 상단 줄 땡기기 */
            box.y += 1;
            box.height -= 1;
        }
        if(!flagB){ /* 하단 줄 땡기기 */
            box.height -= 1;
        }
        if(box.height == 1){ /* for DEBUG */
            cout << "error" << endl;
        }
    }
    
    roi.reset(fgMaskMOG2);
}


/*
    calib() 에서 매 var의 inbox 계산마다 그 inbox의 값과 subst>0 의 여부를 여기로 보낸다.
    그럼 여기에서는 매번 인자를 받을 때마다 멤버변수 personMax의 값을 갱신시킨다.
 */
void FrameHandler::assumePersonRate(bool zerosubst, int rate){
    personMax = (personMax + rate) / 2;
}
