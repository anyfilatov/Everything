#!/usr/bin/env python3
import math
import random
import rclpy
from rclpy.node import Node
from rclpy.qos import QoSProfile, ReliabilityPolicy

from geometry_msgs.msg import Twist, TransformStamped, Vector3Stamped
from tf2_ros import Buffer, TransformListener, TransformException
from tf2_geometry_msgs import do_transform_vector3

# For quaternion to yaw conversion
try:
    from tf_transformations import euler_from_quaternion
except ImportError:
    def euler_from_quaternion(q):
        """
        Simplified for 2D: extract yaw from a quaternion.
        Expects quat as [x, y, z, w] (list or tuple).
        """
        x, y, z, w = q
        siny_cosp = 2.0 * (w * z + x * y)
        cosy_cosp = 1.0 - 2.0 * (y * y + z * z)
        yaw = math.atan2(siny_cosp, cosy_cosp)
        return 0.0, 0.0, yaw  # roll, pitch, yaw


class FleeController(Node):
    """
    Uses TF to get the position of turtle1 in turtle2's frame.
    Commands turtle2 to flee from turtle1.
    """

    def __init__(self):
        super().__init__('flee_controller')

        # Parameters
        self.declare_parameter('turtle1_frame', 'turtle1_frame')
        self.declare_parameter('turtle2_frame', 'turtle2_frame')
        self.declare_parameter('world_frame', 'world')
        self.declare_parameter('flee_threshold', 3.0)        # distance below which fleeing is active
        self.declare_parameter('max_linear_speed', 2.0)      # m/s when very close
        self.declare_parameter('min_linear_speed', 0.2)      # m/s when far away
        self.declare_parameter('angular_gain', 4.0)          # proportional gain for rotation
        self.declare_parameter('max_angular_speed', 3.0)     # rad/s
        self.declare_parameter('random_direction_distance', 0.5)  # below this, pick random heading
        self.declare_parameter('control_rate', 10.0)          # Hz
        self.declare_parameter('boundary_min', 1.0)
        self.declare_parameter('boundary_max', 10.0)
        self.declare_parameter('boundary_gain', 1.0)		# amount of bounce from a border
        self.declare_parameter('boundary_margin', 0.5)		# distance to border

        self.turtle1_frame = self.get_parameter('turtle1_frame').value
        self.turtle2_frame = self.get_parameter('turtle2_frame').value
        self.world_frame = self.get_parameter('world_frame').value
        self.flee_threshold = self.get_parameter('flee_threshold').value
        self.max_linear = self.get_parameter('max_linear_speed').value
        self.min_linear = self.get_parameter('min_linear_speed').value
        self.angular_gain = self.get_parameter('angular_gain').value
        self.max_angular = self.get_parameter('max_angular_speed').value
        self.random_dist = self.get_parameter('random_direction_distance').value
        self.control_rate = self.get_parameter('control_rate').value
        self.boundary_min = self.get_parameter('boundary_min').value
        self.boundary_max = self.get_parameter('boundary_max').value
        self.boundary_gain = self.get_parameter('boundary_gain').value
        self.boundary_margin = self.get_parameter('boundary_margin').value

        # TF buffer and listener
        self.tf_buffer = Buffer()
        self.tf_listener = TransformListener(self.tf_buffer, self)

        # Publisher for turtle2 velocity commands
        qos = QoSProfile(depth=10, reliability=ReliabilityPolicy.RELIABLE)
        self.cmd_vel_pub = self.create_publisher(Twist, '/turtle2/cmd_vel', qos)

        # Control timer
        self.create_timer(1.0 / self.control_rate, self.control_loop)

        self.get_logger().info('FleeController started, waiting for TF...')

    # -------------------------------------------------------------------------
    # TF helpers
    # -------------------------------------------------------------------------
    def _get_relative_turtle_position(self):
        """
        Lookup transform from turtle2_frame to turtle1_frame.
        Returns (dx, dy, distance) or None if lookup fails.
        """
        try:
            transform = self.tf_buffer.lookup_transform(
                target_frame=self.turtle2_frame,
                source_frame=self.turtle1_frame,
                time=rclpy.time.Time()
            )
        except TransformException as ex:
            self.get_logger().debug(f'TF lookup failed: {ex}', throttle_duration_sec=2.0)
            return None

        dx = transform.transform.translation.x
        dy = transform.transform.translation.y
        distance = math.hypot(dx, dy)
        return dx, dy, distance

    def _get_turtle2_world_position(self):
        """Return (x, y) of turtle2 in world frame, or None if lookup fails."""
        try:
            transform = self.tf_buffer.lookup_transform(
                target_frame=self.world_frame,
                source_frame=self.turtle2_frame,
                time=rclpy.time.Time()
            )
            return (transform.transform.translation.x, transform.transform.translation.y)
        except TransformException as ex:
            self.get_logger().debug(f'Turtle2 world position lookup failed: {ex}', throttle_duration_sec=2.0)
            return None

    def _world_vector_to_local(self, vx, vy, target_frame):
        """
        Transform a vector (vx, vy) from world frame to target_frame.
        Returns (local_x, local_y) or (None, None) on failure.
        """
        vec = Vector3Stamped()
        vec.header.frame_id = self.world_frame
        vec.header.stamp = self.get_clock().now().to_msg()
        vec.vector.x = vx
        vec.vector.y = vy
        vec.vector.z = 0.0

        try:
            # Получаем трансформацию между фреймами
            transform = self.tf_buffer.lookup_transform(
                target_frame,
                self.world_frame,
                rclpy.time.Time()
            )
            # Применяем трансформацию к вектору
            transformed = do_transform_vector3(vec, transform)
            return (transformed.vector.x, transformed.vector.y)
        except TransformException as ex:
            self.get_logger().debug(f'Vector transform failed: {ex}', throttle_duration_sec=2.0)
            return (None, None)

    # -------------------------------------------------------------------------
    # Force computation
    # -------------------------------------------------------------------------
    def _compute_escape_local(self, dx, dy, distance):
        """
        Compute escape vector in turtle2 local coordinates.
        If distance is very small (< random_dist), returns a random unit vector.
        Otherwise returns the opposite direction of (dx, dy) (unit vector).
        """
        if distance < 1e-3:
            return (0.0, 0.0)

        if distance < self.random_dist:
            # Random direction
            angle = random.uniform(-math.pi, math.pi)
            return (math.cos(angle), math.sin(angle))
        else:
            # Flee: opposite direction
            norm = math.hypot(dx, dy)
            if norm < 1e-3:
                return (0.0, 0.0)
            return (-dx / norm, -dy / norm)

    def _compute_boundary_force_world(self, x, y):
        """
        Compute repulsion force (bx, by) in world coordinates.
        Returns (bx, by).
        """
        bx, by = 0.0, 0.0
        if x < self.boundary_min + self.boundary_margin:
            bx = (self.boundary_min + self.boundary_margin - x) * self.boundary_gain
        elif x > self.boundary_max - self.boundary_margin:
            bx = (self.boundary_max - self.boundary_margin - x) * self.boundary_gain

        if y < self.boundary_min + self.boundary_margin:
            by = (self.boundary_min + self.boundary_margin - y) * self.boundary_gain
        elif y > self.boundary_max - self.boundary_margin:
            by = (self.boundary_max - self.boundary_margin - y) * self.boundary_gain

        return bx, by

    def _combine_vectors(self, v1, v2):
        """ Sum two 2D vectors. """

        return (v1[0] + v2[0], v1[1] + v2[1])

    # -------------------------------------------------------------------------
    # Speed computation
    # -------------------------------------------------------------------------
    def _compute_desired_heading(self, fx, fy):
        """Given a vector in local coordinates, return the desired heading angle."""

        if abs(fx) < 1e-3 and abs(fy) < 1e-3:
            return None
        return math.atan2(fy, fx)

    def _compute_angular_speed(self, desired_heading):
        """
        Compute angular speed from desired heading (in local frame).
        Since current heading in local frame is always 0, error = desired_heading.
        """
        if desired_heading is None:
            return 0.0

        angle_error = desired_heading
        # Normalize to [-pi, pi]
        angle_error = math.atan2(math.sin(angle_error), math.cos(angle_error))

        angular_speed = self.angular_gain * angle_error
        # Clamp
        return max(-self.max_angular, min(self.max_angular, angular_speed))
        

    def _compute_linear_speed(self, distance):
        """
        Linear speed based on distance to turtle1.
        Speed = max_linear * (threshold / max(distance, threshold)), clamped.
        """
        if distance < 1e-3:
            return 0.0

        raw_speed = self.max_linear * (self.flee_threshold / max(distance, self.flee_threshold))
        linear_speed = max(self.min_linear, min(self.max_linear, raw_speed))
        return linear_speed

    # -------------------------------------------------------------------------
    # Main control loop
    # -------------------------------------------------------------------------
    def control_loop(self):
        # Get relative position of turtle1 in turtle2 frame
        rel = self._get_relative_turtle_position()
        if rel is None:
            return
        dx, dy, distance = rel

        if distance < 1e-3:
            self._publish_zero_twist()
            return

        # Get turtle2 world pose (for boundaries)
        pose = self._get_turtle2_world_position()
        if pose is None:
            return
        x2, y2 = pose

        # Compute boundary repulsion in world coordinates
        bx_world, by_world = self._compute_boundary_force_world(x2, y2)

        # Convert boundary vector to turtle2 local frame using TF
        if abs(bx_world) > 1e-6 or abs(by_world) > 1e-6:
            local_bx, local_by = self._world_vector_to_local(bx_world, by_world, self.turtle2_frame)
            if local_bx is None or local_by is None:
                local_bx, local_by = 0.0, 0.0
        else:
            local_bx, local_by = 0.0, 0.0

        # Compute escape (or random) vector in turtle2 local frame
        escape_local = self._compute_escape_local(dx, dy, distance)

        # Combine vectors
        final_local = self._combine_vectors(escape_local, (local_bx, local_by))

        # Compute desired heading and angular speed
        desired = self._compute_desired_heading(final_local[0], final_local[1])
        angular = self._compute_angular_speed(desired)

        # Compute linear speed
        linear = self._compute_linear_speed(distance)
        
        # Publish
        twist = Twist()
        twist.linear.x = linear
        twist.angular.z = angular
        self.cmd_vel_pub.publish(twist)

    def _publish_zero_twist(self):
        twist = Twist()
        self.cmd_vel_pub.publish(twist)


def main(args=None):
    rclpy.init(args=args)
    node = FleeController()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
