#include<bits/stdc++.h>
#include<random>
using namespace std;

#include<opencv2/opencv.hpp>
#include<opencv2/core/core.hpp>
#include<opencv2/highgui/highgui.hpp>
using namespace cv;

random_device rd;
mt19937 mt(rd());
uniform_int_distribution<int> d(0, 1e9);

typedef long long ll;
char fileName[1010];

const double PI = 3.141592653589793;
const double EXP = 2.71828182845904;
const double SIGMA = 1;
const double SIGMA_COEF = 12.0;
const double COLOR_SIGMA = 64.0;
double GaussianFunction(double x, double sigma = SIGMA) {
	const double tmp = 1.0 / ((sqrt(2 * PI)) * sigma);
	return tmp * exp(-(x * x) / (2 * sigma * sigma));
}
double GaussianFunction2D(double x, double y, double sigma = SIGMA) {
	return GaussianFunction(x, sigma) * GaussianFunction(y, sigma);
}
void BackgroundCorrection(Mat& gt, Mat& pred, int flag) {
	/*
	Magenta: (255, 0, 255)
	Gray: (127, 127, 127)
	*/
	vector<vector<int>> back = { {255, 0, 255}, {127, 127, 127} };
	auto it1 = gt.begin<Vec4b>();
	auto it2 = pred.begin<Vec4b>();
	for (; it1 != gt.end<Vec4b>(); ++it1, ++it2) {
		bool f = 1;
		for (int i = 0; i < 3; ++i) {
			if ((*it1)[i] != back[0][i]) {
				f = 0;
				break;
			}
		}
		if (f)
			for (int i = 0; i < 3; ++i)
				(*it2)[i] = back[flag][i];
	}
}
vector<vector<double>> GetGaussianFilter(int kernelSize, double sigma = -1) {
	if (sigma == -1)
		sigma = kernelSize / SIGMA_COEF;

	vector<vector<double>> kernel(kernelSize);
	for (int i = 0; i < kernelSize; ++i) {
		for (int j = 0; j < kernelSize; ++j) {
			kernel[i].push_back(GaussianFunction2D(i - kernelSize / 2, j - kernelSize / 2, sigma));
		}
	}
	return kernel;
}
Mat MyGaussianFiltering(Mat src, int kernelSize = 5) {
	auto filter = GetGaussianFilter(kernelSize, kernelSize / SIGMA_COEF);
	Mat res = Mat(src.rows, src.cols, CV_8UC3);
	for (int i = 0; i < res.rows; ++i) {
		for (int j = 0; j < res.cols; ++j) {
			for (int c = 0; c < 3; ++c) {
				double sum = 0.0;
				int cnt = 0;
				for (int k = 0; k < kernelSize; ++k) {
					for (int l = 0; l < kernelSize; ++l) {
						int nx = i + (k - kernelSize / 2);
						int ny = j + (l - kernelSize / 2);
						if (0 <= nx && nx < res.rows && 0 <= ny && ny < res.cols) {
							sum += filter[k][l] * src.at<Vec3b>(nx, ny)[c];
						}
					}
				}
				res.at<Vec3b>(i, j)[c] = sum;
			}

		}
	}
	return res;
}
vector<vector<Vec3f>> MatNormalization(Mat img) {
	vector<vector<Vec3f>> res(img.rows);
	for (int i = 0; i < img.rows; ++i) {
		res[i].resize(img.cols);
		for (int j = 0; j < img.cols; ++j) {
			auto cur = img.at<Vec3b>(i, j);
			double d = sqrt(cur[0] * cur[0] + cur[1] * cur[1] + cur[2] * cur[2]);
			for (int k = 0; k < 3; ++k) {
				res[i][j][k] = cur[k] / d;
			}
		}
	}
	return res;
}
Mat JointBilateralFiltering(Mat src, Mat target, int kernelSize = 5, double sigma = -1, double colorSigma = -1) {
	if (sigma == -1)
		sigma = kernelSize / SIGMA_COEF;
	if (colorSigma == -1)
		colorSigma = COLOR_SIGMA;

	auto filter = GetGaussianFilter(kernelSize, sigma);
	Mat res = Mat(src.rows, src.cols, CV_8UC3);
	for (int i = 0; i < res.rows; ++i) {
		for (int j = 0; j < res.cols; ++j) {
			for (int c = 0; c < 3; ++c) {
				double sum = 0.0;
				double sum2 = 0.0;
				int cnt = 0;
				for (int k = 0; k < kernelSize; ++k) {
					for (int l = 0; l < kernelSize; ++l) {
						int nx = i + (k - kernelSize / 2);
						int ny = j + (l - kernelSize / 2);

						auto origin = src.at<Vec3b>(i, j)[c];

						if (0 <= nx && nx < res.rows && 0 <= ny && ny < res.cols) {
							auto np = src.at<Vec3b>(nx, ny)[c];
							sum += filter[k][l] * target.at<Vec3b>(nx, ny)[c] * GaussianFunction(origin - np, colorSigma);
							sum2 += filter[k][l] * GaussianFunction(origin - np, colorSigma);
						}
					}
				}
				res.at<Vec3b>(i, j)[c] = sum / sum2;
			}

		}
	}
	return res;
}
Mat JointBilateralFilteringWithNormalization(Mat src, Mat target, int kernelSize = 5, double sigma = -1, double colorSigma = -1) {
	Mat srcNorm, targetNorm;
	src.convertTo(srcNorm, CV_32FC3, 1.0 / 255);
	target.convertTo(targetNorm, CV_32FC3, 1.0 / 255);
	if (sigma == -1)
		sigma = kernelSize / SIGMA_COEF;
	if (colorSigma == -1)
		colorSigma = COLOR_SIGMA;

	auto filter = GetGaussianFilter(kernelSize, sigma);
	Mat res = Mat(src.rows, src.cols, CV_32FC3);
	for (int i = 0; i < res.rows; ++i) {
		for (int j = 0; j < res.cols; ++j) {
			for (int c = 0; c < 3; ++c) {
				double sum = 0.0;
				double sum2 = 0.0;
				int cnt = 0;
				for (int k = 0; k < kernelSize; ++k) {
					for (int l = 0; l < kernelSize; ++l) {
						int nx = i + (k - kernelSize / 2);
						int ny = j + (l - kernelSize / 2);

						auto origin = srcNorm.at<Vec3f>(i, j)[c];

						if (0 <= nx && nx < res.rows && 0 <= ny && ny < res.cols) {
							auto np = srcNorm.at<Vec3f>(nx, ny)[c];
							sum += filter[k][l] * targetNorm.at<Vec3f>(nx, ny)[c] * GaussianFunction(origin - np, colorSigma);
							sum2 += filter[k][l] * GaussianFunction(origin - np, colorSigma);
						}
					}
				}
				res.at<Vec3f>(i, j)[c] = (sum / sum2) ;
			}

		}
	}
	return res;
}
bool BackgroundCheck(Vec4b p1, int flag) {
	/*
	Magenta: (255, 0, 255)
	Gray: (127, 127, 127)
	*/
	vector<vector<int>> back = { {255, 0, 255}, {127, 127, 127} };
	for (int i = 0; i < 3; ++i)
		if (p1[i] != back[flag][i]) return 1;
	return 0;
}
double GetRMSE(Mat img1, Mat img2) {
	double res = 0.0;
	int cnt = 0;
	auto it1 = img1.begin<Vec4b>();
	auto it2 = img2.begin<Vec4b>();
	for (; it1 != img1.end<Vec4b>(); ++it1, ++it2) {
		if (!BackgroundCheck((*it1), 1) || !BackgroundCheck((*it2), 1)) continue;
		for (int i = 0; i < 3; ++i) {
			auto p1 = ((*it1)[i] - 127.5) / 127.5;
			auto p2 = ((*it2)[i] - 127.5) / 127.5;
			res += (p1 - p2) * (p1 - p2);
			++cnt;
		}
	}
	return sqrt(res / (cnt * 3.0));
}
int main() {
	/*
	auto kern = GetGaussianFilter(255, 32);
	Mat kernImg(Size(255, 255), CV_8UC1);
	for (int i = 0; i < kern.size(); ++i) {
		for (int j = 0; j < kern[i].size(); ++j) {
			kernImg.at<uchar>(i, j) = kern[i][j];
		}
	}
	imshow("kernel", kernImg);
	waitKey(0);
	*/
	//Filtering Test
	/*
	Mat img1 = imread("pp/3.png", 1);
	Mat img2 = imread("pp/3.png", 1);
	BackgroundCorrection(img1, img1, 1);
	imshow("origin", img1);
	auto res1 = MyGaussianFiltering(img1, 9);
	imshow("My Gaussian", res1);
	Mat res2;
	GaussianBlur(img1, res2, Size(9, 9), 9.0 / SIGMA_COEF);
	imshow("CV Gaussian", res2);
	auto res3 = JointBilateralFiltering(img1, img1, 9, -1, 32);
	imshow("My Bilateral", res3);
	auto res4 = JointBilateralFilteringWithNormalization(img1, img1, 9, -1, 32);
	imshow("My Bilateral-2", res4);
	waitKey(0); return 0;
	*/

	//GetRMSE
	string rootDir = "./20210728/";
	string predDir = rootDir + "res/";
	string resDir = rootDir + "concat/";

	int imageCount = 50;
	char fName[1010];
	double rmse = 0.0;
	vector<pair<double, Mat>> result1, result2;

	int kernelCount = 5;
	int kernelStart = 5;
	vector<double> RMSE(kernelCount + 1);
	int fileCount = 10;

	for (int i = 1; i <= imageCount; ++i) {
		printf("%d/%d\n", i, imageCount);
		Mat inp, gt, pred;
		sprintf(fName, "%s(result%d)Ground_Truth.png", predDir.c_str(), i);
		gt = imread(fName);
		sprintf(fName, "%s(result%d)Input_Image.png", predDir.c_str(), i);
		inp = imread(fName);
		sprintf(fName, "%s(result%d)Predicted_Image.png", predDir.c_str(), i);
		pred = imread(fName);

		Mat concatImg = inp;
		hconcat(concatImg, gt, concatImg);
		hconcat(concatImg, pred, concatImg);
		for (int kk = kernelStart; kk <= kernelStart + 2 * kernelCount; kk += 2) {
			//Mat JBF = JointBilateralFiltering(inp, pred, kk);
			//hconcat(concatImg, JBF, concatImg);
			//auto res2 = GetRMSE(gt, JBF);
			//RMSE[(kk - kernelStart) / 2] += res2;

			Mat JBF = JointBilateralFilteringWithNormalization(inp, pred, kk, (kk - 3) / 6.0);
			Mat tmpImg;
			JBF.convertTo(tmpImg, CV_8UC3, 255);
			hconcat(concatImg, tmpImg, concatImg);
			auto res2 = GetRMSE(gt, tmpImg);
			RMSE[(kk - kernelStart) / 2] += res2;
		}

		auto res1 = GetRMSE(gt, pred);

		result1.push_back({ res1, concatImg });
		rmse += res1;
	}
	::sort(result1.begin(), result1.end(), [](pair<double, Mat> p1, pair<double, Mat> p2) {
		return p1.first < p2.first;
		});
	rmse /= imageCount;
	for(int i = 0; i<RMSE.size(); ++i)
		RMSE[i] /= imageCount;
	printf("%lf\n", rmse);
	for (int i = 0; i < RMSE.size(); ++i) {
		//printf("Kernel size = %d(COEF %lf): %lf\n", kernelStart + i * 2, SIGMA_COEF, RMSE[i]);
		printf("%lf\n", RMSE[i]);
	}
	for (int i = 0; i < result1.size(); ++i) {
		sprintf(fName, "%s%d.png", resDir.c_str(), fileCount++);
		imwrite(fName, result1[i].second);
	}
}
