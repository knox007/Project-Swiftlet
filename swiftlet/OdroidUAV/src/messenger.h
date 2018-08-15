#ifndef _MESSENGER_H_
#define _MESSENGER_H_

#include "utility.h"
#include "../lib/cserial/cSerial.h"

#define DATA_MSG_BUF_SIZE 50


using namespace std;

struct PH2_data_t
{
    unsigned int ts_PH2;
    unsigned int ts_odroid;

    float pitch;
    float roll;
    float yaw;

    // control output
};

class messenger
{
public:
    PH2_data_t ph2_data;
    //boost::lockfree::spsc_queue <PH2_data_t, boost::lockfree::capacity<200> > ph2_data_q;
    queue<PH2_data_t> ph2_data_q;

    messenger(cSerial sp);
    void get_data();
    void set_startup_time(unsigned int sys_time);

private:
    cSerial _sp;
    unsigned int _ts_startup;
    char _linebuf[DATA_MSG_BUF_SIZE];
    unsigned int _linebuf_len = 0;

};


#endif
