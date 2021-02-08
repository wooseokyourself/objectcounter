//
//  main.cpp
//
//  Created by wooseokyourself on 23/07/2019.
//  Copyright © 2019 wooseokyourself. All rights reserved.
//

#include "FrameHandler.hpp"

int main(void){
    while(true){
        FrameHandler* system = new FrameHandler("VIDEOPATH");
        if(system->play()){
            delete system;
            break;
        }
        else{
            cout << "---------reboot system---------" << endl;
            delete system;
        }
    }
    return 0;
}
