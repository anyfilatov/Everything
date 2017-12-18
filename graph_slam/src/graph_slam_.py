#!/usr/bin/env python

import math
import rospy, tf
import geometry_msgs
from graph_slam.msg import ScanWithPose
from geometry_msgs.msg import Pose2D
from sensor_msgs.msg import LaserScan
from nav_msgs.msg import OccupancyGrid

i = 0
class GraphNode:
    def __init__(self, scan, cur_pose, prev_pose):
        global i
        self.scan = scan
        self.pose = cur_pose
        self.edge = Pose2D(cur_pose.x - prev_pose.x, cur_pose.y - prev_pose.y, cur_pose.theta - prev_pose.theta)
        i += 1 
        print str(i) + " " + str(cur_pose.x) + " " + str(cur_pose.y) + " " + str(cur_pose.theta) + "\n"

graph=[]

class GridMap:
    def __init__(self):
        self.grid=[-1]*1000000
        self.map_center_x = 500
        self.map_center_y = 500
        self.map_resolution = 0.1

    def point_to_grid(self, x, y):
        # x and y - are positive or negative cartesian coordinates 
        return (round(x/self.map_resolution) + self.map_center_x), (round(y/self.map_resolution) + self.map_center_y)*1000
    
    def scan_to_map(self, robot_pose, scan):
        c, s = math.cos(robot_pose.theta), math.sin(robot_pose.theta)
        cur_angle = scan.angle_min
        for dist in scan.ranges:
            obst_x = dist*math.cos(cur_angle)
            obst_y = dist*math.sin(cur_angle)

            robot_scan_x = c * obst_x - s * obst_y
            robot_scan_y = s * obst_x + c * obst_y

            cur_angle += scan.angle_increment
            grid_x, grid_y = self.point_to_grid(robot_pose.x + robot_scan_x, robot_pose.y + robot_scan_y)
            self.grid[int(round(grid_x + grid_y))] = 100
            
    
    def convert_to_occ_grid(self):
        res = OccupancyGrid()
        res.header.stamp = rospy.Time.now()
        res.header.frame_id = "odom_combined"
        res.info.resolution = 0.1
        res.info.width = 1000
        res.info.height = 1000
        res.info.origin.position.x = -50
        res.info.origin.position.y = -50
        res.data = self.grid
        return res

#def create_map(laser_scan)

def save_scan(data):
     if len(graph) == 0:
         new_node = GraphNode(data.scan, data.position, Pose2D(0,0,0))
     else:
         new_node = GraphNode(data.scan, data.position, graph[-1].pose)
     graph.append(new_node)

if __name__ == '__main__':
    rospy.init_node('graph_slam', anonymous=True, log_level=rospy.INFO)
    rospy.Subscriber('/scan_with_pose', ScanWithPose, save_scan)
    rate = rospy.Rate(100)
    pub = rospy.Publisher("/mmm", LaserScan, queue_size=1)
    map_pub = rospy.Publisher("/map", OccupancyGrid, queue_size = 1)
    br = tf.TransformBroadcaster()
    graph_map = GridMap()
    while not rospy.is_shutdown():
        if len(graph) == 0:
            continue
        lastNode = graph[-1]
        pub.publish(lastNode.scan)
        graph_map.scan_to_map(lastNode.pose, lastNode.scan)
        cur_map = OccupancyGrid();
        cur_map = graph_map.convert_to_occ_grid()
        map_pub.publish(cur_map)
        br.sendTransform((lastNode.pose.x,lastNode.pose.y, 0), 
                          tf.transformations.quaternion_from_euler(0, 0, -lastNode.pose.theta),
                          rospy.Time.now(),
                          "bot", "base_laser_link")
        rate.sleep()
