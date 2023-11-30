#ifndef ALGO_2_H
#define ALGO_2_H

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <string>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <unordered_map>
#include <map>

// Define a custom hash function for std::pair<int, int>
struct PairHash {
	template <class T1, class T2>
	std::size_t operator () (const std::pair<T1, T2>& p) const {
		auto h1 = std::hash<T1>{}(p.first);
		auto h2 = std::hash<T2>{}(p.second);

		// Combine the hash values of the two elements
		return h1 ^ h2;
	}
};

class Station{
public:
	int id;
    std::string name;
    double lat;
    double lon;
    int cap;
	void Show(){std::cout << "Station " << "id:" << id << " lat:" << lat <<" lon:" << lon << " cap:" << cap << " name:"<< name << std::endl;}
	
};


class TripData {
public:
    std::string start_date_str;
    std::string start_time_str;
    std::string end_date_str;
    std::string end_time_str;
    double duration;
    int start_id;
    int end_id;

    // Default constructor
    TripData() = default;

    // Parameterized constructor
    TripData(
        std::string start_date_str,
        std::string start_time_str,
        std::string end_date_str,
        std::string end_time_str,
        double duration,
        int start_id,
        int end_id
    ) : start_date_str(std::move(start_date_str)),
        start_time_str(std::move(start_time_str)),
        end_date_str(std::move(end_date_str)),
        end_time_str(std::move(end_time_str)),
        duration(duration),
        start_id(start_id),
        end_id(end_id) {}

    void Show() const {
		printf("Trip start:%s end:%s duration:%.1lf from:%d to:%d\n",(start_date_str+'_'+start_time_str).c_str(),(end_date_str+'_'+end_time_str).c_str(),duration,start_id,end_id);
	}
};


class GeneratedTrip {
public:
    int origin;
    int destination;
    int start_time;
    int end_time;
    int scenario;

    // Constructor to initialize GeneratedTrip object
    GeneratedTrip(int _origin = -1, int _destination = -1, int _start_time = -1, int _end_time = -1, int _scenario = -1)
        : origin(_origin), destination(_destination), start_time(_start_time), end_time(_end_time), scenario(_scenario) {}

    void Show() const {
        printf("o:%d d:%d start_time:%d end_time:%d scenario:%d\n", origin, destination, start_time, end_time, scenario);
    }
};

struct BSS_Data {
		// Copy constructor
    BSS_Data(const BSS_Data& other) {
        trips = other.trips;
        stations = other.stations;
    }
	
	//To count the data days in the load method ...
	int dayCounter;
	int nb_days_of_data;
	std::unordered_map<std::string, int> dateToDayMap;
	
	//Takes the trips in trips and generates
	void ConvertToGeneratedTrips();
	
    // Default constructor
    BSS_Data() {
		real_trips.reserve(200);
		trips.reserve(14000000);
        stations.reserve(5000);
	}
	
	std::map<int,std::vector<GeneratedTrip>> real_trips_map;
	
	std::vector<std::vector<GeneratedTrip>> real_trips;
	std::vector<TripData> trips;
    std::vector<Station> stations;
};

class LambdaData {
public:
    int f = 1;//Multiplier > 1 to make more trips per scenario
    int start_id;
    int end_id;
    std::vector<double> duration_vec;
    std::vector<int> counts;
    
    // Function to modify counts based on the value of f
    LambdaData& ModifyCounts() {
        if (f > 1) {
            for (int& count : counts) {
                count *= f;
            }
        }
        return *this;
    }
    
    void Show() {
        printf("o:%d d:%d Durations[s]:", start_id, end_id);
        for (int i = 0; i < duration_vec.size(); i++)
            printf("%.1lf ", duration_vec[i]);
        printf("\nCounts:");
        for (int i = 0; i < counts.size(); i++)
            printf("%d ", counts[i]);
        printf("\n");
    }
};


class MyException : public std::exception {
public:
    MyException(const std::string& message) : message_(message) {}

    virtual const char* what() const noexcept {
        return message_.c_str();
    }

private:
    std::string message_;
};

//The main class ...
class Algorithm{
public:

    Algorithm() {
        lambda_object.reserve(1000000);
		tripIndex.reserve(14000000);
    }
	
	void MakeLambdaObject(std::vector<TripData> * trips, int start_hour, int end_hour);
	std::vector<std::vector<GeneratedTrip>> Make_scenarios(int nb_scenarios, int nb_days_of_data, int start_hour, int end_hour, int instance_no);
	
	std::vector<LambdaData> lambda_object;
	std::unordered_map<std::pair<int, int>, std::vector<TripData>, PairHash> tripIndex;
	
	std::vector<LambdaData>* getLambdaPointer() {
        return &lambda_object;
    }
};


#endif