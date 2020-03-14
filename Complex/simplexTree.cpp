#include <string>
#include <algorithm>
#include <vector>
#include <unistd.h>
#include <iostream>
#include "simplexTree.hpp"

simplexTree::simplexTree(double _maxEpsilon, std::vector<std::vector<double>> _distMatrix, int _maxDim){
	indexCounter = 0;
	distMatrix = _distMatrix;
	maxDimension = _maxDim;
	maxEpsilon = _maxEpsilon;
	simplexType = "simplexTree";
	return;
}

//**													**//
//** 				Private Functions 					**//
//**													**//
void simplexTree::recurseInsert(treeNode* node, unsigned curIndex, int depth, double maxE, std::set<unsigned> simp){
	//Incremental insertion
	//Recurse to each child (which we'll use the parent pointer for...)
	treeNode* temp;
	std::set<unsigned> currentSimp = simp;

	double curE = 0;

	if(runningVectorIndices.size() < runningVectorCount+1){
		int offset = runningVectorCount+1 - runningVectorIndices.size();
		curE = distMatrix[node->index - offset][indexCounter - offset];

	}else{
		curE = distMatrix[node->index][indexCounter];
	}

	curE = curE > maxE ? curE : maxE;

	//Check if the node needs inserted at this level
	if(curE < maxEpsilon){
		treeNode* insNode = new treeNode();
		currentSimp.insert(curIndex);
		insNode->index = curIndex;
		insNode->simplex = currentSimp;
		nodeCount++;
		simp.insert(node->index);

		//Get the largest weight of this simplex
		maxE = curE > node->weight ? curE : node->weight;
		insNode->weight = maxE;

		//if depth (i.e. 1 for first iteration) is LT weightGraphSize (starts at 1)
		if(weightEdgeGraph.size() < simp.size()){
			std::vector<std::pair<std::set<unsigned>,double>> tempWEG;
			tempWEG.push_back(std::make_pair(simp, maxE));
			weightEdgeGraph.push_back(tempWEG);
		} else {
			weightEdgeGraph[simp.size() - 1].push_back(std::make_pair(simp, maxE));
		}

		//Check if the node has children already... (remember parent == child for incremental)
		if(node->parent == nullptr){
			node->parent = insNode;

		} else {
			insNode->sibling = node->parent;
			node->parent = insNode;

			temp = insNode->sibling;
			//Have to check the children now...
			if(simp.size() <= maxDimension){
				do {
					recurseInsert(temp, curIndex, depth + 1, maxE, simp);
				} while(temp->sibling != nullptr && (temp = temp->sibling) != nullptr);
			}
		}
	}

	return;
}


void simplexTree::printTree(treeNode* head){
	std::cout << "_____________________________________" << std::endl;
	if(head == nullptr){
		std::cout << "Empty tree... " << std::endl;
		return;
	}

	std::cout << "HEAD: " << head->index << "," << head << "," << head->child << "," << head->sibling << std::endl;

	treeNode* current;
	for(int i = 0; i < dimensions.size() ; i++){
		std::cout << std::endl << "Dim: " << i << std::endl << std::endl;
		current = dimensions[i];

		do{
			std::cout << current->index << "," << current << "," << current->sibling << "\t";
		} while(current->sibling != nullptr && (current = current->sibling) != nullptr);

		std::cout << std::endl;
	}

	std::cout << "_____________________________________" << std::endl;
	return;
}

void simplexTree::insertInductive(){
	//Create our new node to insert
	treeNode* curNode = new treeNode;
	curNode->index = indexCounter;

	//Loop each dimensional list; start at dn, then dn-1, ..., d0 (OLD WAY, INDUCTIVE VR)
	for(int i = (dimensions.size()-1 < maxDimension ? dimensions.size() - 1 : maxDimension); i >= 0; i--){

		//Loop each sibling in the current dimensional list (as a do-while, break when no more siblings)
		curNode = dimensions[i];
		do{

			//Determine if the index needs to inserted below the current node
			//	First, check the distance matrix for curNodeIndex v. curIndex
			//	Second (if first is true), check all parents dist matrix of parentNodeIndex v. curIndex
			if(distMatrix[curNode->index][indexCounter] < maxEpsilon){

				//	This node is a candidate, now we need to check each parent of the current branch
				//		to ensure the distance matrix entry for parentNodeindex v. curIndex is < maxEpsilon
				bool ins = true;
				treeNode* parentNode = curNode; //Temporarily store the parent so we don't lose track of our current

				// This loop will check each parent and set whether to insert a node
				do{
					if(distMatrix[parentNode->index][indexCounter] > maxEpsilon){
						ins = false;
					}
				}while(ins && parentNode->parent != nullptr && (parentNode = parentNode->parent) != nullptr);

				//Insert the node; the distance to each parent is less than epsilon
				if(ins){
					//Allocate a new node to be inserted into the tree
					treeNode* insNode = new treeNode;

					//This new node's parent is the current node we're indexing on
					insNode->parent = curNode;
					insNode->index = indexCounter;
					insNode->sibling = nullptr;

					//Check the dimensional list
					//	if no nodes exist at the dimension, this is the first
					if(i == dimensions.size() - 1){
						dimensions.push_back(insNode);

					//	Nodes currently exist at the dimension, insert as a sibling
					//		This should be inserted with nodes of the same parent
					} else {
						treeNode* iterateNode = dimensions[i+1];
						ins = false; //Store if we've inserted this node while looping through siblings

						//Loop through dimensional node siblings
						do{
							//Check if we've found parent nodes...
							if(iterateNode->parent == insNode->parent){
								ins = true;

								//perform an insertion of the node...
								insNode->sibling = iterateNode->sibling;
								iterateNode->sibling = insNode;

								nodeCount++;
							}
						} while(!ins && iterateNode->sibling != nullptr && (iterateNode = iterateNode->sibling) != nullptr);

						// Check if we successfully inserted a node; if not, insert to end
						if(!ins){
							iterateNode->sibling = insNode;
						}
					}
				}
			}

		}while(curNode->sibling != nullptr && (curNode = curNode->sibling) != nullptr);
	}

	return;
}



//**													**//
//** 				Public Functions 					**//
//**													**//


// Insert a node into the tree using the distance matrix and a vector index to track changes
bool simplexTree::insertIterative(std::vector<double> &currentVector, std::vector<std::vector<double>> &window, EvalParams &defaultVals){
	if(window.size() == 0){
		return true;
	}

	if(streamEval(currentVector, window, defaultVals)){   // Point is deemed 'significant'

		deleteIterative( defaultVals.keyToBeDeleted, defaultVals.indexToBeDeleted );  // Delete from distance matrix and complex.
		runningVectorIndices.erase( runningVectorIndices.begin() + defaultVals.indexToBeDeleted );

		// Add a new column and row to the end of the upper triangular distance matrix.
		for(unsigned int i = 0; i < distMatrix.size(); i++) {
            distMatrix[i].push_back( defaultVals.distsFromCurrVec[i] );

            // Update NN statistics for only those partitions from which the point was deleted or to which the new point is to be added.
            // Case 1: i-th point belongs to a partition the last point was deleted from, but not to the partition the new point is to be added to.
            // And, the point that was deleted was the nearest neighbor of the i-th point.
            if ( defaultVals.partitionLabels[i] == defaultVals.labelToBeDeleted
                    &&  defaultVals.partitionLabels[i] != defaultVals.targetPartition
                    && defaultVals.nnIndices[i] == defaultVals.indexToBeDeleted )
            {

                // If only one point remains in the partition from which the last point was deleted:
                if ( defaultVals.numPointsPartn[defaultVals.labelToBeDeleted] == 1 )
                {
                    defaultVals.avgNNDistPartitions[defaultVals.labelToBeDeleted] = -1;
                    defaultVals.nnIndices[i] = -1;
                    defaultVals.nnDists[i] = -1;
                } else {

                }


            }


		}
		distMatrix.push_back( defaultVals.distMatLastRow );



		insert(distMatrixRow);



		removedSimplices++;

		return true;
	}

	return false;
}

// Delete a node from the tree and from the distance matrix using a vector index
void simplexTree::deleteIterative(int keyToBeDeleted, int indexToBeDeleted)
{
    // Delete the corresponding row and column from the distance matrix.
    distMatrix.erase(distMatrix.begin() + indexToBeDeleted);

    for(auto z : distMatrix)
    {
        if(z.size() >= indexToBeDeleted)
            z.erase(z.begin() + indexToBeDeleted);
    }

    //Delete all entries in the simplex tree with the index...
    // TODO :)
    deleteIndexRecurse(keyToBeDeleted, head);

    return;
}


void simplexTree::deleteIndexRecurse(int vectorIndex, treeNode* curNode){

	if(curNode == nullptr){
		std::cout << "Empty tree" << std::endl;
		return;
	}
	//Handle siblings - either they need to be removed (and 'hopped') or recursed
	if(curNode->sibling != nullptr && curNode->sibling->index == vectorIndex){
		treeNode* tempNode = curNode->sibling;
		curNode->sibling = curNode->sibling->sibling;
		deleteIndexRecurse(vectorIndex, curNode->sibling);

	} else if(curNode->sibling != nullptr){
		deleteIndexRecurse(vectorIndex, curNode->sibling);
	}


	if(curNode->index == vectorIndex){

		if(curNode == head){
			head = curNode->sibling;
			dimensions[0] = head;
		}

		deletion(curNode);
	} else if (curNode->child != nullptr && curNode->child->index == vectorIndex){
		treeNode* tempNode = curNode->child;
		curNode->child = curNode->child->sibling;
		deletion(tempNode);

	} else if(curNode->child != nullptr){
		deleteIndexRecurse(vectorIndex, curNode->child);
	}

	return;
}



// Insert a node into the tree
//
void simplexTree::insert(std::vector<double>&) {

	if(distMatrix.size() == 0){
		ut.writeDebug("simplexTree","Distance matrix is empty, skipping insertion");
		return;
	}

	//Create our new node to insert
	treeNode* curNode = new treeNode;
	curNode->index = indexCounter;
	std::set<unsigned> tempSet = {curNode->index};
	runningVectorIndices.push_back(runningVectorCount);

	//Check if this is the first node (i.e. head)
	//	If so, initialize the head node
	if(head == nullptr){
		head = curNode;
		indexCounter++;
		runningVectorCount++;
		nodeCount++;
		dimensions.push_back(head);

		std::vector<std::pair<std::set<unsigned>,double>> tempWEG;
		tempWEG.push_back(std::make_pair(tempSet, 0));
		weightEdgeGraph.push_back(tempWEG);

		return;
	}

	// This needs to be a recursive span -> (or not!)
	//		if the node has a child, recurse
	//		if the node has a sibling, recurse
	//
	//d0 -->          ({!})
	//			       /
	//			      /
	//d1 -->	   | 0 | 1 | ... |
	//		        /
	//             /
	//d2 -->    | 1 | 2 | 3 |
	//           /         \
	//	        /           \
	//d3 --> | 2 | 3 |     | 4 | 5 |
	//

	treeNode* temp = dimensions[0];

	//Now let's do it the new way - incremental VR;
	//	Start at the first dimension (d0);
	//		if e (d0, dn) < eps
	//			recurse down tree
	//				recurse
	//			insert to current
	//	iterate to d0->sibling

	do{
		recurseInsert(temp, indexCounter, 0, 0, tempSet);
	}while(temp->sibling != nullptr && (temp = temp->sibling) != nullptr);

	//Insert into the right of the tree
	temp = dimensions[0];
	treeNode* ins = new treeNode();
	while(temp->sibling != nullptr && (temp = temp->sibling) != nullptr);
	ins->index = indexCounter;
	ins->sibling = nullptr;
	ins->parent = nullptr;
	temp->sibling = ins;
	weightEdgeGraph[0].push_back(std::make_pair(tempSet, 0));

	nodeCount++;
	indexCounter++;
	runningVectorCount++;

	return;
}


int simplexTree::vertexCount(){
	//Return the number of vertices currently represented in the tree
	if(runningVectorIndices.size() < runningVectorCount+1)
		return runningVectorIndices.size();
	return indexCounter;
}

int simplexTree::simplexCount(){
	//Return the number of simplices currently in the tree
	return nodeCount;
}

double simplexTree::getSize(){
	//Size of node: [int + byte (*) + byte (*)] = 18 Bytes
	return nodeCount * sizeof(treeNode);
}


std::vector<std::vector<std::pair<std::set<unsigned>,double>>> simplexTree::getAllEdges(double epsilon){

	if(!isSorted){
		for (int i = 0; i < weightEdgeGraph.size(); i++)
			std::sort(weightEdgeGraph[i].begin(), weightEdgeGraph[i].end(), ut.sortBySecond);
		isSorted = true;
	}

	return weightEdgeGraph;
}

void simplexTree::reduceComplex(){
	if(weightEdgeGraph.size() == 0){
		ut.writeDebug("simplexTree","Complex is empty, skipping reduction");
		return;
	}

	//Start with the largest dimension
	ut.writeDebug("simplexTree","Reducing complex, starting simplex count: " + std::to_string(simplexCount()));
	treeNode* cur;

	if(dimensions.size() > 0){
		for(auto i = dimensions.size()-1; i > 1; i--){

			std::vector<std::set<unsigned>> removals;
			std::vector<std::set<unsigned>> checked;

			while(checked.size() != weightEdgeGraph[i].size()){
				cur = dimensions[i];
				do {
					if(std::find(checked.begin(),checked.end(),cur->simplex) == checked.end()){
						auto ret = recurseReduce(std::make_pair(cur->simplex, cur->weight), removals, checked);
						removals = ret.first;
						checked = ret.second;
					}
				} while (cur->sibling != nullptr && (cur = cur->sibling) != nullptr);
			}

			//Remove the removals
			for(auto rem : removals){
				deletion(rem);
			}
			std::cout << "Remove " << removals.size() << " from dim " << i << std::endl;

		}
	}
	ut.writeDebug("simplexTree","Finished reducing complex, reduced simplex count: " + std::to_string(simplexCount()));

	return;
}

std::pair<std::vector<std::set<unsigned>>, std::vector<std::set<unsigned>>> simplexTree::recurseReduce(std::pair<std::set<unsigned>,double> simplex, std::vector<std::set<unsigned>> removals, std::vector<std::set<unsigned>> checked){
	checked.push_back(simplex.first);
	auto subsets = ut.getSubsets(simplex.first);
	std::set<unsigned> maxFace;

	bool canRemove = true;

	//Look at each face
	for(auto face : subsets){

		//Check if the face is shared; if so, recurse
		for(auto simp : weightEdgeGraph[simplex.first.size() - 1]){

			if(simp != simplex && std::find(checked.begin(), checked.end(), simp.first) == checked.end()){
				auto sDiff = ut.symmetricDiff(simp.first, face,true);

				//This point intersects;
				if (sDiff.size() == 1){
					auto ret = recurseReduce(simp, removals, checked);
					removals = ret.first;
					checked = ret.second;

					//Check if the simplex was not removed
					if(std::find(removals.begin(), removals.end(), simp.first) == removals.end()){
						canRemove = false;
						break;
					}
				}

			}
		}

		//Check if the face is the max face
		double wt = -1;
		if((wt = findWeight(face)) == simplex.second){
			maxFace = face;
		}

	}

	if(canRemove){
		removals.push_back(simplex.first);
		removals.push_back(maxFace);
	}

	return std::make_pair(removals, checked);

}



std::vector<std::vector<unsigned>> simplexTree::getDimEdges(int d,double){
	std::vector<std::vector<unsigned>> ret;

	if(weightEdgeGraph.size() >= d){
		for(auto z : weightEdgeGraph[d]){
			std::vector<unsigned> temp(z.first.begin(), z.first.end());
			ret.push_back(temp);
		}
	}

	return ret;

}

bool simplexTree::find(std::set<unsigned>){

	ut.writeLog("simplexTree","find(std::set<unsigned>) not implemented!");
	return 0;
}

// A recursive function to delete a simplex (and sub-branches) from the tree.
bool simplexTree::deletion(std::set<unsigned> removalEntry) {
	//Remove the entry in the simplex tree
	bool found = true;
	treeNode* curNode = dimensions[0];
	int d = removalEntry.size() - 1;


	/*for(int i = 0; i < dimensions[d].size(); i++){
		if(removalEntry == dimensions[d][i]->simplexSet){
			curNode = dimensions[d][i];
			dimensions[d]->erase(dimensions[d].begin() + i);
			return deletion(curNode);
		}
	}*/


	ut.writeLog("simplexTree","deletion(std::set<unsigned>) not implemented!");
	return false;

}


// A recursive function to delete a simplex (and sub-branches) from the tree.
bool simplexTree::deletion(treeNode* removalEntry) {
	treeNode* curNode = removalEntry;
	int deletionCount = 0;

	//Iterate to the bottom of branch
	while(curNode->child != nullptr){
		curNode->child->parent = curNode;
		curNode=curNode->child;
		std::cout << "\t\t@ " << curNode->index << std::endl;
	}

	if(curNode->sibling != nullptr){
		deletion(curNode->sibling);
	}

	//If we did go down, remove on the way back up
	while(curNode != removalEntry){
		curNode = curNode->parent;

		if(curNode->sibling != nullptr){
			deletion(curNode->sibling);
		}

		deletionCount++;
		delete curNode->child;
		curNode->child = nullptr;
	}

	//curNode = curNode->parent;
	deletionCount++;
	delete curNode;
	nodeCount--;
	//curNode->child = nullptr;

	std::cout << "\tDeleted " << deletionCount << " nodes" << std::endl;



	return false;
}

double simplexTree::findWeight(std::set<unsigned> simplex){

	ut.writeLog("simplexTree","findWeight(std::set<unsigned>) not implemented!");
	return 1;
}


std::vector<std::pair<double, std::vector<unsigned>>> simplexTree::getd0Pairs(){
	std::vector<std::pair<double, std::vector<unsigned>>> ret;
	ut.writeLog("simplexTree","getd0Pairs() not implemented!");
	return ret;
}

