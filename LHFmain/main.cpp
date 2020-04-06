#include "mpi.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <typeinfo>
#include <thread>
#include "readInput.hpp"
#include "argParser.hpp"
#include "basePipe.hpp"
#include "writeOutput.hpp"
#include "pipePacket.hpp"
#include "preprocessor.hpp"
#include "utils.hpp"

using namespace std;

int nprocs,id;
 
void runPipeline(std::map<std::string, std::string> args, pipePacket* wD){
	auto *ws = new writeOutput();

	// Begin processing parts of the pipeline
	// DataInput -> A -> B -> ... -> DataOutput
	// Parsed by "." -> i.e. A.B.C.D
	auto pipe = args.find("pipeline");
	if(pipe != args.end()){
		auto pipeFuncts = std::string(args["pipeline"]);
		auto lim = count(pipeFuncts.begin(), pipeFuncts.end(), '.') + 1;
		
		//For each '.' separated pipeline function (count of '.' + 1 -> lim)
		for(unsigned i = 0; i < lim; i++){
			auto curFunct = pipeFuncts.substr(0,pipeFuncts.find('.'));
			pipeFuncts = pipeFuncts.substr(pipeFuncts.find('.') + 1);
			
			//Build the pipe component, configure and run
			auto *bp = new basePipe();
			auto *cp = bp->newPipe(curFunct, args["complexType"]);
			
			//Check if the pipe was created and configure
			if(cp != 0 && cp->configPipe(args)){
				//Run the pipe function (wrapper)
				*wD = cp->runPipeWrapper(*wD);
			} else {
				cout << "LHF : Failed to configure pipeline: " << args["pipeline"] << endl;
			}
		}
	}
	//If the pipeline was undefined...
	else {
		cout << "LHF : Failed to find a suitable pipeline, exiting..." << endl;
		return;
	}
	
	//Output the data using writeOutput library
	pipe = args.find("outputFile");
	if(pipe != args.end()){
		if (args["outputFile"] == "console"){
			//ws->writeConsole(wD);
		} else {
			ws->writeStats(wD->stats, args["outputFile"]);
			ws->writeBarcodes(wD->bettiOutput, args["outputFile"]);
			
		}
	}
	
	return;
}



void processDataWrapper(std::map<std::string, std::string> args, pipePacket* wD){
    
	//Start with the preprocessing function, if enabled
	auto pre = args["preprocessor"];
	if(pre != ""){
		auto *preprocess = new preprocessor();
		auto *prePipe = preprocess->newPreprocessor(pre);
		
		if(prePipe != 0 && prePipe->configPreprocessor(args)){
			*wD = prePipe->runPreprocessorWrapper(*wD);
		} else {
			cout << "LHF : Failed to configure pipeline: " << args["pipeline"] << endl;
		}
	}
	
	runPipeline(args, wD);
		
	return;
}	

void processReducedWrapper(std::map<std::string, std::string> args, pipePacket* wD){
    
    std::vector<bettiBoundaryTableEntry> mergedBettiTable;
    
	//Start with the preprocessing function, if enabled
	auto pre = args["preprocessor"];
	if(pre != ""){
		auto *preprocess = new preprocessor();
		auto *prePipe = preprocess->newPreprocessor(pre);
		
		if(prePipe != 0 && prePipe->configPreprocessor(args)){
			*wD = prePipe->runPreprocessorWrapper(*wD);
		} else {
			cout << "LHF : Failed to configure pipeline: " << args["pipeline"] << endl;
		}
	}
	
	utils ut;
	
	auto maxRadius = ut.computeMaxRadius(std::atoi(args["clusters"].c_str()), wD->originalData, wD->fullData, wD->originalLabels);
	auto centroids = wD->originalData;
	std::cout << "Using maxRadius: " << maxRadius << std::endl;
	
	auto partitionedData = ut.separatePartitions(2*maxRadius, wD->originalData, wD->fullData, wD->originalLabels);
	
	std::cout << "Partitions: " << partitionedData.second.size() << std::endl << "Counts: ";
	for(unsigned z = 0; z < partitionedData.second.size(); z++){
		if(partitionedData.second[z].size() > 0){
			std::cout << "Running Pipeline with : " << partitionedData.second[z].size() << " vectors" << std::endl;
			wD->originalData = partitionedData.second[z];
			
			
			runPipeline(args, wD);
			
			wD->complex->clear();
			
			//Map partitions back to original point indexing
			ut.mapPartitionIndexing(partitionedData.first[z], wD->bettiTable);
			
			for(auto betEntry : wD->bettiTable)
				mergedBettiTable.push_back(betEntry);
			
		} else 
			std::cout << "skipping" << std::endl;
	}
	
	std::cout << "Full Data: " << centroids.size() << std::endl;
	if(centroids.size() > 0){
		std::cout << "Running Pipeline with : " << centroids.size() << " vectors" << std::endl;
		wD->originalData = centroids;
		runPipeline(args, wD);
		
		wD->complex->clear();
	} else 
		std::cout << "skipping" << std::endl;
	
	std::cout << std::endl << "_______BETTIS_______" << std::endl;
	
	for(auto a : wD->bettiTable){
		std::cout << a.bettiDim << ",\t" << a.birth << ",\t" << a.death << ",\t";
		ut.print1DVector(a.boundaryPoints);
	}
	return;
}	





void processUpscaleWrapper(std::map<std::string, std::string> args, pipePacket* wD){
    
	//Start with the preprocessing function, if enabled
	std::vector<bettiBoundaryTableEntry> mergedBettiTable;
    utils ut;
 	if(id == 0)
	{
	std::vector<bettiBoundaryTableEntry> mergedMasterBettiTable;	
	auto *rs = new readInput();
	wD->originalData = rs->readCSV(args["inputFile"]);
    wD->fullData = wD->originalData;
	

    for(auto z : args)
		std::cout << z.first << "\t" << z.second << std::endl;
	 
	 
	auto pre = args["preprocessor"];
	if(pre != ""){
		auto *preprocess = new preprocessor();
		auto *prePipe = preprocess->newPreprocessor(pre);
		
		if(prePipe != 0 && prePipe->configPreprocessor(args)){
			*wD = prePipe->runPreprocessorWrapper(*wD);
		} else {
			cout << "LHF : Failed to configure pipeline: " << args["pipeline"] << endl;
		}
	}
	
	//Notes from Nick:
	//	After preprocessor, need to separate the data into bins by index (partition)
	//		-Look at each point, label retrieved from preprocessor
	//			-bin points and send to a respective MPI node / slave
	
	auto maxRadius = ut.computeMaxRadius(std::atoi(args["clusters"].c_str()), wD->originalData, wD->fullData, wD->originalLabels);
	auto centroids = wD->originalData;
	std::cout << "Using maxRadius: " << maxRadius << std::endl;
	
	auto partitionedData = ut.separatePartitions(2*maxRadius, wD->originalData, wD->fullData, wD->originalLabels);
	//	Each node/slave will process at least 1 partition
	//		NOTE: the partition may contain points outside that are within 2*Rmax
	
	int number_of_partitions_per_slave = partitionedData.second.size() / (nprocs-1);
	int dimension = partitionedData.second[0][0].size();
	std::vector<unsigned> partitionsize;
	
	//Get the partition sizes
	for(auto a: partitionedData.second)
		 partitionsize.push_back(a.size());
		 
	ut.print1DVector(partitionsize);
	
	//Sending the data dimensions to slaves	 
	for(int i=1;i<nprocs;i++)
		MPI_Send(&dimension,1,MPI_INT,i,1,MPI_COMM_WORLD);
		
	//Sending size of each partition to slaves	
	for(int i=1;i<nprocs;i++)
		MPI_Send(&partitionsize[0],20,MPI_INT,i,1,MPI_COMM_WORLD);
		
	//Sending number of partitions each slave has to do	
	for(int i=1;i<nprocs;i++)
		MPI_Send(&number_of_partitions_per_slave,1,MPI_INT,i,1,MPI_COMM_WORLD);
	
	// Sending partitions to slaves
	for(int k=1;k<nprocs;k++)	
	
		for(int j=0; j < number_of_partitions_per_slave; j++){
			for(int i=0;i< partitionedData.second[((k-1)*number_of_partitions_per_slave)+j].size();i++)
				MPI_Send( &partitionedData.second[((k-1)*number_of_partitions_per_slave)+j][i][0],dimension, MPI_DOUBLE, k, 1, MPI_COMM_WORLD);	 
		}
		
		for(int k=1;k<nprocs;k++){	
			for(int j=0; j < number_of_partitions_per_slave; j++)
				MPI_Send( &partitionedData.first[((k-1)*number_of_partitions_per_slave)+j][0],dimension, MPI_DOUBLE, k, 1, MPI_COMM_WORLD);	 
		}
  
		for(int k=1; k < nprocs; k++){
			int p=0;
			for(int j=0;j<number_of_partitions_per_slave;j++){
				int sizeOfBettiTable;
				MPI_Status status;
				MPI_Recv(&sizeOfBettiTable,1,MPI_INT,k,j,MPI_COMM_WORLD,&status);
				for(int i=0;i<sizeOfBettiTable;i++){
					int boundarysize;
					MPI_Recv(&boundarysize,1,MPI_INT,k,number_of_partitions_per_slave+(p++),MPI_COMM_WORLD,&status);
					bettiBoundaryTableEntry bettiEntry;
					MPI_Recv(&bettiEntry.bettiDim,1,MPI_UNSIGNED,k,number_of_partitions_per_slave+(p++),MPI_COMM_WORLD,&status);
					MPI_Recv(&bettiEntry.birth,1,MPI_DOUBLE,k,number_of_partitions_per_slave+(p++),MPI_COMM_WORLD,&status);
					MPI_Recv(&bettiEntry.death,1,MPI_DOUBLE,k,number_of_partitions_per_slave+(p++),MPI_COMM_WORLD,&status);
					std:vector<unsigned> boundaryvector(boundarysize);
					//boundaryvector.resize(boundarysize);
					MPI_Recv(&boundaryvector[0],boundarysize,MPI_UNSIGNED,k,number_of_partitions_per_slave+(p++),MPI_COMM_WORLD,&status);
					
					for(auto a:boundaryvector)
						bettiEntry.boundaryPoints.insert(a);
					mergedMasterBettiTable.push_back(bettiEntry);
				}
			}
		}
	
		for(unsigned z = ((nprocs-1)*number_of_partitions_per_slave);z<partitionedData.second.size();z++)
		{
			if(partitionedData.second[z].size() > 0){
				std::cout << "Running Pipeline with : " << partitionedData.second[z].size() << " vectors" << " id :: "<<id<<std::endl;
				wD->originalData = partitionedData.second[z];

				runPipeline(args, wD);
			
				wD->complex->clear();
			
				//Map partitions back to original point indexing
				ut.mapPartitionIndexing(partitionedData.first[z], wD->bettiTable);
			
				for(auto betEntry : wD->bettiTable)
					mergedMasterBettiTable.push_back(betEntry);
			
			} else 
				std::cout << "skipping" << std::endl;
		}
 
		std::cout << "Full Data: " << centroids.size() << std::endl;
		if(centroids.size() > 0){
			std::cout << "Running Pipeline with : " << centroids.size() << " vectors" << std::endl;
			wD->originalData = centroids;
			runPipeline(args, wD);
		
			wD->complex->clear();
	
		} else 
			std::cout << "skipping" << std::endl;
	
		std::cout << std::endl << "_______BETTIS_______" << std::endl;
	
		//for(auto a : mergedMasterBettiTable){
			//std::cout << a.bettiDim << ",\t" << a.birth << ",\t" << a.death << ",\t";
			//ut.print1DVector(a.boundaryPoints);
		//}
		std::cout << std::endl << "mergedMasterBettiTable size is :: " << mergedMasterBettiTable.size() << std::endl;
	} else {
		int dim;
		MPI_Status status;
		MPI_Recv(&dim,1,MPI_INT,0,1,MPI_COMM_WORLD,&status);
		std::vector<int> partsize;
		partsize.resize(20);
		MPI_Recv(&partsize[0],20,MPI_INT,0,1,MPI_COMM_WORLD,&status);
		int perslavepartitions;
		MPI_Recv(&perslavepartitions,1,MPI_INT,0,1,MPI_COMM_WORLD,&status);
	    
		
		std::vector<std::vector<std::vector<double>>> input;
		std::vector<std::vector<unsigned>> inputlabel;
		
		input.resize(perslavepartitions);
		for(int i=0;i<input.size();i++)
			input[i].resize(partsize[((id-1)*perslavepartitions)+i]);
		
		for(int j=0;j<input.size();j++){
			for(int i=0;i<input[j].size();i++)
				input[j][i].resize(dim);
		}
	    
		inputlabel.resize(perslavepartitions);
		for(int i=0;i<inputlabel.size();i++)
			 inputlabel[i].resize(partsize[((id-1)*perslavepartitions)+i]);

		for(int j=0;j<perslavepartitions;j++){		
			for(int i=0;i<partsize[(id-1)*perslavepartitions+j];i++)
	    	   MPI_Recv (&input[j][i][0] ,dim, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD, &status );
		}	
		
		
		for(int j=0;j<perslavepartitions;j++){		
			MPI_Recv (&inputlabel[j][0] ,partsize[((id-1)*perslavepartitions)+j], MPI_DOUBLE, 0, 1, MPI_COMM_WORLD, &status );
		}	
		
		int p=0;			
		for(unsigned z = 0; z < perslavepartitions; z++){
		if(input[z].size() > 0){
			std::cout << "Running Pipeline with : " << input[z].size() << " vectors" << " id :: "<<id<<std::endl;
			wD->originalData = input[z];
			
			runPipeline(args, wD);
			
			wD->complex->clear();
			
			//Map partitions back to original point indexing
			ut.mapPartitionIndexing(inputlabel[z], wD->bettiTable);
			int bettiTableSize = wD->bettiTable.size();
			MPI_Send(&bettiTableSize,1,MPI_INT,0,z,MPI_COMM_WORLD);
			for(auto betEntry : wD->bettiTable){  
				int boundary_size = betEntry.boundaryPoints.size();	
				MPI_Send(&boundary_size,1,MPI_INT,0,perslavepartitions+(p++),MPI_COMM_WORLD);
				MPI_Send(&betEntry.bettiDim,1,MPI_UNSIGNED,0,perslavepartitions+(p++),MPI_COMM_WORLD);
				MPI_Send(&betEntry.birth,1,MPI_DOUBLE,0,perslavepartitions+(p++),MPI_COMM_WORLD);
				MPI_Send(&betEntry.death,1,MPI_DOUBLE,0,perslavepartitions+(p++),MPI_COMM_WORLD);
				vector<unsigned> boundaryvector;
				boundaryvector.assign(betEntry.boundaryPoints.begin(), betEntry.boundaryPoints.end());
				MPI_Send(&boundaryvector[0],boundary_size,MPI_UNSIGNED,0,perslavepartitions+(p++),MPI_COMM_WORLD);
			}		
		
		} else 
			std::cout << "skipping" << std::endl;
    	}
	}
}	

int main(int argc, char* argv[]){
	//	Steps to compute PH with preprocessing:
	//
	//		1.  Preprocess the input data
	//			a. Store the original dataset, reduced dataset, cluster indices
	//
	//		2.	Build a 2-D neighborhood graph
	//			a. Weight each edge less than epsilon
	//			b. Order edges (min to max)
	//
	//		3.  Rips filtration
	//			a. Build higher-level simplices (d > 2)
	//				- Retain the largest of edges as weight (when simplex forms)
	//			b. Build list of **relevant epsilon values
	//
	//		4.	Betti calculation
	//			a. For each relevant epsilon value
	//				i.	Create Boundary Matrix
	//					- if pn[i,j] < n, set to 1
	//					- else set to 0
	//				ii.	Compute RREF
	//				iii.Store constituent boundary points
	//
	//		5.	Upscaling
	//			a. Upscale around boundary points
	//			b. For each upscaled boundary, repeat steps 2-4
	//
	//
	//	**relevant refers to edge weights, i.e. where a simplex of any
	//			dimension is created (or merged into another simplex)
	
	
	//Define external classes used for reading input, parsing arguments, writing output
    MPI_Init(NULL,NULL);
   	MPI_Comm_size(MPI_COMM_WORLD,&nprocs);
	MPI_Comm_rank(MPI_COMM_WORLD,&id);
	
	auto *rs = new readInput();
	auto *ap = new argParser();
    
    //Parse the command-line arguments
    auto args = ap->parse(argc, argv);
    
    //Determine what pipe we will be running
    ap->setPipeline(args);
    
 //   for(auto z : args)
	//	std::cout << z.first << "\t" << z.second << std::endl;
    
	//Create a pipePacket (datatype) to store the complex and pass between engines
    auto *wD = new pipePacket(args, args["complexType"]);	//wD (workingData)
	
	if(args["pipeline"] != "slidingwindow"&&args["mode"] != "mpi"){
		//Read data from inputFile CSV
		wD->originalData = rs->readCSV(args["inputFile"]);
		wD->fullData = wD->originalData;
	}
	
	//If data was found in the inputFile
	if(wD->originalData.size() > 0 || args["pipeline"] == "slidingwindow" || args["mode"] == "mpi"){
		
		//Add data to our pipePacket
		wD->originalData = wD->originalData;
		
		if(args["upscale"] == "true"){
			processUpscaleWrapper(args, wD);
		} else if (args["mode"] == "reduced"){
			processReducedWrapper(args,wD);			
		} else {
			processDataWrapper(args, wD);
		}
	} else {
		ap->printUsage();
	}
	
	
	 MPI_Finalize();
    return 0;
}
