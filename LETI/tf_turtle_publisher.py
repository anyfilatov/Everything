#!/usr/bin/env python3
import math
import rclpy
from rclpy.node import Node
from rclpy.qos import QoSProfile, ReliabilityPolicy

from turtlesim.msg import Pose
from geometry_msgs.msg import TransformStamped
from tf2_ros import TransformBroadcaster

# Utility for yaw -> quaternion (fallback if tf_transformations not available)
try:
    from tf_transformations import quaternion_from_euler
except ImportError:
    def quaternion_from_euler(roll, pitch, yaw):
        """Simple conversion for 2D case (only yaw matters)."""
        return [0.0, 0.0, math.sin(yaw / 2.0), math.cos(yaw / 2.0)]


class TfTurtlePublisher(Node):
    """
    Listens to /turtle1/pose and /turtle2/pose, and broadcasts transforms:
    world -> turtle1_frame, world -> turtle2_frame.
    """

    def __init__(self):
        super().__init__('tf_turtle_publisher')

        # Parameters
        self.declare_parameter('turtle1_name', 'turtle1')
        self.declare_parameter('turtle2_name', 'turtle2')
        self.declare_parameter('world_frame', 'world')

        self.turtle1_name = self.get_parameter('turtle1_name').value
        self.turtle2_name = self.get_parameter('turtle2_name').value
        self.world_frame = self.get_parameter('world_frame').value

        # TF broadcaster
        self.tf_broadcaster = TransformBroadcaster(self)

        # QoS for reliable pose topics
        qos = QoSProfile(depth=10, reliability=ReliabilityPolicy.RELIABLE)

        # Subscribers
        self.create_subscription(
            Pose,
            f'/{self.turtle1_name}/pose',
            self._turtle1_pose_callback,
            qos
        )
        self.create_subscription(
            Pose,
            f'/{self.turtle2_name}/pose',
            self._turtle2_pose_callback,
            qos
        )

        self.get_logger().info('TfTurtlePublisher started, waiting for poses...')

    def _publish_transform(self, turtle_name: str, pose: Pose) -> None:
        """Create and broadcast a TransformStamped from world to turtle frame."""
        t = TransformStamped()
        t.header.stamp = self.get_clock().now().to_msg()
        t.header.frame_id = self.world_frame
        t.child_frame_id = f'{turtle_name}_frame'

        # Translation
        t.transform.translation.x = pose.x
        t.transform.translation.y = pose.y
        t.transform.translation.z = 0.0

        # Rotation (yaw only)
        quat = quaternion_from_euler(0.0, 0.0, pose.theta)
        t.transform.rotation.x = quat[0]
        t.transform.rotation.y = quat[1]
        t.transform.rotation.z = quat[2]
        t.transform.rotation.w = quat[3]

        self.tf_broadcaster.sendTransform(t)

    def _turtle1_pose_callback(self, msg: Pose) -> None:
        self._publish_transform(self.turtle1_name, msg)

    def _turtle2_pose_callback(self, msg: Pose) -> None:
        self._publish_transform(self.turtle2_name, msg)


def main(args=None):
    rclpy.init(args=args)
    node = TfTurtlePublisher()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
