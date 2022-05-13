#include<bits/stdc++.h>
#include<unistd.h>

using namespace std;
#define NODES 10

int nodeId, destination, delay=-1, sequenceNo = 0;
string message, prevHello[NODES];
streampos position = 0;
int lastHello[NODES] = {-1}, lastTC[NODES], recentSeq[NODES];

set<int> mprList, msList, tcTable[NODES], oneHopList, twoHopList;

enum LinkType { NA, UNIDIR, BIDIR, MPR };
LinkType nbrTable[NODES];

set<int> lastHop[NODES], twoHopnbr[NODES], fromOneHop[NODES];

typedef struct routingTableNode 
{
    int distance, nextHop;
}routingTableNode;

map<int, routingTableNode> routingTable;

string getFileName(string prefix)
{
    string fileName;
    if(prefix == "received")
        fileName = to_string(nodeId) + prefix + ".txt";
    else
        fileName = prefix + to_string(nodeId) + ".txt";

    return fileName; 
}

void appendMessage(string msg, string file)
{
    string fileName = getFileName(file);

    ofstream fp;
    fp.open(fileName.c_str(), ios::app);

    if(!fp.is_open())
    {
        cout<<"\nCould not open file"<<endl;
        return;
    }
    fp << msg << endl;

    fp.close();
}

void calculateMpr() 
{
    mprList.clear();
    for(int i=0; i<NODES; ++i)
    {
        if(nbrTable[i] == MPR)
            nbrTable[i] = BIDIR;
    }

    set<int> remTwoHop(twoHopList);
    int cnt[NODES], choose;

    while(!remTwoHop.empty())
    {
        for(int i=0; i<NODES; ++i)
            cnt[i] = 0;
        choose = -1;

        for(auto &itr : remTwoHop)
        {
            for(auto &itr2 : twoHopnbr[itr])
            {
                if(mprList.find(itr2) != mprList.end()) 
                    continue;

                cnt[itr2]++;
                
                if( (choose == -1) || (cnt[itr2] > cnt[choose]) )
                    choose = itr2;
            }
        }

        mprList.insert(choose);
        nbrTable[choose] = MPR;

        for(auto &itr : fromOneHop[choose])
        {
            remTwoHop.erase(itr);
        }
    }
}

void calculateRT()
{
    routingTable.clear();

    for(auto &itr : oneHopList)
    {
        routingTableNode node;
        node.nextHop = itr;
        node.distance = 1;
        routingTable.insert({itr, node});
    }

    for(auto &itr : twoHopList)
    {
        routingTableNode node;
        auto nHop = twoHopnbr[itr].begin();
        node.nextHop = *nHop;
        node.distance = 2;
        routingTable.insert( {itr, node} );
    }

    bool check = true;
    int hops = 2;
    while(check)
    {
        check = false;

        for(int i=0; i<NODES; ++i)
        {  
            for(auto &it : tcTable[i]) {
                
                if(i == nodeId || routingTable.find(i) != routingTable.end())
                    break;
                
                auto ret = routingTable.find(it);
                if(ret != routingTable.end() && (ret->second).distance == hops)
                {
                    routingTableNode node;
                    node.nextHop = (ret->second).nextHop;
                    node.distance = hops+1;
                    routingTable.insert({i, node});
                    
                    check = true;
                    break;
                }
            }
        }

        hops++;
    }
}

string createHello() 
{
    string genHelloMsg = "";
    genHelloMsg += ( "* " + to_string(nodeId) + " HELLO " );

    genHelloMsg += "UNIDIR ";
    for(int i=0; i<NODES; ++i) {
        if(nbrTable[i] == UNIDIR)
            genHelloMsg += to_string(i) + " ";
    }

    genHelloMsg += "BIDIR ";
    for(int i=0; i<NODES; ++i)
    {
        if(nbrTable[i] == BIDIR)
            genHelloMsg += to_string(i) + " ";
    }

    genHelloMsg += "MPR ";
    for(int i=0; i<NODES; ++i) {
        if(nbrTable[i] == MPR)
            genHelloMsg += to_string(i) + " ";
    }

    return genHelloMsg;
}

string generateData(int hop) 
{
    string genDataMsg = "";
    genDataMsg += (to_string(hop) + " " + to_string(nodeId) + " DATA " + to_string(nodeId) + " " + to_string(destination) + " " + message);
    return genDataMsg;
}

void sendDataFurther(string msg, string nextHop, string fromNeighbor, int hop) 
{
    msg.replace(nextHop.size()+1, fromNeighbor.size(), to_string(nodeId));
    msg.replace(0, nextHop.size(), to_string(hop));
    appendMessage(msg, "from");
}

bool processHello(string msg, int time) 
{
    stringstream s(msg);
    string ignore, linkType;
    int fromNeighbor, neighbor;
    vector<int> unidirFromNeigh, bidirFromNeigh, mprFromNeigh;
    bool unidirFromNeighId = false, bidirFromNeighId = false, mprFromNeighId = false;

    s >> ignore >> fromNeighbor >> ignore;
    lastHello[fromNeighbor] = time;

    if(msg == prevHello[fromNeighbor]) 
        return false;
    prevHello[fromNeighbor] = msg;

    while(s >> neighbor || !s.eof()) 
    {
        if(s.fail()) 
        {
            s.clear();
            s >> linkType;
            continue;
        }
        if(linkType == "UNIDIR") 
        {
            if(neighbor == nodeId) 
                unidirFromNeighId = true;
            else    
                unidirFromNeigh.push_back(neighbor);
        }
        else if(linkType ==  "BIDIR") 
        {
            if(neighbor == nodeId)  
                bidirFromNeighId = true;
            else    
                bidirFromNeigh.push_back(neighbor);
        }
        else if(linkType ==  "MPR") 
        {
            if(neighbor == nodeId)  
                mprFromNeighId = true;
            else 
                mprFromNeigh.push_back(neighbor);
        }
    }

    if(unidirFromNeighId || bidirFromNeighId || mprFromNeighId) 
    {
        nbrTable[fromNeighbor] = BIDIR;
        oneHopList.insert(fromNeighbor);
     
        for(auto &itr : fromOneHop[fromNeighbor]) 
        {
            twoHopnbr[itr].erase(fromNeighbor);
            if(twoHopnbr[itr].empty())
                twoHopList.erase(itr);
        }
        fromOneHop[fromNeighbor].clear();
        for(auto &itr : bidirFromNeigh) 
        {
            fromOneHop[fromNeighbor].insert(itr);
            twoHopnbr[itr].insert(fromNeighbor);
        }
        for(auto &itr : mprFromNeigh) 
        {
            fromOneHop[fromNeighbor].insert(itr);
            twoHopnbr[itr].insert(fromNeighbor);
        }

        twoHopList.clear();
        for(int i=0; i<NODES; ++i) 
        {
            if(!twoHopnbr[i].empty()) 
               twoHopList.insert(i);
        }
                
        if(msList.find(fromNeighbor) != msList.end() && !mprFromNeighId) 
            msList.erase(fromNeighbor);
        else if(mprFromNeighId) 
            msList.insert(fromNeighbor);
    }
    else 
        nbrTable[fromNeighbor] = UNIDIR;

    return true;
}

bool processTC(string msg, int time) 
{
    stringstream s(msg);
    string ignore;
    int fromNeighbor, sourceNode, seqNumber, msNode;
    s >> ignore >> fromNeighbor >> ignore >> sourceNode >> seqNumber >> ignore;
    lastTC[sourceNode] = time;

    if(sourceNode == nodeId or seqNumber <= recentSeq[sourceNode]) 
        return false;
    
    recentSeq[sourceNode] = seqNumber;

    if(msList.find(fromNeighbor) != msList.end())
    {
        string temp = msg;
        temp.replace(2, to_string(fromNeighbor).size(), to_string(nodeId));
        appendMessage(temp, "from");
    }

    for(auto &itr : lastHop[sourceNode]) 
    {
        tcTable[itr].erase(sourceNode);
    }
    lastHop[sourceNode].clear();

    while(s >> msNode) 
    {
        lastHop[sourceNode].insert(msNode);
        tcTable[msNode].insert(sourceNode);
    }
    return true;
}

void processData(string msg) 
{
    stringstream s(msg);
    int nextHop, fromNeighbor, sourceNode, dstNode;
    string ignore, temp;

    s >> nextHop >> fromNeighbor >> ignore >> sourceNode >> dstNode;

    temp = msg;
    temp.replace(0, (to_string(nextHop)).size() + (to_string(fromNeighbor)).size() + ignore.size() + (to_string(sourceNode)).size() + (to_string(dstNode)).size() + 5, "" );

    if(dstNode == nodeId)
    {
        appendMessage(temp, "received");
        return;
    }

    auto node = routingTable.find(dstNode);
    
    if(node != routingTable.end())
    {
        sendDataFurther(msg, to_string(nextHop), to_string(fromNeighbor), (node->second).nextHop);
    }
}

void removeNbr(int n) 
{
    oneHopList.erase(n);
    nbrTable[n] = NA;
    msList.erase(n);

    for(auto &itr : fromOneHop[n])
    {
        twoHopnbr[itr].erase(n);
        if(twoHopnbr[itr].empty())  
            twoHopList.erase(itr);
    }
    fromOneHop[n].clear();

    if(!twoHopnbr[n].empty()) 
       twoHopList.insert(n);
}

void RemoveTcInfoFrom(int i)
{

    for (auto &itr : lastHop[i]) 
    {
        tcTable[itr].erase(i);
    }
    lastHop[i].clear();

}

bool processToFile(int time) 
{
    bool neighborhoodChanged = false, tcChanged = false;

    string filename = getFileName("to");
    ifstream inputfile;
    inputfile.open(filename.c_str(), ios::in);
    if(!inputfile.is_open()) 
        return false;

    streampos lastByteRead = 0;
    inputfile.seekg(position);
    string line;
    
    while(getline(inputfile, line))
    {

        lastByteRead = inputfile.tellg();

        stringstream s(line);
        string useless, type;
        int fromNeighbor;
        s >> useless >> fromNeighbor >> type;
        
        if(type == "HELLO") 
        {
            neighborhoodChanged |= processHello(line, time);
        }        
        else if(type == "TC") 
        {
            tcChanged |= processTC(line, time);
        }
        else if(type == "DATA") 
        {
            processData(line);
        }
            
    }
        
    position = max(lastByteRead, position);
    inputfile.close();

    for(int i=0; i<NODES; ++i) 
    {
        if(oneHopList.find(i) !=oneHopList.end() && (time - lastHello[i]) >= 15) 
        {
            removeNbr(i);
            neighborhoodChanged |= true;
        }
    }

    if(neighborhoodChanged) 
        calculateMpr();

    return (neighborhoodChanged or tcChanged);
}

int main(int argc, char **argv)
{
    for(int i=1; i<argc; ++i)
    {
        stringstream s(argv[i]);
        
        if(i == 1)
            s >> nodeId;
        else if(i == 2)
            s >> destination;
        else if(i == 3)
            message = s.str();
        else if(i == 4)
            s >> delay;
    }
    
    bool dataSent = false, changeRT = false;
    int virtualTime = 0;

    while(virtualTime < 120) 
    {
        changeRT = false;
        changeRT = changeRT or processToFile(virtualTime);

        for(int i=0; i<NODES; ++i) 
        {
            if(virtualTime - lastTC[i] >= 30 && !lastHop[i].empty()) 
            {
                RemoveTcInfoFrom(i);
                changeRT = changeRT or true;
            }
        }

        if(changeRT) 
            calculateRT();

        if(nodeId != destination and !dataSent and virtualTime == delay) 
        {
            auto itr = routingTable.find(destination);

            if(itr != routingTable.end())
            {
                string temp = generateData((itr->second).nextHop);
                appendMessage(temp, "from");
                dataSent = true;
            }
            else
                delay += 30;
        }
        
        if(virtualTime % 5 == 0)
        {
            string s = createHello();
            appendMessage(s, "from");
        }

        if(virtualTime % 10 == 0 && !msList.empty())
        {
            sequenceNo++;

            string s = "";
            s += ( "* " + to_string(nodeId) + " TC " + to_string(nodeId) + " " + to_string(sequenceNo) + " MS ");

            for(auto &itr : msList)
            {
                s += ( to_string(itr) + " " );
            }

            appendMessage(s, "from");
        }

        sleep(1);
        virtualTime++;
    }
}