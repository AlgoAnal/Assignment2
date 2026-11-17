#include "algorithms/seam_dp.h"
#include "common/seam.h"
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

template <typename T>
static cv::Mat removeSeam(const cv::Mat& src, const std::vector<int>& seam)
{
	const int m = src.rows, n = src.cols;
	cv::Mat out(m, n - 1, src.type());
	for (int i = 0; i < m; ++i)
	{
		const T* in = src.ptr<T>(i);
		T* dst = out.ptr<T>(i);
		const int s = seam[i];
		std::copy(in, in + s, dst);
		std::copy(in + s + 1, in + n, dst + s);
	}
	return out;
}

cv::Mat removeVerticalSeam(const cv::Mat& img, const std::vector<int>& seam)
{
	return removeSeam<cv::Vec3b>(img, seam);
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

cv::Mat carveTracked(cv::Mat img, int count, cv::Mat& origin, cv::Mat& marked,
	const cv::Vec3b& color)
{
	for (int k = 0; k < count; ++k)
	{
		cv::Mat energy = computeEnergy(img);
		std::vector<int> seam = findVerticalSeam(energy);
		for (int i = 0; i < img.rows; ++i)
		{
			const cv::Vec2i o = origin.at<cv::Vec2i>(i, seam[i]);  // (x, y) in the original
			marked.at<cv::Vec3b>(o[1], o[0]) = color;
		}
		img = removeSeam<cv::Vec3b>(img, seam);
		origin = removeSeam<cv::Vec2i>(origin, seam);
	}
	return img;
}

cv::Mat seamCarve(const cv::Mat& img, int newWidth, int newHeight, cv::Mat* marked)
{
	if (img.empty() || img.type() != CV_8UC3)
	{
		throw std::invalid_argument("Image must be a non-empty 8-bit 3-channel image");
	}
	if (newWidth < 1 || newHeight < 1 || newWidth > img.cols || newHeight > img.rows)
	{
		throw std::invalid_argument("Target size must be between 1x1 and " +
			std::to_string(img.cols) + "x" + std::to_string(img.rows) +
			" (seam removal only shrinks)");
	}
	if (marked == nullptr)
	{
		cv::Mat result = carveVertical(img, img.cols - newWidth);  // vertical seams
		return carveHorizontal(result, img.rows - newHeight);      // horizontal seams
	}

	// With seam visualisation
	*marked = img.clone();
	cv::Mat origin(img.size(), CV_32SC2);
	for (int y = 0; y < img.rows; ++y)
	{
		for (int x = 0; x < img.cols; ++x)
		{
			origin.at<cv::Vec2i>(y, x) = cv::Vec2i(x, y);
		}
	}

	const cv::Vec3b red(0, 0, 255), yellow(0, 255, 255);
	cv::Mat result = carveTracked(img, img.cols - newWidth, origin, *marked, red);

	cv::Mat t, tOrigin;
	cv::transpose(result, t);
	cv::transpose(origin, tOrigin);
	t = carveTracked(t, img.rows - newHeight, tOrigin, *marked, yellow);
	cv::transpose(t, result);
	return result;
}

// ---- Adapter for the shared comparison harness (common/seam.h) -------------
// The harness passes a CV_32F energy map; the DP above works in CV_64F.
seam_t find_seam_dp(const cv::Mat& energy, float& total_cost)
{
	CV_Assert(energy.type() == CV_32F && energy.rows > 0 && energy.cols > 0);
	cv::Mat energy64;
	energy.convertTo(energy64, CV_64F);
	seam_t seam = findVerticalSeam(energy64);

	total_cost = 0.0f;
	for (int i = 0; i < energy.rows; ++i)
	{
		total_cost += energy.at<float>(i, seam[i]);
	}
	return seam;
}
