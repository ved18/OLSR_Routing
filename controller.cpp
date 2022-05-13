#include<bits/stdc++.h>
#include<unistd.h>
#define TOPOLOGY "topology.txt"
#define NODES 10

using namespace std;

struct TopologyNode {
    int time, src, destn;
    string status;
};

map<pair<int, int>, string> nodeLink;
static int lineNo = 0;
vector<streampos> position;
vector<TopologyNode> topologyTable;

void addToTopology(int time, string status, int src, int destn)
{
    TopologyNode tp;
    tp.time = time;
    tp.status = status;
    tp.src = src;
    tp.destn = destn;
    topologyTable.push_back(tp);
}

void updateTopologyTable(int checkTime)
{
    int i = lineNo;

    while(i<topologyTable.size() and topologyTable[i].time <= checkTime)
    {
        nodeLink[{topologyTable[i].src, topologyTable[i].destn}] = topologyTable[i].status;
        i++;
    } 

    lineNo = i;
}

void processFile(string filename)
{
   
    
    if(filename == TOPOLOGY)
    {
        ifstream fp;
        fp.open(filename, ios::in);

        if(!fp.is_open())
        {
            cout<<"\nCould not open file "<<filename<<endl;
            return;
        }
        string line;

        while(getline(fp, line))
        {
            int time, src, destn;
            string status;
            stringstream s(line);

            s >> time >> status >> src >> destn;
            addToTopology(time, status, src, destn);
        }
        lineNo = 0;
        fp.close();
    }
    else
    {
        ifstream fp;
        fp.open(filename, ios::in);

        filename.erase(0, 4);
        filename.erase(filename.end()-4, filename.end());
        int id = stoi(filename);

        string line;
        streampos lastpos;
        fp.seekg(position[id]);

        while(getline(fp, line))
        {
            lastpos = fp.tellg();
            if(line[0] == '*')
            {
                for(int i=0; i<NODES; i++)
                {
                    if(id != i and nodeLink[{id,i}] == "UP")
                    {
                        string toFilename = "to" + to_string(i) + ".txt";
                        ofstream ofp;
                        ofp.open(toFilename, ios::app);
                        if(!ofp.is_open())
                        {
                            cout<<"\nCould not open file "<<toFilename<<endl;
                            return;
                        }
                        ofp << line <<endl;
                        ofp.close();
                    }
                    else
                        continue;    
                }
            }
            else
            {
                stringstream s(line);
                int destination;
                s >> destination;

                if(destination != id and nodeLink[{id, destination}] == "UP")
                {
                    string toFilename = "to" + to_string(destination) + ".txt";
                    ofstream ofp;
                    ofp.open(toFilename, ios::app);  
                    if(!ofp.is_open())
                    {
                        cout<<"\nCould not open file "<<toFilename<<endl;
                        return;
                    }
                    ofp << line <<endl;
                    ofp.close();
                }
            }
        }
        position[id] = max(lastpos, position[id]);
        fp.close();
    }
}

void initTopologyTable()
{
    processFile(TOPOLOGY);
}

void init()
{
    initTopologyTable();

    for(int i=0; i<NODES; i++)
    {
        position.push_back(0);
        for(int j=0; j<NODES; j++)
        {
            nodeLink[{i,j}] = "DOWN";
        }
    }
}

int main()
{
    init();
    
    int virtualTime = 0;

    while (virtualTime < 120)
    {
        updateTopologyTable(virtualTime);

        for (int i=0; i<10; i++)
        {
            string fromFile = "from" + to_string(i) + ".txt";
            processFile(fromFile);
        }

        sleep(1);
        virtualTime++;
    }

}
