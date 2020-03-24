#include "pch.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <memory>
#include <math.h>
#include <utility>
#include <tuple>

using namespace std;

#define gamma 0.0691

ofstream output_file{ "results.txt" };


class Acces_Radar_Data
{
public:
	Acces_Radar_Data(string radar_log_file_name)
		:radar_log_file_name(radar_log_file_name)
	{
		ifstream radar_log{ radar_log_file_name };
		try
		{
			if(!radar_log)
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
			if (radar_log.eof())
			{
				break;
			}

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
			if (cv_log.eof())
			{
				break;
			}

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
	{
		CalculateMatch();
	}

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
			CvLeftAngle = gamma * (x-640);
			CvRightAngle = gamma * ((x + width)-640);
		}
		if (x < 640 && (x + width) > 640)
		{
			CvLeftAngle = -gamma * (640 - x);
			CvRightAngle = gamma * (x + width - 640);
		}

		float CvMiddleAngle = (CvLeftAngle + CvRightAngle) / 2;

		pair<vector<float>, vector<int>> MatchDiff;

		for (int i = numbers[0]; i < numbers[numbers.size()-1]; ++i)
		{
			if (stoi((*Radar_data_ptr).at(i).at(6)) == 7 || stoi((*Radar_data_ptr).at(i).at(6)) == 6)
			{
				float phi = -atan2f(stof((*Radar_data_ptr).at(i).at(4)), stof((*Radar_data_ptr).at(i).at(3))) * 180;
				if (phi > CvLeftAngle && phi < CvRightAngle)
				{
					MatchDiff.first.push_back(abs(CvMiddleAngle - phi));
					MatchDiff.second.push_back(i);
				}
			}
		}

		return MatchDiff;
	}

public:
	string CalculateMatch()
	{
		FixIfXIsNeg(x);
		//I'll have to search the vector number, which ones are relevant
		for (int i = 0; i< size(*Radar_data_ptr); ++i)
		{
			if(stof((*Radar_data_ptr).at(i).at(0)) < RadarTimeMarginUp && stof((*Radar_data_ptr).at(i).at(0)) > RadarTimeMarginDown)
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
	string ReturnString(pair<vector<float>, vector<int>> matched)
	{
		if (matched.first.size() == 0)
		{
			return "";
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

		return to_string(frame_time) + ";" + type + ";" + (*Radar_data_ptr).at(RadarLine).at(3) + ";"+ (*Radar_data_ptr).at(RadarLine).at(4) +
			";" + (*Radar_data_ptr).at(RadarLine).at(5) + ";";
	}

	void FixIfXIsNeg(int& x)
	{
		if (x < 0)
		{
			x = 0;
		}
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


void CV_parser(const vector<string> line, const float &frame_time, shared_ptr <vector<vector<string>>> Radar_data_ptr)
{
	vector<road_objects> names;

	size_t len = line.size();
	for (size_t i = 0; i < len; ++i)
	{
		if (i % 6 == 0)
		{
			if(line[i] == "traffic_light" || line[i] == "train")
			{
				continue;
			}
			else
			{
				names.push_back(road_objects(line[i], frame_time, stoi(line[i + 2]), stoi(line[i + 4]), Radar_data_ptr));
			}
		}
	}

	for (auto x : names)
	{
		::output_file << x.CalculateMatch();
	}
	::output_file << "\n";
}

int main()
{
	auto cv_data = make_unique<Acces_CV_Data>("log_csv.txt");
	auto radar_data = make_unique<Acces_Radar_Data>("radar_1451.txt");
	float frame_time{0.0000000};
	for (auto x : cv_data->GetCVData())
	{
		frame_time += 0.0166667;
		CV_parser(x, frame_time, radar_data->GetRadarPtr());


	}

	
	output_file.close();
	return 0;
}