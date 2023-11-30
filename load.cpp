#include "load.h"
#include "json.hpp"
#include "csv.h"

void Load::Load_CSV_File(std::string filename_csv, std::vector<Station> stations, int start_hour, int end_hour)
{
	printf("Loading using csv.h library ...\n");
	
	//to count the data days ... Initialize variables to track earliest and latest dates
    std::tm earliestDate = {0};
    std::tm latestDate = {0};
    bool firstDate = true;
	data.dayCounter = 0; data.nb_days_of_data = 0;
	
	// Open files
    std::ifstream file_csv(filename_csv); 
    if (!file_csv.is_open()) {
        std::cerr << "Error: Could not open csv file " << filename_csv << " " << ". Exiting ..." <<std::endl;
        exit(1);
    }
	//std::string line;
	//std::getline(file_csv, line); //the headers of the columns
	//std::cout << line <<std::endl;
	//std::getline(file_csv, line); //the next line 
	//std::cout << line <<std::endl;
	//file_csv.close();
	
	std::vector<TripData> trips;
	int line_counter=0; int exception_counter=0; int no_id_counter =0; int out_of_time =0; int no_time_difference=0; int trip_with_missing_data=0;
	
	int nb_found_by_name, nb_found_from_coord = 0;
	
	try{		
		//io::CSVReader<13> reader(filename_csv); // Create a CSV reader with 13 columns
		io::CSVReader<13, io::trim_chars<>, io::double_quote_escape<',','\"'>, io::throw_on_overflow, io::no_comment> reader(filename_csv);

		reader.read_header(io::ignore_extra_column,
						   "ride_id","rideable_type","started_at","ended_at","start_station_name","start_station_id",
						   "end_station_name","end_station_id","start_lat","start_lng","end_lat","end_lng","member_casual");

		std::string ride_id, rideable_type, started_at, ended_at, start_station_name, end_station_name, member_casual;
		std::string start_station_id_str, end_station_id_str; 
		std::string start_lat_str; std::string start_lng_str; std::string end_lat_str; std::string end_lng_str;
		
		int row_counter=0;
		
		while (reader.read_row(ride_id,rideable_type,started_at,ended_at,start_station_name,start_station_id_str,end_station_name,end_station_id_str,start_lat_str,start_lng_str,end_lat_str,end_lng_str,member_casual)) 
		{
			row_counter++;

			//std::cout << "Row: ";
				//std::cout << ride_id << ", " << rideable_type << ", " << started_at << ", " << ended_at << ", "
              //<< start_station_name << ", " << start_station_id_str << ", " << end_station_name << ", "
              //<< end_station_id_str << ", " << start_lat_str << ", " << start_lng_str << ", "
              //<< end_lat_str << ", " << end_lng_str << ", " << member_casual << std::endl;
			//getchar();
			if (row_counter % 500000 == 0) 
				std::cout << "Read " << row_counter << " lines from .csv ..." << std::endl;
			
			// Check if any of the required columns are empty
			if (started_at.empty() || ended_at.empty() || start_station_name.empty() || end_station_name.empty() || start_lat_str.empty() || start_lng_str.empty() || end_lat_str.empty() || end_lng_str.empty()) {
				trip_with_missing_data++; continue; // Skip this row if any required column is empty
			}

			double start_lat_double; double start_lon_double; double end_lat_double; double end_lon_double;
			try
			{
				start_lat_double = std::stod(start_lat_str); start_lon_double = std::stod(start_lng_str);
				end_lat_double = std::stod(end_lat_str); end_lon_double = std::stod(end_lng_str);				
			} catch (const std::exception& e)
			{
				exception_counter++;
				//std::cout << line << std::endl;
				//for(int i=0; i<tokens.size(); i++)
					//std::cout << tokens[i] << " ";
				//printf("\n");
				//getchar();
				//std::cerr << "Exception encountered: " << e.what() << std::endl;
				continue;
			}
						
			std::string start_date_str, start_time_str;
			std::string end_date_str, end_time_str;

			// Find the position of the space character separating date and time
			size_t spacePos = started_at.find(' ');
			if (spacePos != std::string::npos) {
				// Extract the date and time substrings
				start_date_str = started_at.substr(0, spacePos);
				start_time_str = started_at.substr(spacePos + 1);
			}
			spacePos = ended_at.find(' ');
			if (spacePos != std::string::npos) {
				// Extract the date and time substrings
				end_date_str = ended_at.substr(0, spacePos);
				end_time_str = ended_at.substr(spacePos + 1);
			}

			int hour, minute, second;
			if (sscanf(start_time_str.c_str(), "%d:%d:%d", &hour, &minute, &second) == 3) 
			{
				if (hour < start_hour || hour > end_hour-1) 
				{
					out_of_time++; continue;
				}
			}
			
			double duration_seconds = 0.0;
			// Parse the date and time strings
			struct std::tm time1 = {};
			struct std::tm time2 = {};
			if (strptime((start_date_str + " " + start_time_str).c_str(), "%Y-%m-%d %H:%M:%S", &time1) &&
				strptime((end_date_str + " " + end_time_str).c_str(), "%Y-%m-%d %H:%M:%S", &time2)) {
				// Calculate the time difference in seconds
				duration_seconds = std::difftime(std::mktime(&time2), std::mktime(&time1));

				//std::cout << "Duration in seconds: " << duration_seconds << std::endl;
			} else {
				std::cerr << "Failed to parse time strings." << std::endl;
				no_time_difference++; std::cout << start_time_str << " " << end_time_str << std::endl; getchar();
			}
			if(duration_seconds<60.0) continue;					
			
			TripData trip;	
			trip.start_date_str = start_date_str;
			trip.start_time_str = start_time_str;
			trip.end_date_str = end_date_str;
			trip.end_time_str = end_time_str;
			trip.duration = duration_seconds;
			trip.start_id = -1; trip.end_id = -1;
			
			auto it_start = station_name_map.find(start_station_name);
			auto it_end = station_name_map.find(end_station_name);
			
			if(it_start != station_name_map.end() && it_end != station_name_map.end())
			{
				trip.start_id = it_start->second.id;
				trip.end_id = it_end->second.id;
				nb_found_by_name++;
				//trip.Show();
				//getchar();
			} else {
				auto it_start_lat = station_lat_map.find(start_lat_double);
				auto it_start_lon = station_lon_map.find(start_lon_double);
				auto it_end_lat = station_lat_map.find(end_lat_double);
				auto it_end_lon = station_lon_map.find(end_lon_double);
				
				
				if(it_start_lat != station_lat_map.end() && it_start_lon != station_lon_map.end() && it_end_lat != station_lat_map.end() && it_end_lon != station_lon_map.end() )
				{
					trip.start_id = it_start_lat->second.id;
					trip.end_id = it_end_lat->second.id;
					//it_start_lat->second.Show(); getchar();
					nb_found_from_coord++;
				}
			}
			
			if(trip.start_id==-1 || trip.end_id==-1)
			{
				no_id_counter++;
				//trip.Show();
				//std::cout << "start_lat:" << start_lat << " start_lon:" << start_lon << std::endl; 
				//std::cout << "end_lat:" << end_lat << " end_lon:" << end_lon << std::endl;
				//getchar();
				continue; //printf("Could not find station ids ... \n"); //getchar();
			}
			
			//trip.Show();
			//getchar();
			data.trips.push_back(trip);	

			//to count the data days ...
			std::tm date = {0};
			std::istringstream dateStream(trip.start_date_str);
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
			if (data.dateToDayMap.find(trip.start_date_str) == data.dateToDayMap.end()) 
			data.dateToDayMap[trip.start_date_str] = data.dayCounter++;
			
		}

	}catch (const io::error::no_digit& e) {
        std::cerr << "Error: Couldn't read the CSV file. " << e.what() << std::endl;
    } catch (MyException& e) {
        std::cerr << "Error: An unknown error occurred while reading the CSV file." << e.what() << std::endl;
    }
	
	printf("The .csv has %d rows ...\n",(int)data.trips.size()+exception_counter+no_id_counter+out_of_time+no_time_difference+trip_with_missing_data);
	printf("Found %d by station name and %d by coordinates ... \n",nb_found_by_name,nb_found_from_coord);
	printf("There were %d rows with exceptions in long long ints or floats (trips that didn't have ending data)... \n",exception_counter);
	printf("There were %d rows that could not be added because station_id was not found from the .json file ... \n",no_id_counter);
	printf("There were %d rows where the time difference could not be computed from the .csv ... \n",no_time_difference);
	printf("There were %d rows with some missing data ... \n",trip_with_missing_data);
	printf("There were %d rows representing trips out_of_time [%d,%d] ... Continuing ... \n",out_of_time,start_hour,end_hour);
	
	// Calculate the difference in days
    double diffInSeconds = difftime(mktime(&latestDate), mktime(&earliestDate));
    data.nb_days_of_data = (int)(diffInSeconds / (60 * 60 * 24));
	printf("nb_days_of_data:%d dayCounter:%d\n",data.nb_days_of_data,data.dayCounter);	

}


void Load::Load_BlueBike(std::string filename_csv, std::string filename_json, int start_hour, int end_hour)
{

	printf("In Load_BlueBike reading %s and %s ...\n", filename_csv.c_str(), filename_json.c_str());
	// Initialize elapsed time tracking variables
    clock_t startTime = clock();
    clock_t lastPrintTime = startTime;
	
	// Open files
    std::ifstream file_csv(filename_csv); std::ifstream file_json(filename_json);
    if (!file_csv.is_open() || !file_json.is_open()) {
        std::cerr << "Error: Could not open files " << filename_csv << " " << filename_json << ". Exiting ..." <<std::endl;
        exit(1);
    }
	
	// Parse the JSON data; 
	nlohmann::json jsonData;
	file_json >> jsonData;
	nlohmann::json json_stations = jsonData["data"]["stations"];
	
	station_name_map.clear();
	station_lat_map.clear();
	station_lon_map.clear();
	
	Station depot; depot.name = std::string("depot"); depot.id = 0; depot.lon = -74.00812771723888; depot.lat = 40.65686421320091; depot.cap = 0;
	data.stations.push_back(depot);
	
	//depot.Show();
	// Citi Bike's headquarters are located at Brooklyn, 3A 220 36th St, United States
	//40.65686421320091, -74.00812771723888
	int station_counter=1;
	for (const auto& json_station : json_stations)
	{
		//std::cout << json_station << std::endl;
		Station station;
		std::string station_name_str = json_station["name"]; station.name = station_name_str;
		//std::string station_id_str = json_station["station_id"]; station.id = atoi(station_id_str.c_str());
		station.id = station_counter;
		station.lon = json_station["lon"];
		station.lat = json_station["lat"];
		station.cap = json_station["capacity"];
		//station.Show();
		//getchar();
		station_name_map.insert(std::pair<std::string,Station>(station.name,station));
		station_lat_map.insert(std::pair<double,Station>(station.lat,station));
		station_lon_map.insert(std::pair<double,Station>(station.lon,station));
		
		data.stations.push_back(station);
		station_counter++;
	}
	file_json.close();
	printf("Stations:%d Sample station object:\n",(int)data.stations.size()); data.stations[0].Show();
	//exit(1);

	// --------------------------------------------------- //
	// Parse the CSV data
	/*int line_counter=0; int exception_counter=0; int no_id_counter =0; int out_of_time =0; int no_time_difference=0; int trip_with_missing_data=0;
	std::string line;
	std::getline(file_csv, line); //the headers of the columns
    
	//std::cout << line << std::endl;
	while (std::getline(file_csv, line))
	{		
		line_counter++;
		
		//std::cout << line << std::endl;
		//getchar();
		
		if (line_counter % 500000 == 0)
			std::cout << "Read " << line_counter << " lines from .csv ..." << std::endl;
		
			std::vector<std::string> tokens;
			std::string token;
			bool insideQuotes = false;

			for (char c : line) {
				if (c == '"') {
					insideQuotes = !insideQuotes; // Toggle the insideQuotes flag
				} else if (c == ',' && !insideQuotes) {
					// Found a comma outside of quotes, push the token
					if (!token.empty()) {
						if (token.front() == '"' && token.back() == '"') {
							token = token.substr(1, token.length() - 2);  // Remove opening and closing quotes
						}
						tokens.push_back(token);
					}
					token.clear(); // Clear the token for the next one
				} else {
					token += c; // Append the character to the current token
				}
			}

			// Push the last token (if any)
			if (!token.empty()) {
				if (token.front() == '"' && token.back() == '"') {
					token = token.substr(1, token.length() - 2);  // Remove opening and closing quotes
				}
				tokens.push_back(token);
			}

		
		if(tokens.size() == 13)
		{
			std::string start_buffer_str; std::string end_buffer_str;
			double start_lat; double start_lon; double end_lat; double end_lon;
			try
			{
				start_buffer_str = tokens[2];
				end_buffer_str = tokens[3];
				start_lat = std::stod(tokens[8]); start_lon = std::stod(tokens[9]);
				end_lat = std::stod(tokens[10]); end_lon = std::stod(tokens[11]);				
			} catch (const std::exception& e)
			{
				exception_counter++;
				//std::cout << line << std::endl;
				//for(int i=0; i<tokens.size(); i++)
					//std::cout << tokens[i] << " ";
				//printf("\n");
				//getchar();
				//std::cerr << "Exception encountered: " << e.what() << std::endl;
				continue;
			}
			
			std::string start_date_str, start_time_str;
			std::string end_date_str, end_time_str;

			// Find the position of the space character separating date and time
			size_t spacePos = start_buffer_str.find(' ');
			if (spacePos != std::string::npos) {
				// Extract the date and time substrings
				start_date_str = start_buffer_str.substr(0, spacePos);
				start_time_str = start_buffer_str.substr(spacePos + 1);
			}
			spacePos = end_buffer_str.find(' ');
			if (spacePos != std::string::npos) {
				// Extract the date and time substrings
				end_date_str = end_buffer_str.substr(0, spacePos);
				end_time_str = end_buffer_str.substr(spacePos + 1);
			}
			
			int hour, minute, second;
			if (sscanf(start_time_str.c_str(), "%d:%d:%d", &hour, &minute, &second) == 3) 
			{
				if (hour < start_hour || hour > end_hour-1) 
				{
					out_of_time++; continue;
				}
			}
			
			double duration_seconds = 0.0;
			// Parse the date and time strings
			struct std::tm time1 = {};
			struct std::tm time2 = {};
			if (strptime((start_date_str + " " + start_time_str).c_str(), "%Y-%m-%d %H:%M:%S", &time1) &&
				strptime((end_date_str + " " + end_time_str).c_str(), "%Y-%m-%d %H:%M:%S", &time2)) {
				// Calculate the time difference in seconds
				duration_seconds = std::difftime(std::mktime(&time2), std::mktime(&time1));

				//std::cout << "Duration in seconds: " << duration_seconds << std::endl;
			} else {
				std::cerr << "Failed to parse time strings." << std::endl;
				no_time_difference++; std::cout << start_time_str << " " << end_time_str << std::endl; getchar();
			}
			if(duration_seconds<60.0) continue;
			//std::cout << start_time_str << std::endl;
			//getchar();
			
			TripData trip;	
			trip.start_date_str = start_date_str;
			trip.start_time_str = start_time_str;
			trip.end_date_str = end_date_str;
			trip.end_time_str = end_time_str;
			trip.duration = duration_seconds;
			trip.start_id = -1; trip.end_id = -1;
			
			for (const auto& station : data.stations)
			{
				if(std::abs(station.lat - start_lat) < 0.0001 && std::abs(station.lon - start_lon) < 0.0001)
				{
					trip.start_id = station.id; break;
				}
				if(station.name == tokens[4])
				{
					trip.start_id = station.id; break;
				}	
				
			}
			for (const auto& station : data.stations)
			{
				if(std::abs(station.lat - end_lat) < 0.0001 && std::abs(station.lon - end_lon) < 0.0001)
				{
					trip.end_id = station.id; break;
				}
				if(station.name == tokens[6])
				{
					trip.end_id = station.id; break;
				}		
			}
			if(trip.start_id==-1 || trip.end_id==-1)
			{
				no_id_counter++;
				//trip.Show();
				//std::cout << "start_lat:" << start_lat << " start_lon:" << start_lon << std::endl; 
				//std::cout << "end_lat:" << end_lat << " end_lon:" << end_lon << std::endl;
				//getchar();
				continue; //printf("Could not find station ids ... \n"); //getchar();
			}
			
			//trip.Show();
			//getchar();
			data.trips.push_back(trip);
		}else {
			//std::cerr << ".csv line with missing data." << std::endl;
			//std::cout << "tokens size:" << (int)tokens.size() << std::endl;
			//for(const std::string& tok : tokens)
				//printf("%s \n",tok.c_str());
			//printf("\n");
			//getchar();
			trip_with_missing_data++;
		}
		
	}
	file_csv.close();
	printf("Sample trip object loaded:\n"); data.trips[0].Show();
	
	printf("The .csv has %d rows ...\n",(int)data.trips.size()+exception_counter+no_id_counter+out_of_time+no_time_difference+trip_with_missing_data);
	printf("There were %d rows with exceptions in long long ints or floats (trips that didn't have ending data)... \n",exception_counter);
	printf("There were %d rows that could not be added because station_id was not found from the .json file ... \n",no_id_counter);
	printf("There were %d rows where the time difference could not be computed from the .csv ... \n",no_time_difference);
	printf("There were %d rows with some missing data ... \n",trip_with_missing_data);
	printf("There were %d rows representing trips out_of_time [%d,%d] ... Continuing ... \n",out_of_time,start_hour,end_hour);
	*/
	//---------------------------------------------------------------------------------------//
	
	Load_CSV_File(filename_csv, data.stations, start_hour, end_hour);
	
	printf("Loaded %d trips and %d stations.\n",(int)data.trips.size(),(int)data.stations.size());
	// Print elapsed time
    clock_t endTime = clock();
    double elapsedSeconds = (double)(endTime - startTime) / CLOCKS_PER_SEC;
    std::cout << "Elapsed Time: " << std::setprecision(3) << elapsedSeconds << " seconds" << std::endl;	
	
	//for(int i=0;i<data.trips.size();i++)
	//{ printf("i:%d\n",i); data.trips[i].Show(); }

}

void Load::Load_CitiBike(std::string filename_csv, std::string filename_json, int start_hour, int end_hour)
{

	printf("In Load_CitiBike reading %s and %s ...\n", filename_csv.c_str(), filename_json.c_str());
	// Initialize elapsed time tracking variables
    clock_t startTime = clock();
    clock_t lastPrintTime = startTime;
	
	// Open files
    std::ifstream file_csv(filename_csv); std::ifstream file_json(filename_json);
    if (!file_csv.is_open() || !file_json.is_open()) {
        std::cerr << "Error: Could not open files " << filename_csv << " " << filename_json << ". Exiting ..." <<std::endl;
        exit(1);
    }
	
	// Parse the JSON data; 
	nlohmann::json jsonData;
	file_json >> jsonData;
	nlohmann::json json_stations = jsonData["data"]["stations"];
	
	station_name_map.clear();
	station_lat_map.clear();
	station_lon_map.clear();
	
	Station depot; depot.name = std::string("depot"); depot.id = 0; depot.lon = -74.00812771723888; depot.lat = 40.65686421320091; depot.cap = 0;
	data.stations.push_back(depot);
	//depot.Show();
	// Citi Bike's headquarters are located at Brooklyn, 3A 220 36th St, United States
	//40.65686421320091, -74.00812771723888
	int station_counter=1;
	for (const auto& json_station : json_stations)
	{
		//std::cout << json_station << std::endl;
		Station station;
		std::string station_name_str = json_station["name"]; station.name = station_name_str;
		station.id = station_counter;
		station.lon = json_station["lon"];
		station.lat = json_station["lat"];
		station.cap = json_station["capacity"];
		//station.Show();
		//getchar();
		station_name_map.insert(std::pair<std::string,Station>(station.name,station));
		station_lat_map.insert(std::pair<double,Station>(station.lat,station));
		station_lon_map.insert(std::pair<double,Station>(station.lon,station));
		
		data.stations.push_back(station);
		station_counter++;
	}
	file_json.close();
	printf("Stations:%d Sample station object:\n",(int)data.stations.size()); data.stations[0].Show();
	//exit(1);
	
	Load_CSV_File(filename_csv, data.stations, start_hour, end_hour);
	
	printf("Loaded %d trips and %d stations.\n",(int)data.trips.size(),(int)data.stations.size());
	// Print elapsed time
    clock_t endTime = clock();
    double elapsedSeconds = (double)(endTime - startTime) / CLOCKS_PER_SEC;
    std::cout << "Elapsed Time: " << std::setprecision(3) << elapsedSeconds << " seconds" << std::endl;	
	
	//for(int i=0;i<data.trips.size();i++)
	//{ printf("i:%d\n",i); data.trips[i].Show(); }

}

void Load::Load_CapitalBikeShare(std::string filename_csv, std::string filename_json, int start_hour, int end_hour)
{
	printf("In Load_CapitalBikeShare reading %s and %s ...\n", filename_csv.c_str(), filename_json.c_str());
	// Initialize elapsed time tracking variables
    clock_t startTime = clock();
    clock_t lastPrintTime = startTime;
	
	//to count the data days ... Initialize variables to track earliest and latest dates
    std::tm earliestDate = {0};
    std::tm latestDate = {0};
    bool firstDate = true;
	data.dayCounter = 0; data.nb_days_of_data = 0;	
	
	// Open files
    std::ifstream file_csv(filename_csv); std::ifstream file_json(filename_json);
    if (!file_csv.is_open() || !file_json.is_open()) {
        std::cerr << "Error: Could not open files " << filename_csv << " " << filename_json << ". Exiting ..." <<std::endl;
        exit(1);
    }
	
	// Parse the JSON data; 
	nlohmann::json jsonData;
	file_json >> jsonData;
	nlohmann::json json_stations = jsonData["data"]["stations"];
	
	Station depot; depot.name = std::string("depot"); depot.id = 0; depot.lon = -77.0742465; depot.lat = 38.895389; depot.cap = 0;
	data.stations.push_back(depot);
	//depot.Show();
	// Capital Bikeshare's headquarters are located at 1501 Wilson Blvd Ste 1100, Arlington, Virginia, 22209
	//38.89538947941347, -77.07424659999998
	int station_counter=1;
	for (const auto& json_station : json_stations)
	{
		//std::cout << json_station << std::endl;
		Station station;
		std::string station_name_str = json_station["name"]; station.name = station_name_str;
		//std::string station_id_str = json_station["station_id"]; station.id = atoi(station_id_str.c_str());
		station.id = station_counter;
		station.lon = json_station["lon"];
		station.lat = json_station["lat"];
		station.cap = json_station["capacity"];
		//station.Show();
		//getchar();
		data.stations.push_back(station);
		station_counter++;
	}
	file_json.close();
	printf("Station:%d Sample station object:\n",(int)data.stations.size()); data.stations[0].Show();
	//exit(1);

	// --------------------------------------------------- //
	// Parse the CSV data
	int line_counter=0; int exception_counter=0; int no_id_counter =0; int out_of_time =0; int no_time_difference=0; int trip_with_missing_data=0;
	std::string line;
	std::getline(file_csv, line); //the headers of the columns
    
	//std::cout << line << std::endl;
	while (std::getline(file_csv, line))
	{		
		line_counter++;
		
		//std::cout << line << std::endl;
		//getchar();
		
		if (line_counter % 500000 == 0)
			std::cout << "Read " << line_counter << " lines from .csv ..." << std::endl;
		
			std::vector<std::string> tokens;
			std::string token;
			bool insideQuotes = false;

			for (char c : line) {
				if (c == '"') {
					insideQuotes = !insideQuotes; // Toggle the insideQuotes flag
				} else if (c == ',' && !insideQuotes) {
					// Found a comma outside of quotes, push the token
					if (!token.empty()) {
						if (token.front() == '"' && token.back() == '"') {
							token = token.substr(1, token.length() - 2);  // Remove opening and closing quotes
						}
						tokens.push_back(token);
					}
					token.clear(); // Clear the token for the next one
				} else {
					token += c; // Append the character to the current token
				}
			}

			// Push the last token (if any)
			if (!token.empty()) {
				if (token.front() == '"' && token.back() == '"') {
					token = token.substr(1, token.length() - 2);  // Remove opening and closing quotes
				}
				tokens.push_back(token);
			}

		
		if(tokens.size() == 13)
		{
			std::string start_buffer_str; std::string end_buffer_str;
			double start_lat; double start_lon; double end_lat; double end_lon;
			try
			{
				start_buffer_str = tokens[2];
				end_buffer_str = tokens[3];
				start_lat = std::stod(tokens[8]); start_lon = std::stod(tokens[9]);
				end_lat = std::stod(tokens[10]); end_lon = std::stod(tokens[11]);				
			} catch (const std::exception& e)
			{
				exception_counter++;
				//std::cout << line << std::endl;
				//for(int i=0; i<tokens.size(); i++)
					//std::cout << tokens[i] << " ";
				//printf("\n");
				//getchar();
				//std::cerr << "Exception encountered: " << e.what() << std::endl;
				continue;
			}
			
			std::string start_date_str, start_time_str;
			std::string end_date_str, end_time_str;

			// Find the position of the space character separating date and time
			size_t spacePos = start_buffer_str.find(' ');
			if (spacePos != std::string::npos) {
				// Extract the date and time substrings
				start_date_str = start_buffer_str.substr(0, spacePos);
				start_time_str = start_buffer_str.substr(spacePos + 1);
			}
			spacePos = end_buffer_str.find(' ');
			if (spacePos != std::string::npos) {
				// Extract the date and time substrings
				end_date_str = end_buffer_str.substr(0, spacePos);
				end_time_str = end_buffer_str.substr(spacePos + 1);
			}
			
			int hour, minute, second;
			if (sscanf(start_time_str.c_str(), "%d:%d:%d", &hour, &minute, &second) == 3) 
			{
				if (hour < start_hour || hour > end_hour-1) 
				{
					out_of_time++; continue;
				}
			}
			
			double duration_seconds = 0.0;
			// Parse the date and time strings
			struct std::tm time1 = {};
			struct std::tm time2 = {};
			if (strptime((start_date_str + " " + start_time_str).c_str(), "%Y-%m-%d %H:%M:%S", &time1) &&
				strptime((end_date_str + " " + end_time_str).c_str(), "%Y-%m-%d %H:%M:%S", &time2)) {
				// Calculate the time difference in seconds
				duration_seconds = std::difftime(std::mktime(&time2), std::mktime(&time1));

				//std::cout << "Duration in seconds: " << duration_seconds << std::endl;
			} else {
				std::cerr << "Failed to parse time strings." << std::endl;
				no_time_difference++; std::cout << start_time_str << " " << end_time_str << std::endl; getchar();
			}
			if(duration_seconds<60.0) continue;
			//std::cout << start_time_str << std::endl;
			//getchar();
			
			TripData trip;	
			trip.start_date_str = start_date_str;
			trip.start_time_str = start_time_str;
			trip.end_date_str = end_date_str;
			trip.end_time_str = end_time_str;
			trip.duration = duration_seconds;
			trip.start_id = -1; trip.end_id = -1;
			
			for (const auto& station : data.stations)
			{
				if(std::abs(station.lat - start_lat) < 0.0001 && std::abs(station.lon - start_lon) < 0.0001)
				{
					trip.start_id = station.id; break;
				}
				if(station.name == tokens[4])
				{
					trip.start_id = station.id; break;
				}	
				
			}
			for (const auto& station : data.stations)
			{
				if(std::abs(station.lat - end_lat) < 0.0001 && std::abs(station.lon - end_lon) < 0.0001)
				{
					trip.end_id = station.id; break;
				}
				if(station.name == tokens[6])
				{
					trip.end_id = station.id; break;
				}		
			}
			if(trip.start_id==-1 || trip.end_id==-1)
			{
				no_id_counter++;
				//trip.Show();
				//std::cout << "start_lat:" << start_lat << " start_lon:" << start_lon << std::endl; 
				//std::cout << "end_lat:" << end_lat << " end_lon:" << end_lon << std::endl;
				//getchar();
				continue; //printf("Could not find station ids ... \n"); //getchar();
			}
			
			//trip.Show();
			//getchar();
			data.trips.push_back(trip);
			
			//to count the data days ...
			std::tm date = {0};
			std::istringstream dateStream(trip.start_date_str);
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
			if (data.dateToDayMap.find(trip.start_date_str) == data.dateToDayMap.end()) 
			data.dateToDayMap[trip.start_date_str] = data.dayCounter++;	
			
		}else {
			//std::cerr << ".csv line with missing data." << std::endl;
			//std::cout << "tokens size:" << (int)tokens.size() << std::endl;
			//for(const std::string& tok : tokens)
				//printf("%s \n",tok.c_str());
			//printf("\n");
			//getchar();
			trip_with_missing_data++;
		}
		
	}
	file_csv.close();
	printf("Sample trip object loaded:\n"); data.trips[0].Show();
	
	printf("The .csv has %d rows ...\n",(int)data.trips.size()+exception_counter+no_id_counter+out_of_time+no_time_difference+trip_with_missing_data);
	printf("There were %d rows with exceptions in long long ints or floats (trips that didn't have ending data)... \n",exception_counter);
	printf("There were %d rows that could not be added because station_id was not found from the .json file ... \n",no_id_counter);
	printf("There were %d rows where the time difference could not be computed from the .csv ... \n",no_time_difference);
	printf("There were %d rows with some missing data ... \n",trip_with_missing_data);
	printf("There were %d rows representing trips out_of_time [%d,%d] ... Continuing ... \n",out_of_time,start_hour,end_hour);
	
	printf("Loaded %d trips and %d stations.\n",(int)data.trips.size(),(int)data.stations.size());
	// Print elapsed time
    clock_t endTime = clock();
    double elapsedSeconds = (double)(endTime - startTime) / CLOCKS_PER_SEC;
    std::cout << "Elapsed Time: " << std::setprecision(3) << elapsedSeconds << " seconds" << std::endl;	
	
		// Calculate the difference in days
    double diffInSeconds = difftime(mktime(&latestDate), mktime(&earliestDate));
    data.nb_days_of_data = (int)(diffInSeconds / (60 * 60 * 24));
	printf("nb_days_of_data:%d dayCounter:%d\n",data.nb_days_of_data,data.dayCounter);	
	//return data;
}	

void Load::Load_Bixi(std::string filename_csv, std::string filename_json, int start_hour, int end_hour)
{
	printf("In Load_Bixi reading %s and %s ...\n", filename_csv.c_str(), filename_json.c_str());
	// Initialize elapsed time tracking variables
    clock_t startTime = clock();
    clock_t lastPrintTime = startTime;
	
	//to count the data days ... Initialize variables to track earliest and latest dates
    std::tm earliestDate = {0};
    std::tm latestDate = {0};
    bool firstDate = true;
	data.dayCounter = 0; data.nb_days_of_data = 0;
	
	int nb_found_by_name, nb_found_from_coord = 0;
	
	// Open files
    std::ifstream file_csv(filename_csv); std::ifstream file_json(filename_json);
    if (!file_csv.is_open() || !file_json.is_open()) {
        std::cerr << "Error: Could not open files " << filename_csv << " " << filename_json << ". Exiting ..." <<std::endl;
        exit(1);
    }
	
	// Parse the JSON data;

	station_name_map.clear();
	station_lat_map.clear();
	station_lon_map.clear();
	
	nlohmann::json jsonData;
	file_json >> jsonData;
	nlohmann::json json_stations = jsonData["data"]["stations"];
	Station depot; depot.name = std::string("depot"); depot.id = 0; depot.lon = -73.6017523; depot.lat = 45.5298971; depot.cap = 0;
	data.stations.push_back(depot);
	//depot_row = {'station_id': 0, 'name': 'depot', 'lat': 45.5298971, 'lon': -73.6017523, 'capacity': 0}
	for (const auto& json_station : json_stations)
	{
		Station station;
		std::string station_name_str = json_station["name"]; station.name = station_name_str;
		std::string station_id_str = json_station["station_id"]; station.id = atoi(station_id_str.c_str());
		station.lon = json_station["lon"];
		station.lat = json_station["lat"];
		station.cap = json_station["capacity"];
		//station.Show();
		//getchar();
		
		station_name_map.insert(std::pair<std::string,Station>(station.name,station));
		station_lat_map.insert(std::pair<double,Station>(station.lat,station));
		station_lon_map.insert(std::pair<double,Station>(station.lon,station));		
		
		data.stations.push_back(station);
	}
	file_json.close();
	printf("Station:%d Sample station object:\n",(int)data.stations.size()); data.stations[0].Show();
	// --------------------------------------------------- //
	// Parse the CSV data
	
	int line_counter=0; int exception_counter=0; int no_id_counter =0; int out_of_time =0;
	std::string line;
	std::getline(file_csv, line); //the headers of the columns
    while (std::getline(file_csv, line))
	{		
		line_counter++;
		if (line_counter % 500000 == 0)
			std::cout << "Read " << line_counter << " lines from .csv ..." << std::endl;
		
		std::vector<std::string> tokens;
		size_t startPos = 0;
		// Find the positions of commas and extract substrings
		for (size_t pos = line.find(','); pos != std::string::npos; pos = line.find(',', startPos)) {
			tokens.push_back(line.substr(startPos, pos - startPos));
			startPos = pos + 1;
		}
		// Add the last token after the final comma
		tokens.push_back(line.substr(startPos));
		
		if(tokens.size() == 10)
		{
			long long start_time_ms; long long end_time_ms;
			double start_lat_double; double start_lon_double; double end_lat_double; double end_lon_double;
			try
			{
				start_time_ms = std::stoll(tokens[8]);
				end_time_ms = std::stoll(tokens[9]);
				start_lat_double = std::stod(tokens[2]); start_lon_double = std::stod(tokens[3]);
				end_lat_double = std::stod(tokens[6]); end_lon_double = std::stod(tokens[7]);				
			} catch (const std::exception& e)
			{
				exception_counter++;
				//std::cout << line << std::endl;
				//for(int i=0; i<tokens.size(); i++)
					//std::cout << tokens[i] << " ";
				//printf("\n");
				//getchar();
				//std::cerr << "Exception encountered: " << e.what() << std::endl;
				continue;
			}
			

            // Convert milliseconds to seconds
            time_t start_time_posix = start_time_ms / 1000;
            time_t end_time_posix = end_time_ms / 1000;
			
			long long duration_ms = end_time_ms - start_time_ms;
            long long duration_seconds = duration_ms / 1000;

			//std::cout << start_time_posix << " " << tokens[8] << std::endl;
			if(0<duration_seconds && duration_seconds<60) continue;
			
			//Local time returns time in time zone of my machine. Type in bash:date "+%Z"
            char start_buffer[80]; char end_buffer[80];
			strftime(start_buffer, sizeof(start_buffer), "%Y-%m-%d %H:%M:%S", localtime(&start_time_posix));
			strftime(end_buffer, sizeof(end_buffer), "%Y-%m-%d %H:%M:%S", localtime(&end_time_posix));
			//std::cout << "Posix:" << start_time_posix << " Formatted Start Date and Time: " << start_buffer << std::endl;
			//std::cout << "Posix:" << end_time_posix << " Formatted End Date and Time: " << end_buffer << std::endl;
			std::string start_buffer_str(start_buffer); std::string end_buffer_str(end_buffer);
			std::string start_date_str, start_time_str;
			std::string end_date_str, end_time_str;

			// Find the position of the space character separating date and time
			size_t spacePos = start_buffer_str.find(' ');
			if (spacePos != std::string::npos) {
				// Extract the date and time substrings
				start_date_str = start_buffer_str.substr(0, spacePos);
				start_time_str = start_buffer_str.substr(spacePos + 1);
			}
			spacePos = end_buffer_str.find(' ');
			if (spacePos != std::string::npos) {
				// Extract the date and time substrings
				end_date_str = end_buffer_str.substr(0, spacePos);
				end_time_str = end_buffer_str.substr(spacePos + 1);
			}
			
			int hour, minute, second;
			if (sscanf(start_time_str.c_str(), "%d:%d:%d", &hour, &minute, &second) == 3) 
			{
				if (hour < start_hour || hour > end_hour-1) 
				{
					out_of_time++; continue;
				}
			}
			
			//std::cout << start_time_str << std::endl;
			//getchar();
			
			TripData trip;	
			trip.start_date_str = start_date_str;
			trip.start_time_str = start_time_str;
			trip.end_date_str = end_date_str;
			trip.end_time_str = end_time_str;
			trip.duration = duration_seconds;
			trip.start_id = -1; trip.end_id = -1;
			
			std::string start_station_name = tokens[0];
			std::string end_station_name = tokens[4];
			
			auto it_start = station_name_map.find(start_station_name);
			auto it_end = station_name_map.find(end_station_name);
			
			if(it_start != station_name_map.end() && it_end != station_name_map.end())
			{
				trip.start_id = it_start->second.id;
				trip.end_id = it_end->second.id;
				nb_found_by_name++;
				//trip.Show();
				//getchar();
			} else {
				auto it_start_lat = station_lat_map.find(start_lat_double);
				auto it_start_lon = station_lon_map.find(start_lon_double);
				auto it_end_lat = station_lat_map.find(end_lat_double);
				auto it_end_lon = station_lon_map.find(end_lon_double);
				
				
				if(it_start_lat != station_lat_map.end() && it_start_lon != station_lon_map.end() && it_end_lat != station_lat_map.end() && it_end_lon != station_lon_map.end() )
				{
					trip.start_id = it_start_lat->second.id;
					trip.end_id = it_end_lat->second.id;
					//it_start_lat->second.Show(); getchar();
					nb_found_from_coord++;
				}
			}			

			if(trip.start_id==-1 || trip.end_id==-1)
			{
				no_id_counter++;
				//trip.Show();
				//std::cout << "start_lat:" << start_lat << " start_lon:" << start_lon << std::endl; 
				//std::cout << "end_lat:" << end_lat << " end_lon:" << end_lon << std::endl;
				//getchar();
				continue; //printf("Could not find station ids ... \n"); //getchar();
			}
			
			//trip.Show();
			//getchar();
			data.trips.push_back(trip);
			
			//to count the data days ...
			std::tm date = {0};
			std::istringstream dateStream(trip.start_date_str);
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
			if (data.dateToDayMap.find(trip.start_date_str) == data.dateToDayMap.end()) 
			data.dateToDayMap[trip.start_date_str] = data.dayCounter++;			
			
		}else {
			std::cerr << "Failed to parse the .csv line." << std::endl;
		}
		
	}
	file_csv.close();

	printf("The .csv has %d rows ...\n",(int)data.trips.size()+exception_counter+no_id_counter+out_of_time);
	printf("Found %d by station name and %d by coordinates ... \n",nb_found_by_name,nb_found_from_coord);
	printf("There were %d rows with exceptions in long long ints or floats (trips that didn't have ending data)... \n",exception_counter);
	printf("There were %d rows that could not be added because station_id was not found from the .json file ... \n",no_id_counter);
	printf("There were %d rows representing trips out_of_time [%d,%d] ... Continuing ... \n",out_of_time,start_hour,end_hour);
	
	printf("Loaded %d trips and %d stations.\n",(int)data.trips.size(),(int)data.stations.size());
	// Print elapsed time
    clock_t endTime = clock();
    double elapsedSeconds = (double)(endTime - startTime) / CLOCKS_PER_SEC;
    std::cout << "Elapsed Time: " << std::setprecision(3) << elapsedSeconds << " seconds" << std::endl;
	
	// Calculate the difference in days
    double diffInSeconds = difftime(mktime(&latestDate), mktime(&earliestDate));
    data.nb_days_of_data = (int)(diffInSeconds / (60 * 60 * 24));
	printf("nb_days_of_data:%d dayCounter:%d\n",data.nb_days_of_data,data.dayCounter);	
}	

std::vector<TripData> Load::LoadTripsFromTripsObject(std::string filename) 
{
    printf("In LoadTripsFromTripsObject() reading %s ...\n", filename.c_str());
	// Initialize elapsed time tracking variables
    clock_t startTime = clock();
    clock_t lastPrintTime = startTime;
	
	std::vector<TripData> trips;

    // Open the file
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return trips;
    }

    std::string line;
	std::getline(file, line); //the headers of the columns
    while (std::getline(file, line)) {
		line.erase(std::remove(line.begin(), line.end(), '"'), line.end()); //remove " from string
		
		//std::cout << line << std::endl;
		
		TripData trip;
		char start_date_str[20], start_time_str[20], end_date_str[20], end_time_str[20];
		double start_id_double; double end_id_double;

		// Use sscanf to parse the line
		if (sscanf(line.c_str(), "%s %s %s %s %lf %lf %lf",start_date_str, start_time_str, end_date_str, end_time_str,&trip.duration, &start_id_double, &end_id_double) == 7) {
			trip.start_date_str = start_date_str;
			trip.start_time_str = start_time_str;
			trip.end_date_str = end_date_str;
			trip.end_time_str = end_time_str;
			trip.start_id = (int)start_id_double;
			trip.end_id = (int)end_id_double;

			//trip.Show();
			trips.push_back(trip);
			//getchar();
		} 
		else {
			std::cerr << "Error reading line: " << line << std::endl << "Exiting ... " << std::endl;
			exit(1);
		}
    }

    file.close();
	printf("Number of trips:%d\n",(int)trips.size());
	
	// Print elapsed time
    clock_t endTime = clock();
    double elapsedSeconds = (double)(endTime - startTime) / CLOCKS_PER_SEC;
    std::cout << "Elapsed Time: " << std::setprecision(3) << elapsedSeconds << " seconds" << std::endl;
	
    return trips;
}

void Load::Load_Bixi(std::string filename_csv, std::string filename_json, int start_hour, int end_hour, int max_stations_allowed)
{
	printf("In Load_Bixi reading %s and %s ...\n", filename_csv.c_str(), filename_json.c_str());
	// Initialize elapsed time tracking variables
    clock_t startTime = clock();
    clock_t lastPrintTime = startTime;
	
	//to count the data days ... Initialize variables to track earliest and latest dates
    std::tm earliestDate = {0};
    std::tm latestDate = {0};
    bool firstDate = true;
	data.dayCounter = 0; data.nb_days_of_data = 0;
	
    std::ifstream file_csv(filename_csv); std::ifstream file_json(filename_json);
    if (!file_csv.is_open() || !file_json.is_open()) {
        std::cerr << "Error: Could not open files " << filename_csv << " " << filename_json << ". Exiting ..." <<std::endl;
        exit(1);
    }
	
	// Parse the JSON data; 
	nlohmann::json jsonData;
	file_json >> jsonData;
	nlohmann::json json_stations = jsonData["data"]["stations"];
	Station depot; depot.name = std::string("depot"); depot.id = 0; depot.lon = -73.6017523; depot.lat = 45.5298971; depot.cap = 0;
	data.stations.push_back(depot);
	//depot_row = {'station_id': 0, 'name': 'depot', 'lat': 45.5298971, 'lon': -73.6017523, 'capacity': 0}
	int nb_added=1;
	for (const auto& json_station : json_stations)
	{
		Station station;
		std::string station_name_str = json_station["name"]; station.name = station_name_str;
		std::string station_id_str = json_station["station_id"]; station.id = atoi(station_id_str.c_str());
		station.lon = json_station["lon"];
		station.lat = json_station["lat"];
		station.cap = json_station["capacity"];
		//station.Show();
		//getchar();
		data.stations.push_back(station);
		nb_added++;
		if(nb_added>max_stations_allowed)
			break;
	}
	file_json.close();
	printf("Sample station object:\n"); data.stations[0].Show();
	// --------------------------------------------------- //
	// Parse the CSV data
	
	int line_counter=0; int exception_counter=0; int no_id_counter =0; int out_of_time =0;
	std::string line;
	std::getline(file_csv, line); //the headers of the columns
    while (std::getline(file_csv, line))
	{		
		line_counter++;
		if (line_counter % 500000 == 0)
			std::cout << "Read " << line_counter << " lines from .csv ..." << std::endl;
		
		std::vector<std::string> tokens;
		size_t startPos = 0;
		// Find the positions of commas and extract substrings
		for (size_t pos = line.find(','); pos != std::string::npos; pos = line.find(',', startPos)) {
			tokens.push_back(line.substr(startPos, pos - startPos));
			startPos = pos + 1;
		}
		// Add the last token after the final comma
		tokens.push_back(line.substr(startPos));
		
		if(tokens.size() == 10)
		{
			long long start_time_ms; long long end_time_ms;
			double start_lat; double start_lon; double end_lat; double end_lon;
			try
			{
				start_time_ms = std::stoll(tokens[8]);
				end_time_ms = std::stoll(tokens[9]);
				start_lat = std::stod(tokens[2]); start_lon = std::stod(tokens[3]);
				end_lat = std::stod(tokens[6]); end_lon = std::stod(tokens[7]);				
			} catch (const std::exception& e)
			{
				exception_counter++;
				//std::cout << line << std::endl;
				//for(int i=0; i<tokens.size(); i++)
					//std::cout << tokens[i] << " ";
				//printf("\n");
				//getchar();
				//std::cerr << "Exception encountered: " << e.what() << std::endl;
				continue;
			}
			

            // Convert milliseconds to seconds
            time_t start_time_posix = start_time_ms / 1000;
            time_t end_time_posix = end_time_ms / 1000;
			
			long long duration_ms = end_time_ms - start_time_ms;
            long long duration_seconds = duration_ms / 1000;

			//std::cout << start_time_posix << " " << tokens[8] << std::endl;
			if(0<duration_seconds && duration_seconds<60) continue;
			
			//Local time returns time in time zone of my machine. Type in bash:date "+%Z"
            char start_buffer[80]; char end_buffer[80];
			strftime(start_buffer, sizeof(start_buffer), "%Y-%m-%d %H:%M:%S", localtime(&start_time_posix));
			strftime(end_buffer, sizeof(end_buffer), "%Y-%m-%d %H:%M:%S", localtime(&end_time_posix));
			//std::cout << "Posix:" << start_time_posix << " Formatted Start Date and Time: " << start_buffer << std::endl;
			//std::cout << "Posix:" << end_time_posix << " Formatted End Date and Time: " << end_buffer << std::endl;
			std::string start_buffer_str(start_buffer); std::string end_buffer_str(end_buffer);
			std::string start_date_str, start_time_str;
			std::string end_date_str, end_time_str;

			// Find the position of the space character separating date and time
			size_t spacePos = start_buffer_str.find(' ');
			if (spacePos != std::string::npos) {
				// Extract the date and time substrings
				start_date_str = start_buffer_str.substr(0, spacePos);
				start_time_str = start_buffer_str.substr(spacePos + 1);
			}
			spacePos = end_buffer_str.find(' ');
			if (spacePos != std::string::npos) {
				// Extract the date and time substrings
				end_date_str = end_buffer_str.substr(0, spacePos);
				end_time_str = end_buffer_str.substr(spacePos + 1);
			}
			
			int hour, minute, second;
			if (sscanf(start_time_str.c_str(), "%d:%d:%d", &hour, &minute, &second) == 3) 
			{
				if (hour < start_hour || hour > end_hour-1) 
				{
					out_of_time++; continue;
				}
			}
			
			//std::cout << start_time_str << std::endl;
			//getchar();
			
			TripData trip;	
			trip.start_date_str = start_date_str;
			trip.start_time_str = start_time_str;
			trip.end_date_str = end_date_str;
			trip.end_time_str = end_time_str;
			trip.duration = duration_seconds;
			trip.start_id = -1; trip.end_id = -1;
			
			for (const auto& station : data.stations)
			{
				if(std::abs(station.lat - start_lat) < 0.0001 && std::abs(station.lon - start_lon) < 0.0001)
				{
					trip.start_id = station.id; break;
				}
				if(station.name == tokens[0])
				{
					trip.start_id = station.id; break;
				}	
				
			}
			for (const auto& station : data.stations)
			{
				if(std::abs(station.lat - end_lat) < 0.0001 && std::abs(station.lon - end_lon) < 0.0001)
				{
					trip.end_id = station.id; break;
				}
				if(station.name == tokens[4])
				{
					trip.end_id = station.id; break;
				}		
			}
			if(trip.start_id==-1 || trip.end_id==-1)
			{
				no_id_counter++;
				//trip.Show();
				//std::cout << "start_lat:" << start_lat << " start_lon:" << start_lon << std::endl; 
				//std::cout << "end_lat:" << end_lat << " end_lon:" << end_lon << std::endl;
				//getchar();
				continue; //printf("Could not find station ids ... \n"); //getchar();
			}
			
			//trip.Show();
			//getchar();
			data.trips.push_back(trip);
			
			//to count the data days ...
			std::tm date = {0};
			std::istringstream dateStream(trip.start_date_str);
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
			if (data.dateToDayMap.find(trip.start_date_str) == data.dateToDayMap.end()) 
			data.dateToDayMap[trip.start_date_str] = data.dayCounter++;					
			
		}else {
			std::cerr << "Failed to parse the .csv line." << std::endl;
		}
		
	}
	file_csv.close();
	
	printf("The .csv has %d rows ...\n",(int)data.trips.size()+exception_counter+no_id_counter+out_of_time);
	printf("There were %d rows with exceptions in long long ints or floats (trips that didn't have ending data)... \n",exception_counter);
	printf("There were %d rows that could not be added because station_id was not found from the .json file ... \n",no_id_counter);
	printf("There were %d rows representing trips out_of_time [%d,%d] ... Continuing ... \n",out_of_time,start_hour,end_hour);
	
	printf("Loaded %d trips and %d stations.\n",(int)data.trips.size(),(int)data.stations.size());
	// Print elapsed time
    clock_t endTime = clock();
    double elapsedSeconds = (double)(endTime - startTime) / CLOCKS_PER_SEC;
    std::cout << "Elapsed Time: " << std::setprecision(3) << elapsedSeconds << " seconds" << std::endl;
	
	// Calculate the difference in days
    double diffInSeconds = difftime(mktime(&latestDate), mktime(&earliestDate));
    data.nb_days_of_data = (int)(diffInSeconds / (60 * 60 * 24));
	printf("nb_days_of_data:%d dayCounter:%d\n",data.nb_days_of_data,data.dayCounter);	
}

void Load::Load_CapitalBikeShare(std::string filename_csv, std::string filename_json, int start_hour, int end_hour, int max_stations_allowed)
{
	printf("In Load_CapitalBikeShare reading %s and %s ...\n", filename_csv.c_str(), filename_json.c_str());
	// Initialize elapsed time tracking variables
    clock_t startTime = clock();
    clock_t lastPrintTime = startTime;
	
	//to count the data days ... Initialize variables to track earliest and latest dates
    std::tm earliestDate = {0};
    std::tm latestDate = {0};
    bool firstDate = true;
	data.dayCounter = 0; data.nb_days_of_data = 0;	
	
	// Open files
    std::ifstream file_csv(filename_csv); std::ifstream file_json(filename_json);
    if (!file_csv.is_open() || !file_json.is_open()) {
        std::cerr << "Error: Could not open files " << filename_csv << " " << filename_json << ". Exiting ..." <<std::endl;
        exit(1);
    }
	
	// Parse the JSON data; 
	nlohmann::json jsonData;
	file_json >> jsonData;
	nlohmann::json json_stations = jsonData["data"]["stations"];
	
	Station depot; depot.name = std::string("depot"); depot.id = 0; depot.lon = -77.0742465; depot.lat = 38.895389; depot.cap = 0;
	data.stations.push_back(depot);
	//depot.Show();
	// Capital Bikeshare's headquarters are located at 1501 Wilson Blvd Ste 1100, Arlington, Virginia, 22209
	//38.89538947941347, -77.07424659999998
	int station_counter=1;
	for (const auto& json_station : json_stations)
	{
		//std::cout << json_station << std::endl;
		Station station;
		std::string station_name_str = json_station["name"]; station.name = station_name_str;
		//std::string station_id_str = json_station["station_id"]; station.id = atoi(station_id_str.c_str());
		station.id = station_counter;
		station.lon = json_station["lon"];
		station.lat = json_station["lat"];
		station.cap = json_station["capacity"];
		//station.Show();
		//getchar();
		data.stations.push_back(station);
		station_counter++;
		if(station_counter>max_stations_allowed)
			break;
	}
	file_json.close();
	printf("Sample station object:\n"); data.stations[0].Show();
	//exit(1);

	// --------------------------------------------------- //
	// Parse the CSV data
	int line_counter=0; int exception_counter=0; int no_id_counter =0; int out_of_time =0; int no_time_difference=0; int trip_with_missing_data=0;
	std::string line;
	std::getline(file_csv, line); //the headers of the columns
    
	//std::cout << line << std::endl;
	while (std::getline(file_csv, line))
	{		
		line_counter++;
		
		//std::cout << line << std::endl;
		//getchar();
		
		if (line_counter % 500000 == 0)
			std::cout << "Read " << line_counter << " lines from .csv ..." << std::endl;
		
			std::vector<std::string> tokens;
			std::string token;
			bool insideQuotes = false;

			for (char c : line) {
				if (c == '"') {
					insideQuotes = !insideQuotes; // Toggle the insideQuotes flag
				} else if (c == ',' && !insideQuotes) {
					// Found a comma outside of quotes, push the token
					if (!token.empty()) {
						if (token.front() == '"' && token.back() == '"') {
							token = token.substr(1, token.length() - 2);  // Remove opening and closing quotes
						}
						tokens.push_back(token);
					}
					token.clear(); // Clear the token for the next one
				} else {
					token += c; // Append the character to the current token
				}
			}

			// Push the last token (if any)
			if (!token.empty()) {
				if (token.front() == '"' && token.back() == '"') {
					token = token.substr(1, token.length() - 2);  // Remove opening and closing quotes
				}
				tokens.push_back(token);
			}

		
		if(tokens.size() == 13)
		{
			std::string start_buffer_str; std::string end_buffer_str;
			double start_lat; double start_lon; double end_lat; double end_lon;
			try
			{
				start_buffer_str = tokens[2];
				end_buffer_str = tokens[3];
				start_lat = std::stod(tokens[8]); start_lon = std::stod(tokens[9]);
				end_lat = std::stod(tokens[10]); end_lon = std::stod(tokens[11]);				
			} catch (const std::exception& e)
			{
				exception_counter++;
				//std::cout << line << std::endl;
				//for(int i=0; i<tokens.size(); i++)
					//std::cout << tokens[i] << " ";
				//printf("\n");
				//getchar();
				//std::cerr << "Exception encountered: " << e.what() << std::endl;
				continue;
			}
			
			std::string start_date_str, start_time_str;
			std::string end_date_str, end_time_str;

			// Find the position of the space character separating date and time
			size_t spacePos = start_buffer_str.find(' ');
			if (spacePos != std::string::npos) {
				// Extract the date and time substrings
				start_date_str = start_buffer_str.substr(0, spacePos);
				start_time_str = start_buffer_str.substr(spacePos + 1);
			}
			spacePos = end_buffer_str.find(' ');
			if (spacePos != std::string::npos) {
				// Extract the date and time substrings
				end_date_str = end_buffer_str.substr(0, spacePos);
				end_time_str = end_buffer_str.substr(spacePos + 1);
			}
			
			int hour, minute, second;
			if (sscanf(start_time_str.c_str(), "%d:%d:%d", &hour, &minute, &second) == 3) 
			{
				if (hour < start_hour || hour > end_hour-1) 
				{
					out_of_time++; continue;
				}
			}
			
			double duration_seconds = 0.0;
			// Parse the date and time strings
			struct std::tm time1 = {};
			struct std::tm time2 = {};
			if (strptime((start_date_str + " " + start_time_str).c_str(), "%Y-%m-%d %H:%M:%S", &time1) &&
				strptime((end_date_str + " " + end_time_str).c_str(), "%Y-%m-%d %H:%M:%S", &time2)) {
				// Calculate the time difference in seconds
				duration_seconds = std::difftime(std::mktime(&time2), std::mktime(&time1));

				//std::cout << "Duration in seconds: " << duration_seconds << std::endl;
			} else {
				std::cerr << "Failed to parse time strings." << std::endl;
				no_time_difference++; std::cout << start_time_str << " " << end_time_str << std::endl; getchar();
			}
			if(duration_seconds<60.0) continue;
			//std::cout << start_time_str << std::endl;
			//getchar();
			
			TripData trip;	
			trip.start_date_str = start_date_str;
			trip.start_time_str = start_time_str;
			trip.end_date_str = end_date_str;
			trip.end_time_str = end_time_str;
			trip.duration = duration_seconds;
			trip.start_id = -1; trip.end_id = -1;
			
			for (const auto& station : data.stations)
			{
				if(std::abs(station.lat - start_lat) < 0.0001 && std::abs(station.lon - start_lon) < 0.0001)
				{
					trip.start_id = station.id; break;
				}
				if(station.name == tokens[4])
				{
					trip.start_id = station.id; break;
				}	
				
			}
			for (const auto& station : data.stations)
			{
				if(std::abs(station.lat - end_lat) < 0.0001 && std::abs(station.lon - end_lon) < 0.0001)
				{
					trip.end_id = station.id; break;
				}
				if(station.name == tokens[6])
				{
					trip.end_id = station.id; break;
				}		
			}
			if(trip.start_id==-1 || trip.end_id==-1)
			{
				no_id_counter++;
				//trip.Show();
				//std::cout << "start_lat:" << start_lat << " start_lon:" << start_lon << std::endl; 
				//std::cout << "end_lat:" << end_lat << " end_lon:" << end_lon << std::endl;
				//getchar();
				continue; //printf("Could not find station ids ... \n"); //getchar();
			}
			
			//trip.Show();
			//getchar();
			data.trips.push_back(trip);
			
			//to count the data days ...
			std::tm date = {0};
			std::istringstream dateStream(trip.start_date_str);
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
			if (data.dateToDayMap.find(trip.start_date_str) == data.dateToDayMap.end()) 
			data.dateToDayMap[trip.start_date_str] = data.dayCounter++;					
			
		}else {
			//std::cerr << ".csv line with missing data." << std::endl;
			//std::cout << "tokens size:" << (int)tokens.size() << std::endl;
			//for(const std::string& tok : tokens)
				//printf("%s \n",tok.c_str());
			//printf("\n");
			//getchar();
			trip_with_missing_data++;
		}
		
	}
	file_csv.close();
	printf("Sample trip object loaded:\n"); data.trips[0].Show();
	
	printf("The .csv has %d rows ...\n",(int)data.trips.size()+exception_counter+no_id_counter+out_of_time+no_time_difference+trip_with_missing_data);
	printf("There were %d rows with exceptions in long long ints or floats (trips that didn't have ending data)... \n",exception_counter);
	printf("There were %d rows that could not be added because station_id was not found from the .json file ... \n",no_id_counter);
	printf("There were %d rows where the time difference could not be computed from the .csv ... \n",no_time_difference);
	printf("There were %d rows with some missing data ... \n",trip_with_missing_data);
	printf("There were %d rows representing trips out_of_time [%d,%d] ... Continuing ... \n",out_of_time,start_hour,end_hour);
	
	printf("Loaded %d trips and %d stations.\n",(int)data.trips.size(),(int)data.stations.size());
	// Print elapsed time
    clock_t endTime = clock();
    double elapsedSeconds = (double)(endTime - startTime) / CLOCKS_PER_SEC;
    std::cout << "Elapsed Time: " << std::setprecision(3) << elapsedSeconds << " seconds" << std::endl;	
	
	// Calculate the difference in days
    double diffInSeconds = difftime(mktime(&latestDate), mktime(&earliestDate));
    data.nb_days_of_data = (int)(diffInSeconds / (60 * 60 * 24));
	printf("nb_days_of_data:%d dayCounter:%d\n",data.nb_days_of_data,data.dayCounter);	
}	

BSS_Data Load::CopyAndFilterBSSData(const BSS_Data& original_data, int max_nb_stations) 
{
    printf("In CopyAndFilterBSSData ... removing stations to meet the sought number:%d\n", max_nb_stations);

    // Define a structure to store station information and counts
	struct station_struct {
		int id;
		int count;
		station_struct() : id(-1), count(0) {}
		station_struct(int i, int c) : id(i), count(c) {}
	};


    std::vector<station_struct> vec;

    // Count the trips associated with each station
    for (const Station& station : original_data.stations) 
	{
        vec.push_back(station_struct(station.id, 0));
        for (const TripData& trip : original_data.trips)
		{
			if (trip.start_id == station.id) 
				vec.back().count++;
			if (trip.end_id == station.id) 
				vec.back().count++;
        }
    }

    // Sort stations in decreasing order of count
    std::sort(vec.begin(), vec.end(), [](const station_struct& a, const station_struct& b) {
        return a.count > b.count;
    });

    // Remove stations to meet max_nb_stations
    if (vec.size() > max_nb_stations) {
        vec.resize(max_nb_stations);
    }

    // Create a new BSS_Data object with filtered stations
    BSS_Data copied_data;
	copied_data.stations.insert(copied_data.stations.begin(), original_data.stations[0]);

    for (const Station& station : original_data.stations) {
        for (const station_struct& str : vec) {
            if (station.id == str.id) {
                copied_data.stations.push_back(station);
                break;
            }
        }
    }

    // Filter trips associated with the selected stations
    for (const TripData& trip : original_data.trips) 
	{
        bool found_start = false; bool found_end = false;
		for (const Station& station : copied_data.stations) 
		{	
            if (trip.start_id == station.id) 
			{
                found_start = true; break;
            }
        }
		for (const Station& station : copied_data.stations) 
		{	
            if (trip.end_id == station.id) 
			{
                found_end = true; break;
            }
        }
		
		if(found_start && found_end)
			copied_data.trips.push_back(trip);
    }
	
	printf("copied_data stations:%d trips:%d\n",(int)copied_data.stations.size(),(int)copied_data.trips.size());
    return copied_data;
}
