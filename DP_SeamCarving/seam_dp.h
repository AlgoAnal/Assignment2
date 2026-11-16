#pragma once

#include <opencv2/opencv.hpp>
#include <vector>

// Energy map
cv::Mat computeEnergy(const cv::Mat& img);

// Minimum vertical
//   M[0][j] = e[0][j]
//   M[i][j] = e[i][j] + min(M[i-1][j-1], M[i-1][j], M[i-1][j+1])
std::vector<int> findVerticalSeam(const cv::Mat& energy);

// Removes a vertical seam (image becomes one pixel narrower)
cv::Mat removeVerticalSeam(const cv::Mat& img, const std::vector<int>& seam);

// Recompute energy -> find seam -> remove, repeated `count` times
cv::Mat carveVertical(cv::Mat img, int count);

// Horizontal seams = vertical seams on the transposed image
cv::Mat carveHorizontal(const cv::Mat& img, int count);

// Shrinks img to newWidth x newHeight (vertical seams first, then horizontal)
cv::Mat seamCarve(const cv::Mat& img, int newWidth, int newHeight);
