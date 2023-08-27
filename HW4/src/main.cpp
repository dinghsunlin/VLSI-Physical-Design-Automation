#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

// #define __DEBUG_MODE__  true
#define __DEBUG_MODE__  false

using namespace std;

struct nodeData;
struct cluster;
struct rowData;

enum direction
{
    UP,
    DOWN,
    RIGHT,
    LEFT
};

struct nodeData
{
    int width, height, legalX, legalY, tmpX, tmpY;
    double globalX, globalY;

    double distance()
    {
        return sqrt(double(pow(globalX - tmpX, 2) + pow(globalY - tmpY, 2)));
    }
};

struct cluster
{
    int e = 0, w = 0, x = 0;
    int eNew =0, wNew = 0, xNew = 0;
    double q = 0, cost = -1;
    double qNew = 0, costNew = 0;
    vector<string> cells, cellsNew;

    void update(unordered_map<string, nodeData> & cellInfo, const int & y)
    {
        e = eNew;
        q = qNew;
        w = wNew;
        x = xNew;
        cost = costNew;
        cells = cellsNew;

        int nowX = x;
        for(const string & i : cells)
        {
            cellInfo[i].legalX = nowX;
            cellInfo[i].legalY = y;
            nowX += cellInfo[i].width;
        }
    }

    void copy()
    {
        cellsNew = cells;
        eNew = e;
        qNew = q;
        wNew = w;
    }

    int rightLegalX()
    {
        return x + w;
    }

    bool findLegalX(rowData & rowData);
    // {
    //     // int tmp = round(double((qNew / eNew - rowData.x) / rowData.siteWidth));
    //     // if(tmp < 0)
    //     //     tmp = 0;
    //     // else if(tmp + wNew / rowData.siteWidth > rowData.numSites)
    //     //     tmp = rowData.numSites - wNew / rowData.siteWidth;
    //     // xNew = rowData.x + tmp * rowData.siteWidth;
    //     // return rowData.isOverlap(tmp, wNew / rowData.siteWidth);

    //     int middleX = round(double((qNew / eNew - rowData.x) / rowData.siteWidth));
    //     direction nextDirX = (double(qNew / eNew) >= rowData.x + rowData.siteWidth * middleX) ? RIGHT : LEFT;
    //     bool noRight = false, noLeft = false;
    //     if(middleX < 0)
    //     {
    //         middleX = 0;
    //         noLeft = true;
    //     }
    //     else if(middleX + wNew / rowData.siteWidth > rowData.numSites)
    //     {
    //         middleX = rowData.numSites - wNew / rowData.siteWidth;
    //         noRight = true;
    //     }
    //     if(!rowData.isOverlap(middleX, wNew / rowData.siteWidth))
    //     {
    //         xNew = middleX;
    //         return true;
    //     }
    //     int displacement = 1, round = 1;
    //     do
    //     {
    //         if(nextDirX == RIGHT && !noRight)
    //         {
    //             int nowX = middleX + displacement;
    //             if(nowX + wNew / rowData.siteWidth > rowData.numSites)
    //                 noRight = true;
    //             if(!noRight)
    //             {
    //                 if(!rowData.isOverlap(nowX, wNew / rowData.siteWidth))
    //                 {
    //                     xNew = nowX;
    //                     return true;
    //                 }
    //             }
    //         }
    //         else if(nextDirX == LEFT && !noLeft)
    //         {
    //             int nowX = middleX - displacement;
    //             if(nowX < 0)
    //                 noLeft = true;
    //             if(!noLeft)
    //             {
    //                 if(!rowData.isOverlap(nowX, wNew / rowData.siteWidth))
    //                 {
    //                     xNew = nowX;
    //                     return true;
    //                 }
    //             }
    //         }
    //         if(round++ % 2 == 0)
    //             displacement++;
    //     }
    //     while(noRight && noLeft);
    //     return false;
    // }
    
    void addCell(const string & cellName,
                 const nodeData & cellData)
    {
        cellsNew.emplace_back(cellName);
        eNew += 1;
        qNew += cellData.globalX - wNew;
        wNew += cellData.width;
    }

    void mergeCluster(vector<cluster> & clusters, int nextID)
    {
        cluster & nextCluster = clusters[nextID];
        cellsNew.reserve(cellsNew.size() + nextCluster.cellsNew.size());
        cellsNew.insert(cellsNew.end(), nextCluster.cellsNew.begin(), nextCluster.cellsNew.end());
        eNew += nextCluster.eNew;
        qNew += nextCluster.qNew - nextCluster.eNew * wNew;
        wNew += nextCluster.wNew;
    }

    bool collapse(rowData & rowData, const int & frontID);
    // {
    //     if(frontID < 0)
    //         return true;
    //     cluster & frontCluster = rowData.clusters[frontID];
    //     if(frontCluster.rightLegalX() > xNew)
    //     {
    //         frontCluster.copy();
    //         frontCluster.mergeCluster(rowData.clusters, frontID + 1);
    //         if(frontCluster.findLegalX(rowData))
    //             return frontCluster.collapse(rowData, frontID - 1);
    //         else
    //             return false;
    //     }
    //     else
    //         return true;
    // }

    double assignTmpXY(unordered_map<string, nodeData> & cellInfo, const int & y, const double & maxDisplacement, bool & feasible)
    {
        int nowX = xNew, count = 0;
        costNew = 0;
        for(auto i : cellsNew)
        {
            cellInfo[i].tmpX = nowX;
            cellInfo[i].tmpY = y;
            nowX += cellInfo[i].width;
            double dis = cellInfo[i].distance();
            // cout << " *" << dis << "* ";
            if(dis > maxDisplacement)
            {
                count++;
                if(feasible)
                {
                    // cout << " BYE ";
                    feasible = false;
                    return 0;
                }
                // cout << " FFFFF ";
            }
            costNew += dis;
        }
        if(count == 0)
            feasible = true;
        if(cost == -1)
            return costNew;
        else
            return (costNew - cost);
    }
};

struct rowData
{
    int x, y, height, siteWidth, numSites;
    vector<bool> occupiedSite;
    vector<cluster> clusters;

    // void assignLegalXY(unordered_map<string, nodeData> & cellInfo, const double & maxDisplacement)
    // {
    //     cout << x << ' ' << y << endl;
    //     for(auto i : clusters)
    //         i.assignLegalXY(cellInfo, y, maxDisplacement);
    //     char a;
    //     cin >> a;
    // }

    bool isOverlap(const int & x, const int & width)
    {
        for(int i = 0; i < width; i++)
        {
            if(occupiedSite[x + i])
                return true;
        }
        return false;
    }

    vector<array<int, 2>> findInitialX(const nodeData & cellData)
    {
        vector<array<int, 2>> info;
        int middleX = round(double((cellData.globalX - x) / siteWidth));
        // cout << middleX;cout.flush();
        direction nextDirX = (cellData.globalX >= x + siteWidth * middleX) ? RIGHT : LEFT;
        bool noLeft = false, noRight = false;
        if(middleX < 0)
        {
            middleX = 0;
            noLeft = true;
        }
        else if(middleX + cellData.width / siteWidth > numSites)
        {
            middleX = numSites - cellData.width / siteWidth;
            noRight = true;
        }
        if(!isOverlap(middleX, cellData.width / siteWidth))
        {
            array<int,2> place = {middleX, x + siteWidth * middleX};
            info.emplace_back(place);
            return info;
        }
        else
        {
            // cout << "HHH";cout.flush();
            // char aa;
            // cin >> aa;
            int displacement = 1, round = 1;
            do
            {
                if(nextDirX == LEFT && !noLeft)
                {
                    int nowX = middleX - displacement;
                    // cout << nowX << 'l';cout.flush();
                    if(nowX < 0)
                        noLeft = true;
                    if(!noLeft && !isOverlap(nowX, cellData.width / siteWidth))
                    {
                        noLeft = true;
                        array<int,2> place = {nowX, x + siteWidth * nowX};
                        info.emplace_back(place);
                    }
                }
                else if(nextDirX == RIGHT && !noRight)
                {
                    int nowX = middleX + displacement;
                    // cout << nowX << 'r';cout.flush();
                    if(nowX + cellData.width / siteWidth > numSites)
                        noRight = true;
                    if(!noRight && !isOverlap(nowX, cellData.width / siteWidth))
                    {
                        noRight = true;
                        array<int,2> place = {nowX, x + siteWidth * nowX};
                        info.emplace_back(place);
                    }
                }
                if(round++ % 2 == 0)
                    displacement++;
                if(nextDirX == LEFT)
                    nextDirX = RIGHT;
                else if(nextDirX == RIGHT)
                    nextDirX = LEFT;
            }
            while(!noLeft || !noRight);
            return info;
        }
    }
};

bool cluster::findLegalX(rowData & rowData)
{
    // int tmp = round(double((qNew / eNew - rowData.x) / rowData.siteWidth));
    // if(tmp < 0)
    //     tmp = 0;
    // else if(tmp + wNew / rowData.siteWidth > rowData.numSites)
    //     tmp = rowData.numSites - wNew / rowData.siteWidth;
    // xNew = rowData.x + tmp * rowData.siteWidth;
    // return rowData.isOverlap(tmp, wNew / rowData.siteWidth);

    int middleX = round(double((qNew / eNew - rowData.x) / rowData.siteWidth));
    direction nextDirX = (double(qNew / eNew) >= rowData.x + rowData.siteWidth * middleX) ? RIGHT : LEFT;
    bool noRight = false, noLeft = false;
    if(middleX < 0)
    {
        middleX = 0;
        noLeft = true;
    }
    else if(middleX + wNew / rowData.siteWidth > rowData.numSites)
    {
        middleX = rowData.numSites - wNew / rowData.siteWidth;
        noRight = true;
    }
    if(!rowData.isOverlap(middleX, wNew / rowData.siteWidth))
    {
        cout.flush();
        xNew = rowData.x + middleX * rowData.siteWidth;
        return true;
    }
    int displacement = 1, round = 1;
    do
    {
        // cout << "RRRRRRRRR " << nextDirX;
        if(nextDirX == RIGHT && !noRight)
        {
            int nowX = middleX + displacement;
            if(nowX + wNew / rowData.siteWidth > rowData.numSites)
                noRight = true;
            if(!noRight)
            {
                if(!rowData.isOverlap(nowX, wNew / rowData.siteWidth))
                {
                    xNew = rowData.x + nowX * rowData.siteWidth;
                    // cout << " AA ";cout.flush();
                    return true;
                }
            }
        }
        else if(nextDirX == LEFT && !noLeft)
        {
            int nowX = middleX - displacement;
            if(nowX < 0)
                noLeft = true;
            if(!noLeft)
            {
                if(!rowData.isOverlap(nowX, wNew / rowData.siteWidth))
                {
                    xNew = rowData.x + nowX * rowData.siteWidth;
                    // cout << " BB ";cout.flush();
                    return true;
                }
            }
        }
        if(round++ % 2 == 0)
            displacement++;
        if(nextDirX == LEFT)
            nextDirX = RIGHT;
        else if(nextDirX == RIGHT)
            nextDirX = LEFT;
    }
    while(noRight && noLeft);
    return false;
}

bool cluster::collapse(rowData & rowData, const int & frontID)
{
    if(frontID < 0)
        return true;
    cluster & frontCluster = rowData.clusters[frontID];
    if(frontCluster.rightLegalX() > xNew)
    {
        frontCluster.copy();
        frontCluster.mergeCluster(rowData.clusters, frontID + 1);
        if(frontCluster.findLegalX(rowData))
            return frontCluster.collapse(rowData, frontID - 1);
        else
            return false;
    }
    else
        return true;
}

void readFiles(const char * file,
               double & maxDisplacement,
               unordered_map<string, nodeData> & cellInfo,
               unordered_map<string, nodeData> & blockageInfo,
               vector<rowData> & rowInfo,
               vector<string> & sortedCell)
{
    fstream aux;
    string tmp, nodeFile, plFile, sclFile;
    aux.open(file, ios::in);
    aux >> tmp >> tmp >> nodeFile >> plFile >> sclFile
        >> tmp >> tmp >> maxDisplacement;
    aux.close();

    string fileDirectory = file;
    while(fileDirectory.back() != '/')
        fileDirectory.pop_back();
    
    nodeFile = fileDirectory + nodeFile;
    plFile = fileDirectory + plFile;
    sclFile = fileDirectory + sclFile;

    fstream node;
    int numNodes, numCells, numTerminals;
    node.open(nodeFile, ios::in);
    node >> tmp >> tmp >> numNodes
         >> tmp >> tmp >> numTerminals;
    numCells = numNodes - numTerminals;
    cellInfo.reserve(numCells);
    blockageInfo.reserve(numTerminals);
    sortedCell.reserve(numCells);
    for(int i = 0; i < numCells; i++)
    {
        string name;
        nodeData data;
        node >> name >> data.width >> data.height;
        cellInfo.emplace(name, data);
        sortedCell.emplace_back(name);
    }
    for(int i = 0; i < numTerminals; i++)
    {
        string name;
        nodeData data;
        node >> name >> data.width >> data.height >> tmp;
        blockageInfo.emplace(name, data);
    }
    node.close();

    fstream pl;
    pl.open(plFile, ios::in);
    for(int i = 0; i < numCells; i++)
    {
        string name;
        pl >> name;
        nodeData & data = cellInfo[name];
        pl >> data.globalX >> data.globalY >> tmp >> tmp;
    }
    sort(sortedCell.begin(), sortedCell.end(), [&](string i, string j){return (cellInfo[i].globalX < cellInfo[j].globalX);});
    // sort(sortedCell.begin(), sortedCell.end(), [&](string i, string j){return ((cellInfo[i].globalX == cellInfo[j].globalX) ? (cellInfo[i].width < cellInfo[j].width) : (cellInfo[i].globalX < cellInfo[j].globalX));});
    for(int i = 0; i < numTerminals; i++)
    {
        string name;
        pl >> name;
        nodeData & data = blockageInfo[name];
        pl >> data.globalX >> data.globalY >> tmp >> tmp >> tmp;
        data.legalX = data.globalX;
        data.legalY = data.globalY;
    }
    pl.close();

    fstream scl;
    int numRows;
    scl.open(sclFile, ios::in);
    scl >> tmp >> tmp >> numRows;
    rowInfo.reserve(numRows);
    for(int i = 0; i < numRows; i++)
    {
        rowData data;
        scl >> tmp >> tmp
            >> tmp >> tmp >> data.y
            >> tmp >> tmp >> data.height
            >> tmp >> tmp >> data.siteWidth
            >> tmp >> tmp >> data.numSites
            >> tmp >> tmp >> data.x
            >> tmp;
        data.occupiedSite.reserve(data.numSites);
        data.occupiedSite.insert(data.occupiedSite.begin(), data.numSites, false);
        rowInfo.emplace_back(data);
    }
    scl.close();

    for(const auto & i : blockageInfo)
    {
        int startY = (i.second.legalY - rowInfo.front().y) / rowInfo.front().height;
        int rangeY = i.second.height / rowInfo.front().height;
        for(int j = 0; j < rangeY; j++)
        {   
            rowData & data = rowInfo[startY + j];
            int startX = (i.second.legalX - data.x) / data.siteWidth;
            int rangeX = i.second.width / data.siteWidth;
            for(int k = 0; k < rangeX; k++)
                data.occupiedSite[startX + k] = true;
        }
    }

    if(__DEBUG_MODE__)
    {
        cout << "\tFile Name: " << file << endl
             << "\t.node File: " << nodeFile << endl
             << "\t.pl File: " << plFile << endl
             << "\t.scl File: " << sclFile << endl
             << "\tMax Displacement: " << maxDisplacement << endl
             << "\t# of cells: " << cellInfo.size() << endl
             << "\t# of blockages: " << blockageInfo.size() << endl
             << "\t# of rows: " << rowInfo.size() << endl;
        cout.flush();

        bool sameHeight = true, sameSiteWidth = true;
        int height = rowInfo[0].height, siteWidth = rowInfo[0].siteWidth;
        for(int i = 1; i < rowInfo.size(); i++)
        {
            const rowData & data = rowInfo[i];
            if(height != data.height)
                sameHeight = false;
            if(siteWidth != data.siteWidth)
                sameSiteWidth = false;
        }
        cout << "\tRow Info:" << endl
             << "\t\tSame Height? " << ((sameHeight) ? "YES" : "NO") << endl
             << "\t\tSame Site Width? " << ((sameSiteWidth) ? "YES" : "NO") << endl;
        cout.flush();

        bool fillSite = true, inOneRow = true;
        for(const auto & i : cellInfo)
        {
            if(i.second.width % siteWidth != 0)
                fillSite = false;
            if(i.second.height != height)
                inOneRow = false;
        }
        cout << "\tCell Info:" << endl
             << "\t\tFill Site? " << ((fillSite) ? "YES" : "NO") << endl
             << "\t\tIn One Row? " << ((inOneRow) ? "YES" : "NO") << endl;
        cout.flush();
        
        bool fillRow = true;
        fillSite = true;
        for(const auto & i : blockageInfo)
        {
            if(i.second.width % siteWidth != 0)
                fillSite = false;
            if(i.second.height % height != 0)
                fillRow = false;
        }
        cout << "\tBlockage Info:" << endl
             << "\t\tFill Site? " << ((fillSite) ? "YES" : "NO") << endl
             << "\t\tFill Row? " << ((fillRow) ? "YES" : "NO") << endl;
        cout.flush();

        cout << "\tSorted Cell:" << endl << "\t";
        for(const auto & i : sortedCell)
            cout << "\t|" << i << ": " << cellInfo[i].globalX << ' ' << cellInfo[i].width;
        cout.flush();

        cout << "\tOccupied Site: " << endl;
        for(const auto & i : rowInfo)
        {
            cout << "\t";
            int count = 0;
            cout << i.y << ": ";
            for(int l = 0; l < i.occupiedSite.size(); l++)
            {
                bool j = i.occupiedSite[l];
                if(!j && count != 0)
                {
                    cout << i.x + l - count << ' ' << count << " | ";
                    count = 0;
                }
                else if(j)
                    count++;
            }
            cout << endl;
        }
        cout.flush();
        char a;
        cin >> a;
    }
}

double PlaceRow(const string & name,
                unordered_map<string, nodeData> & cellInfo,
                rowData & rowData,
                const double & maxDisplacement,
                bool & feasiblePlace)
{
    nodeData & cellData = cellInfo[name];
    if(!rowData.clusters.empty() && rowData.clusters.back().cost == -1)
        rowData.clusters.pop_back();
    vector<array<int, 2>> candidateX = rowData.findInitialX(cellData);
    if(candidateX.empty())
    {
        feasiblePlace = false;
        return -1;
    }
    else
    {
        bool last, record = false;
        double minDeltaCost = numeric_limits<double>::max();
        cluster minCostSite;
        reverse(candidateX.begin(), candidateX.end());
        for(const auto & i : candidateX)
        {
            bool findFeasiblePlace = feasiblePlace;
            // cout << "nowX: " << i[0] << ", X: " << i[1] << " ~ ";
            cout.flush();
            last = false;
            if(!rowData.clusters.empty() && rowData.clusters.back().rightLegalX() > i[1])
            {
                // cout << " in a ";cout.flush();
                cluster & lastCluster = rowData.clusters.back();
                lastCluster.copy();
                lastCluster.addCell(name, cellData);
                if(lastCluster.findLegalX(rowData))
                {
                    // cout << " in b ";cout.flush();
                    if(!lastCluster.collapse(rowData, rowData.clusters.size() - 2))
                    {
                        // cout << " in c ";cout.flush();
                        findFeasiblePlace = false;
                        continue;
                    }
                }
                else
                {
                    // cout << " in d ";cout.flush();
                    findFeasiblePlace = false;
                    continue;
                }

                int nowCluster;
                for(nowCluster = rowData.clusters.size(); nowCluster >= 1; nowCluster--)
                {
                    // cout << " in e ";cout.flush();
                    if(rowData.clusters[nowCluster - 1].cellsNew.back() != lastCluster.cellsNew.back())
                        break;
                }
                double tmp = rowData.clusters[nowCluster].assignTmpXY(cellInfo, rowData.y, maxDisplacement, findFeasiblePlace);
                
                // cout << "+" << tmp << "-" << findFeasiblePlace;
                if(findFeasiblePlace && !feasiblePlace)
                {
                    // cout << " First Meet ";
                    minDeltaCost = tmp;
                    minCostSite = lastCluster;
                    record = true;
                    last = true;
                    feasiblePlace = true;
                }
                else if(findFeasiblePlace == feasiblePlace)
                {
                    // if(feasiblePlace)
                    //     cout << " T ";
                    // else
                    //     cout << " F ";
                    if(tmp < minDeltaCost)
                    {
                        minDeltaCost = tmp;
                        minCostSite = lastCluster;
                        record = true;
                        last = true;
                    }
                }
            }
            else
            {
                cluster lastCluster;
                lastCluster.addCell(name, cellData);
                lastCluster.xNew = i[1];
                double tmp = lastCluster.assignTmpXY(cellInfo, rowData.y, maxDisplacement, findFeasiblePlace);
                // cout << "++" << tmp << "--" << findFeasiblePlace;
                if(findFeasiblePlace && !feasiblePlace)
                {
                    // cout << " First Meet ";
                    minDeltaCost = tmp;
                    minCostSite = lastCluster;
                    record = true;
                    last = true;
                    feasiblePlace = true;
                }
                else if(findFeasiblePlace == feasiblePlace)
                {
                    // if(feasiblePlace)
                    //     cout << " T ";
                    // else
                    //     cout << " F ";
                    if(tmp < minDeltaCost)
                    {
                        minDeltaCost = tmp;
                        minCostSite = lastCluster;
                        record = true;
                        last = true;
                    }
                }
            }
        }
        // if(name == "o154421")
        // {
        //     char a;
        //     cin >> a;
        // }
        if(!record)
        {
            // cout << minCostSite.cellsNew.size() << last;
            cout.flush();
            feasiblePlace = false;
            // char a;
            // cin >> a;
            return -1;
        }
        if(minCostSite.cellsNew.size() == 1)
        {
            // cout << " 111 ";
            rowData.clusters.emplace_back(minCostSite);
            return minDeltaCost;
        }
        if(last)
            return minDeltaCost;
        else
        {
            // cout << " HHH";
            cout.flush();
            minCostSite.collapse(rowData, rowData.clusters.size() - 2);
            rowData.clusters.back() = minCostSite;
            cout.flush();
            // char a;
            // cin >> a;
            return minDeltaCost;
        }
    }
}

void updateRow(rowData & rowData, unordered_map<string, nodeData> & cellInfo)
{
    // cout << rowData.clusters.back().cost << ' ';
    if(rowData.clusters.back().cost == -1)
        rowData.clusters.back().update(cellInfo, rowData.y);
    else
    {
        string lastCell = rowData.clusters.back().cellsNew.back();
        auto iter = rowData.clusters.rbegin();
        iter++;
        // cout << "a ";
        while(iter != rowData.clusters.rend() && iter->cellsNew.back() == lastCell)
        {
            rowData.clusters.pop_back();
            iter++;
            // cout << "b ";
        }
        iter--;
        iter->update(cellInfo, rowData.y);
    }
}

void Abacus(const vector<string> & sortedCell,
            unordered_map<string, nodeData> & cellInfo,
            vector<rowData> & rowInfo,
            const double & maxDisplacement)
{
    int y0 = rowInfo.front().y, rowHeight = rowInfo.front().height;
    for(const auto & i : sortedCell)
    {
        // cout << "cell: " << i << " width: " << cellInfo[i].width << " X: " << cellInfo[i].globalX << " Y: " << cellInfo[i].globalY << "\t";
        cout.flush();
        int middleY = round(double((cellInfo[i].globalY - y0) / rowHeight));
        direction nextDirY = (cellInfo[i].globalY >= y0 + rowHeight * middleY) ? UP : DOWN;
        double minDeltaCost = numeric_limits<double>::max();
        int minCostRow = -1, displacement = 0, round = 0;
        bool noDown = false, noUp = false, cont = true, feasiblePlace = false;
        do
        {
            int nowY = (nextDirY == UP) ? middleY + displacement : middleY - displacement;
            if(displacement != 0)
                if(nextDirY == UP)
                    nextDirY = DOWN;
                else if(nextDirY == DOWN)
                    nextDirY = UP;
            if(round++ % 2 == 0)
                displacement++;
            
            if(nowY < 0)
            {
                noDown = true;
                if(noUp)
                    cont = false;
                continue;
            }
            if(nowY >= rowInfo.size())
            {
                noUp = true;
                if(noDown)
                    cont = false;
                continue;
            }
            if(double((displacement - 0.5) * rowHeight) > maxDisplacement && minCostRow >= 0)
            {
                cont = false;
                continue;
            }

            // cout << "nowY: " << nowY << ", Y: " << rowInfo[nowY].y << ", " << feasiblePlace << ", ";
            cout.flush();
            
            bool findFeasiblePlace = feasiblePlace;
            double nowDeltaCost = PlaceRow(i,
                                           cellInfo,
                                           rowInfo[nowY],
                                           maxDisplacement,
                                           findFeasiblePlace);
            // cout << nowDeltaCost << ' ';

            if(findFeasiblePlace && !feasiblePlace)
            {
                // cout << " First Meet ";
                minDeltaCost = nowDeltaCost;
                minCostRow = nowY;
                feasiblePlace = true;
            }
            else if(findFeasiblePlace == feasiblePlace)
            {
                // if(feasiblePlace)
                //     cout << " T ";
                // else
                //     cout << " F ";
                if(nowDeltaCost >= 0 && nowDeltaCost < minDeltaCost)
                {
                    minDeltaCost =  nowDeltaCost;
                    minCostRow = nowY;
                }
            }
            // cout << " | " << minCostRow << ' ' << minDeltaCost << " | ";
            // if(!feasiblePlace)
            // {
            //     char a;
            //     cin >> a;
            // }
            cout.flush();
        }
        while(double((displacement - 0.5) * rowHeight) <= minDeltaCost && cont);
        // cout << " Best Y: " << minCostRow << " - ";
        // if(minCostRow == 0)
        // {
        //     cout << rowInfo[minCostRow].clusters.size();
        //     char a;
        //     cin >> a;
        // }
        updateRow(rowInfo[minCostRow], cellInfo);
        // bool f = false;
        // if(/*i == "o60861" || */i == "o207824")
        // {   
        //     // f = true;
            
        //     cout << rowInfo[minCostRow].clusters.back().e << rowInfo[minCostRow].clusters.back().q << rowInfo[minCostRow].clusters.back().w << rowInfo[minCostRow].clusters.back().x;
        //     cout.flush();
        //     char a;
        //     cin >> a;
        // }
        // if(f)
        // {
        //     if(cellInfo["a1352"].legalX == -33066)
        //     {
        //         cout << cellInfo["a1352"].legalX << "***";
        //         cout.flush();
        //         char a;
        //         cin >> a;
        //     }
        // }
    }

    // for(rowData & i : rowInfo)
    //     i.assignLegalXY(cellInfo, -1);
    // cout << cellInfo["a1352"].legalX << "+++";
    // cout.flush();
    // char ch;
    // cin >> ch;
    if(!__DEBUG_MODE__)
        return;
    for(rowData & i :rowInfo)
    {
        cout << endl << i.x << ' ' << i.y << ' ' << i.numSites << ' ' << i.clusters.size() << endl;
        int rightmost = i.x;
        for(cluster & j : i.clusters)
        {
            cout << "Cluster: " << j.rightLegalX() << ' ';
            for(string & k : j.cells)
            {
                cout << ' ' << rightmost << ' ' << k << ' ' << cellInfo[k].legalX << ' ';
                if(rightmost > cellInfo[k].legalX || cellInfo[k].legalY != i.y)
                {
                    cout << "aaa";cout.flush();
                    char c;
                    cin >> c;
                }
                rightmost = cellInfo[k].legalX + cellInfo[k].width;

            }
            if(i.isOverlap((j.x - i.x) / i.siteWidth, j.w / i.siteWidth))
            {
                char a;
                cin >> a;
            }
        }
        if(rightmost > i.x + i.numSites * i.siteWidth)
        {
            cout << "bbb";cout.flush();
            char b;
            cin >> b;
        }
    }
}

void writeFile(char * file,
               unordered_map<string, nodeData> & cellInfo,
               unordered_map<string, nodeData> & blockageInfo)
{
    fstream result;
    result.open(file, ios::out);
    for(const auto & i : cellInfo)
    {
        result << setiosflags(ios::left) << setw(11) << i.first
               << setiosflags(ios::left) << setw(11) << i.second.legalX
               << i.second.legalY << endl;
    }
    for(const auto & i : blockageInfo)
    {
        result << setiosflags(ios::left) << setw(11) << i.first
               << setiosflags(ios::left) << setw(11) << i.second.legalX
               << i.second.legalY << endl;
    }
    result.close();
}

int main(int argc, char * argv[])
{
    auto inputStart = chrono::steady_clock::now();

    double maxDisplacement;
    unordered_map<string, nodeData> cellInfo, blockageInfo;
    vector<rowData> rowInfo;
    vector<string> sortedCell;
    readFiles(argv[1],
              maxDisplacement,
              cellInfo,
              blockageInfo,
              rowInfo,
              sortedCell);

    // for(auto i : sortedCell)
    //     cout << i << ": " << cellInfo[i].globalX << ", " << cellInfo[i].width << '\t';
    // cout.flush();
    // char a;
    // cin >> a;

    auto inputEnd = chrono::steady_clock::now();
    auto AbacusStart = chrono::steady_clock::now();

    Abacus(sortedCell,
           cellInfo,
           rowInfo,
           maxDisplacement);

    auto AbacusEnd = chrono::steady_clock::now();
    auto outputStart = chrono::steady_clock::now();

    writeFile(argv[2],
              cellInfo,
              blockageInfo);

    auto outputEnd = chrono::steady_clock::now();
    cout << "----- Timing Result -----\n"
         << "  Input Time:\t\t" << chrono::duration<float>(inputEnd - inputStart).count() << "\tsec." << endl
         << "+ Abacus Time:\t" << chrono::duration<float>(AbacusEnd - AbacusStart).count() << "\tsec." << endl
         << "+ Output Time:\t\t" << chrono::duration<float>(outputEnd - outputStart).count() << "\tsec." << endl
         << "= Total Runtime:\t" << chrono::duration<float>(outputEnd - inputStart).count() << "\tsec." << endl;
}