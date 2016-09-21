#include "ros/ros.h"
#include "sensor_msgs/LaserScan.h"
#include "olson_scan_matcher.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>


void from_file_to_scan(std::ifstream& scan_file, std::ifstream& odom_file, TransformedLaserScan& tls, double& angle_min, double& angle_max, double& angle_inc) {
  std::string scan_line;
  std::string odom_line;
  getline(scan_file,scan_line);
  getline(odom_file,odom_line);
  std::stringstream scan_stream(scan_line);
  std::stringstream odom_stream(odom_line);
  scan_stream >> angle_min >> angle_max >> angle_inc;
  //std::cout << scan_line << std::endl;
  //std::cout << angle_min << " " <<angle_max << std::endl;

  double range;
  int i = 0;
  while (scan_stream >> range) {
    tls.points.push_back({range, angle_min+angle_inc*i});
    i++;
  }
  odom_stream >> tls.d_x >> tls.d_y >> tls.d_yaw;

}

int main(int argv, char** argc) {
  ros::init(argv, argc, "tester");

  ros::NodeHandle nh;
  ros::Publisher pub = nh.advertise<sensor_msgs::LaserScan>("/testmap",1);
  sleep(1);

  std::ifstream fin("/home/anton/base_scan.txt");
  std::ifstream fin_odom("/home/anton/odom.txt");
  std::string base_scan;
  getline(fin, base_scan);
  for (int i = 0; i < 735; i++) {
    getline(fin, base_scan);
    getline(fin_odom, base_scan);
  }
  //std::cout << base_scan << std::endl;
  double ang_min,ang_max,ang_inc;



  Olson_scan_matcher o_s_m(1.0, 0.5, 0.1);

  sensor_msgs::LaserScan msg;
  RobotState init_pose(0.0,0.0,0.0);
  RobotState pose_delta;
  TransformedLaserScan scan;
  GridMap map;
  from_file_to_scan(fin, fin_odom, scan, ang_min, ang_max, ang_inc);

  //first call
  o_s_m.process_scan(init_pose, scan, map, pose_delta);
  //init_pose.update(pose_delta.x, pose_delta.y, pose_delta.theta);

  msg.header.frame_id = "my_frame";
  msg.angle_increment = ang_inc;
  msg.angle_min = ang_min;
  msg.angle_max = ang_max;

  double min = 100000.0;
  double max = -1.0;
  for (auto point : scan.points) {
    if(max < point.range) max = point.range;
    if(min > point.range) min = point.range;
    msg.ranges.push_back(point.range);
  }

  msg.range_min = min;
  msg.range_max = max;
  pub.publish(msg);

  //msg.ranges.clear();

  sensor_msgs::LaserScan msg2;
  //second call
  TransformedLaserScan scan2;
  from_file_to_scan(fin, fin_odom, scan2, ang_min, ang_max, ang_inc);

  init_pose.update(scan2.d_x, scan2.d_y, scan2.d_yaw);
  o_s_m.process_scan(init_pose, scan2, map, pose_delta);
  //init_pose.update(pose_delta.x, pose_delta.y, pose_delta.theta);

  ros::Publisher pub2 = nh.advertise<sensor_msgs::LaserScan>("/testmapp",1);
  sleep(1);
  msg2.header.frame_id = "my_frame";
    msg2.angle_increment = ang_inc;
    msg2.angle_min = ang_min + init_pose.theta;
    msg2.angle_max = ang_max + init_pose.theta;

    int i = 0;
    min = 100000.0;
    max = -1.0;
    for (auto point : scan2.points) {
      double x = point.range * std::cos(point.angle);
      double d_x = x - init_pose.x;
      double y = point.range * std::sin(point.angle);
      double d_y = y - init_pose.y;
      msg2.ranges.push_back(std::sqrt(d_x*d_x + d_y*d_y));
      if(max < std::sqrt(d_x*d_x + d_y*d_y)) max = std::sqrt(d_x*d_x + d_y*d_y);
      if(min > std::sqrt(d_x*d_x + d_y*d_y)) min = std::sqrt(d_x*d_x + d_y*d_y);
      i++;
      //msg2.ranges.push_back(point.range);
    }

    msg2.range_min = min;
    msg2.range_max = max;
    std::cout << scan2.d_x << " " << scan2.d_y<< " " << scan2.d_yaw<< std::endl;
    std::cout << pose_delta.x << " " << pose_delta.y<< " " << pose_delta.theta<< std::endl;
    pub2.publish(msg2);

  //init_pose.update(pose_delta.x, pose_delta.y, pose_delta.theta);
  //fin.close();
  ros::spin();

  return 0;
}
