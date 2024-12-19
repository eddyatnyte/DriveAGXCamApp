#include <iostream>
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>
#include "CCamera.cpp"

#include <vector>
#include <math.h>

using namespace std;
using namespace cv;

int main()
{
	cout << "Starting application..." << endl
		 << endl;

	CCamera cameras(GROUP_B);
	vector<Mat> rawFrames;
	vector<Mat> processedImage(4);

	while (true)
	{
		// aktualisiert die Kameras
		cameras.getFrame();

		// speichert und verarbeitet die Bilder
		for (int i = 0; i < 4; ++i)
		{
			// erstellt 4 Bilder aus den Rohdaten
			rawFrames.push_back(Mat(cameras.getHeight(i), cameras.getWidth(i), CV_8UC4, cameras.getFrameData(i)));

			// konvertiert die Bilder in den entsprechenden Farbraum
			cvtColor(rawFrames[i], processedImage[i], COLOR_RGB2BGR);
		}

		// ordnet die Bilder entsprechend an
		Mat top_row, bottom_row, combined_image;
		hconcat(processedImage[0], processedImage[1], top_row);
		hconcat(processedImage[2], processedImage[3], bottom_row);
		vconcat(top_row, bottom_row, combined_image);

		// öffnet das Fenster mit dem entsprechenden Namen
		namedWindow("Output of cameras", WINDOW_NORMAL);
		resizeWindow("Output of cameras", combined_image.cols / 2, combined_image.rows / 2);

		// gibt das Bild aus
		imshow("Output of cameras", combined_image);

		// Wenn eine Taste betätigt wird,
		if (waitKey(1) >= 0)
		{
			// wird die Endlosschleife abgebrochen
			break;
		}
	}

	// gibt alle Ressourcen frei
	cameras.releaseHandle();

	return 0;
}
