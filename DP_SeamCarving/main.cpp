// Usage: dp_seam.exe <input> <output> <newWidth> <newHeight>

#include "seam_dp.h"

#include <chrono>
#include <exception>
#include <iostream>
#include <string>

int main(int argc, char** argv)
{
	if (argc != 5)
	{
		std::cerr << "Usage: " << argv[0] << " <input> <output> <newWidth> <newHeight>\n";
		return 1;
	}
	try
	{
		cv::Mat img = cv::imread(argv[1], cv::IMREAD_COLOR);
		if (img.empty())
		{
			std::cerr << "Cannot read image: " << argv[1] << "\n";
			return 1;
		}
		const int newW = std::stoi(argv[3]), newH = std::stoi(argv[4]);

		auto t0 = std::chrono::steady_clock::now();
		cv::Mat result = seamCarve(img, newW, newH);
		auto t1 = std::chrono::steady_clock::now();

		if (!cv::imwrite(argv[2], result))
		{
			std::cerr << "Cannot write image: " << argv[2] << "\n";
			return 1;
		}
		std::cout << "Resized " << img.cols << "x" << img.rows << " -> " <<
			result.cols << "x" << result.rows << " in " <<
			std::chrono::duration<double>(t1 - t0).count() << "s\n";
	}
	catch (const std::exception& ex)
	{
		std::cerr << "Error: " << ex.what() << "\n";
		return 1;
	}
	return 0;
}
