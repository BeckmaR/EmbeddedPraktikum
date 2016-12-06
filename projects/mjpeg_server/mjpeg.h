/*
 * mjpeg.h
 *
 *  Created on: 29.11.2016
 *      Author: rene
 */

#ifndef MJPEG_H_
#define MJPEG_H_

#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
using namespace cv;

class Mjpeg {
	private:
		int videoID;
		VideoCapture stream;

	public:
		Mjpeg(int);
		std::vector<uchar> getJpegImage();
		cv::Mat getImage();

};


#endif /* MJPEG_H_ */
