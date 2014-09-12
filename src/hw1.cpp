#include <cv.h>
#include <highgui.h>
#include <iostream>
#include <exception>

using namespace cv;
using namespace std;

int THRESH_NORMAL = 1, THRESH_ADAPT_MEAN = 2, THRESH_ADAPT_GAUSS = 3;
double seventh = 1.0/7.0;

vector<Point>* getCircleCentersFromContour(vector<Point> contour) {
	if(contour.size() < 2) {
		cerr << "contour size < 2: can't be line" << endl;
		throw Exception();
	}
	vector<Point>* centers = new vector<Point>();
	for(unsigned int i = 0; i < contour.size(); i++) {
		int nextIndex = i+1;
		if(i == contour.size() - 1) {
			// if last point, wrap around and get first for line
			nextIndex = 0;
		}
		// take this and next point, compute sevenths
		Point curr = contour[i], next = contour[nextIndex];
//		cout << "curr:" << curr << " next:" << next << endl;
		for(int j = 1; j <= 6; j++) {
			Point a = ((next - curr) * j * seventh) + curr;
//			cout << j << "/7:" << a << endl;
			centers->push_back(a);
		}
	}
	return centers;
}

void findDisplayRects(Mat image, int threshMode, string winName) {
	cout << threshMode << endl;
	namedWindow(winName, CV_WINDOW_AUTOSIZE);
	int const max_BINARY_value = 255;
	Mat canny_output, src_gray, thresh_img, dilation_out, erosion_out;
	cvtColor(image, src_gray, CV_BGR2GRAY);
	// try the thresholding algorithms
	if(threshMode == THRESH_NORMAL) {
		int threshold_value = 100;
		int threshold_type = 0;
		int const max_value = 255;
		int const max_type = 4;
		String thresTypeDescrip = "Type: \n 0: Binary \n 1: Binary Inverted \n 2: Truncate \n 3: To Zero \n 4: To Zero Inverted";
		createTrackbar("Threshold value", winName, &threshold_value, max_value);
		createTrackbar(thresTypeDescrip, winName, &threshold_type, max_type);
		threshold(src_gray, thresh_img, threshold_value, max_BINARY_value, threshold_type);
	}
	else if(threshMode == THRESH_ADAPT_MEAN) {
		adaptiveThreshold(src_gray, thresh_img, max_BINARY_value, ADAPTIVE_THRESH_MEAN_C, THRESH_BINARY, 15, 2);
	}
	else {
		adaptiveThreshold(src_gray, thresh_img, max_BINARY_value, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, 15, 2);
	}

	vector<vector<Point> > contours;
	vector<Vec4i> hierarchy;
	int thresh = 100;
	int minAreaToConsider = 50;
	// Detect edges using canny
	Canny(thresh_img, canny_output, thresh, thresh * 2, 3);

	// dilate then erode to fill gaps?
//	int dilation_size = 2;
//	createTrackbar("Kernel size", winName, &dilation_size, 4);
//	Mat kernel = getStructuringElement( MORPH_ELLIPSE,
//            Size( 2*dilation_size + 1, 2*dilation_size+1 ),
//            Point( dilation_size, dilation_size ) );
//	dilate(canny_output, dilation_out, kernel);
//	erode(dilation_out, erosion_out, kernel);
	erosion_out = canny_output.clone();

	// find contours given edges
	findContours(erosion_out, contours, hierarchy, CV_RETR_LIST, CV_CHAIN_APPROX_NONE );
	// given contours, approximate each with a polygon
	// and check if that approximate polygon is a rectangle
	vector<vector<Point> > approxContours(contours.size());
//	vector<Rect> boundRect( contours.size() );
	vector<bool> considerPoly(contours.size());
	for (unsigned int i = 0; i < contours.size(); i++) {
		considerPoly[i] = false;
		approxPolyDP(Mat(contours[i]), approxContours[i], 3, true);
		if(approxContours[i].size() == 4) {
			Rect r = boundingRect(Mat(approxContours[i]));
//			boundRect[i] = r;
			if(r.area() >= minAreaToConsider) {
				considerPoly[i] = true;
			}
		}
	}

	// Draw contours
//	Mat drawing2 = Mat::zeros(canny_output.size(), CV_8UC3);
	Mat drawing = image.clone();
	for (unsigned int i = 0; i < approxContours.size(); i++) {
		Scalar red = Scalar(0,0,255), blue = Scalar(255,0,0); // blue, green, red
		if(considerPoly[i]) {
//			rectangle( drawing, boundRect[i].tl(), boundRect[i].br(), blue, 2, 8, 0 );
			drawContours(drawing, approxContours, i, red, 2, 8, hierarchy, 0, Point());
//			rectangle( drawing2, boundRect[i].tl(), boundRect[i].br(), blue, 2, 8, 0 );
//			drawContours(drawing2, approxContours, i, red, 2, 8, hierarchy, 0, Point());
			for(unsigned int j = 0; j < approxContours[i].size(); j++) {
				cout << "Point " << j << ":" << approxContours[i][j] << endl;
			}
			// each contour is a list of 4 points
			// given a contour, return a list of center points for circles
			vector<Point>* centers = getCircleCentersFromContour(approxContours[i]);
			vector<Point> c = *centers;
			cout << "after func center size:" << centers->size() << endl;
			for(unsigned int j = 0; j < c.size(); j++) {
				if(j % 6 == 0) {
					cout << "----" << endl;
				}
				cout << j << "th circle center:" << c[j] << endl;
				circle(drawing, c[j], 5, blue);
			}
		}
	}

	// Show in a window
	imshow(winName, drawing);
//	imshow(winName+"_2", drawing2);
}

void loopVC(VideoCapture vc) {
	// loop through video
	// declare image structures, and integers for trackbars and thresholds
	String win = "Display Video";
	int max = 500000, curr = 0;
	Mat image, gray_image, thresh_img, adapt_thresh_img_mean, adapt_thresh_img_gauss,
		canny_output;
	char key;
	// while still time left
	while (curr < max) {
//		{
		// read image
		bool result = vc.read(image);
		if (!result) {
			cout << "no more frames, at " << curr << endl;
			return;
		}
		findDisplayRects(image, THRESH_NORMAL, win);
		findDisplayRects(image, THRESH_ADAPT_MEAN, win+"_mean");
		findDisplayRects(image, THRESH_ADAPT_GAUSS, win+"_gauss");
		// and wait for esc
		key = waitKey(0);
		if (key == 27) {
			return;
		}
		curr++;
		if (curr % 1000 == 0) {
			cout << curr << " -- quitting at " << max << endl;
		}
	}
}

//int openMovie() {
void openMovie() {
	// try file instead
	VideoCapture movie("MarkerMovie.mpg");
	if (movie.isOpened()) {
		cout << "movie opened" << endl;
		loopVC(movie);
//		return 0;
	} else {
		cerr << "Unable to open movie." << endl;
//		return -1;
	}
}

int main(int argc, char** argv) {
	if(argc == 1) {
//		for(int i = 0; i < 6; i++) {
			openMovie();
//		}
//		return openMovie();
	}
	else if(argc == 2 && string(argv[1]) == "-c") {
		VideoCapture vc(0);
		if(vc.isOpened()) {
			cout << "camera opened" << endl;
			loopVC(vc);
		}
		else {
			cerr << " unable to open camera. trying movie." << endl;
			openMovie();
		}
	}
	else {
		cerr << "Usage: " << argv[0] << "[-c]"<< endl;
		cerr << " for optional camera argument" << endl;
		return -1;
	}
	return 0;
}
