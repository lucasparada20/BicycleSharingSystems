#include "algo.h"
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <cmath> //std::log
#include <algorithm> //std::min, std::max
#include <ctime>
#include <cstdlib>
#include <iomanip>
#include <set>
#include <unordered_set>
#include <unordered_map> 

// Custom hash function for double keys
struct DoubleHash {
    size_t operator()(const double& key) const {
        // Convert the double to an integer for hashing
        return std::hash<double>{}(key);
    }
};

// Custom equality function for double keys
struct DoubleEqual {
    bool operator()(const double& lhs, const double& rhs) const {
        // Compare doubles within a tolerance epsilon
        double epsilon = 0.0001; // Define your desired precision
        return std::abs(lhs - rhs) < epsilon;
    }
};


class Load {
public:
	
	//Generic method for BSS with commong data format
	void Load_CSV_File(std::string filename_csv, std::vector<Station> stations, int start_hour, int end_hour);	

	//BlueBike methods
	void Load_BlueBike(std::string filename_csv, std::string filename_json, int start_hour, int end_hour);
	
	//CitiBike methods
	void Load_CitiBike(std::string filename_csv, std::string filename_json, int start_hour, int end_hour);
	
	//Bixi methods
	void Load_Bixi(std::string filename_csv, std::string filename_json, int start_hour, int end_hour);
	void Load_Bixi(std::string filename_csv, std::string filename_json, int start_hour, int end_hour, int max_stations_allowed);

	//CapitalBikeShare methods
	void Load_CapitalBikeShare(std::string filename_csv, std::string filename_json, int start_hour, int end_hour);
	void Load_CapitalBikeShare(std::string filename_csv, std::string filename_json, int start_hour, int end_hour, int max_stations_allowed);

	std::vector<TripData> LoadTripsFromTripsObject(std::string filename); 

	BSS_Data CopyAndFilterBSSData(const BSS_Data& original_data, int max_nb_stations);	
	//Maps
	std::unordered_map<std::string, Station> station_name_map;
	std::unordered_map<double, Station, DoubleHash, DoubleEqual> station_lat_map;
	std::unordered_map<double, Station, DoubleHash, DoubleEqual> station_lon_map;

    //The member object that is populated by the methods
    BSS_Data data;
	// Function to return a pointer to the BSS_Data member
    BSS_Data* getDataPointer() {
        return &data;
    }
};

