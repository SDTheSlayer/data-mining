// ISCAN Class
#include <bits/stdc++.h>
#include "graph.h"
#include "bfsTree.h"


#define CORE 0
#define NON_MEMBER 1
#define NON_CORE_MEMBER 2
#define HUB 0
#define OUTLIER 1 

using namespace std;



class iscan
{
public:
    // epsilon parameter 
    float epsilon;  

    // mu parameter
    int mu;  

    // graph to be analysed
    graph* inputGraph; 

    bfsTree* bfsTreeObject;

    // Number of threads
    int number_of_threads = 4;
    
    // Stores similarity values for each edge
    unordered_map<pair<vertex*,vertex*>,float,hash_pair> epsilon_values;

    // Constructor with epsilon, lambda and graph as parameters
    iscan(float, int, graph*);

    // constructor with epsilon, lambda, graph and number of threads as parameters
    iscan(float, int, graph*, int);

    // Returns similarity between two vertices from epsilon_values
    float getSimilarity(vertex*, vertex*);

    // Calculates similarity between two vertices
    float calculateSimilarity(vertex*, vertex*);

    // Update similarity of all edges in Ruv using single thread
    void updateRuvSimilaritySingleThreaded(unordered_set<pair<vertex*,vertex*>,hash_pair> edges);
    
    // Update similarity of all edges in Ruv using multiple thread
    void updateRuvSimilarityMultiThreaded(unordered_set<pair<vertex*,vertex*>,hash_pair> edges);
    
    // Util function to check if cycle present in BFS tree
    bool checkCycle();

    // Returns epsilon neighbourhood of a neighbourhood
    vector<vertex*> getEpsilonNeighbourhood(vertex*);

    // Checks if a vertex is core
    bool isCore(vertex*);

    // Calculate similarity of all edges using single thread
    void calculateAllSimilaritySingleThreaded();

    // Calculate similarity of all edges using multiple thread
    void calculateAllSimilarityMultiThreaded();

    // Executes SCAN for initial clustering
    void executeSCAN(bool multithreading);

    // Output formed cluster to intermediate file
    void printClusterToFile(vector<vertex*>, ofstream &, int);

    // Output epsilon neighbourhood of all vertices in intermediate file
    void printEpsilonNeighbours(ofstream &);

    // Returns Nuv for two vertices with ids id1 and id2 
    unordered_set<vertex*> getNuv(int Id1, int Id2);

    // Returns Ruv for two vertices with ids id1 and id2 and set Nuv
    unordered_set<pair<vertex*,vertex*>,hash_pair> getRuv(int Id1, int Id2, unordered_set<vertex*> Nuv);

    // Main incremental function to update edge between a and b
    void updateEdge(int id1, int id2, bool isAdded, bool multithreading);

    void mergeCluster(vertex* w);

    void splitCluster(vertex* u, vertex* v2, unordered_set<vertex*>& old_cores);

    void printVector(vector<vertex*> neighbours);
    
};

// constructor
iscan::iscan(float ep,int mu, graph* inputGraph)
{
    this->epsilon = ep;
    this->mu = mu;
    this->inputGraph = inputGraph;
    this->bfsTreeObject = new bfsTree();
}

// constructor
iscan::iscan(float ep,int mu, graph* inputGraph, int number_of_threads)
{
    this->epsilon = ep;
    this->mu = mu;
    this->inputGraph = inputGraph;
    this->bfsTreeObject = new bfsTree();
    this->number_of_threads = number_of_threads;
}

// calculates similarity between two vertices
float iscan::getSimilarity(vertex* v1, vertex* v2)
{
    if(epsilon_values.find({v1, v2})!=epsilon_values.end())
    {
        return epsilon_values[{v1, v2}];
    }
    else return 0.0;

}

float iscan::calculateSimilarity(vertex* v1, vertex* v2)
{
    vector<vertex*>neighbour1 = inputGraph->graphObject[v1];
    vector<vertex*>neighbour2 = inputGraph->graphObject[v2];

    unordered_set<vertex*> s1(neighbour1.begin(), neighbour1.end());
    s1.insert(v1);

    neighbour2.push_back(v2);

    int count=0;
    for(auto it=neighbour2.begin();it!=neighbour2.end();it++)
    {
        if(s1.find(*it)!=s1.end())
        {
            count++;
        }
    }
    return ((float)count)/(sqrt(s1.size()*neighbour2.size()));
}


// calculates epsilon neighbourhood of a vertex
vector<vertex*> iscan::getEpsilonNeighbourhood(vertex* v)
{
    vector<vertex*>ret;
    ret.push_back(v);
    vector<vertex*>neighbour = inputGraph->graphObject[v];

    for (int i = 0; i < neighbour.size(); i++)
    {
        if(getSimilarity(v,neighbour[i])>=epsilon)
        {
            ret.push_back(neighbour[i]);
        }
    }
    return ret;
}

// tells whether a vertex is core or not
bool iscan::isCore(vertex* v)
{
    if(getEpsilonNeighbourhood(v).size()>=mu){
        return true;
    }
    else {
        return false;
    }
}



void iscan::calculateAllSimilaritySingleThreaded(){
    for(auto iter = inputGraph->graphObject.begin(); iter != inputGraph->graphObject.end(); iter++)
    {
        for(auto it = iter->second.begin(); it!=iter->second.end();it++)
        {
            float sim = calculateSimilarity(iter->first, *it);
            epsilon_values[{iter->first, *it}] = sim;

        }
    }
}

// Worker function to calculate similarities for all edges in thread
void worker_func(unordered_map<pair<vertex *, vertex *>, float, hash_pair>& epsilon_values, vector<pair<vertex *, vertex *>>& edges, unordered_map<vertex *, vector<vertex *>>& graphObject){
    for(auto iter = edges.begin(); iter != edges.end(); iter++){

        vertex* v1 = iter->first;
        vertex* v2 = iter->second;
        vector<vertex*>neighbour1 = graphObject[v1];
        vector<vertex*>neighbour2 = graphObject[v2];

        unordered_set<vertex*> s1(neighbour1.begin(), neighbour1.end());
        s1.insert(v1);

        neighbour2.push_back(v2);
        int count=0;
        for(auto it=neighbour2.begin();it!=neighbour2.end();it++)
        {
            if(s1.find(*it)!=s1.end())
            {
                count++;
            }
        }
        epsilon_values[*iter] = ((float)count)/(sqrt(s1.size()*neighbour2.size()));
    }


}

// Distribute edges between threads and assign worker function to each of them
void iscan::calculateAllSimilarityMultiThreaded(){
    vector<thread> threads;
    vector<vector<pair<vertex*, vertex*>>> edges_for_threads(number_of_threads);
    
    int number_of_edges = 0;

    for(auto iter = inputGraph->graphObject.begin(); iter != inputGraph->graphObject.end(); iter++)
    {
        for(auto it = iter->second.begin(); it!=iter->second.end();it++)
        {
            edges_for_threads[number_of_edges % number_of_threads].push_back({iter->first, *it});
            number_of_edges++;
        }
    }

    number_of_threads = min(number_of_edges, number_of_threads);   


    for(int i = 0; i < number_of_threads; i++){
        threads.push_back(thread(worker_func, std::ref(epsilon_values), std::ref(edges_for_threads[i]), std::ref(inputGraph->graphObject)));
    }

    // Wait for all the threads to finish their work
    for(int i = 0; i < number_of_threads; i++){
        threads[i].join();
    }


}

// creates clustering and classifies non member vertices as hubs or outliers
void iscan::executeSCAN(bool multithreading = false)
{
    epsilon_values.clear();

    for(auto iter = inputGraph->graphObject.begin(); iter != inputGraph->graphObject.end(); iter++)
    {
        for(auto it = iter->second.begin(); it!=iter->second.end();it++)
        {
            epsilon_values[{iter->first, *it}] = 0;

        }
    }
    // Compute initial similarities
    if (!multithreading){
        calculateAllSimilaritySingleThreaded();
    }
    else{

        calculateAllSimilarityMultiThreaded();
    }

    int cluster_id  = 0;
    vector<vertex*> sequence;
    for(auto iter = inputGraph->graphObject.begin(); iter != inputGraph->graphObject.end(); iter++){
        sequence.push_back(iter->first);
    } 

    // starts iteration on all vertices
    for(int start = 0; start < sequence.size(); start++){
        // if vertex already visited, continue
        if(sequence[start]->isClassified == 1){
            continue;
        }

        // if vertex is core, use BFS to generate cluster
        if(isCore(sequence[start]) == 1){
            vector<vertex*> cluster;
            cluster.push_back(sequence[start]);
            sequence[start]->memberType = CORE;
            sequence[start]->isClassified = 1;
            sequence[start]->clusterId = cluster_id;

            
            queue<vertex*> q;
            q.push(sequence[start]);

            while (q.size() > 0){
                vertex* temp_node = q.front();
                q.pop();
                vector<vertex*> R = getEpsilonNeighbourhood(temp_node);  // generate epsilon neighbourhood to push in the queue

                int flag = 1;

                for(int index = 0; index < R.size(); index++){
                    

                    vertex* neighbour = R[index];
                    if(neighbour->ID == temp_node->ID )
                    {
                        continue;
                    }
                    
                    // If node already a member of cluster, continue
                    if ((neighbour->isClassified == 1) && (neighbour->memberType != NON_MEMBER)){
                        // Add edge edge only if neighbour and temp_node have different ids
                        if(neighbour->ID != temp_node->ID)
                        {
                            // cout<<"Phi1:"<<neighbour->ID<<" "<<temp_node->ID<<endl;
                            if(bfsTreeObject->findInBfsSet(neighbour, temp_node) == 2)
                            {
                                bfsTreeObject->addEdgeToPhi(neighbour, temp_node);
                            }
                            
                        }
                        continue;
                    }

                    // If node not already classified then classify it as core or non-core member
                    if(neighbour->isClassified == 0){
                        // Core
                        if(isCore(neighbour) == true){
                            neighbour->memberType = CORE;
                            q.push(neighbour);
                            if(bfsTreeObject->findInPhi(temp_node, neighbour) == 2)
                            {
                                bfsTreeObject->addEdgeToBfsSet(temp_node, neighbour);
                            }
                            
                        }
                        // Non-core member
                        else{
                            neighbour->memberType = NON_CORE_MEMBER;
                            if(bfsTreeObject->findInPhi(temp_node, neighbour) == 2)
                            {
                                bfsTreeObject->addEdgeToBfsSet(temp_node, neighbour);
                            }
                            
                        }
                    }
                    // If already classified then Non-core member
                    else{

                        neighbour->memberType = NON_CORE_MEMBER;
                        if(bfsTreeObject->findInBfsSet(temp_node, neighbour) == 2)
                        {
                            bfsTreeObject->addEdgeToPhi(temp_node, neighbour);
                        }
                    }

                    flag = 0;
                    neighbour->clusterId = cluster_id;
                    neighbour->isClassified = 1;
                    cluster.push_back(neighbour);

                }
            }

            inputGraph->clusters[cluster_id] = cluster;
            cluster_id++;  

        }
        else{  
            // label the vertex as a non_member
            sequence[start]->isClassified = 1;
            sequence[start]->memberType = NON_MEMBER;
        }

    }

    // for each non_member vertex check whether its a hub or an outlier
    for(int start = 0; start < sequence.size(); start++){
        if(sequence[start]->memberType == NON_MEMBER){ 
            vector<vertex*> neighbours = inputGraph->graphObject[sequence[start]];
            unordered_set<int> cluster_ids;
            
            for(int i = 0; i < neighbours.size(); i++){
                if (neighbours[i]->memberType != NON_MEMBER)
                    cluster_ids.insert(neighbours[i]->clusterId);
            }

            if (cluster_ids.size() >=  2){
                sequence[start]->hub_or_outlier = HUB;
                inputGraph->hubs.push_back(sequence[start]);
            }
            else{
                sequence[start]->hub_or_outlier = OUTLIER;
                inputGraph->outliers.push_back(sequence[start]);
            } 

        }
    }

}

// Output formed cluster to intermediate file
void iscan::printClusterToFile(vector<vertex*> cluster, ofstream &outputFile, int cluster_id)
{
    outputFile<<"Finalised a Cluster with ID "<<cluster_id<<" : ";
    for(int i=0;i<cluster.size();i++)
    {
        outputFile<<cluster[i]->ID<<" ";
    }
    outputFile<<endl<<"--------------------------------------------------"<<endl;
    outputFile<<endl;
}

// Output epsilon neighbourhood of all vertices in intermediate file
void iscan::printEpsilonNeighbours(ofstream &outputFile)
{
    outputFile<<"-------------------------Epsilon neighbourboods-------------------------"<<endl;
    outputFile<<"VERTEX ID: EPSILON NEIGHBOURS"<<endl;
    unordered_map<vertex*,vector<vertex*>> go = inputGraph->graphObject;
    vector<vertex*> vertices;
    for(auto it=go.begin(); it!=go.end();it++)
    {
        vertices.push_back(it->first);
    }   

    sort(vertices.begin(), vertices.end(), comp);
    for(int i=0;i<vertices.size();i++)
    {
        outputFile<<vertices[i]->ID<<": ";
        vector<vertex*> temp = getEpsilonNeighbourhood(vertices[i]);
        for(int i=0;i<temp.size();i++)
        {
            outputFile<<temp[i]->ID<<" ";
        }
        outputFile<<endl;
    }

    outputFile<<endl<<"-------------------------SCAN EXECUTION starts-------------------------"<<endl;

}

// Returns the union of neighbours of v1 and v2
unordered_set<vertex*> iscan::getNuv(int Id1, int Id2){

    vertex* v1 = inputGraph->vertexMap[Id1];
    vertex* v2 = inputGraph->vertexMap[Id2];

    vector<vertex*> neighbour1 = inputGraph->graphObject[v1];
    vector<vertex*> neighbour2 = inputGraph->graphObject[v2];
    
    unordered_set<vertex*> s1(neighbour1.begin(), neighbour1.end()); 
    s1.insert(v1);

    neighbour2.push_back(v2);
    for(auto it=neighbour2.begin();it!=neighbour2.end();it++)
    {
        s1.insert(*it);
    }

    return s1;
}

// Returns edges with one vertex being v1/v2 and other in Nuv
unordered_set<pair<vertex*,vertex*>,hash_pair> iscan::getRuv(int Id1, int Id2, unordered_set<vertex*> Nuv){

    vertex* v1 = inputGraph->vertexMap[Id1];
    vertex* v2 = inputGraph->vertexMap[Id2];

    vector<vertex*> neighbour1 = inputGraph->graphObject[v1];
    vector<vertex*> neighbour2 = inputGraph->graphObject[v2];

    unordered_set<pair<vertex*,vertex*>,hash_pair> ret;

    for(auto it=neighbour1.begin();it!=neighbour1.end();it++)
    {
        if(Nuv.find(*it)!=Nuv.end())
        {
            ret.insert({v1,*it});
        }
    }

    for(auto it=neighbour2.begin();it!=neighbour2.end();it++)
    {
        if(Nuv.find(*it)!=Nuv.end())
        {
            ret.insert({v2,*it});
        }
    }

    // These are inserted twice
    ret.erase({v1, v2});
    ret.erase({v2, v1});

    ret.insert({v1,v2});

    return ret;
}

void iscan::updateRuvSimilaritySingleThreaded(unordered_set<pair<vertex*,vertex*>,hash_pair> edges)
{

    for( auto i : edges)
    {
        epsilon_values[i] = calculateSimilarity(i.first,i.second); 
        epsilon_values[{i.second, i.first}] = epsilon_values[i];
    }
}

void iscan::updateRuvSimilarityMultiThreaded(unordered_set<pair<vertex*,vertex*>,hash_pair> edges)
{
    vector<thread> threads;
    vector<vector<pair<vertex*, vertex*>>> edges_for_threads(number_of_threads);

    int number_of_edges = 0;

    for( auto i : edges)
    {
        edges_for_threads[number_of_edges % number_of_threads].push_back({i.first, i.second});
        number_of_edges++;
        edges_for_threads[number_of_edges % number_of_threads].push_back({i.second, i.first});
        number_of_edges++;
    }

    number_of_threads = min(number_of_edges, number_of_threads);

    for(int i = 0; i < number_of_threads; i++){
        threads.push_back(thread(worker_func, std::ref(epsilon_values), std::ref(edges_for_threads[i]), std::ref(inputGraph->graphObject)));
    }

    for(int i = 0; i < number_of_threads; i++){
        threads[i].join();
    }


}

// Main incremental algorithm
void iscan::updateEdge(int id1, int id2, bool isAdded, bool multithreading = false){

    unordered_set<vertex*> Nuv = getNuv(id1,id2);

    map<pair<vertex*,vertex*>,float> sigmaOld;
    
    // Store cores uptil now
    unordered_set<vertex*> old_cores;

    for(auto it: Nuv){
        
        if (isCore(it)){
            old_cores.insert(it);
        }
    }

    // Store current similarities of edges in Ruv
    unordered_set<pair<vertex*,vertex*>,hash_pair> Ruv = getRuv(id1, id2, Nuv);
    for(auto it: Ruv)
    {
        sigmaOld[it] = getSimilarity(it.first,it.second);
    }
    
    // If adding edge to graph
    if(isAdded)
    {
        inputGraph->addEdge(id1,id2);
        epsilon_values[{inputGraph->vertexMap[id2], inputGraph->vertexMap[id1]}] = 0;
        epsilon_values[{inputGraph->vertexMap[id1], inputGraph->vertexMap[id2]}] = 0;
    }
    // Removing edge from graph
    else
    {
        inputGraph->removeEdge(id1,id2);
    }

    if(multithreading)
        updateRuvSimilarityMultiThreaded(Ruv);
    else
        updateRuvSimilaritySingleThreaded(Ruv);

    if(!isAdded)
    {
        epsilon_values.erase({inputGraph->vertexMap[id1], inputGraph->vertexMap[id2]});
        epsilon_values.erase({inputGraph->vertexMap[id2], inputGraph->vertexMap[id1]});
    }

    // Call mergeCluster for all cores in Nuv
    for(auto it:Nuv)
    {
        if(isCore(it))
        {
            mergeCluster(it);
        }
    }

    // Call splitCluster if similarity value goes below epsilon and was greater earlier
    for(auto it:Ruv)
    {
        if(sigmaOld[it]>= epsilon && getSimilarity(it.first,it.second)<epsilon)
            splitCluster(it.first,it.second, old_cores);
    }


    // Removed Cluster Ids of all vertices
    for(auto iter = inputGraph->graphObject.begin(); iter != inputGraph->graphObject.end(); iter++){
        iter->first->clusterId = -1;
        iter->first->memberType = 1;
    }

    // Run BFS across bfs tree to reassign Cluster Ids
    int tempId = 0;
    for(auto iter = inputGraph->graphObject.begin(); iter != inputGraph->graphObject.end(); iter++){
        if(iter->first->clusterId == -1 && (iter->first->parent != NULL || !(iter->first->children.empty()))){
            bfsTreeObject->recurseParent(iter->first, iter->first->parent, tempId);
            bfsTreeObject->recurseChildren(iter->first, tempId);
            tempId++;
        }
    }

    // Change member types for all vertices which are part of clusters
    for(auto iter = inputGraph->graphObject.begin(); iter != inputGraph->graphObject.end(); iter++){
        if(iter->first->clusterId != -1)
        {
            if(isCore(iter->first))
            {
                iter->first->memberType = 0;
            }
            else iter->first->memberType = 2;
        }
    }

    // Form clusters using assigned clusterids
    inputGraph->clusters.clear();
    inputGraph->clusters[-1] = vector<vertex*>();
    for(auto iter = inputGraph->graphObject.begin(); iter != inputGraph->graphObject.end(); iter++)
    {
        if(iter->first->clusterId != -1)inputGraph->clusters[iter->first->clusterId].push_back(iter->first);
    }

    vector<pair<vertex*, vertex*>> temp;

    for(auto it:bfsTreeObject->phi)
    {
        temp.push_back({it.first, it.second});
    }
    for(auto it:temp)
    {
        if((it.first)->clusterId != (it.second)->clusterId)
        {
            if(isCore(it.first) && isCore(it.second)){
                if(inputGraph->clusters[(it.first)->clusterId].size() < inputGraph->clusters[(it.second)->clusterId].size())
                {
                    bfsTreeObject->merge(it.first,it.second);
                }
                else
                {
                    bfsTreeObject->merge(it.second,it.first);
                }
                it.first->memberType = 0;
                it.second->memberType = 0;
            }
            else if(isCore(it.first) && it.second->memberType ==1)
            {
                it.first->memberType = 0;
                it.second->clusterId = it.first->clusterId;
                it.second->memberType = 2;
                bfsTreeObject->addEdgeToBfsSet(it.first,it.second);
                bfsTreeObject->removeEdgeFromPhi(it.first,it.second);
            }
            else if(isCore(it.second) && it.first->memberType ==1)
            {
                it.second->memberType = 0;
                it.first->clusterId = it.second->clusterId;
                it.first->memberType = 2;
                bfsTreeObject->addEdgeToBfsSet(it.second,it.first);
                bfsTreeObject->removeEdgeFromPhi(it.first,it.second);
            }
        }
    }
    temp.clear();
    for(auto it:bfsTreeObject->phi)
    {
        temp.emplace_back(it.first, it.second);
    }
    for(auto it:temp)
    {
        if((it.first)->clusterId == (it.second)->clusterId && (it.first)->clusterId == -1)
        {
            if(isCore(it.first) && isCore(it.second))
            {
                (it.first)->clusterId = tempId;
                tempId++;
                bfsTreeObject->merge(it.first, it.second);
                it.first->memberType = 0;
                it.second->memberType = 0;
            }
            else if(isCore(it.first) && it.second->memberType ==1)
            {
                it.first->memberType = 0;
                it.second->clusterId = tempId;
                tempId++;
                it.second->memberType = 2;
                bfsTreeObject->addEdgeToBfsSet(it.first,it.second);
                bfsTreeObject->removeEdgeFromPhi(it.first,it.second);

            }
            else if(isCore(it.second) && it.first->memberType ==1)
            {
                it.second->memberType = 0;
                it.first->clusterId = tempId;
                tempId++;
                it.first->memberType = 2;
                bfsTreeObject->addEdgeToBfsSet(it.second,it.first);
                bfsTreeObject->removeEdgeFromPhi(it.first,it.second);
            }
        }
        
    }
    

    inputGraph->hubs.clear();
    inputGraph->outliers.clear();
    
    // for each non_member vertex check whether its a hub or an outlier
    for(auto it : inputGraph->vertexMap){
        if(it.second->memberType == NON_MEMBER){ 
            vector<vertex*> neighbours = inputGraph->graphObject[it.second];
            unordered_set<int> cluster_ids;
            
            for(int i = 0; i < neighbours.size(); i++){
                if (neighbours[i]->memberType != NON_MEMBER)
                    cluster_ids.insert(neighbours[i]->clusterId);
            }

            if (cluster_ids.size() >=  2){
                it.second->hub_or_outlier = HUB;
                inputGraph->hubs.push_back(it.second);
            }
            else{
                it.second->hub_or_outlier = OUTLIER;
                inputGraph->outliers.push_back(it.second);
            } 

        }
    }

    inputGraph->clusters.clear();
    for(auto iter = inputGraph->graphObject.begin(); iter != inputGraph->graphObject.end(); iter++)
    {
        if(iter->first->clusterId != -1)inputGraph->clusters[iter->first->clusterId].push_back(iter->first);
    }

}

// MergeCluster algorithm as described in report
void iscan::mergeCluster(vertex* w){

    if (w->memberType == 1|| w->memberType == -1){
        int newClusterID= 0;
        if(!inputGraph->clusters.empty()){
            newClusterID = (1+inputGraph->clusters.rbegin()->first);
        }
        w->clusterId = newClusterID;
        w->memberType=0;
        inputGraph->clusters[newClusterID] = {w}; 
    }
    for(auto u: getEpsilonNeighbourhood(w)){
        
        if(u->ID == w->ID) 
        {
            continue;
        }
        if(u->memberType != 1 && u->memberType!= -1){
            if(u->memberType == 2){
                if(((u->clusterId == w->clusterId) && (u->parent != w)) || (u->clusterId != w->clusterId)){
                    bfsTreeObject->addEdgeToPhi(u, w);
                }
            }
            else{
                if((u->clusterId == w->clusterId) && (u->parent != w) && (w->parent != u)){
                    bfsTreeObject->addEdgeToPhi(u, w);
                }
                if(u->clusterId != w->clusterId){

                    if(inputGraph->clusters[u->clusterId].size() < inputGraph->clusters[w->clusterId].size()){

                        bfsTreeObject->merge(u, w);
                    }
                    else{
                        bfsTreeObject->merge(w, u);
                    }
                }                    
            }
        }
        else{

            if(isCore(u))
            {
                u->memberType = 0;
            }
            else 
            {
                u->memberType = 2;
            }
            u->hub_or_outlier = -1;
            u->clusterId = w->clusterId;
            bfsTreeObject->addEdgeToBfsSet(w, u);
        }
    }


}

// SplitCluster algorithm as described in report
void iscan::splitCluster(vertex* u, vertex* v, unordered_set<vertex*>& old_cores){

    if((u->memberType == 0) && (v->memberType == 0)){
        if((u->parent != v) && (v->parent != u)){
            bfsTreeObject->removeEdgeFromPhi(u, v);
        }
        else{
            bfsTreeObject->removeEdgeFromBfsSet(u, v);
        }
    }

    if((u->memberType == 0) && (v->memberType != 0)){
        if (v->parent == u){
            bfsTreeObject->removeEdgeFromBfsSet(u, v);
        }
        else{
            bfsTreeObject->removeEdgeFromPhi(u, v);
        }
    }


    if((v->memberType == 0) && (u->memberType != 0)){
        if (u->parent == v){
            bfsTreeObject->removeEdgeFromBfsSet(u, v);
        }
        else{
            bfsTreeObject->removeEdgeFromPhi(u, v);
        }
    }

    if((u->memberType != 0) && (v->memberType != 0)){
        if(old_cores.find(u) != old_cores.end()){
            if (v->parent == u){
                bfsTreeObject->removeEdgeFromBfsSet(u, v);
            }
            else{
                bfsTreeObject->removeEdgeFromPhi(u, v);
            }
        }

        if(old_cores.find(v) != old_cores.end()){
            if (u->parent == v){
                bfsTreeObject->removeEdgeFromBfsSet(u, v);
            }
            else{
                bfsTreeObject->removeEdgeFromPhi(u, v);
            }
        }

    }



}

// Util function to print a vector
void iscan::printVector(vector<vertex*> n)
{
    for(int i=0;i<n.size();i++)
    {
        cout<<n[i]->ID<<" ";
    }
    cout<<endl;
}

bool iscan::checkCycle()
{
    bool val = false;
    bool curnoparent= false;
    for(auto it:inputGraph->clusters)
    {
        curnoparent= false;
        for(auto nodes :it.second)
        {
            if(nodes->parent==nullptr)
            {
                curnoparent=true;
            }

        }
        if(!curnoparent) {
            val = true;
        }
    }
    for(auto node:inputGraph->graphObject)
    {
        int numberchild= 0;
        for(auto parents:inputGraph->graphObject )
        {
            for(auto childerns:parents.first->children)
            {
                if(node.first == childerns)
                {
                    numberchild++;
                }

            }

        }
        if(numberchild >1)
        {
            val =true;
        }


    }
    assert(!val);
    return val;

}
