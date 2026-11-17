// Usage: dp_seam.exe <input> <output> <newWidth> <newHeight>

#include "seam_dp.h"
#include <chrono>
#include <exception>
#include <filesystem>
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
		cv::Mat marked;
		cv::Mat result = seamCarve(img, newW, newH, &marked);
		auto t1 = std::chrono::steady_clock::now();

		// Seam image is created next to the output image
		const std::filesystem::path outPath(argv[2]);
		std::filesystem::path seamPath = outPath.parent_path() / 
			(outPath.stem().string() + "_visualized" + 
				(outPath.has_extension() ? outPath.extension().string() :
				".png"));

		if (!cv::imwrite(argv[2], result))
		{
			std::cerr << "Cannot write image: " << argv[2] << "\n";
			return 1;
		}
		if (!cv::imwrite(seamPath.string(), marked))
		{
			std::cerr << "Cannot write image: " << seamPath.string() << "\n";
			return 1;
		}
		std::cout << "Resized " << img.cols << "x" << img.rows << " -> " <<
			result.cols << "x" << result.rows << " in " <<
			std::chrono::duration<double>(t1 - t0).count() << "s\n" <<
			"Seams drawn on the original saved to " << seamPath.string() <<
			" (vertical = red, horizontal = yellow)\n";
	}
	catch (const std::exception& ex)
	{
		std::cerr << "Error: " << ex.what() << "\n";
		return 1;
	}
	return 0;
}
