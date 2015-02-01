#include "ncl.h"

#include <string>
#include <iostream>
#include <vector>
#include <fstream>
using namespace std;

bool gNclInitialized = true;	//Global variable to maintain the state of the NCL
int gHandle = -1; //Global variable to maintain the current connected Nymi handle
std::vector<NclProvision> gProvisions; //Global vector for storing the list of provisioned Nymi

void saveProvisionsToFile()
{
	ofstream file("provisions.txt");
	file << gProvisions.size() << "\n";
	for (unsigned i = 0; i<gProvisions.size(); ++i){
		for (unsigned j = 0; j<NCL_PROVISION_KEY_SIZE; ++j) file << (int)gProvisions[i].key[j] << " ";
		file << " ";
		for (unsigned j = 0; j<NCL_PROVISION_ID_SIZE; ++j) file << (int)gProvisions[i].id[j] << " ";
		file << "\n";
	}
	file.close();
}

void getProvisionsFromFile(){
	ifstream file("provisions.txt");
	if (file.good()){
		cout << "Loaded saved provision ID: ";
		unsigned size;
		file >> size;
		for (unsigned i = 0; i<size; ++i){
			gProvisions.push_back(NclProvision());
			for (unsigned j = 0; j<NCL_PROVISION_KEY_SIZE; ++j){
				unsigned b;
				file >> b;
				gProvisions.back().key[j] = b;
			}
			for (unsigned j = 0; j<NCL_PROVISION_ID_SIZE; ++j){
				unsigned b;
				file >> b;
				gProvisions.back().id[j] = b;
				cout << b;
			}
			cout << "\n";
		}
	}
}

/*
Function for handling the events thrown by the NCL
@param[in] event NclEvent that contains the event type and member variables
@param[in] userData Data that needs to be passed to the callback functions if provided
*/
void callback(NclEvent event, void* userData){
	NclBool res;
	switch (event.type){
	case NCL_EVENT_INIT:
		if (event.init.success){
			gNclInitialized = true;
			saveProvisionsToFile();
		}
		else exit(-1);
		break;
	case NCL_EVENT_ERROR:
		exit(-1);
		break;
	case NCL_EVENT_DISCOVERY:
		if (event.find.rssi > -5){
			std::cout << "log: Nymi found\n";
			res = nclStopScan(); //Stops scanning to prevent more find events
			if (res){
				std::cout << "Stopping Scan successful\n";
			}
			else{
				std::cout << "Stopping Scan failed\n";
			}

			getProvisionsFromFile();

			gHandle = event.discovery.nymiHandle;
			res = nclAgree(gHandle); //Initiates the provisioning process with discovered Nymi
		}
		else {
			std::cout << "Nymi not in range\n";
		}
		break;

	case NCL_EVENT_FIND:

		if (res){
			std::cout << "Stopping Scan successful\n";
		}
		else{
			std::cout << "Stopping Scan failed\n";
		}

		gHandle = event.find.nymiHandle;
		res = nclValidate(gHandle); //Validates the found Nymi
		if (res){
			std::cout << "Validate request successful\n";
		}
		else{
			std::cout << "Validaterequest failed\n";
		}
		break;
	case NCL_EVENT_DISCONNECTION:
		std::cout << "log: disconnected\n";
		gHandle = -1; //Uninitialize the Nymi handle
		break;
	case NCL_EVENT_AGREEMENT:
		//Displays the LED pattern for user confirmation
		std::cout << "Is this:\n";
		for (unsigned i = 0; i<NCL_AGREEMENT_PATTERNS; ++i){
			for (unsigned j = 0; j<NCL_LEDS; ++j)
				std::cout << event.agreement.leds[i][j];
			std::cout << "\n";
		}
		//Automatically agrees
		res = nclProvision(gHandle, NCL_FALSE);
		break;
	case NCL_EVENT_PROVISION:
		//Store the provision information in a vector. Ideally this information
		//is stored in persistent memory to be used later for future validations
		gProvisions.push_back(event.provision.provision);
		std::cout << "log: provisioned\n";
		break;
	case NCL_EVENT_VALIDATION:
		std::cout << "Nymi validated! Now trusted user requests can happen, such as request Symmetric Keys!\n";
		break;
	default: break;
	}
}

/*
Main program function
*/
int main(){
	//Only if using the Nymulator.
	//127.0.0.1 is the localhost computer. Supply a different IP if using a different host computer
	//9089 is the port the Nymulator is listening on
	if (!nclSetIpAndPort("127.0.0.1", 9089)) return -1;

	//Initializes the Nymi Communication Library
	//'callback' refers to the function that will be handling the NCL callbacks
	//NULL indicates there is no data to be passed to the NCL callbacks
	//'HelloNymi' is the name of this NEA program that will be provisioned in the Nymi
	//NCL_MODE_DEFAULT to run NCL in the default mode
	//stderr refers to the stream where the NCL logs will be printed
	if (!nclInit(callback, NULL, "HelloNymi", NCL_MODE_DEFAULT, stderr)) return -1;

	//Main loop for continuously polling user input

	while (true){

		//Ensures no commands are handled until NCL has completed initialization
		if (!gNclInitialized){
			std::cout << "error: NCL didn't finished initializing yet!\n";
			continue;
		}

		// Start discovery
		NclBool res = nclStartDiscovery();

	}

	nclFinish(); //closes the NCL
	return 0; //Quits program
}
