# Compiler and flags
CC = g++ -std=c++11
CFLAGS = -O3 -march=native -mtune=native
#to dbg
#CFLAGS = -ggdb3 -m64 -fPIC -fexceptions -DNDEBUG -DIL_STD -ldl -fsanitize=undefined -fsanitize=address
#to profile
#CFLAGS = -O3 -march=native -mtune=native -pg

LIBS = -lgsl -lgslcblas -lpthread
LIB_DIRS =
INCLUDE_DIRS =
# Source files
SRC_FILES = algo.cpp load.cpp

# Detect macOS and set compiler and libraries accordingly
UNAME := $(shell uname)
ifeq ($(UNAME), Darwin)
	# Use clang++ for macOS
    CC := clang++ 
	# Link GSL as a framework for macOS
    LIBS := -framework gsl 
    LIB_DIRS := -L/usr/local/lib
    INCLUDE_DIRS := -I/usr/local/include/
    OS_MESSAGE := "macOS detected"
else
    OS_MESSAGE := "Linux detected"
endif

# Default target
OUTPUT = gen
all: $(OUTPUT)

OBJ_FILES = algo.o load.o

# Compile algo.cpp
algo.o: algo.cpp algo.h csv.h
	$(CC) $(CFLAGS) -c algo.cpp -o algo.o $(LIBS) $(LIB_DIRS) $(INCLUDE_DIRS)

# Compile load.cpp
load.o: load.cpp load.h
	$(CC) $(CFLAGS) -c load.cpp -o load.o $(LIBS) $(LIB_DIRS) $(INCLUDE_DIRS)

# Link object files to generate the final executable
$(OUTPUT): $(OBJ_FILES)
	$(CC) $(CFLAGS) $(OBJ_FILES) -o $(OUTPUT) $(LIBS) $(LIB_DIRS) $(INCLUDE_DIRS)

usage:
	@echo "Usage: ./gen trips.csv station_data.json start_hour end_hour nb_scenarios"
	@echo "\n"
	@echo "Bixi (Montréal)"
	@echo "Sample usage: ./gen instance_type=bixi trips_data_file=../data/DonneesOuverte2023-04050607.csv stations_data_file=../data/station_information.json start_hour=8 end_hour=22 nb_scenarios=2 nb_instances=1 output_trips=random"
	@echo "\n"
	@echo "Capital Bike Share (Washington D.C.)"
	@echo "Sample usage: ./gen instance_type=cbs trips_data_file=../data/CapitalBikeShare2023-04050607.csv stations_data_file=../data/station_information_capital_bike_share.json start_hour=8 end_hour=22 nb_scenarios=2 nb_instances=1 output_trips=random"
	@echo "\n"
	@echo "Citi Bike (New York)"
	@echo "Sample usage: ./gen instance_type=citibike trips_data_file=../data/CitiBike2023-04050607.csv stations_data_file=../data/station_information_citi_bike.json start_hour=8 end_hour=22 nb_scenarios=2 nb_instances=1 output_trips=random"
	@echo "\n"
	@echo "Blue Bike (Boston)"
	@echo "Sample usage: ./gen instance_type=bluebike trips_data_file=../data/BlueBike2023-04050607.csv stations_data_file=../data/station_information_blue_bike.json start_hour=8 end_hour=22 nb_scenarios=2 nb_instances=1 output_trips=random"
	@echo "\n"	
	@echo "Additionally you can add one last interger max_nb_stations=nb to use nb number of stations only"
	@echo "If you want to make an instance will all the real trips add the parameter output_trips=real"

clean:
	rm -f $(OUTPUT) $(OBJ_FILES)

print_os:
	@echo $(OS_MESSAGE)	
