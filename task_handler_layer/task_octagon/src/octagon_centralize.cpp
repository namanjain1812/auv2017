// Copyright 2016 AUV-IITK
#include <cv.h>
#include <highgui.h>
#include <ros/ros.h>
#include "std_msgs/String.h"
#include "std_msgs/Int8.h"
#include <fstream>
#include <vector>
#include <std_msgs/Bool.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <opencv/highgui.h>
#include <image_transport/image_transport.h>
#include <dynamic_reconfigure/server.h>
#include <task_octagon/octagonConfig.h>
#include "std_msgs/Float32MultiArray.h"
#include <cv_bridge/cv_bridge.h>
#include <sstream>
#include <string>
#include <std_msgs/Float64MultiArray.h>

bool IP = true;
bool flag = false;
bool video = false;
cv::Mat frame;
cv::Mat newframe;
int count = 0, count_avg = 0;
int t1min, t1max, t2min, t2max, t3min, t3max;

void callback(task_octagon::octagonConfig &config, double level)
{
  t1min = config.t1min_param;
  t1max = config.t1max_param;
  t2min = config.t2min_param;
  t2max = config.t2max_param;
  t3min = config.t3min_param;
  t3max = config.t3max_param;
  ROS_INFO("centralize_Reconfigure Request:New params: %d %d %d %d %d %d ", t1min, t1max, t2min, t2max, t3min, t3max);
}

float mod(float x, float y)
{
  if (x - y > 0)
    return x;
  else
    return y;
}
void Switch_callback(std_msgs::Bool msg)
{
  IP = msg.data;
}

void imageCallback(const sensor_msgs::ImageConstPtr &msg)
{
  try
  {
    count++;
    newframe = cv_bridge::toCvShare(msg, "bgr8")->image;
    cvNamedWindow("newframe", CV_WINDOW_NORMAL);
    ///////////////////////////// DO NOT REMOVE THIS, IT COULD BE INGERIOUS TO HEALTH /////////////////////
    newframe.copyTo(frame);
    cv::imshow("newframe", newframe);
    ////////////////////////// FATAL ///////////////////////////////////////////////////
  }
  catch (cv_bridge::Exception &e)
  {
    ROS_ERROR("Could not convert from '%s' to 'bgr8'.", msg->encoding.c_str());
  }
}

int main(int argc, char *argv[])
{
  int height, width, step, channels;  // parameters of the image we are working on
  cv::Scalar color(255, 255, 255);
  std::string Video_Name = "Random_Video";
  if (argc >= 2)
    flag = true;
  if (argc == 3)
  {
    video = true;
    std::string avi = ".avi";
    Video_Name = (argv[2]) + avi;
  }

  cv::VideoWriter output_cap(Video_Name, CV_FOURCC('D', 'I', 'V', 'X'), 9, cv::Size(640, 480));

  ros::init(argc, argv, "octagon_centralize");
  ros::NodeHandle n;
  ros::Publisher pub = n.advertise<std_msgs::Float64MultiArray>("/varun/ip/octagon_centralize", 1000);
  ros::Subscriber sub = n.subscribe<std_msgs::Bool>("octagon_centralize_switch", 1000, &Switch_callback);
  ros::Rate loop_rate(10);

  image_transport::ImageTransport it(n);
  image_transport::Subscriber sub1 = it.subscribe("/varun/sensors/bottom_camera/image_raw", 1, imageCallback);

  dynamic_reconfigure::Server<task_octagon::octagonConfig> server;
  dynamic_reconfigure::Server<task_octagon::octagonConfig>::CallbackType f;
  f = boost::bind(&callback, _1, _2);
  server.setCallback(f);

  n.getParam("octagon_centralize/t1max", t1max);
  n.getParam("octagon_centralize/t1min", t1min);
  n.getParam("octagon_centralize/t2max", t2max);
  n.getParam("octagon_centralize/t2min", t2min);
  n.getParam("octagon_centralize/t3max", t3max);
  n.getParam("octagon_centralize/t3min", t3min);

  task_octagon::octagonConfig config;
  config.t1min_param = t1min;
  config.t1max_param = t1max;
  config.t2min_param = t2min;
  config.t2max_param = t2max;
  config.t3min_param = t3min;
  config.t3max_param = t3max;
  callback(config, 0);

  cvNamedWindow("Contours", CV_WINDOW_NORMAL);
  cvNamedWindow("COM", CV_WINDOW_NORMAL);
  cvNamedWindow("After Color Filtering", CV_WINDOW_NORMAL);

  if (flag)
  {
    cvNamedWindow("F1", CV_WINDOW_NORMAL);
    cvNamedWindow("F2", CV_WINDOW_NORMAL);
    cvNamedWindow("F3", CV_WINDOW_NORMAL);
  }

  // capture size -
  CvSize size = cvSize(width, height);

  cv::Mat hsv_frame, thresholded, thresholded1, thresholded2, thresholded3, filtered;  // image converted to HSV plane
  while (ros::ok())
  {
    std_msgs::Float64MultiArray array;
    loop_rate.sleep();

    if (frame.empty())
    {
      ROS_INFO("%s: empty frame", ros::this_node::getName().c_str());
      ros::spinOnce();
      continue;
    }

    if (video)
      output_cap.write(frame);

    // get the image data
    height = frame.rows;
    width = frame.cols;
    step = frame.step;

    // Covert color space to HSV as it is much easier to filter colors in the HSV color-space.
    cv::cvtColor(frame, hsv_frame, CV_BGR2HSV);
    cv::Scalar hsv_min = cv::Scalar(t1min, t2min, t3min, 0);
    cv::Scalar hsv_max = cv::Scalar(t1max, t2max, t3max, 0);
    // Filter out colors which are out of range.
    cv::inRange(hsv_frame, hsv_min, hsv_max, thresholded);
    // Split image into its 3 one dimensional images
    cv::Mat thresholded_hsv[3];
    cv::split(hsv_frame, thresholded_hsv);

    // Filter out colors which are out of range.
    cv::inRange(thresholded_hsv[0], cv::Scalar(t1min, 0, 0, 0), cv::Scalar(t1max, 0, 0, 0), thresholded_hsv[0]);
    cv::inRange(thresholded_hsv[1], cv::Scalar(t2min, 0, 0, 0), cv::Scalar(t2max, 0, 0, 0), thresholded_hsv[1]);
    cv::inRange(thresholded_hsv[2], cv::Scalar(t3min, 0, 0, 0), cv::Scalar(t3max, 0, 0, 0), thresholded_hsv[2]);
    cv::GaussianBlur(thresholded, thresholded, cv::Size(9, 9), 0, 0, 0);
    cv::imshow("After Color Filtering", thresholded);  // The stream after color filtering

    if (flag)
    {
      cv::imshow("F1", thresholded_hsv[0]);  // individual filters
      cv::imshow("F2", thresholded_hsv[1]);
      cv::imshow("F3", thresholded_hsv[2]);
    }

    if ((cvWaitKey(10) & 255) == 27)
      break;

    if ((!IP))
    {
      // find contours
      std::vector<std::vector<cv::Point> > contours;
      cv::Mat thresholded_Mat;
      thresholded.copyTo(thresholded_Mat);
      cv::findContours(thresholded_Mat, contours, CV_RETR_TREE, CV_CHAIN_APPROX_SIMPLE);  // Find the contours
      double largest_area = 0, largest_contour_index = 0;
      if (contours.empty())
      {
        array.data.push_back(0);
        array.data.push_back(0);

        pub.publish(array);
        ros::spinOnce();
        // If ESC key pressed, Key=0x10001B under OpenCV 0.9.7(linux version),
        // remove higher bits using AND operator
        if ((cvWaitKey(10) & 255) == 27)
          break;
        continue;
      }

      for (int i = 0; i < contours.size(); i++)  // iterate through each contour.
      {
        double a = contourArea(contours[i], false);  //  Find the area of contour
        if (a > largest_area)
        {
          largest_area = a;
          largest_contour_index = i;  // Store the index of largest contour
        }
      }

      // Convex HULL
      cv::Mat Drawing(thresholded.rows, thresholded.cols, CV_8UC1, cv::Scalar::all(0));
      std::vector<std::vector<cv::Point> > hull(1);
      cv::convexHull(cv::Mat(contours[largest_contour_index]), hull[0], false);

      cv::Moments mu;
      std::vector<cv::Vec4i> hierarchy;
      mu = cv::moments(hull[0], false);
      cv::Point2f center_of_mass;

      center_of_mass = cv::Point2f(mu.m10 / mu.m00, mu.m01 / mu.m00);

      cv::drawContours(Drawing, hull, 0, color, 2, 8, hierarchy);
      cv::circle(frame, center_of_mass, 5, cv::Scalar(0, 250, 0), -1, 8, 1);
      cv::imshow("COM", frame);
      cv::imshow("Contours", Drawing);

      cv::Point2f pt;
      pt.x = 320;  // size of my screen
      pt.y = 240;

      array.data.push_back((320 - center_of_mass.x));
      array.data.push_back((240 - center_of_mass.y));
      pub.publish(array);

      ros::spinOnce();
      // If ESC key pressed, Key=0x10001B under OpenCV 0.9.7(linux version),
      // remove higher bits using AND operator
      if ((cvWaitKey(10) & 255) == 27)
        break;
    }
    else
    {
      ros::spinOnce();
    }
  }
  output_cap.release();
  return 0;
}
