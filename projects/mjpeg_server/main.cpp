/*
 * main.cpp
 *
 *  Created on: 29.11.2016
 *      Author: rene
 */

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "mjpeg.h"

int main() {
	Mjpeg mjpg(0);

	while(1) {
		cv::Mat image;
		image = mjpg.getImage();
		cv::imshow("cam", image);
		if (cv::waitKey(30) >= 0)
					break;
	}
}
