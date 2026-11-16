#include "seam_dp.h"
#include <algorithm>
#include <stdexcept>
#include <string>

cv::Mat computeEnergy(const cv::Mat& img)
{
	cv::Mat gray, gx, gy, absX, absY, energy;
	cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
	cv::Sobel(gray, gx, CV_64F, 1, 0, 3, 1, 0, cv::BORDER_REPLICATE);
	cv::Sobel(gray, gy, CV_64F, 0, 1, 3, 1, 0, cv::BORDER_REPLICATE);
	absX = cv::abs(gx);
	absY = cv::abs(gy);
	energy = absX + absY;
	return energy;
}

std::vector<int> findVerticalSeam(const cv::Mat& energy)
{
	const int m = energy.rows, n = energy.cols;
	cv::Mat M(m, n, CV_64F);
	std::vector<std::vector<signed char>> back(m, std::vector<signed char>(n, 0));

	for (int j = 0; j < n; ++j)
	{
		M.at<double>(0, j) = energy.at<double>(0, j);
	}

	for (int i = 1; i < m; ++i)
	{
		const double* prev = M.ptr<double>(i - 1);
		const double* e = energy.ptr<double>(i);
		double* cur = M.ptr<double>(i);
		for (int j = 0; j < n; ++j)
		{
			double best = prev[j];
			int off = 0;
			if (j > 0 && prev[j - 1] < best)
			{
				best = prev[j - 1];
				off = -1;
			}
			if (j < n - 1 && prev[j + 1] < best)
			{
				best = prev[j + 1];
				off = 1;
			}
			cur[j] = e[j] + best;
			back[i][j] = static_cast<signed char>(off);
		}
	}

	const double* last = M.ptr<double>(m - 1);
	int col = static_cast<int>(std::min_element(last, last + n) - last);
	std::vector<int> seam(m);
	for (int i = m - 1; i >= 0; --i)
	{
		seam[i] = col;
		col += back[i][col];
	}
	return seam;
}

cv::Mat removeVerticalSeam(const cv::Mat& img, const std::vector<int>& seam)
{
	const int m = img.rows, n = img.cols;
	cv::Mat out(m, n - 1, img.type());
	for (int i = 0; i < m; ++i)
	{
		const cv::Vec3b* src = img.ptr<cv::Vec3b>(i);
		cv::Vec3b* dst = out.ptr<cv::Vec3b>(i);
		const int s = seam[i];
		std::copy(src, src + s, dst);
		std::copy(src + s + 1, src + n, dst + s);
	}
	return out;
}

cv::Mat carveVertical(cv::Mat img, int count)
{
	for (int k = 0; k < count; ++k)
	{
		cv::Mat energy = computeEnergy(img);
		std::vector<int> seam = findVerticalSeam(energy);
		img = removeVerticalSeam(img, seam);
	}
	return img;
}

cv::Mat carveHorizontal(const cv::Mat& img, int count)
{
	cv::Mat t, out;
	cv::transpose(img, t);
	t = carveVertical(t, count);
	cv::transpose(t, out);
	return out;
}

cv::Mat seamCarve(const cv::Mat& img, int newWidth, int newHeight)
{
	if (img.empty() || img.type() != CV_8UC3)
		throw std::invalid_argument("Image must be a non-empty 8-bit 3-channel image");
	if (newWidth < 1 || newHeight < 1 || newWidth > img.cols || newHeight > img.rows)
		throw std::invalid_argument("Target size must be between 1x1 and " +
			std::to_string(img.cols) + "x" + std::to_string(img.rows) +
			" (seam removal only shrinks)");

	cv::Mat result = carveVertical(img, img.cols - newWidth);  // vertical seams
	return carveHorizontal(result, img.rows - newHeight);      // horizontal seams
}
