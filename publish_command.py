#!/usr/bin/env python
import rospy
import math
from duckietown_msgs.msg import Twist2DStamped

if __name__ == '__main__':
  rospy.init_node('pub')
  pub = rospy.Publisher("commander/car_cmd", Twist2DStamped, queue_size=10)

  t = 1;
  rate=rospy.Rate(10)
  while not rospy.is_shutdown():
    msg = Twist2DStamped()
    msg.v = 1
    msg.omega = t
    t=-t

    pub.publish(msg)
    rate.sleep()
