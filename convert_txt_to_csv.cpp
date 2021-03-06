#include "pch.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <memory>
#include <math.h>
#include <utility>
#include <tuple>
#include <algorithm>

using namespace std;

#define gamma 0.0691
#define varphi_tolerance 3

ofstream output_file{ "results.txt" };

vector<int> CheckList{ 1, 109, 216, 324, 432, 539, 647, 755,
862, 970, 1078, 1186, 1293, 1401, 1509, 1616, 1724, 1832, 1939,
2047, 2155, 2262, 2370, 2478, 2585, 2693, 2801, 2909, 3016, 3124,
3232, 3339, 3447, 3555, 3662, 3770, 3878, 3985, 4093, 4201, 4308, 4416, 4524,
4632, 4739, 4847, 4955, 5062, 5170, 5278, 5385, 5493, 5601, 5708, 5816,
5924, 6031, 6139, 6247, 6355, 6462, 6570, 6678, 6785, 6893, 7001, 7108,
7216, 7324, 7431, 7539, 7647, 7754, 7862, 7970, 8078, 8185, 8293, 8401,
8508, 8616, 8724, 8831, 8939, 9047, 9154, 9262, 9370, 9477, 9585, 9693,
9801, 9908, 10016, 10124, 10231, 10339, 10447, 10554, 10662 };

class Acces_Radar_Data
{
public:
	Acces_Radar_Data(string radar_log_file_name)
		:radar_log_file_name(radar_log_file_name)
	{
		ifstream radar_log{ radar_log_file_name };
		try
		{
			if (!radar_log)
			{
				throw std::logic_error("Failed to open file");
			}
		}
		catch (...)
		{
			cout << "error occured" << endl;
		}
		vector<string> all_puffer{};
		string puffer{};
		string newline{ "" };

		while (getline(radar_log, newline))
		{


			for (auto x : newline)
			{
				if (x == ';')
				{
					all_puffer.push_back(puffer);
					puffer = "";
					continue;
				}
				puffer += x;
			}
			all_puffer.push_back(puffer);
			puffer = "";
			radar_data.push_back(all_puffer);
			all_puffer.clear();

			if (radar_log.eof())
			{
				break;
			}
		}
		radar_log.close();
	}

	auto GetRadarPtr()
	{
		return make_shared<vector<vector<string>>>(radar_data);
	}

private:
	string radar_log_file_name;
	vector<vector<string>> radar_data;
};


class Acces_CV_Data
{
public:
	Acces_CV_Data(string cv_log_file_name)
		:cv_log_file_name(cv_log_file_name)
	{
		ifstream cv_log{ cv_log_file_name };
		try
		{
			if (!cv_log)
			{
				throw std::logic_error("Failed to open file");
			}
		}
		catch (...)
		{
			cout << "error occured" << endl;
		}
		vector<string> all_puffer{};
		string puffer{};
		string newline{ "" };
		vector<vector<string>> cv_data;

		while (getline(cv_log, newline))
		{

			for (auto x : newline)
			{
				if (x == ';')
				{
					all_puffer.push_back(puffer);
					puffer = "";
					continue;
				}
				puffer += x;
			}
			all_puffer.push_back(puffer);
			puffer = "";
			cv_data.push_back(all_puffer);
			all_puffer.clear();

			if (cv_log.eof())
			{
				break;
			}
		}
		cv_log.close();
		*cv_data_ptr = cv_data;
	}

	auto& GetCVData()
	{
		return *cv_data_ptr;
	}

private:
	string cv_log_file_name;
	shared_ptr <vector<vector<string>>> cv_data_ptr{ new vector<vector<string>>{} };
};


class road_objects
{
public:
	road_objects(const string type, const float frame_time, const int x, const int width, shared_ptr <vector<vector<string>>> Radar_data_ptr)
		:type(type), frame_time(frame_time), x(x), width(width), Radar_data_ptr(Radar_data_ptr)
	{}

private:
	auto CalculateOutput()
	{
		float CvLeftAngle = 0;
		float CvRightAngle = 0;

		if (x < 640 && (x + width) < 640)
		{
			CvLeftAngle = -gamma * (640 - x);
			CvRightAngle = -gamma * (640 - (x + width));
		}
		if (x > 640 && (x + width) > 640)
		{
			CvLeftAngle = gamma * (x - 640);
			CvRightAngle = gamma * ((x + width) - 640);
		}
		if (x < 640 && (x + width) > 640)
		{
			CvLeftAngle = -gamma * (640 - x);
			CvRightAngle = gamma * (x + width - 640);
		}

		float CvMiddleAngle = (CvLeftAngle + CvRightAngle) / 2;

		pair<vector<float>, vector<int>> MatchDiff;

		for (int i = numbers[0]; i < numbers[numbers.size() - 1]; ++i)
		{
			if (stoi((*Radar_data_ptr).at(i).at(6)) == 7 || stoi((*Radar_data_ptr).at(i).at(6)) == 6 || stoi((*Radar_data_ptr).at(i).at(6)) == 5
				|| stoi((*Radar_data_ptr).at(i).at(6)) == 4)
			{
				float phi = -atan2f(stof((*Radar_data_ptr).at(i).at(4)), stof((*Radar_data_ptr).at(i).at(3))) * 180;
				if (phi > (CvLeftAngle - varphi_tolerance) && phi < (CvRightAngle + varphi_tolerance))
				{
					MatchDiff.first.push_back(abs(CvMiddleAngle - phi));
					MatchDiff.second.push_back(i);
				}
			}
		}
		return MatchDiff;
	}

public:
	auto CalculateMatch()
	{
		//I'll have to search the vector number, which ones are relevant
		for (int i = 0; i < size(*Radar_data_ptr); ++i)
		{
			if (stof((*Radar_data_ptr).at(i).at(0)) < RadarTimeMarginUp && stof((*Radar_data_ptr).at(i).at(0)) > RadarTimeMarginDown)
			{
				numbers.push_back(i);
			}

			if (stof((*Radar_data_ptr).at(i).at(0)) > RadarTimeMarginUp)
			{
				break;
			}
		}

		return ReturnString(CalculateOutput());
	}
private:
	vector<string> ReturnString(pair<vector<float>, vector<int>> matched)
	{
		vector<string> temp;
		if (matched.first.size() == 0)
		{
			temp.push_back("empty");
			return temp;
		}

		float diff{ INT16_MAX };
		int ChosenRadarObjectLine{ 0 };
		int RadarLine{ 0 };
		for (int i = 0; i < size(matched.first); ++i)
		{
			if (matched.first[i] < diff)
			{
				diff = matched.first[i];
				RadarLine = matched.second[i];
			}
		}

		temp.push_back(to_string(frame_time));
		temp.push_back(type);
		temp.push_back((*Radar_data_ptr).at(RadarLine).at(3));
		temp.push_back((*Radar_data_ptr).at(RadarLine).at(4));
		temp.push_back(((*Radar_data_ptr).at(RadarLine).at(5)));

		return temp;
	}


	int x{ 0 };
	int width{ 0 };
	float frame_time{ 0 };
	string type{ "" };
	float RadarTimeMarginUp = frame_time + 0.1;
	float RadarTimeMarginDown = frame_time - 0.1;
	shared_ptr <vector<vector<string>>> Radar_data_ptr;
	vector<int> numbers;
};


void CV_parser(const vector<string> line, const float& frame_time, shared_ptr <vector<vector<string>>> Radar_data_ptr)
{
	vector<road_objects> names;

	size_t len = line.size();
	for (size_t i = 0; i < len; ++i)
	{
		if (i % 6 == 0)
		{
			if (stoi(line[i + 2]) < 0)
			{
				continue;
			}
			if (stoi(line[i + 4]) > 1200)
			{
				continue;
			}
			if (line[i] == "traffic_light" || line[i] == "train" || line[i] == "parking_meter" || line[i] == "person")
			{
				continue;
			}
			else
			{
				names.push_back(road_objects(line[i], frame_time, stoi(line[i + 2]), stoi(line[i + 4]), Radar_data_ptr));
			}
		}
	}



//concenate vector
	vector<string> temp;
	vector<vector<string>> Output;
	for (auto& x : names)
	{
		if (x.CalculateMatch()[0] == "empty")
		{
			continue;
		}
		for (auto& x : x.CalculateMatch())
		{
			temp.push_back(x);
		}
		Output.push_back(temp);
		temp.clear();
	}


	vector<int> deletable{};
	for (int k = 0; k < Output.size()-1; ++k)
	{
		for (int l = 1; l < Output.size(); ++l)
		{
			if (k == l)
			{
				continue;
			}

			if (Output[l] == Output[k])
			{
				Output.erase(Output.begin() + l-1);
				l = 1;
				k = 0;
			}

		}
	}



	
	for (auto x : Output)
	{
		for (auto y : x)
		{
			::output_file << y;
			::output_file << ";";
		}
	}

		::output_file << "\n";

}


int main()
{
	auto cv_data = make_unique<Acces_CV_Data>("log_csv.txt");
	auto radar_data = make_unique<Acces_Radar_Data>("radar_1451.txt");
	float frame_time{ 0.0166667 };

	for (auto x : CheckList)
	{

		CV_parser(cv_data->GetCVData()[x - 1], frame_time * (x - 1), radar_data->GetRadarPtr());
	}

	
	output_file.close();
	return 0;
}