#include "algo.h"
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <cmath> //std::log
#include <algorithm> //std::min, std::max
#include <ctime>
#include <cstdlib>
#include <iomanip>
#include <set>
#include "load.h"
#include <unordered_map>

//Global variables
int nb_days_of_data;
int dayCounter;
int nb_instances;

std::unordered_map<std::string, int> dateToDayMap;

//kwrg parameters;
std::unordered_map<std::string, std::string> parameters; //The container

std::pair<double, double> computeMeanAndVariance(std::vector<double> duration_vec) {
    int n = duration_vec.size();

    if (n == 0) {
        // Handle the case of an empty vector
        throw MyException("Empty vector - cannot compute mean and variance.");
    }

    double sum = 0.0;
    double sum_of_squares = 0.0;

    // Calculate the sum and sum of squares
    for (double x : duration_vec) {
        sum += x;
        sum_of_squares += x * x;
    }

    double mean = sum / (double)n;
    double variance = (sum_of_squares / n) - (mean * mean);

    return std::make_pair(mean, variance);
}

std::pair<double,double> computeAlphaAndBeta(double mean, double variance)
{
	double alpha = 0.0; double beta=0.0;
	alpha = mean*( ( mean*(1-mean) )/variance -1 );
	beta = (1-mean)*( ( mean*(1-mean) )/variance -1 );
	
	if (alpha < 0.0 || beta < 0.0) {
		std::string errorMsg;
		errorMsg = "Could not compute Alpha, Beta -> Alpha: " + std::to_string(alpha) + " Beta: " + std::to_string(beta);

		throw MyException(errorMsg);
	}
			
	return std::make_pair(alpha,beta);
}

std::vector<double> scale_to_01(std::vector<double> vec) {
    double min_val = *std::min_element(vec.begin(), vec.end());
    double max_val = *std::max_element(vec.begin(), vec.end());

    if (min_val == max_val) {
		std::string errorMsg;
		errorMsg = "Could not scale duration_vec. Min_element:" + std::to_string(min_val) + " is equal to Max_element:" + std::to_string(max_val)	;

		throw MyException(errorMsg);
    }

    std::vector<double> scaled_vec;
    for (double x : vec) {
        double scaled_value = (x - min_val) / (max_val - min_val);
        scaled_vec.push_back(scaled_value);
    }

    return scaled_vec;
}

double descale_from_01(double scaled_value, double min_val, double max_val) 
{
    double descaled_value = scaled_value * (max_val - min_val) + min_val;
    return descaled_value;
}

void Algorithm::MakeLambdaObject(std::vector<TripData> * trips, int start_hour, int end_hour) 
{
	int T = end_hour - start_hour;
	printf("Making lambda_object ...\n");
	printf("Sample trip object:\n"); (*trips)[0].Show();
	lambda_object.clear();
	// Initialize elapsed time tracking variables
    clock_t startTime = clock();
	clock_t currentTime = clock();
	
	// Initialize the maps with unique pairs
	//std::pair<int, int>: start_id, end_id of trip
	//std::pair<std::vector<double>: duration in seconds of the trip
	//std::vector<int>: counts of the trips in the interval hours of 8am,9am,...,10pm
	std::map<std::pair<int, int>, std::pair<std::vector<double>, std::vector<int>>> unique_pairs;
    for (int i=0; i<trips->size(); i++) 
	{
		if ((i + 1) % (trips->size() / 10) == 0) 
		{			
			// Print your output here, e.g., percentage completion
			double percentage = (double)(i + 1) / trips->size() * 100.0;
			currentTime = clock();
			double elapsedSeconds = (double)(currentTime - startTime) / CLOCKS_PER_SEC;
			std::cout << "Processed " << (int)(percentage+0.5) << "% of trips. ElapsedTime:" << std::fixed << std::setprecision(2) << elapsedSeconds << std::endl;
		}
		
		std::pair<int, int> pair((*trips)[i].start_id, (*trips)[i].end_id);
        //Check if the pair is not already in the set before inserting
        if (unique_pairs.find(pair) == unique_pairs.end())
		{
			//The unique pair has not been created ...
			auto& unique_pair = unique_pairs[pair];
			
			unique_pair.first.push_back((*trips)[i].duration);
			unique_pair.second = std::vector<int>(T, 0);
			
			int colonPos = (*trips)[i].start_time_str.find(':');
			if (colonPos != std::string::npos) {
				std::string hour_str = (*trips)[i].start_time_str.substr(0, colonPos);
				int hour = std::stoi(hour_str);

				if (hour >= start_hour && hour <= end_hour-1) {
					unique_pair.second[hour - start_hour]++;
				} else {
					(*trips)[i].Show();
					throw MyException("Wrong hour extracted at the following trip. Did you forget to filter the trips in relation to the times...");
				}
			}			
		}else { //The unique pair already exists
		//Update duration and count
			
			auto& unique_pair = unique_pairs[pair];
			unique_pair.first.push_back((*trips)[i].duration);
			
			int colonPos = (*trips)[i].start_time_str.find(':');
			if (colonPos != std::string::npos) {
				std::string hour_str = (*trips)[i].start_time_str.substr(0, colonPos);
				int hour = std::stoi(hour_str);

				if (hour >= start_hour && hour <= end_hour-1) {
					unique_pair.second[hour - start_hour]++;
				} else {
					(*trips)[i].Show();
					throw MyException("Wrong hour extracted at the following trip. Did you forget to filter the trips in relation to the times...");
				}
			}		
				
		}			
    }
	currentTime = clock();
	printf("Unique pairs:%d Time taken[s]:%.1lf\n",(int)unique_pairs.size(),(double)((currentTime - startTime) / CLOCKS_PER_SEC));
	//exit(1);
	
	for (auto it = unique_pairs.begin(); it != unique_pairs.end(); ++it) {
		
		LambdaData lambda;
		const auto& unique_pair = it->first; // Access the key pair

		lambda.start_id = unique_pair.first; // Access the first integer
		lambda.end_id = unique_pair.first; // Access the second integer

		// Accessing vectors from the map using the key
		const auto& vectors_pair = it->second; // Access the pair of vectors
		lambda.duration_vec = vectors_pair.first; // Access the vector of durations
		lambda.counts = vectors_pair.second; // Access the vector of counts
		
		lambda_object.push_back(lambda);
	}

	
	//To check
	int count=0;
	for(int i=0; i<lambda_object.size(); i++)
		for(int t=0; t<T; t++)
		{
			count+=lambda_object[i].counts[t];
		}
	
	if(count!=trips->size())
	{
		printf("Lambda_object has different trips than trips object. Exiting ...\n");
		exit(1);
	}else{printf("Correct count of trips from lambda_object equals nb trips! count from lambda_object:%d nb_trips:%d\n",count,(int)trips->size());}
	// Print elapsed time
    clock_t endTime = clock();
    double elapsedSeconds = (double)(endTime - startTime) / CLOCKS_PER_SEC;
    std::cout << "Total Elapsed Time for lambda_object: " << elapsedSeconds << " seconds" << std::endl;
	printf("Size of lambda_object:%d\n",(int)lambda_object.size());

}


/*void Algorithm::MakeLambdaObject(std::vector<TripData> * trips, int start_hour, int end_hour) 
{
	int T = end_hour - start_hour;
	printf("Making lambda_object ...\n");
	printf("Sample trip object:\n"); (*trips)[0].Show();
	lambda_object.clear();
	// Initialize elapsed time tracking variables
    clock_t startTime = clock();
    clock_t lastPrintTime = startTime;
	
	// Initialize the lambda_object with unique pairs
	std::set<std::pair<int, int>> unique_pairs;
    for (int i=0; i<trips->size(); i++) 
	{
		std::pair<int, int> pair((*trips)[i].start_id, (*trips)[i].end_id);
        //Check if the pair is not already in the set before inserting
        if (unique_pairs.find(pair) == unique_pairs.end()) 
            unique_pairs.insert(pair);	
    }
	printf("Unique pairs from trips_object:%d Time taken[s]:%.1lf\n",(int)unique_pairs.size(),(double)((clock() - lastPrintTime) / CLOCKS_PER_SEC));

	// Create a map to index trips by start_id and end_id: For each index, the map will hold an std::vector<TripData> i.e. a vector of trips
	//std::unordered_map is faster than std::map ...	
	printf("Generating a map of all trips where the index is the pair start_id, end_id ... \n");
	// Index trips by start_id and end_id
	//tripIndex is of the form: std::unordered_map<std::pair<int, int>, std::vector<TripData>, PairHash> tripIndex;
	for(int i=0; i<trips->size(); i++)
	{
		std::pair<int, int> pair((*trips)[i].start_id, (*trips)[i].end_id);
		tripIndex[pair].push_back((*trips)[i]);
		
		if ((i + 1) % (trips->size() / 10) == 0) 
		{
			// Print your output here, e.g., percentage completion
			double percentage = (double)(i + 1) / trips->size() * 100.0;
			std::cout << "Processed " << percentage << "% of trips" << std::endl;
		}
	}

	int total_iterations = unique_pairs.size();
	int iterations_to_print = total_iterations / 10; // Print after nb% of iterations
	int iteration_count = 0;
	clock_t start_time = clock(); // Record start time
	printf("Computing the counts of each unique pair in the [8h,22h] intervals ...\n");
	
	//Fast: Const reference are required otherwise the code gets killed ...
	for (const auto& unique_pair : unique_pairs) 
	{
		const std::vector<TripData>& matching_trips = tripIndex[unique_pair];

		LambdaData lambda;
		lambda.start_id = unique_pair.first;
		lambda.end_id = unique_pair.second;
		lambda.counts.resize(T);
		try
		{
			for (const TripData& trip : matching_trips) 
			{
				lambda.duration_vec.push_back(trip.duration);

				int colonPos = trip.start_time_str.find(':');
				if (colonPos != std::string::npos) {
					std::string hour_str = trip.start_time_str.substr(0, colonPos);
					int hour = std::stoi(hour_str);

					if (hour >= start_hour && hour <= end_hour-1) {
						lambda.counts[hour - start_hour]++;
					} else {
						trip.Show();
						throw MyException("Wrong hour extracted at the following trip. Did you forget to filter the trips in relation to the times...");
					}
				}
			}
		} catch (const MyException& e)
		{
			std::cerr << "Exception encountered: " << e.what() << std::endl;
			continue;
		}

		lambda_object.push_back(lambda);
		
		iteration_count++;
		if (iteration_count % iterations_to_print == 0) {
			clock_t end_time = clock();
			double elapsed_time = (double)(end_time - start_time) / CLOCKS_PER_SEC;
			std::cout << "Processed " << iteration_count << " out of " << total_iterations << " unique pairs (" << elapsed_time << "s elapsed)" << std::endl;
		}
	}

	int count=0;
	for(int i=0; i<lambda_object.size(); i++)
		for(int t=0; t<T; t++)
		{
			count+=lambda_object[i].counts[t];
		}
	
	if(count!=trips->size())
	{
		printf("Lambda_object has different trips than trips object. Exiting ...");
		exit(1);
	}else{printf("Correct count of trips from lambda_object equals nb trips! count from lambda_object:%d nb_trips:%d\n",count,(int)trips->size());}
	// Print elapsed time
    clock_t endTime = clock();
    double elapsedSeconds = (double)(endTime - startTime) / CLOCKS_PER_SEC;
    std::cout << "Total Elapsed Time for lambda_object: " << elapsedSeconds << " seconds" << std::endl;
	printf("Size of lambda_object:%d\n",(int)lambda_object.size());

}*/


int ComputeDaysOfData(const std::vector<TripData>* tripsPtr)
{
	clock_t startTime = clock();
	
	printf("Computing nb_days_of_data ...\n");
	int nb_days_of_data=0;
    // Initialize variables to track earliest and latest dates
    std::tm earliestDate = {0};
    std::tm latestDate = {0};
    bool firstDate = true;
	const std::vector<TripData>& trips = *tripsPtr;
	for(int i=0; i<trips.size(); i++)
	{
		std::tm date = {0};
		std::istringstream dateStream(trips[i].start_date_str);
		dateStream >> std::get_time(&date, "%Y-%m-%d"); 
		if (firstDate) 
		{
			earliestDate = latestDate = date;
            firstDate = false;
        }else 
		{
			if (mktime(&date) < mktime(&earliestDate)) 
			{
                earliestDate = date;
            }
            if (mktime(&date) > mktime(&latestDate)) 
			{
                latestDate = date;
            }
        }
	}
	
	// Calculate the difference in days
    double diffInSeconds = difftime(mktime(&latestDate), mktime(&earliestDate));
    nb_days_of_data = (int)(diffInSeconds / (60 * 60 * 24));
	
	printf("nb_days_of_data:%d\n",nb_days_of_data);

	//To make the real trips
	dayCounter=0;
    for (const auto& trip : trips) 
	{
		// Check if the date already exists in the map
		if (dateToDayMap.find(trip.start_date_str) == dateToDayMap.end()) 
			dateToDayMap[trip.start_date_str] = dayCounter++;
    }
	printf("dayCounter:%d\n",dayCounter);

	// Print elapsed time
    clock_t endTime = clock();
    double elapsedSeconds = (double)(endTime - startTime) / CLOCKS_PER_SEC;
    std::cout << "Elapsed time to compute nbDaysOfData: " << std::setprecision(3) << elapsedSeconds << " seconds" << std::endl;
	
	return nb_days_of_data;
}

std::vector<std::vector<GeneratedTrip>> Algorithm::Make_scenarios(int nb_scenarios, int nb_days_of_data, int start_hour, int end_hour, int instance_no)
{
	std::vector<int> start_hours;
	for(int i=start_hour; i<end_hour; i++) start_hours.push_back(i);
	int nb_time_intervals = (int)start_hours.size();
	int end_of_day_time = 60*nb_time_intervals;
	
	//Initialize GSL pointers and environment
	const gsl_rng_type* T;
    gsl_rng* r;
	gsl_rng_env_setup();
    T = gsl_rng_default;
    r = gsl_rng_alloc(T);
	int seed = nb_scenarios*instance_no+1;
	gsl_rng_set(r, seed); //Seeding the random numbers
	
	printf("In Make_scenarios with nb_time_intervals:%d start_of_day_hour (t=0):%02dh end_of_day_time[minutes]:%d seed:%d\n",nb_time_intervals,start_hours[0],end_of_day_time,seed);	
	std::vector<std::vector<GeneratedTrip>> global_trips;
	for(int e=0; e<nb_scenarios; e++)
	{
		std::vector<GeneratedTrip> scenario_trips;
		for(int i=0; i<lambda_object.size(); i++)
		{
			if(i%50000 == 0)
				printf("sce:%d looping at pair:%d ...\n",e,i);
			
			LambdaData od = lambda_object[i];
			if(od.duration_vec.size()==1) continue;
			if(od.counts.size() < start_hours.size())
			{
				printf("The following OD pair has less lambdas than the amount of specified time intervals. Exiting ..."); od.Show(); exit(1);
			}
			for(int start_h=0; start_h<start_hours.size(); start_h++)
			{
				double lambda_val = od.counts[start_h]/(60.0*(double)nb_days_of_data);
				if(od.counts[start_h] == 0) continue;
							
				int t=0;
				while(true)
				{
					double u = gsl_rng_uniform(r);
					//printf("u:%.1lf\n",u);
					//getchar();
					int t_next = t + std::ceil(-std::log(1 - u) / lambda_val);
					if(t_next > 60) break;
					
					std::vector<double> duration_vec = od.duration_vec;
					
					double alpha = 0.0; double beta = 0.0;
					try{
						
						std::vector<double> duration_vec_01 = scale_to_01(duration_vec);
						// Fit the beta distribution using the method of moments
						std::pair<double, double> result = computeMeanAndVariance(duration_vec_01);
						std::pair<double,double> parameter = computeAlphaAndBeta(result.first,result.second);
						
						//std::cout << "Mean:" << result.first << ", Variance:" << result.second << std::endl;
						//std::cout << "Alpha:" << parameter.first << ", Beta:" << parameter.second << std::endl;
						//getchar();
						alpha = parameter.first; beta = parameter.second;
					}catch (MyException& ex) {
						std::cout << "Caught exception: " << ex.what() << " for the following OD pair:" <<std::endl;
						od.Show();
						//getchar();
						break;
					}
					
					if(alpha < 0.0001 || beta < 0.0001) 
						break;
					
					double y_01 = gsl_ran_beta(r, alpha, beta);
					double min_val = *std::min_element(duration_vec.begin(), duration_vec.end());
					double max_val = *std::max_element(duration_vec.begin(), duration_vec.end());
					int y = (std::ceil(descale_from_01(y_01,min_val,max_val)))/60;
					if(y<0)
					{
						printf("Negative y:%d\nYou made a mistake somewhere! Exiting ...",y); od.Show(); exit(1);
					}
					
					if( y>=1 && t_next + start_h*60 + y <= end_of_day_time )
					{
						//Generate OD trip
						GeneratedTrip trip;
						trip.origin=od.start_id; trip.destination=od.end_id; trip.start_time=t_next + start_h*60; trip.end_time=t_next + start_h*60+y; trip.scenario=e;
						scenario_trips.push_back(trip);
						
						if((int)scenario_trips.size()%2000 == 0)
							printf("sce:%d generated trips:%d ...\n",e,(int)scenario_trips.size());
					}
					t=t_next;
				}
				
			}
		}
		global_trips.push_back(scenario_trips);
	}
	gsl_rng_free(r);
	
	return global_trips;
}


// Helper function to convert time string to minutes since midnight
int TimeStringToMinutes(const std::string& time_str) 
{
    std::istringstream stream(time_str);
    int hours, minutes;
    char colon;
    stream >> hours >> colon >> minutes;

    return hours * 60 + minutes;
}

void BSS_Data::ConvertToGeneratedTrips()
{
    printf("Making the real trips ...\n");
	for (const auto& trip : trips) 
	{	
		GeneratedTrip generated_trip;
        generated_trip.origin = trip.start_id;
        generated_trip.destination = trip.end_id;

        // Convert start_time_str and end_time_str to minutes
        generated_trip.start_time = TimeStringToMinutes(trip.start_time_str);
        generated_trip.end_time = TimeStringToMinutes(trip.end_time_str);
		//Discard the trips that start on one day an end on the next ...
		if(generated_trip.end_time>(60*24)) continue;
		generated_trip.scenario = dateToDayMap[trip.start_date_str];
		
		real_trips_map[generated_trip.scenario].push_back(generated_trip);
    }
	
	int max_scenario = 0;
	if (!real_trips_map.empty()) {
		max_scenario = real_trips_map.rbegin()->first;
	}
	printf("Maximum scenario in real trips: %d\n", max_scenario);

}

void writeTripsToFile(const std::map<int, std::vector<GeneratedTrip>> *trips, std::string filename) {
    FILE* file = fopen(filename.c_str(), "w");
    if (!file) {
        std::cerr << "Failed to open trips output file: " << filename << std::endl;
        return;
    }
    printf("Printing output trips to file:%s\n", filename.c_str());

    for (auto it = trips->begin(); it != trips->end(); ++it) {
        int scenario = it->first;
        const std::vector<GeneratedTrip>& trips_vec = it->second;

        for (const auto& trip : trips_vec) {
            fprintf(file, "%d %d %d %d %d\n",
                    trip.origin, trip.destination, trip.start_time, trip.end_time, scenario);
        }
    }
    fclose(file);
}

void writeStationsToFile(const std::vector<Station>& stations, const std::string& filename, const int& Qtot) {
    FILE* output_file = fopen(filename.c_str(), "w");

    if (!output_file) {
        std::cerr << "Error: Unable to open the station output file." << std::endl;
        return;
    }
	printf("Printing output stations to file:%s\n",filename.c_str());
    fprintf(output_file, "%d\n", (int)stations.size());
    fprintf(output_file, "%d\n",Qtot);

    // Initialize random number generator
    std::srand((unsigned int)(std::time(nullptr)));

    for (const Station& station : stations)
        fprintf(output_file, "%d ", station.cap);
	fprintf(output_file,"\n");
	for (const Station& station : stations)
		fprintf(output_file, "%d ", std::rand() % (station.cap + 1));
	fprintf(output_file,"\n");
	
	double min_lat = 9999999.0;
	double max_lat = -999999.0; 
	double min_lon = 9999999.0;
	double max_lon = -999999.0; 
	for (const Station& station : stations) 
	{
        if (station.lat < min_lat) min_lat = station.lat;
        if (station.lat > max_lat) max_lat = station.lat;
        if (station.lon < min_lon) min_lon = station.lon;
        if (station.lon > max_lon) max_lon = station.lon;
	}


    for (const Station& station : stations) {
        double scaled_lat = (station.lat - min_lat) * 100.0 / (max_lat - min_lat);
        double scaled_lon = (station.lon - min_lon) * 100.0 / (max_lon - min_lon);

        fprintf(output_file, "%d ", station.id);
        fprintf(output_file, "%.1f ", scaled_lat);
        fprintf(output_file, "%.1f\n", scaled_lon);
    }

    fclose(output_file);
}

void ProcessKeywordParameters(int argc, char* argv[]) {
    // Loop through the arguments starting from index 1
    for (int i = 1; i < argc; ++i) {
        std::string argument = argv[i];

        // Check if the argument contains "="
        size_t pos = argument.find('=');
        if (pos != std::string::npos) {
            // Split the argument into parameter name and value
            std::string parameter_name = argument.substr(0, pos);
            std::string parameter_value = argument.substr(pos + 1);

            parameters[parameter_name] = parameter_value;
        }
    }

    // Print the parsed parameters
    for (const auto& param : parameters) {
        std::cout << "Parameter Name: " << param.first << ", Value: " << param.second << std::endl;
    }
}


int main(int argc, char* argv[])
{
	ProcessKeywordParameters(argc, argv);
	
	if (parameters.find("trips_data_file") == parameters.end() ||
		parameters.find("stations_data_file") == parameters.end() ||
		parameters.find("instance_type") == parameters.end() ||
		parameters.find("output_trips") == parameters.end()) {
		
		printf("Wrong usage: You did not specify the trips, station data and/or the output trips to be random or the real ones. Exiting. Hit `make usage' for options ... \n");
		exit(1);
	}
	auto tripsData = parameters.find("trips_data_file");
	auto stationsData = parameters.find("stations_data_file");
	auto instanceType = parameters.find("instance_type");
	auto outputTrips = parameters.find("output_trips");

	printf("Executing generator for %s and %s of type %s ... Output trips:%s \n",tripsData->second.c_str(),stationsData->second.c_str(),instanceType->second.c_str(),outputTrips->second.c_str());
	
	clock_t startTime = clock();
	int start_hour = std::stoi(parameters["start_hour"]); int end_hour = std::stoi(parameters["end_hour"]); 
	
	int nb_scenarios=0; nb_instances=0; int max_stations_allowed=0;
	if (parameters.find("nb_scenarios") != parameters.end()) {
        nb_scenarios = std::stoi(parameters["nb_scenarios"]);
    } else {
        std::cout << "Parameter 'nb_scenarios' not provided." << std::endl;
    }
	if (parameters.find("nb_instances") != parameters.end()) {
        nb_instances = std::stoi(parameters["nb_instances"]);
    } else {
        std::cout << "Parameter 'nb_instances' not provided." << std::endl;
    }
	if (parameters.find("max_stations_allowed") != parameters.end()) {
        max_stations_allowed = std::stoi(parameters["max_stations_allowed"]);
    } else {
        std::cout << "Parameter 'max_stations_allowed' not provided." << std::endl;
    }

	nb_days_of_data = 0;
	int Qtot = 0;
	BSS_Data* pData; Load load_object;
	
	if(max_stations_allowed==0)
	{
		printf("Setting nb_instances:%d nb_scenarios:%d start_hour:%d end_hour:%d\n",nb_instances,nb_scenarios,start_hour,end_hour);
		
		if(instanceType->second=="bixi")
			load_object.Load_Bixi(tripsData->second,stationsData->second,start_hour,end_hour);
		if(instanceType->second=="cbs")
			load_object.Load_CapitalBikeShare(tripsData->second,stationsData->second,start_hour,end_hour);
		if(instanceType->second=="citibike")	
			load_object.Load_CitiBike(tripsData->second,stationsData->second,start_hour,end_hour);
		if(instanceType->second=="bluebike")	
			load_object.Load_BlueBike(tripsData->second,stationsData->second,start_hour,end_hour);
	}
	else{ 
		printf("Setting nb_instances:%d nb_scenarios:%d max_stations_allowed:%d start_hour:%d end_hour:%d\n",nb_instances,nb_scenarios,max_stations_allowed,start_hour,end_hour);
		
		if(instanceType->second=="bixi")
			load_object.Load_Bixi(tripsData->second,stationsData->second,start_hour,end_hour,max_stations_allowed);
		if(instanceType->second=="cbs")
			load_object.Load_CapitalBikeShare(tripsData->second,stationsData->second,start_hour,end_hour,max_stations_allowed);
	}
	pData = load_object.getDataPointer();
	
	printf("back in the main ...\n");	
	//getchar();
	
	//This is now done directly while loading the data ...
	//nb_days_of_data = ComputeDaysOfData(&(pData->trips));
	
	printf("With %d trips and %d days of data then each scenario should have on average %d trips ...\n",(int)pData->trips.size(),pData->nb_days_of_data,(int)pData->trips.size()/pData->nb_days_of_data);
	
	Algorithm algo;
	algo.MakeLambdaObject(&(pData->trips),start_hour,end_hour);
	std::vector<LambdaData> * lambda_object = algo.getLambdaPointer();
	
	if(instanceType->second=="bluebike") Qtot = 4000; //Boston
	if(instanceType->second=="cbs") Qtot = 6000; //Washington D.C.
	if(instanceType->second=="bixi") Qtot = 10000; //Montréal
	if(instanceType->second=="citibike") Qtot = 32000; //NYC
	
	printf("Qtot:%d\n",Qtot);
	
	char filename_stations[100];
	if(instanceType->second=="bixi")
		snprintf(filename_stations,sizeof(filename_stations),"../instances/BIXI_n%d.txt",(int)pData->stations.size());
	if(instanceType->second=="cbs")
		snprintf(filename_stations,sizeof(filename_stations),"../instances/CBS_n%d.txt",(int)pData->stations.size());
	if(instanceType->second=="citibike")
		snprintf(filename_stations,sizeof(filename_stations),"../instances/CITI_n%d.txt",(int)pData->stations.size());
	if(instanceType->second=="bluebike")
		snprintf(filename_stations,sizeof(filename_stations),"../instances/BLUE_n%d.txt",(int)pData->stations.size());	
	writeStationsToFile(pData->stations,filename_stations,Qtot);

		
	if(outputTrips->second == "real")
	{
		char * filename_trips;
		pData->ConvertToGeneratedTrips();
		if(instanceType->second=="bixi")
			filename_trips = (char*)"../instances/BIXI_real_trips.txt";
		if(instanceType->second=="cbs")
			filename_trips = (char*)"../instances/CBS_real_trips.txt";
		if(instanceType->second=="citibike")
			filename_trips = (char*)"../instances/CITI_real_trips.txt";
		if(instanceType->second=="bluebike")
			filename_trips = (char*)"../instances/BLUE_real_trips.txt";

		writeTripsToFile(&(pData->real_trips_map), filename_trips);
		printf("Real trips written to %s.\n",filename_trips);
	}
	if(outputTrips->second == "random")
	{
		//ModifyCounts() is hard coded in the header file. If > 1, then it increases the number of generated random trips
		for(int i=0;i<lambda_object->size();i++)
			(*lambda_object)[i].ModifyCounts();
		
		for(int i=0;i<nb_instances;i++)
		{
			//Create a matrix where first index is the scenario and second index is an std::vector of random trips			
			std::vector<std::vector<GeneratedTrip>> trips = algo.Make_scenarios(nb_scenarios,pData->nb_days_of_data,start_hour,end_hour,i+1); //last integer is to seed the random generator
			
			printf("Instance:%d Generated scenarios:\n",i);
			for(int e=0;e<nb_scenarios;e++)
				printf("Scenario %d:%d\n",e,(int)trips[e].size());
			
			char filename_trips[100];
			
			if(instanceType->second=="bixi")
				snprintf(filename_trips,sizeof(filename_trips),"../instances/BIXI_trips%d.txt",i);
			if(instanceType->second=="cbs")
				snprintf(filename_trips,sizeof(filename_trips),"../instances/CBS_trips%d.txt",i);
			if(instanceType->second=="bluebike")
				snprintf(filename_trips,sizeof(filename_trips),"../instances/BLUE_trips%d.txt",i);
			if(instanceType->second=="citibike")
				snprintf(filename_trips,sizeof(filename_trips),"../instances/CITI_trips%d.txt",i);

			printf("Instance:%d Random trips written to %s.\n",i,filename_trips);
			writeTripsToFile(&trips, filename_trips);
		}		
	}

	// Print elapsed time
    clock_t endTime = clock();
    double elapsedSeconds = (double)(endTime - startTime) / CLOCKS_PER_SEC;
    std::cout << "Total generator time: " << std::setprecision(3) << elapsedSeconds << " seconds" << std::endl;
	
	return 0;
}


