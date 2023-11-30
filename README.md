# BicycleSharingSystems
# A generator of random instances for bicycle sharing systems that can also parse real data that follows the GBFS standard.
# Data parsed with the help of the csv.h and json.hpp headers
# Input data is twofold: A csv file with the bike trips and a json file with the station information. According to GBFS, there are many bicycle sharing systems whose data is open and free.
# The generator parses data from 4 (four) systems but it can be extended to parse more.

1.-Building is via Makefile.

2.-type 'make usage' for options of usage provided in the Makefile. This is the output you should get:

Usage: ./gen trips.csv station_data.json start_hour end_hour nb_scenarios


Bixi (Montréal)
Sample usage: ./gen instance_type=bixi trips_data_file=../data/bixi_trips.csv stations_data_file=../data/station_information_bixi.json start_hour=8 end_hour=22 nb_scenarios=2 nb_instances=1 output_trips=random


Capital Bike Share (Washington D.C.)
Sample usage: ./gen instance_type=cbs trips_data_file=../data/cbs_trips.csv stations_data_file=../data/station_information_capital_bike_share.json start_hour=8 end_hour=22 nb_scenarios=2 nb_instances=1 output_trips=random


Citi Bike (New York)
Sample usage: ./gen instance_type=citibike trips_data_file=../data/citi_trips.csv stations_data_file=../data/station_information_citi_bike.json start_hour=8 end_hour=22 nb_scenarios=2 nb_instances=1 output_trips=random


Blue Bike (Boston)
Sample usage: ./gen instance_type=bluebike trips_data_file=../data/blue_trips.csv stations_data_file=../data/station_information_blue_bike.json start_hour=8 end_hour=22 nb_scenarios=2 nb_instances=1 output_trips=random


Additionally you can add one last interger max_nb_stations=nb to use nb number of stations only
If you want to make an instance will all the real trips change the parameter output_trips=real
