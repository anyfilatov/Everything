#include "ros/ros.h"
#include "visualization_msgs/Marker.h"
#include "sensor_msgs/LaserScan.h"
#include "olson_scan_matcher.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include <math.h>


/*void from_file_to_scan(std::ifstream& scan_file, std::ifstream& odom_file, TransformedLaserScan& tls, double& angle_min, double& angle_max, double& angle_inc) {
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

}*/

void from_file_to_scan(std::ifstream& scan_file, TransformedLaserScan& tls,  double& angle_min, double& angle_max, double& angle_inc) {
  std::string scan_line;
  getline(scan_file,scan_line);
  std::stringstream scan_stream(scan_line);
  double range;
  bool is_occ;
  scan_stream >> angle_min >> angle_max >> angle_inc >> tls.d_x >> tls.d_y >> tls.d_yaw;
  int i = 0;
  while (scan_stream >> range) {
    scan_stream >> is_occ;
    tls.points.push_back({range, angle_min+angle_inc*i, is_occ});
    i++;
  }
}

int main(int argv, char** argc) {
  ros::init(argv, argc, "tester");

  ros::NodeHandle nh;
  ros::Publisher pub = nh.advertise<visualization_msgs::Marker>("/testmap",1);
  sleep(1);

  std::ifstream fin("/home/anton/base_scan.txt");
  std::ifstream fin_odom("/home/anton/odom.txt");
  std::string base_scan;
  /*getline(fin, base_scan);
  for (int i = 0; i < 300; i++) {
    getline(fin, base_scan);
    getline(fin_odom, base_scan);
  }*/
  //std::cout << base_scan << std::endl;
  double ang_min,ang_max,ang_inc;



  Olson_scan_matcher o_s_m(1.0, 0.5, 0.1);

  visualization_msgs::Marker msg;
  RobotState init_pose(0.0,0.0,0.0);
  RobotState pose_delta;
  TransformedLaserScan scan;
  GridMap map;
  from_file_to_scan(fin, scan, ang_min, ang_max, ang_inc);

  //first call
  o_s_m.process_scan(init_pose, scan, map, pose_delta);
  //init_pose.update(pose_delta.x, pose_delta.y, pose_delta.theta);

  msg.header.frame_id = "my_frame";
  msg.action = visualization_msgs::Marker::ADD;
  msg.type = visualization_msgs::Marker::POINTS;
  msg.color.r = 1.0;
  msg.color.g = 1.0;
  msg.color.b = 0.0;
  msg.color.a = 1.0;

  msg.scale.x = 2;
  msg.scale.y = 2;
  msg.id = 0;


  geometry_msgs::Point p;
  double min = 100000.0;
  double max = -1.0;
  for (auto point : scan.points) {
    p.x = point.range*std::cos(point.angle);
    p.y =  point.range*std::sin(point.angle);
    p.z = 0;
    msg.points.push_back(p);
  }


  pub.publish(msg);

  //msg.ranges.clear();

  visualization_msgs::Marker msg2;
  //second call
  TransformedLaserScan scan2;
  from_file_to_scan(fin, scan2, ang_min, ang_max, ang_inc);

  init_pose.update(scan2.d_x, scan2.d_y, scan2.d_yaw);
  o_s_m.process_scan(init_pose, scan2, map, pose_delta);
  init_pose.update(pose_delta.x, pose_delta.y, pose_delta.theta);

  ros::Publisher pub2 = nh.advertise<visualization_msgs::Marker>("/testmapp",1);
  sleep(1);
  msg2.header.frame_id = "my_frame";
  msg2.id = 1;
  msg2.action = visualization_msgs::Marker::ADD;
    msg2.type = visualization_msgs::Marker::POINTS;
    msg2.color.r = 1.0;
    msg2.color.g = 0.0;
    msg2.color.b = 1.0;
    msg2.color.a = 1.0;

    msg2.scale.x = 2;
    msg2.scale.y = 2;

    int i = 0;
    min = 100000.0;
    max = -1.0;
    for (auto point : scan2.points) {
      p.x = point.range*std::cos(point.angle + init_pose.theta) + init_pose.x;
      p.y = point.range*std::sin(point.angle + init_pose.theta) + init_pose.y;
      p.z = 0;
      msg2.points.push_back(p);
      //if(max < point.range) max = point.range;
      //if(min > point.range) min = point.range;
      //msg2.ranges.push_back(point.range);
    }


    std::cout << scan2.d_x << " " << scan2.d_y<< " " << scan2.d_yaw<< std::endl;
    std::cout << pose_delta.x << " " << pose_delta.y<< " " << pose_delta.theta<< std::endl;
    std::cout << init_pose.x << " " << init_pose.y<< " " << init_pose.theta<< std::endl;
    pub2.publish(msg2);

  //init_pose.update(pose_delta.x, pose_delta.y, pose_delta.theta);
  //fin.close();
  ros::spin();

  return 0;
}
