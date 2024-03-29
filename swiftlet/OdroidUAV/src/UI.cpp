//*****************************************************************************
// UI class
//*****************************************************************************
#include "UI.h"

UI::UI()
{
    _input = "";
    data_log_density = 1;   // default

    init_log();
}

void UI::init_log()
{
    _log_list.open("../data/log_list.txt");
    _log_list >> nlog;
    _log_list.close();

    flag.file_is_opened = false;
}

void UI::start_log(deque<pos_data_t> ldata_q, deque<PH2_data_t> ph2_data_q)
{
    flag.file_is_closed = false;

    if (!flag.file_is_opened)
    {
        string filename;
        string dir = "../data/";

        // file for control data
        filename = dir + "control_" + to_string(nlog) + ".txt";
        control_log.open(filename);
        filename = "";

        control_log << "Control data log\n";
//        control_log << "pos_y z alt roll ch1 ch3 ch5 thr_hover thr_avg_max thr_in u1 ez iez dez AC_alt_tar AC_cr dist_err rngAlt_target tsO tsPH2\n";
        control_log << "pos_y z alt roll ch1 ch3 ch5 thr_hover thr_avg_max thr_in u1 ez iez dez tsPH2\n";


        // file for info data
        filename = dir + "info_" + to_string(nlog) + ".txt";
        info_log.open(filename);
        filename = "";

        info_log << "Pixhawk 2 data log\n";
//        info_log << "r p y ch1 2 3 4 5 6 7 8 thr_hover thr_avg_max thr_in u1 ez iez dez AC_alt_tar AC_cr dist_err rngAlt_target tsO tsPH2 PH2_ts\n";
        info_log << "r p y ch1 2 3 4 5 6 7 8 thr_hover thr_avg_max thr_in u1 ez iez dez tsPH2 PH2_ts\n";

        // file for lidar scan
        filename = dir + "lscan_" + to_string(nlog) + ".dat";
        lscan_log.open(filename, ios::out | ios::binary);

        //*********************************************************************
        // file format
        //*********************************************************************
        /*  data oder for one set:
         *  ts_odroid ts_lidar angle1 range1 angle2 range2 . . . angle1080 range1080
         *
         *  note: angle is in degree, need to divide stored value by 100
         *        angle_real = angle/100
        */

        flag.file_is_opened = true;

        cout << "starting to log ...\n";
    }
    else
    {

    //=========================================================================
    // CONTROL LOG
    //=========================================================================
        pos_data_t loc_data;
        PH2_data_t cdata;
        loc_data = ldata_q.back();
        cdata    = ph2_data_q.back();

        control_log << loc_data.pos.y << ',';
        control_log << loc_data.pos.z << ',';
        control_log << loc_data.alt << ',';
        control_log << cdata.roll << ',';
        control_log << cdata.ch.roll << ',';
        control_log << cdata.ch.thr << ',';
        control_log << cdata.ch.aux5 << ',';
        control_log << cdata.thr_hover << ',';
        control_log << cdata.throttle_avg_max << ',';
        control_log << cdata.throttle_in << ',';
        control_log << fixed << setprecision(6) << cdata.u1 << ',';
        control_log << fixed << setprecision(6) << cdata.perr.ez << ',';
        control_log << fixed << setprecision(6) << cdata.perr.iterm_z << ',';
        control_log << fixed << setprecision(6) << cdata.perr.dterm_z << ',';
        //control_log << fixed << setprecision(6) << cdata.AC_alt_target << ',';
        //control_log << fixed << setprecision(6) << cdata.AC_cr << ',';
        //control_log << fixed << setprecision(6) << cdata.dist_err << ',';
        //control_log << fixed << setprecision(6) << cdata.target_rangefinder_alt << ',';
        control_log << loc_data.ts_odroid << ',';
        control_log << cdata.ts_PH2;
        control_log << '\n';
    //=========================================================================
    // DATA INFO LOG
    //=========================================================================
        PH2_data_t pdata;
        while (!ph2_data_q.empty())
        {
            lock_guard<mutex>   lock(_ui_mtx);

            pdata = ph2_data_q.front();
            ph2_data_q.pop_front();

            info_log << pdata.roll << ',';
            info_log << pdata.pitch << ',';
            info_log << pdata.yaw << ',';
            info_log << pdata.ch.roll << ',';
            info_log << pdata.ch.pitch << ',';
            info_log << pdata.ch.thr << ',';
            info_log << pdata.ch.yaw << ',';
            info_log << pdata.ch.aux5 << ',';
            info_log << pdata.ch.aux6 << ',';
            info_log << pdata.ch.aux7 << ',';
            info_log << pdata.ch.aux8 << ',';
            info_log << pdata.thr_hover << ',';
            info_log << pdata.throttle_avg_max << ',';
            info_log << pdata.throttle_in << ',';
            info_log << fixed << setprecision(6) << pdata.u1 << ',';
            info_log << fixed << setprecision(6) << pdata.perr.ez << ',';
            info_log << fixed << setprecision(6) << pdata.perr.iterm_z << ',';
            info_log << fixed << setprecision(6) << pdata.perr.dterm_z << ',';
            //info_log << fixed << setprecision(6) << pdata.AC_alt_target << ',';
            //info_log << fixed << setprecision(6) << pdata.AC_cr << ',';
            //info_log << fixed << setprecision(6) << pdata.dist_err << ',';
            //info_log << fixed << setprecision(6) << pdata.target_rangefinder_alt << ',';
            info_log << pdata.ts_PH2;
            info_log << '\n';
        }

    //=========================================================================
    // LIDAR SCAN DATA LOG - in binary
    //=========================================================================
        pos_data_t ldata;
        while (!ldata_q.empty())
        {
            lock_guard<mutex>   lock(_ui_mtx);  // protecting queue pop in this scope

            ldata = ldata_q.front();
            ldata_q.pop_front();

            lscan_log.write((char*) &ldata.ts_odroid, sizeof(int));
            lscan_log.write((char*) &ldata.ts_lidar, sizeof(int));

            for (int i=0;i<540*2;i+=data_log_density)
            {
                lscan_log.write((char*) &ldata.range[i] , sizeof(int));
            }
        }
    }
}


void UI::end_log()
{
    // reset file open flag
    flag.file_is_opened = false;

    if (!flag.file_is_closed)
    {
        info_log.close();
        lscan_log.close();
        control_log.close();

        // update log number
        _log_list.open("../data/log_list.txt", ios::out);
        _log_list << ++nlog;
        _log_list.close();

        cout << "Closed log file set: " << nlog - 1 << '\n';

        flag.file_is_closed = true;
    }
}

void UI::_set_log_num(int num)
{
    _log_list.open("../data/log_list.txt", ios::out);
    _log_list << num;
    _log_list.close();
    nlog = num;
}

void UI::set_startup_time(unsigned int sys_time)
{
    _ts_startup = sys_time;
}

void UI::run()
{
    if (!flag.startup)
    {
        _print_help();
        flag.startup = true;
    }

    cin >> _input;
// debug
    if (_input == "debug")  //TODO can choose what to print
    {
        flag.debug_print = true;

    }
// log
    else if (_input == "logd")
    {
        cout << "log data density (highest) 1-4 (lowest):\n";
        cout << "1080 542 360 270 pts, select: ";
        cin >> data_log_density;
        flag.log_data = true;
    }
    else if (_input == "log")
    {
        flag.log_data = true;
    }
// sl: stop log
    else if (_input == "sl")
        flag.log_data = false;
// sd: stop debug
    else if (_input == "sd")
        flag.debug_print = false;
// s: stop all display
    else if (_input == "s")       // idle the display, reset most flags
    {
        flag.log_data = false;
        flag.debug_print = false;
    }
// set log number
    else if (_input == "set_log")
    {
        int n;
        cout << "Set log file number to: ";
        cin >> n;
        _set_log_num(n);
        cout << "done\n";
    }
// rename log
    else if (_input == "rename_log")
    {
        string newname;
        string newfile;
        string dir = "../data/";
        string oldname;
        int n;


        cout << "Which file set to rename: ";
        cin >> n;

        cout << "New file name is: ";
        cin >> newfile;


        // file for info data
        oldname = dir + "info_" + to_string(n) + ".txt";
        newname = dir + "info_" + newfile + ".txt";
        if (rename(oldname.c_str(), newname.c_str()))
            cout << "error renaming info file!";

        // file for lidar scan data
        oldname = dir + "lscan_" + to_string(n) + ".dat";
        newname = dir + "lscan_" + newfile + ".dat";
        if (rename(oldname.c_str(), newname.c_str()))
            cout << "error renaming lscan file!";

        cout << "done\n";
    }
// set altitude type
    else if (_input == "alt")
    {
        int n = 0;
        cout << "Setting altitude type to:\n";
        cout << "0 - floor\n";
        cout << "1 - roof\n";
        cout << "2 - tunnel\n";
        cin >> n;

        switch(n)
        {
            case 0:
                lidar_CMD.alt_type = FLOOR;
                break;
            case 1:
                lidar_CMD.alt_type = ROOF;
                break;
            case 2:
                lidar_CMD.alt_type = TUNNEL;
                break;
        }

        lidar_CMD.set_type = true;
    }
// help: should always be last
    else if (_input == "help")
    {
        _print_help();
    }

}

void UI::_print_help()
{
    vector <string> cmd_list = {
            "log or logd to choose log data density - default:1",
            "set_log",
            "rename_log",
            "sl: stop log",
            "sd, stop debug print",
            "s: stop all display",
            "alt",
            "debug" //debug should always be last
    };

    vector <string> debug_list; //TODO: choose what to print

    cout << "******** list of commands ****************\n\n";
    for (int i=0; i<cmd_list.size(); i++)
    {
        cout << "   " << i+1 << ") " << cmd_list[i] << '\n';
    }
    cout << "\n******************************************\n";
}
