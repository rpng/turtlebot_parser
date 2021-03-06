#include <string>
#include <iostream>
#include <ros/ros.h>
#include <ros/topic.h>
#include <rosbag/bag.h>
#include <rosbag/view.h>
#include <nav_msgs/Odometry.h>
#include <geometry_msgs/TransformStamped.h>
#include <sensor_msgs/Imu.h>
#include <cv_bridge/cv_bridge.h>
#include <apriltags_ros/AprilTagDetectionArray.h>

#include <boost/filesystem.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>


// Matlab header, this is needed to save mat files
// Note that we use the FindMatlab.cmake to get this
#include "mat.h"

using namespace cv;
using namespace std;

int main(int argc, char **argv) {

    // Debug message
    ROS_INFO("Starting up");

    // Check if there is a path to a dataset
    if(argc < 2) {
        ROS_ERROR("Error please specify a rosbag file");
        ROS_ERROR("Command Example: ./turtlebot_parser <rosbag>");
        return EXIT_FAILURE;
    }

    // Startup this node
    ros::init(argc, argv, "turtlebot_parser");

    // Parse the input
    string pathBag = argv[1];

    // Get path
    boost::filesystem::path p(pathBag);
    string pathParent = p.parent_path().c_str();
    string pathMat = pathParent + "/" + p.stem().c_str() + ".mat";


    // Load rosbag here, and find messages we can play
    rosbag::Bag bag;
    bag.open(pathBag, rosbag::bagmode::Read);


    // We should load the bag as a view
    // Here we go from beginning of the bag to the end of the bag
    rosbag::View view(bag);

    // Debug
    ROS_INFO("BAG Path is: %s", pathBag.c_str());
    ROS_INFO("MAT Path is: %s", pathMat.c_str());
    ROS_INFO("Reading in rosbag file...");


    // Create the matlab mat file
    MATFile *pmat = matOpen(pathMat.c_str(), "w");
    if (pmat == NULL) {
        ROS_ERROR("Error could not create the mat file");
        return(EXIT_FAILURE);
    }

    // Our data vector
    vector<double> dataIMU = vector<double>();
    vector<double> dataVICON = vector<double>();
    vector<double> dataODOM = vector<double>();
    vector<vector<double>> dataAPRIl = vector<vector<double>>();

    // Step through the rosbag and send to algo methods
    for (const rosbag::MessageInstance& m : view) {

        // Handle IMU message
        sensor_msgs::Imu::ConstPtr s1 = m.instantiate<sensor_msgs::Imu>();
        if (s1 != NULL && m.getTopic() == "/mobile_base/sensors/imu_data_raw") {
            dataIMU.push_back(m.getTime().toSec());
            dataIMU.push_back(s1->linear_acceleration.x);
            dataIMU.push_back(s1->linear_acceleration.y);
            dataIMU.push_back(s1->linear_acceleration.z);
            dataIMU.push_back(s1->angular_velocity.x);
            dataIMU.push_back(s1->angular_velocity.y);
            dataIMU.push_back(s1->angular_velocity.z);
        }

        // Handle vicon poses
        geometry_msgs::TransformStamped::ConstPtr s2 = m.instantiate<geometry_msgs::TransformStamped>();
        if (s2 != NULL && m.getTopic() == "/vicon/turtlebot/body") {
            dataVICON.push_back(m.getTime().toSec());
            dataVICON.push_back(s2->transform.rotation.x);
            dataVICON.push_back(s2->transform.rotation.y);
            dataVICON.push_back(s2->transform.rotation.z);
            dataVICON.push_back(s2->transform.rotation.w);
            dataVICON.push_back(s2->transform.translation.x);
            dataVICON.push_back(s2->transform.translation.y);
            dataVICON.push_back(s2->transform.translation.z);
        }

        // Handle turtlebot odomentry
        nav_msgs::Odometry::ConstPtr s3 = m.instantiate<nav_msgs::Odometry>();
        if (s3 != NULL && m.getTopic() == "/odom") {
            dataODOM.push_back(m.getTime().toSec());
            dataODOM.push_back(s3->pose.pose.orientation.x);
            dataODOM.push_back(s3->pose.pose.orientation.y);
            dataODOM.push_back(s3->pose.pose.orientation.z);
            dataODOM.push_back(s3->pose.pose.orientation.w);
            dataODOM.push_back(s3->pose.pose.position.x);
            dataODOM.push_back(s3->pose.pose.position.y);
            dataODOM.push_back(s3->pose.pose.position.z);
            dataODOM.push_back(s3->twist.twist.angular.x);
            dataODOM.push_back(s3->twist.twist.angular.y);
            dataODOM.push_back(s3->twist.twist.angular.z);
            dataODOM.push_back(s3->twist.twist.linear.x);
            dataODOM.push_back(s3->twist.twist.linear.y);
            dataODOM.push_back(s3->twist.twist.linear.z);
        }

        // Handle turtlebot apriltag_ros detections
        apriltags_ros::AprilTagDetectionArray::ConstPtr s4 = m.instantiate<apriltags_ros::AprilTagDetectionArray>();
        if (s4 != NULL && m.getTopic() == "/tag_detections") {
            // Loop through each detection (they have the same timestamp)
            for(const apriltags_ros::AprilTagDetection& tag : s4->detections) {
                vector<double> temp = vector<double>();
                temp.push_back(m.getTime().toSec());
                //todo figure out why i need to do this with the ID
                double idt = tag.id;
                temp.push_back(idt);
                temp.push_back(tag.size);
                temp.push_back(tag.pose.pose.orientation.x);
                temp.push_back(tag.pose.pose.orientation.y);
                temp.push_back(tag.pose.pose.orientation.z);
                temp.push_back(tag.pose.pose.orientation.w);
                temp.push_back(tag.pose.pose.position.x);
                temp.push_back(tag.pose.pose.position.y);
                temp.push_back(tag.pose.pose.position.z);
                dataAPRIl.push_back(temp);
            }
        }

    }

    // Debug message
    ROS_INFO("Done processing bag");

    // ====================================================================
    // ==========              IMU DATA                  ==================
    // ====================================================================
    mxArray *pa1 = mxCreateDoubleMatrix(dataIMU.size()/7,7,mxREAL);
    if (pa1 == NULL) {
        printf("%s : Out of memory on line %d\n", __FILE__, __LINE__);
        printf("Unable to create mxArray.\n");
        return(EXIT_FAILURE);
    }
    // Correctly copy data over (column-wise)
    double* pt1 = mxGetPr(pa1);
    for(size_t i=0; i<dataIMU.size(); i+=7) {
        pt1[i/7] = dataIMU.at(i);
        pt1[(i + dataIMU.size())/7] = dataIMU.at(i+1);
        pt1[(i + 2*dataIMU.size())/7] = dataIMU.at(i+2);
        pt1[(i + 3*dataIMU.size())/7] = dataIMU.at(i+3);
        pt1[(i + 4*dataIMU.size())/7] = dataIMU.at(i+4);
        pt1[(i + 5*dataIMU.size())/7] = dataIMU.at(i+5);
        pt1[(i + 6*dataIMU.size())/7] = dataIMU.at(i+6);
    }
    // Add it to the matlab mat file
    int status = matPutVariable(pmat, "data_imu", pa1);
    if(status != 0) {
        printf("%s :  Error using matPutVariable on line %d\n", __FILE__, __LINE__);
        return(EXIT_FAILURE);
    }
    // Cleanup
    mxDestroyArray(pa1);
    ROS_INFO("Done processing IMU data");


    // ====================================================================
    // ==========            VICON DATA                  ==================
    // ====================================================================
    pa1 = mxCreateDoubleMatrix(dataVICON.size()/8,8,mxREAL);
    if (pa1 == NULL) {
        printf("%s : Out of memory on line %d\n", __FILE__, __LINE__);
        printf("Unable to create mxArray.\n");
        return(EXIT_FAILURE);
    }
    // Correctly copy data over (column-wise)
    pt1 = mxGetPr(pa1);
    for(size_t i=0; i<dataVICON.size(); i+=8) {
        pt1[i/8] = dataVICON.at(i);
        pt1[(i + dataVICON.size())/8] = dataVICON.at(i+1);
        pt1[(i + 2*dataVICON.size())/8] = dataVICON.at(i+2);
        pt1[(i + 3*dataVICON.size())/8] = dataVICON.at(i+3);
        pt1[(i + 4*dataVICON.size())/8] = dataVICON.at(i+4);
        pt1[(i + 5*dataVICON.size())/8] = dataVICON.at(i+5);
        pt1[(i + 6*dataVICON.size())/8] = dataVICON.at(i+6);
        pt1[(i + 7*dataVICON.size())/8] = dataVICON.at(i+7);
    }
    // Add it to the matlab mat file
    status = matPutVariable(pmat, "data_vicon", pa1);
    if(status != 0) {
        printf("%s :  Error using matPutVariable on line %d\n", __FILE__, __LINE__);
        return(EXIT_FAILURE);
    }
    // Cleanup
    mxDestroyArray(pa1);
    ROS_INFO("Done processing VICON data");


    // ====================================================================
    // ==========             ODOM DATA                  ==================
    // ====================================================================
    pa1 = mxCreateDoubleMatrix(dataODOM.size()/14,14,mxREAL);
    if (pa1 == NULL) {
        printf("%s : Out of memory on line %d\n", __FILE__, __LINE__);
        printf("Unable to create mxArray.\n");
        return(EXIT_FAILURE);
    }
    // Correctly copy data over (column-wise)
    pt1 = mxGetPr(pa1);
    for(size_t i=0; i<dataODOM.size(); i+=14) {
        pt1[i/14] = dataODOM.at(i);
        pt1[(i + 1*dataODOM.size())/14] = dataODOM.at(i+1);
        pt1[(i + 2*dataODOM.size())/14] = dataODOM.at(i+2);
        pt1[(i + 3*dataODOM.size())/14] = dataODOM.at(i+3);
        pt1[(i + 4*dataODOM.size())/14] = dataODOM.at(i+4);
        pt1[(i + 5*dataODOM.size())/14] = dataODOM.at(i+5);
        pt1[(i + 6*dataODOM.size())/14] = dataODOM.at(i+6);
        pt1[(i + 7*dataODOM.size())/14] = dataODOM.at(i+7);
        pt1[(i + 8*dataODOM.size())/14] = dataODOM.at(i+8);
        pt1[(i + 9*dataODOM.size())/14] = dataODOM.at(i+9);
        pt1[(i + 10*dataODOM.size())/14] = dataODOM.at(i+10);
        pt1[(i + 11*dataODOM.size())/14] = dataODOM.at(i+11);
        pt1[(i + 12*dataODOM.size())/14] = dataODOM.at(i+12);
        pt1[(i + 13*dataODOM.size())/14] = dataODOM.at(i+13);
    }
    // Add it to the matlab mat file
    status = matPutVariable(pmat, "data_odom", pa1);
    if(status != 0) {
        printf("%s :  Error using matPutVariable on line %d\n", __FILE__, __LINE__);
        return(EXIT_FAILURE);
    }
    // Cleanup
    mxDestroyArray(pa1);
    ROS_INFO("Done processing ODOM data");


    // ====================================================================
    // ==========             APRIL DATA                 ==================
    // ====================================================================
    pa1 = mxCreateDoubleMatrix(dataAPRIl.size(),10,mxREAL);
    if (pa1 == NULL) {
        printf("%s : Out of memory on line %d\n", __FILE__, __LINE__);
        printf("Unable to create mxArray.\n");
        return(EXIT_FAILURE);
    }
    // Correctly copy data over (column-wise)
    pt1 = mxGetPr(pa1);
    for(size_t i=0; i<dataAPRIl.size(); i++) {
        pt1[i] = dataAPRIl.at(i).at(0);
        pt1[i + 1*dataAPRIl.size()] = dataAPRIl.at(i).at(1);
        pt1[i + 2*dataAPRIl.size()] = dataAPRIl.at(i).at(2);
        pt1[i + 3*dataAPRIl.size()] = dataAPRIl.at(i).at(3);
        pt1[i + 4*dataAPRIl.size()] = dataAPRIl.at(i).at(4);
        pt1[i + 5*dataAPRIl.size()] = dataAPRIl.at(i).at(5);
        pt1[i + 6*dataAPRIl.size()] = dataAPRIl.at(i).at(6);
        pt1[i + 7*dataAPRIl.size()] = dataAPRIl.at(i).at(7);
        pt1[i + 8*dataAPRIl.size()] = dataAPRIl.at(i).at(8);
        pt1[i + 9*dataAPRIl.size()] = dataAPRIl.at(i).at(9);
    }
    // Add it to the matlab mat file
    status = matPutVariable(pmat, "data_april", pa1);
    if(status != 0) {
        printf("%s :  Error using matPutVariable on line %d\n", __FILE__, __LINE__);
        return(EXIT_FAILURE);
    }
    // Cleanup
    mxDestroyArray(pa1);
    ROS_INFO("Done processing APRILTAG data");


    // Close the mat file
    if (matClose(pmat) != 0) {
        ROS_ERROR("Error closing the mat file");
        return(EXIT_FAILURE);
    }
    return EXIT_SUCCESS;
}



