#include<bits/stdc++.h>
#include<unistd.h>

using namespace std;
#define NODES 10

int nodeId, destination, delay=-1;
string message;

int sequenceNo = 0;
streampos position = 0;

int lastHello[NODES] = {-1};
string prevHello[NODES];
int lastTC[NODES];
int recentSeq[NODES];

set<int> mprList, msList;
set<int> tcTable[NODES];

set<int> oneHopList, twoHopList;

enum LinkType { NA, UNIDIR, BIDIR, MPR };
LinkType nbrTable[NODES];

set<int> lastHop[NODES], twoHopnbr[NODES], fromOneHop[NODES];

typedef struct routingTableNode {
    int distance, next_hop;
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

//done refactoring
void CalculateMprSet() {

    mprList.clear();
    for (int i=0; i<NODES; ++i)
    {
        if (nbrTable[i] == MPR)
            nbrTable[i] = BIDIR;
    }

    set<int> remTwoHop(twoHopList);
    int cnt[NODES], choose;

    while (!remTwoHop.empty()) {        
        for (int i=0; i<NODES; ++i) cnt[i] = 0;
        choose = -1;

        for (auto &itr : remTwoHop)
        {
            for (auto &itr2 : twoHopnbr[itr])
            {
                
                if (mprList.find(itr2) != mprList.end()) continue;

                ++cnt[itr2];
                
                if ( (choose == -1) || (cnt[itr2] > cnt[choose]) )
                    choose = itr2;
            }
        }

        mprList.insert(choose);
        nbrTable[choose] = MPR;

        for (auto &itr : fromOneHop[choose])
        {
            remTwoHop.erase(itr);
        }
    }
}


//done refactoring
void calculateRT() {

    routingTable.clear();

    for (auto &itr : oneHopList)
    {
        routingTableNode node;
        node.next_hop = itr;
        node.distance = 1;
        routingTable.insert({itr, node});
    }

    for (auto &itr : twoHopList)
    {
        routingTableNode node;
        auto nHop = twoHopnbr[itr].begin();
        node.next_hop = *nHop;
        node.distance = 2;
        routingTable.insert( {itr, node} );
    }

    bool check = true;
    int hops = 2;
    while (check)
    {
        check = false;

        for (int i=0; i<NODES; ++i)
        {  
            for (auto &it : tcTable[i]) {
                
                if (i == nodeId || routingTable.find(i) != routingTable.end())
                    break;
                
                auto ret = routingTable.find(it);
                if( ret != routingTable.end() && (ret->second).distance == hops) {
                    routingTableNode node;
                    node.next_hop = (ret->second).next_hop;
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

string GenerateHelloMsg() {
    string hello_str = "";

    hello_str += ( "* " + to_string(nodeId) + " HELLO " );

    hello_str += "UNIDIR ";
    for (int i=0; i<NODES; ++i) {
        if (nbrTable[i] == UNIDIR)
            hello_str += to_string(i) + " ";
    }

    hello_str += "BIDIR ";
    for (int i=0; i<NODES; ++i) {
        if (nbrTable[i] == BIDIR)
            hello_str += to_string(i) + " ";
    }

    hello_str += "MPR ";
    for (int i=0; i<NODES; ++i) {
        if (nbrTable[i] == MPR)
            hello_str += to_string(i) + " ";
    }

    return hello_str;
}

string GenerateTcMsg() {
    ++sequenceNo;
    
    string tc_str = "";
    tc_str += ( "* " + to_string(nodeId) + " TC " + to_string(nodeId) + " " + to_string(sequenceNo) + " MS ");

    for (set<int>::iterator it = msList.begin(); it != msList.end(); ++it) {
        tc_str += ( to_string(*it) + " " );
    }

    return tc_str;
}

string GenerateDataMsg(int next_hop) {
    string data_str = "";
    data_str += (to_string(next_hop) + " " + to_string(nodeId) + " DATA " + to_string(nodeId) + " " + to_string(destination) + " " + message);
    return data_str;
}

void SendHelloMsg() {
    string hello_msg = GenerateHelloMsg();
    appendMessage(hello_msg, "from");
}

void SendTcMsg() {
    string tc_msg = GenerateTcMsg();
    appendMessage(tc_msg, "from");
}

void SendDataMsg(int next_hop) {
    string data_msg = GenerateDataMsg(next_hop);

    appendMessage(data_msg, "from");
}

void ForwardTcMsg(string msg, string fromnbr) {
    msg.replace(2, fromnbr.size(), to_string(nodeId));

    appendMessage(msg, "from");
}

void ForwardDataMsg(string msg, string nxthop, string fromnbr, int next_hop) {
    msg.replace(nxthop.size()+1, fromnbr.size(), to_string(nodeId));
    msg.replace(0, nxthop.size(), to_string(next_hop));

    appendMessage(msg, "from");
}

bool HandleHelloMsg(string msg, int time) {
    stringstream str_stream(msg);
    string dumb, link_type;
    int fromnbr, nbr;
    
    // Use vector to record the neighbor information of node fromnbr.
    vector<int> unidir_of_fromnbr;
    vector<int> bidir_of_fromnbr;
    vector<int> mpr_of_fromnbr;

    // Use these flags to represent the neighborship between node id and fromnbr. 
    bool id_is_unidir_of_fromnbr = false;
    bool id_is_bidir_of_fromnbr = false;
    bool id_is_mpr_of_fromnbr = false;

    // Read HELLO message: * <fromnbr> HELLO.
    str_stream >> dumb >> fromnbr >> dumb;
    
    lastHello[fromnbr] = time;

    // Compare previous HELLO message with this one.
    // If they are the same, nothing changes, no need to handle. (Since we don't have expire time.)
    if (msg == prevHello[fromnbr]) return false;
    prevHello[fromnbr] = msg;

    // Read and process remaining HELLO message content.
    while (str_stream >> nbr || !str_stream.eof()) {
        // Identify neighbor link types: UNIDIR, BIDIR, or MPR.
        if (str_stream.fail()) {
            str_stream.clear();
            str_stream >> link_type;
            continue;
        }

        // Put neighbor information of node fromnbr into seperate vectors.
        // Note that do not put id itself into any vector.
        if (link_type == "UNIDIR") {
            if (nbr == nodeId)  id_is_unidir_of_fromnbr = true;
            else    unidir_of_fromnbr.push_back(nbr);
        }
        else if (link_type ==  "BIDIR") {
            if (nbr == nodeId)  id_is_bidir_of_fromnbr = true;
            else    bidir_of_fromnbr.push_back(nbr);
        }
        else if (link_type ==  "MPR") {
            if (nbr == nodeId)  id_is_mpr_of_fromnbr = true;
            else mpr_of_fromnbr.push_back(nbr);
        }
    }

    // Based on the neighbor information of fromnbr, update neighbor table of node id itself.
    if (id_is_unidir_of_fromnbr || id_is_bidir_of_fromnbr || id_is_mpr_of_fromnbr) {
        // Update one-hop table and set.
        nbrTable[fromnbr] = BIDIR;
       oneHopList.insert(fromnbr);

        // Clear two-hop information based on old HELLO of fromnbr.
        for (set<int>::iterator it = fromOneHop[fromnbr].begin(); it != fromOneHop[fromnbr].end(); ++it) {
            twoHopnbr[*it].erase(fromnbr);
            if (twoHopnbr[*it].empty())twoHopList.erase(*it);
        }
        fromOneHop[fromnbr].clear();

        // Update two-hop tables.
        for (vector<int>::iterator it = bidir_of_fromnbr.begin(); it != bidir_of_fromnbr.end(); ++it) {
            fromOneHop[fromnbr].insert(*it);
            twoHopnbr[*it].insert(fromnbr);
        }
        for (vector<int>::iterator it = mpr_of_fromnbr.begin(); it != mpr_of_fromnbr.end(); ++it) {
            fromOneHop[fromnbr].insert(*it);
            twoHopnbr[*it].insert(fromnbr);
        }

        // Update two-hop set.
       twoHopList.clear();
        for (int i=0; i<NODES; ++i) {
            if (!twoHopnbr[i].empty()) {
               twoHopList.insert(i);
            }
        }
                
        // Update MS set.
        if (msList.find(fromnbr) != msList.end() && !id_is_mpr_of_fromnbr) msList.erase(fromnbr);
        else if (id_is_mpr_of_fromnbr) msList.insert(fromnbr);
    }
    else {
        nbrTable[fromnbr] = UNIDIR;
    }

    return true;
}

// Handle incoming TC message: 
//  1. Forward this TC message if necessary.
//  2. Update TC table if necessary.
// Return true if TC table is changed because of this TC message;
// otherwise, return false.
bool HandleTcMsg(string msg, int time) {
    stringstream str_stream(msg);
    string dumb;
    int fromnbr, srcnode, seqno, msnode;

    str_stream >> dumb >> fromnbr >> dumb >> srcnode >> seqno >> dumb;

    lastTC[srcnode] = time;

    // No need to process TC generating from itself and TC with old information.
    if(srcnode == nodeId || seqno <= recentSeq[srcnode]) return false;
    
    recentSeq[srcnode] = seqno;

    // Forward this TC message if it is coming from the node in MS set.
    if (msList.find(fromnbr) != msList.end()) {
        ForwardTcMsg(msg, to_string(fromnbr));
    }

    // Clear old information from srcnode.
    for (set<int>::iterator it =lastHop[srcnode].begin(); it !=lastHop[srcnode].end(); ++it) {
        tcTable[*it].erase(srcnode);
    }
   lastHop[srcnode].clear();

    // Update TC table.
    while (str_stream >> msnode) {
       lastHop[srcnode].insert(msnode);
        tcTable[msnode].insert(srcnode);
    }

    return true;
}

void HandleDataMsg(string msg) {

    stringstream s(msg);
    int nxthop, fromnbr, srcnode, dstnode;
    string dumb, temp;

    s >> nxthop >> fromnbr >> dumb >> srcnode >> dstnode;

    temp = msg;
    temp.replace(0, (to_string(nxthop)).size() + (to_string(fromnbr)).size() + dumb.size() + 
                           (to_string(srcnode)).size() + (to_string(dstnode)).size() + 5, "" );

    if (dstnode == nodeId)
    {
        appendMessage(temp, "received");
        return;
    }

    auto node = routingTable.find(dstnode);
    
    if (node != routingTable.end())
    {
        ForwardDataMsg(msg, to_string(nxthop), to_string(fromnbr), (node->second).next_hop);
    }
}


void RemoveNeighbor(int n) {

    oneHopList.erase(n);
    nbrTable[n] = NA;
    msList.erase(n);

    for(auto &itr : fromOneHop[n])
    {
        twoHopnbr[itr].erase(n);
        if (twoHopnbr[itr].empty())  twoHopList.erase(itr);
    }
    fromOneHop[n].clear();

    if (!twoHopnbr[n].empty()) { 
       twoHopList.insert(n);
    }
}

// Remove TC information from node i.
void RemoveTcInfoFrom(int i) {
#ifdef DEBUG
    cout << "* Remove TC information from node " << i << endl;
#endif

    // Remove information based on TC from node i.
    for (set<int>::iterator it =lastHop[i].begin(); it !=lastHop[i].end(); ++it) {
        tcTable[*it].erase(i);
    }
   lastHop[i].clear();

}

// Calculate and update MPR set.
// Use traditional greedy algorithm to select MPRs.

bool processToFile(int time) {

    // Use flags to indicate if there is any change in neighborhood or TC table.
    bool neighborhood_changed = false;
    bool tc_changed = false;

    // Open toX.txt for reading.
    string filename = getFileName("to");
    ifstream inputfile;
    inputfile.open(filename.c_str(), ios::in);
    if (!inputfile.is_open()) {
        //cout<<"\nCould not open file:"<<filename<<endl;
        return false;
    }


    // Only process new messages.
    streampos last_byte_read = 0;
    inputfile.seekg(position);
    string line;
    
    while (getline(inputfile, line)) {

#ifdef DEBUG
        cout << "* Processing message: " << line << endl;
#endif

        last_byte_read = inputfile.tellg();

        // Get the node id where this message from and the type of this message.
        // Type of messages and the formats: 
        //  - HELLO msg: * <node> HELLO UNIDIR <neighbor> ... <neighbor> 
        //                              BIDIR <neighbor> ... <neighbor> 
        //                              MPR <neighbor> ... <neighbor>
        //  - TC msg: * <fromnbr> TC <srcnode> <seqno> MS <msnode> ... <msnode>
        //  - DATA msg: <nxthop> <fromnbr> DATA <srcnode> <dstnode> <string>
        
        stringstream str_stream(line);
        string useless, type;
        int fromnbr;
        str_stream >> useless >> fromnbr >> type;
        
        if (type == "HELLO") {
            neighborhood_changed |= HandleHelloMsg(line, time);
        }        
        else if (type == "TC") {
            tc_changed |= HandleTcMsg(line, time);
        }
        else if (type == "DATA") {
            HandleDataMsg(line);
        }
            
    }
        
    position = max(last_byte_read, position);
    inputfile.close();

    // Check lastHello[i] for all node i. 
    // If have not receive HELLO for 15 seconds, remove node i from neighbor table.
    for (int i=0; i<NODES; ++i) {
        if (oneHopList.find(i) !=oneHopList.end() 
                && (time - lastHello[i]) >= 15 ) {
            RemoveNeighbor(i);
            neighborhood_changed |= true;
        }
    }

    // From HELLOs, update MPRs if necessary.
    if (neighborhood_changed) {
        CalculateMprSet();
    }

    return (neighborhood_changed || tc_changed);
}


int main(int argc, char **argv)
{
    for (int i=1; i<argc; ++i)
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
    
    bool dataSent = false;
    bool changeRT = false;
    
    int virtualTime = 0;

    while(virtualTime < 120) 
    {
        
        changeRT = false;
        
        changeRT = changeRT or processToFile(virtualTime);

        for (int i=0; i<NODES; ++i) {
            if (virtualTime - lastTC[i] >= 30 && !lastHop[i].empty()) {
                RemoveTcInfoFrom(i);
                changeRT = changeRT or true;
            }
        }


        // Recalculate routing table if necessary.
        if (changeRT) {
            calculateRT();
        }

        if (nodeId != destination and !dataSent and virtualTime == delay) 
        {
            auto itr = routingTable.find(destination);

            if(itr != routingTable.end())
            {
                SendDataMsg( (itr->second).next_hop );
                dataSent = true;
            }
            else
            {
                delay += 30;
            }
        }
        
        // If virtualTime is a multiple of 5, send HELLO.
        if (virtualTime % 5 == 0)
        {
            SendHelloMsg();
        }

        // If virtualTime is a multiple of 10, and MS set is not empty, create and send TC.
        if (virtualTime % 10 == 0 && !msList.empty())
        {
            SendTcMsg();
        }

        
        sleep(1);
        virtualTime++;
    }
}