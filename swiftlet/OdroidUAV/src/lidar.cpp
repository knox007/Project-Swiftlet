#include "lidar.h"


Hokuyo_lidar::Hokuyo_lidar()
{


    // open ethernet com
    if (!urg.open(connect_address, connect_port, Urg_driver::Ethernet)) cout << "Error: unable to connect to lidar!!" << endl;

    // start measurement
    urg.start_measurement(Urg_driver::Distance, Urg_driver::Infinity_times, 0);

}

void Hokuyo_lidar::set_startup_time(unsigned int sys_time)
{
    _ts_startup = sys_time;

}

void Hokuyo_lidar::read(float roll)
{
/*
    -----------> +y
    |
    |
    |
    |
    |
    v +z

*/
    //cout << "starting to read" << endl;
    // Clear old data vector
    ldata.range.clear();
    ldata.angle.clear();
    ldata.pc_z.clear();
    ldata.pc_y.clear();

    //long t_temp;
    if (!urg.get_distance(ldata.range, &ldata.ts_lidar))
    {
      _flag_lidar_error  = 1;
      cout << "Error reading lidar scan!" << endl;
    }
    else
    {
      _flag_lidar_error = 0;
    }

    ldata.ts_odroid = millis() - _ts_startup;    // NULL = no geting time stamp

    // removing outliers
    //vector<int> idx = lonely_pts_detector();
    vector<int> idx;  // for disabling lonely point detector, need to comment out the lonely function too

    // converting raw data to cartesian
    int i2 = 0; // index for removing idx vector
    int n = 0;
    for (int i=0; i < 540*2; i++)
    {
        // angle of the range
        double rad = urg.index2rad(i);
        ldata.angle.push_back(urg.index2deg(i));

        if (idx.size() > 0 && idx[i2] == i && i2 < idx.size())
        {
          i2++;
          continue;
        }
        else
        {
          // only store meaningfull data
          if ((fabs(ldata.angle[i]) > 81.0f && fabs(ldata.angle[i]) < 95.0f) || ldata.angle[i] > 125.0f)
          {
              // condition is for the QAV500 frame
              // redundant: can rely on lonely_pts_detector
          }
          else
          {
            if ((ldata.range[i] > 350) && (ldata.range[i] <= 10000))  // redundant
            {
            	ldata.pc_z.push_back((double) (ldata.range[i] * cos(rad)));
                ldata.pc_y.push_back((double) (ldata.range[i] * sin(rad)));
            	n++;
            }
          }
        }
    }


    ldata.nyz = n;

    _data_loss = (float) n/1080;  // calc data loss

   // cout << "ldata.nyz " << ldata.nyz << endl;
    // rotation (y,z)
    for (int i=0; i < ldata.nyz; i++)
    {
        double y_temp = cos(roll*M_PI/180.0f) * ldata.pc_y[i] - sin(roll*M_PI/180.0f) * ldata.pc_z[i];
        double z_temp = sin(roll*M_PI/180.0f) * ldata.pc_y[i] + cos(roll*M_PI/180.0f) * ldata.pc_z[i];

        ldata.pc_y[i] = y_temp;
        ldata.pc_z[i] = z_temp;
    }

    //get_symmetry_pt();
    // overriding get_symmetry_pt for QAV500
    yz_start_pt = 0;
    yz_end_pt = ldata.nyz;


    //get_centroid1();
    get_centroid2();

    // pushing lidar data to the queue
    ldata_q.push(ldata);

    //cout << "done reading lidar ... " << ldata.range[540] << endl;
    //cout << "y2  " << setw(10) << pos_loc_y2 << " y3 " << setw(10) << pos_loc_y3 << " R " << setw(10) << dist_wallR << endl;
}  // end of lidar read();


// centroid
void Hokuyo_lidar::get_centroid1()
{
    float ciy = 0;
    float ciz = 0;
    float cy = 0;
    float cz = 0;
    float temp = 0;
    float Area = 0;
    int n = ldata.nyz;


    // compute total area
    for (int i=0;i<=n-1;i++)
    {
        if (i<n-1)
            temp += ldata.pc_y[i] * ldata.pc_z[i+1] - ldata.pc_y[i+1] * ldata.pc_z[i];
        else
            temp += ldata.pc_y[i] * ldata.pc_z[0] - ldata.pc_y[0] * ldata.pc_z[i];
    }
    float A = fabs(0.5*temp);

    temp = 0; //reset temp variable

    int nTri = n-2;

    // compute centre of area

    for (int i=0;i<=nTri-1;i++)
    {

        temp = ldata.pc_y[0]*ldata.pc_z[i+1] - ldata.pc_y[i+1]*ldata.pc_z[0] + ldata.pc_y[i+1]*ldata.pc_z[i+2] - ldata.pc_y[i+2]*ldata.pc_z[i+1] + ldata.pc_y[i+2]*ldata.pc_z[0] - ldata.pc_y[0]*ldata.pc_z[i+2];

        ciy = ldata.pc_y[0] + ldata.pc_y[i+1] + ldata.pc_y[i+2];
        ciz = ldata.pc_z[0] + ldata.pc_z[i+1] + ldata.pc_z[i+2];

        ciy /= 3;
        ciz /= 3;

        float iTriA = fabs(0.5*temp);

        cy += ciy*iTriA;
        cz += ciz*iTriA;

        temp = 0;
        ciy = 0;
        ciz = 0;

        Area += iTriA;
    }

    // centroid of the local scan, also equal to the position of the quad relative
    // to local scan
    ldata.pos_y = (cy/Area);//(cy/A);
    ldata.pos_z = (cz/Area);

    ldata.area = Area / (1000*1000);



} // end centroid

void Hokuyo_lidar::get_centroid2()
{
    double A = 0;
    double cy = 0;
    double cz = 0;

    for (int i=yz_start_pt; i <= yz_end_pt; i++)
    {
        A +=  (ldata.pc_y[i] * ldata.pc_z[i+1]) - (ldata.pc_y[i+1] * ldata.pc_z[i]);

        cy += (ldata.pc_y[i] + ldata.pc_y[i+1]) * (ldata.pc_y[i] * ldata.pc_z[i+1] - ldata.pc_y[i+1] * ldata.pc_z[i]);
        cz += (ldata.pc_z[i] + ldata.pc_z[i+1]) * (ldata.pc_y[i] * ldata.pc_z[i+1] - ldata.pc_y[i+1] * ldata.pc_z[i]);
        //cout << cy << endl;
    }

    A /= 2.0f;


    ldata.pos_y = cy/(6.0f * A);
    ldata.pos_z = cz/(6.0f * A);

    //cout << pos_loc_y2 << endl;

    A /= -(1000.0f*1000.0f);
    ldata.area = A;
    //cout << pos_loc_y2 << ' ' <<  A << endl;
}

void Hokuyo_lidar::get_symmetry_pt()
{
    yz_start_pt = 0;
    yz_end_pt = ldata.nyz - 1;

    // check if points are connected
    // left side
    for (int i = 20; i > 0; i--)
    {
        // 86mm is threshold, max vertical distance between points, assuming 10m range
        if (fabs(ldata.pc_z[i] - ldata.pc_z[i-1]) <= 86)
        {
            yz_start_pt = i - 1;
        }
        else    {break;}
    }

    // right side
    for (int i = ldata.nyz-21; i < ldata.nyz - 1; i++)
    {
        // 86mm is threshold, max vertical distance between points, assuming 10m range
        if (fabs(ldata.pc_z[i] - ldata.pc_z[i+1]) <= 86)
        {
            yz_end_pt = i + 1;
        }
        else    {break;}
    }

    // compare which side is lower
    if (ldata.pc_z[yz_start_pt] > ldata.pc_z[yz_end_pt])
    {
        for (int i=ldata.nyz-1; i > (int) ldata.nyz/2; i--)
        {
            if ((ldata.pc_z[i] >= ldata.pc_z[yz_start_pt]) && (ldata.pc_z[i-1] >= ldata.pc_z[yz_start_pt]))
            {
                yz_end_pt = i;
                break;
            }
        }
    }
    else if (ldata.pc_z[yz_start_pt] < ldata.pc_z[yz_end_pt])
    {
        for (int i=0; i < (int) ldata.nyz/2; i++)
        {
            if ((ldata.pc_z[i] >= ldata.pc_z[yz_end_pt]) && (ldata.pc_z[i+1] >= ldata.pc_z[yz_end_pt]))
            {
                yz_start_pt = i;

  break;
            }
        }
    }
}


int Hokuyo_lidar::lidar_check_outof_boundary()
{
/*
    TO DO:
        extra condition on the huge change in centroid
*/

    // trigger flag (flag=1) when out of boundary

    return 0;
}

/*
vector<int> Hokuyo_lidar::lonely_pts_detector()
{
  // output
  vector<int> I;

  // finding quantiles
  int m = median(range);

  vector<long> temp = range;

  sort(temp.begin(), temp.end());

  int m_idx = 0;

  for (int i=0;i<range.size();i++)
  {
    if (abs(temp[i]-m) < 10)  m_idx = i;// 10 is threshold
  }

  vector<long> v2(temp.begin(), temp.begin() + m_idx);
  vector<long> v3(temp.begin() + m_idx + 1, temp.end());

  int q2 = median(v2);
  int q3 = median(v3);
  int IQR = q3 - q2;

  int rmax = 1.5 * IQR + q3;
  int rmin = q2 - 1.5 * IQR;

  // removing the lonely points
  int c = 0;

  for (int i=0;i<range.size();i++)
  {
    if (ldata.range[i] > rmax || ldata.range[i] < rmin) I.push_back(i);
  }

  return I;
}
*/

void Hokuyo_lidar::wake()
{
    urg.wakeup();
}

void Hokuyo_lidar::sleep()
{
    urg.sleep();
}

void Hokuyo_lidar::close()
{
    urg.close();
}