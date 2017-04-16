# turtlebot_parser

This package allows for easy converting of rosbags into a matlab format.
This allows for quick development times in matlab instead of directly implementing everything in c++.
To use this, ensure that all dependencies are installed (see package.xml).
One of the other key packages that this uses is the [apriltags_ros](https://github.com/RIVeR-Lab/apriltags_ros) package.
This has custom headers, so make sure it is clones and built in your workspace.


## matlab file format

* **data_april**
    * timestamp (seconds)
    * apriltag id
    * apriltag size (meters?)
    * x,y,z,w quaternion orientation
    * x,y,z 3d position of tag (meters)
* **data_imu**
    * timestamp (seconds)
    * x,y,z linear acceleration (m/s^2)
    * x,y,z angular velocity (rad/s)
* **data_odom**
    * timestamp (seconds)
    * x,y,z,w quaternion orientation
    * x,y,z position of robot (meters)
    * x,y,z linear acceleration (m/s^2)
    * x,y,z angular velocity (rad/s)
* **data_vicon**
    * timestamp (seconds)
    * x,y,z,w quaternion orientation
    * x,y,z position of robot (meters)