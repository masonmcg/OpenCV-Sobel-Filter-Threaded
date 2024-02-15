#include <opencv2/opencv.hpp>
#include <pthread.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RED 0.2126
#define GREEN 0.7152
#define BLUE 0.0722

#define ESC 27

#define GX {1, 0, -1, 2, 0, -2, 1, 0, -1}
#define GY {1, 2, 1, 0, 0, 0, -1, -2, -1}

struct ThreadData 
{
    cv::Mat* input;
    cv::Mat* output;
};

void to442_grayscale(cv::Mat *rgbImage);
void to442_sobel(ThreadData *data);

cv::Mat frame_split_stitch(cv::Mat& frame);
void frame_to_sobel(ThreadData *data);

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <video_file>" << std::endl;
        return 1;
    }
    
    cv::VideoCapture cap(argv[1]);
    if (!cap.isOpened()) {
		std::cerr << "Error: Couldn't open the video file." << std::endl;
        return 1;
	}
	
	while (true) {
		
		cv::Mat frame;
		cap >> frame;
		if (frame.empty())
			break;
			
		//cv::Mat grayscale = to442_grayscale(frame);
		
		//cv::Mat sobelFiltered = to442_sobel(grayscale);
		
		//cv::Mat sobelFiltered = frame_to_sobel(frame);
		
		cv::Mat sobelFiltered = frame_split_stitch(frame);
		
		cv::imshow("Processed Frame", sobelFiltered);
		
		int key = cv::waitKey(30);
		if (key == ESC)
			break;
		
	}
	
	cap.release();
	cv::destroyAllWindows();
	
}

cv::Mat frame_split_stitch(cv::Mat& frame)
{
	cv::Size sz = frame.size();
    int cols = sz.width;
    int rows = sz.height;
    int midRow = rows / 2;
    int midCol = cols / 2;
    
    cv::Rect quad1(0, 0, midCol + 1, midRow + 1);
    cv::Rect quad2(midCol - 1, 0, cols - midCol + 1, midRow + 1);
    cv::Rect quad3(0, midRow - 1, midCol + 1, rows - midRow + 1);
    cv::Rect quad4(midCol - 1, midRow - 1, cols - midCol + 1, rows - midRow + 1);
    
    cv::Mat quad1Mat = frame(quad1);
    cv::Mat quad2Mat = frame(quad2);
    cv::Mat quad3Mat = frame(quad3);
    cv::Mat quad4Mat = frame(quad4);
    
    cv::Mat sobelFiltered1(quad1Mat.size().height-2, quad1Mat.size().width-2, CV_8UC3);
    cv::Mat sobelFiltered2(quad2Mat.size().height-2, quad2Mat.size().width-2, CV_8UC3);
    cv::Mat sobelFiltered3(quad3Mat.size().height-2, quad3Mat.size().width-2, CV_8UC3);
    cv::Mat sobelFiltered4(quad4Mat.size().height-2, quad4Mat.size().width-2, CV_8UC3);
    
    pthread_t frame_to_sobel_1_thread, frame_to_sobel_2_thread, frame_to_sobel_3_thread, frame_to_sobel_4_thread;
    
    ThreadData data1 = { &quad1Mat, &sobelFiltered1 };
    ThreadData data2 = { &quad2Mat, &sobelFiltered2 };
    ThreadData data3 = { &quad3Mat, &sobelFiltered3 };
    ThreadData data4 = { &quad4Mat, &sobelFiltered4 };
    
    pthread_create(&frame_to_sobel_1_thread, NULL, (void* (*)(void*)) frame_to_sobel, &data1);
    pthread_create(&frame_to_sobel_2_thread, NULL, (void* (*)(void*)) frame_to_sobel, &data2);
    pthread_create(&frame_to_sobel_3_thread, NULL, (void* (*)(void*)) frame_to_sobel, &data3);
    pthread_create(&frame_to_sobel_4_thread, NULL, (void* (*)(void*)) frame_to_sobel, &data4);
    
    pthread_join(frame_to_sobel_1_thread, NULL);
    pthread_join(frame_to_sobel_2_thread, NULL);
    pthread_join(frame_to_sobel_3_thread, NULL);
    pthread_join(frame_to_sobel_4_thread, NULL);
    
    cv::Mat sobelFiltered, sobelFilteredTop, sobelFilteredBottom;
    
    cv::hconcat(sobelFiltered1, sobelFiltered2, sobelFilteredTop);
    cv::hconcat(sobelFiltered3, sobelFiltered4, sobelFilteredBottom);
    cv::vconcat(sobelFilteredTop, sobelFilteredBottom, sobelFiltered);
    
    return sobelFiltered;
	}

// just a placeholder, edit this later to combine the 2 functions
void frame_to_sobel(ThreadData *data)
{
	
	to442_grayscale(data->input);
	to442_sobel(data);
	//return sobelFiltered;
}

void to442_grayscale(cv::Mat *rgbImage)
{	
    cv::Size sz = rgbImage->size();
    int imageWidth = sz.width;
    int imageHeight = sz.height;

    int i = 0; // width (column) index
    int j = 0; // height (row) index
    for (i = 0; i < imageWidth; i++)
    {
        for (j = 0; j < imageHeight; j++)
        {
            cv::Vec3b& pixel = rgbImage->at<cv::Vec3b>(j, i); // Vec<uchar, 3>
            uchar blue = pixel[0];
            uchar green = pixel[1];
            uchar red = pixel[2];
            
            blue = blue * BLUE;
            green = green * GREEN;
            red = red * RED;
            
            uchar gray = blue + green + red;

            pixel[0] = gray;
            pixel[1] = gray;
            pixel[2] = gray;

        }
    }
}


void to442_sobel(ThreadData *data) 
{
    cv::Size sz = data->input->size();
    int imageWidth = sz.width;
    int imageHeight = sz.height;
    
    int gx[] = GX;
    int gy[] = GY;
    int grayValues[9];

    int i = 0; // width (column) index
    int j = 0; // height (row) index
    
    for (i = 1; i < imageWidth - 1; i++)
    {
        for (j = 1; j < imageHeight - 1; j++)
        {
            int index = 0;
            for (int dj = -1; dj <= 1; ++dj) 
            {
                for (int di = -1; di <= 1; ++di) 
                {
                    int nj = j + dj;
                    int ni = i + di;

                    grayValues[index] = data->input->at<cv::Vec3b>(nj, ni)[0];
                    index++;
                }      
            }
            
            int gx_sum = 0;
            int gy_sum = 0;
            
            for (index = 0; index < 9; index++) 
            {
				gx_sum += gx[index] * grayValues[index];
				gy_sum += gy[index] * grayValues[index];
			}
			
			int sum = abs(gx_sum) + abs(gy_sum);
			
			// Clamp sum value to 255
            sum = std::min(sum, 255);
            
            data->output->at<cv::Vec3b>(j - 1, i - 1) = cv::Vec3b(sum, sum, sum);
        }
    }
}
