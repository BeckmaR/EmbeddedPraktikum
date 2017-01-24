/*
 * mjpeg.cpp
 *
 *  Created on: 29.11.2016
 *      Author: rene
 */

#include "mjpeg.h"
#include <iostream>

using namespace cv;

Mjpeg::Mjpeg(int vidID) :
		stream(vidID) {
	videoID = vidID;

	if (!stream.isOpened()) { //check if video device has been initialised
		std::cout << "cannot open camera";
	}
}

std::vector<uchar> Mjpeg::getJpegImage() {
	cv::Mat image = getImage();

	std::vector<uchar> buf;
	std::vector<int> params;
	params.push_back(CV_IMWRITE_JPEG_QUALITY);
	params.push_back(95);
	std::string ext = "*.jpg";
	cv::imencode(ext, image, buf, params);
	return buf;
}

cv::Mat Mjpeg::getImage() {
	cv::Mat image;
	stream.read(image);

	return image;
}
