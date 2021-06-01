//
// Created by shiva1422 on 6/1/2021.
//

#ifndef DJMAGIC_TIME_H
#define DJMAGIC_TIME_

#include <linux/time.h>
#include <time.h>
#include "Commons.h"

class TimeDiff{
    timespec startTime={.tv_sec=0,.tv_nsec=0};
    timespec endTime={.tv_sec=0,.tv_nsec=0};
    timespec timeDiff={.tv_sec=0,.tv_nsec=0};
    int CLOCKTYPE=CLOCK_REALTIME;

public:
    //////////class methods are inline by default just recheck or if move to cpp keep
    __attribute__((always_inline)) void start()
    {
        clock_gettime(CLOCKTYPE,&startTime);//check Thread Instead of Process
    }
    __attribute__((always_inline))void end()
    {
        clock_gettime(CLOCKTYPE,&endTime);
    }
    void getTimeDiff()
    {
        if(endTime.tv_nsec < startTime.tv_nsec)
        {
            int numSecs= (startTime.tv_nsec - endTime.tv_nsec) / 1000000000 + 1;
            startTime.tv_nsec-= 1000000000 * numSecs;
            startTime.tv_sec+=numSecs;
        }
        if((endTime.tv_nsec - startTime.tv_nsec) > 1000000000)
        {
            int numSecs= (endTime.tv_nsec - startTime.tv_nsec) / 1000000000;
            startTime.tv_nsec+= 1000000000 * numSecs;
            startTime.tv_sec-=numSecs;
        }

        timeDiff.tv_sec= endTime.tv_sec - startTime.tv_sec;
        timeDiff.tv_nsec= endTime.tv_nsec - startTime.tv_nsec;//should be positive check
        Loge("TImeDiff",",Time taken:","%.10lf secs ",timeDiff.tv_sec+timeDiff.tv_nsec/1000000000.0);
        endTime={.tv_sec=0,.tv_nsec=0};
        startTime={.tv_sec=0,.tv_nsec=0};
    }


};

#endif //DJMAGIC_TIME_H
