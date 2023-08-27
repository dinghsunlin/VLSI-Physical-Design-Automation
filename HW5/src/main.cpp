#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

#define CS_WIDTH        7100
#define CS_HEIGHT       6600
#define M3_WIDTH        440
#define M3_SPACING      310
#define M4_WIDTH        1000
#define M4_SPACING      490

#define CS_X1_TO_DRAIN  1260
#define CS_Y1_TO_DRAIN  4100
#define CS_LIB_NAME     "MSBCS"
#define VIA34_LIB_NAME  "Via34"

struct Component
{
    string libName, instName;
    int x, y;

    Component(const string & libName, const string & instName, const int & x, const int & y)
        : libName(libName), instName(instName), x(x), y(y) {}

    string write()
    {
        return "- " + instName + " " + libName + "\n  + PLACED ( " + to_string(x) + " " + to_string(y) + " ) N ;\n";
    }
};

struct SpecialNet
{
    string instName, layer;
    int x1, y1, x2, y2;

    SpecialNet(const string & instName, const string & layer, const int & x1, const int & y1, const int & x2, const int & y2)
        : instName(instName), layer(layer), x1(x1), y1(y1), x2(x2), y2(y2) {}

    string write()
    {
        if(layer == "ME3")
            return "- " + instName + "\n  + ROUTED " + layer + " 440 ( " + to_string((x1 + x2) / 2) + " " + to_string(y1) + " ) ( * " + to_string(y2) + " ) ;\n";
        else if(layer == "ME4")
            return "- " + instName + "\n  + ROUTED " + layer + " 1000 ( " + to_string(x1) + " " + to_string((y1 + y2) / 2) + " ) ( " + to_string(x2) + " * ) ;\n";
        return "ERROR!";
    }
};

int main(int argc, char * argv[])
{
    const int numSources = stoi(argv[1]);
    const int numRC = sqrt(numSources) * 2;

    const int die_x1 = 0;
    const int die_y1 = 0;
    const int die_x2 = CS_WIDTH * numRC + M3_WIDTH * numSources * 2 + M3_SPACING * ((numRC / 2 + 1) * numRC - 1);
    const int die_y2 = CS_HEIGHT * numRC + M4_WIDTH * numSources + M4_SPACING * (numSources + numRC - 1);

    int numComponent = 0;
    vector<vector<Component *>> csArray (numRC);
    for(int i = 0; i < numRC; i++)
    {
        csArray[i].resize(numRC);
        for(int j = 0; j < numRC; j++)
        {
            string instName = "Transistor" + to_string(i * numRC + j);
            int x = i * (CS_WIDTH + M3_WIDTH * sqrt(numSources) + M3_SPACING * (sqrt(numSources) + 1));
            int y = (M4_WIDTH + M4_SPACING) * (numSources / numRC) + j * (CS_HEIGHT + M4_WIDTH * numSources / numRC + M4_SPACING * (numSources / numRC + 1));
            numComponent++;
            csArray[i][j] = new Component(CS_LIB_NAME, instName, x, y);
        }
    }

    int numSpecialNet = 0;
    vector<vector<SpecialNet *>> ME3SpecialNet (numRC);
    for(int i = 0; i < numRC; i++)
    {
        ME3SpecialNet[i].resize(sqrt(numSources));
        for(int j = 0; j < sqrt(numSources); j++)
        {
            string instName = "Metal3_" + to_string(int(i * sqrt(numSources) + j));
            int x1 = csArray[i][0]->x + CS_WIDTH + j * M3_WIDTH + (j + 1) * M3_SPACING;
            int x2 = x1 + M3_WIDTH;
            int y1 = 0;
            int y2 = die_y2;
            numSpecialNet++;
            ME3SpecialNet[i][j] = new SpecialNet(instName, "ME3", x1, y1, x2, y2);
        }
    }

    vector<vector<SpecialNet *>> ME4SpecialNetDrain (numRC);
    for(int i = 0; i < sqrt(numSources); i++)
    {  
        ME4SpecialNetDrain[i].resize(numRC);
        ME4SpecialNetDrain[numRC - i - 1].resize(numRC);
        for(int j = 0; j < sqrt(numSources); j++)
        {
            string instName;
            int x1, x2, y1, y2;
            numSpecialNet += 4;
            instName = "Metal4_drain_" + to_string(int(i * sqrt(numSources) + j + 0 * numSources));
            x1 = csArray[i][j]->x + CS_X1_TO_DRAIN;
            x2 = ME3SpecialNet[i][j]->x2;
            y1 = csArray[i][j]->y + CS_Y1_TO_DRAIN;
            y2 = y1 + M4_WIDTH;
            ME4SpecialNetDrain[i][j] = new SpecialNet(instName, "ME4", x1, y1, x2, y2);
            instName = "Metal4_drain_" + to_string(int(i * sqrt(numSources) + j + 1 * numSources));
            x1 = csArray[numRC - i - 1][j]->x + CS_X1_TO_DRAIN;
            x2 = ME3SpecialNet[numRC - i - 1][j]->x2;
            y1 = csArray[numRC - i - 1][j]->y + CS_Y1_TO_DRAIN;
            y2 = y1 + M4_WIDTH;
            ME4SpecialNetDrain[numRC - i - 1][j] = new SpecialNet(instName, "ME4", x1, y1, x2, y2);
            instName = "Metal4_drain_" + to_string(int(i * sqrt(numSources) + j + 2 * numSources));
            x1 = csArray[i][numRC - j - 1]->x + CS_X1_TO_DRAIN;
            x2 = ME3SpecialNet[i][j]->x2;
            y1 = csArray[i][numRC - j - 1]->y + CS_Y1_TO_DRAIN;
            y2 = y1 + M4_WIDTH;
            ME4SpecialNetDrain[i][numRC - j - 1] = new SpecialNet(instName, "ME4", x1, y1, x2, y2);
            instName = "Metal4_drain_" + to_string(int(i * sqrt(numSources) + j + 3 * numSources));
            x1 = csArray[numRC - i - 1][numRC - j - 1]->x + CS_X1_TO_DRAIN;
            x2 = ME3SpecialNet[numRC - i - 1][j]->x2;
            y1 = csArray[numRC - i - 1][numRC - j - 1]->y + CS_Y1_TO_DRAIN;
            y2 = y1 + M4_WIDTH;
            ME4SpecialNetDrain[numRC - i - 1][numRC - j - 1] = new SpecialNet(instName, "ME4", x1, y1, x2, y2);
        }
    }

    vector<vector<SpecialNet *>> ME4SpecialNetPort (numRC);
    for(int i = 0; i < numRC; i++)
    {
        ME4SpecialNetPort[i].resize(numSources / numRC);
        for(int j = 0; j < numSources / numRC; j++)
        {
            string instName = "Metal4_port_" + to_string(i * numSources / numRC + j);
            int x1 = 0;
            int x2 = die_x2;
            int y1 = csArray[0][i]->y - (numSources / numRC - j) * (M4_WIDTH + M4_SPACING);
            int y2 = y1 + M4_WIDTH;
            numSpecialNet++;
            ME4SpecialNetPort[i][j] = new SpecialNet(instName, "ME4", x1, y1, x2, y2);
        }
    }

    vector<vector<Component *>> Via34Drain2ME3 (numRC);
    for(int i = 0; i < sqrt(numSources); i++)
    {
        Via34Drain2ME3[i].resize(numRC);
        Via34Drain2ME3[numRC - i - 1].resize(numRC);
        for(int j = 0; j < sqrt(numSources); j++)
        {
            string instName;
            int x, y;
            numComponent += 4;
            instName = "Via34_drain2ME3_" + to_string(int(i * sqrt(numSources) + j + 0 * numSources));
            x = ME3SpecialNet[i][j]->x1;
            y = csArray[i][j]->y + CS_Y1_TO_DRAIN;
            Via34Drain2ME3[i][j] = new Component(VIA34_LIB_NAME, instName, x, y);
            instName = "Via34_drain2ME3_" + to_string(int(i * sqrt(numSources) + j + 1 * numSources));
            x = ME3SpecialNet[numRC - i - 1][j]->x1;
            y = csArray[numRC - i - 1][j]->y + CS_Y1_TO_DRAIN;
            Via34Drain2ME3[numRC - i - 1][j] = new Component(VIA34_LIB_NAME, instName, x, y);
            instName = "Via34_drain2ME3_" + to_string(int(i * sqrt(numSources) + j + 2 * numSources));
            x = ME3SpecialNet[i][j]->x1;
            y = csArray[i][numRC - j - 1]->y + CS_Y1_TO_DRAIN;
            Via34Drain2ME3[i][numRC - j - 1] = new Component(VIA34_LIB_NAME, instName, x, y);
            instName = "Via34_drain2ME3_" + to_string(int(i * sqrt(numSources) + j + 3 * numSources));
            x = ME3SpecialNet[numRC - i - 1][j]->x1;
            y = csArray[numRC - i - 1][numRC - j - 1]->y + CS_Y1_TO_DRAIN;
            Via34Drain2ME3[numRC - i - 1][numRC - j - 1] = new Component(VIA34_LIB_NAME, instName, x, y);
        }
    }

    vector<vector<Component *>> Via34Port2ME3 (numSources);
    for(int i = 0; i < sqrt(numSources); i++)
    {
        for(int j = 0; j < sqrt(numSources); j++)
        {
            Via34Port2ME3[i * sqrt(numSources) + j].resize(2);
            string instName;
            int x, y = ME4SpecialNetPort[i * 2 + j / (numSources / numRC)][j % (numSources / numRC)]->y1;
            numComponent += 2;
            instName = "Via34_port2ME3_" + to_string(int(i * sqrt(numSources) + j + 0 * numSources));
            x = ME3SpecialNet[i][j]->x1;
            Via34Port2ME3[i * sqrt(numSources) + j][0] = new Component(VIA34_LIB_NAME, instName, x, y);
            instName = "Via34_port2ME3_" + to_string(int(i * sqrt(numSources) + j + 1 * numSources));
            x = ME3SpecialNet[numRC - i - 1][j]->x1;
            Via34Port2ME3[i * sqrt(numSources) + j][1] = new Component(VIA34_LIB_NAME, instName, x, y);
        }
    }

    fstream def;
    def.open(argv[2], ios::out);
    def << "VERSION 5.6 ;\nDIVIDERCHAR \"/\" ;\nBUSBITCHARS \"[]\" ;\nDESIGN CS_APR ;\n\n"
        << "UNITS DISTANCE MICRONS 1000 ;\n\n"
        << "PROPERTYDEFINITIONS\n  COMPONENTPIN text STRING ;\nEND PROPERTYDEFINITIONS\n\n"
        << "DIEAREA ( " << die_x1 << " " << die_y1 << " ) ( " << die_x2 << " " << die_y2 << " ) ;\n\n";
    def << "COMPONENTS " << numComponent << " ;\n";
    for(int i = 0; i < numRC; i++)
    {
        for(int j = 0; j < numRC; j++)
        {
            def << csArray[i][j]->write();
            delete csArray[i][j];
        }
        csArray[i].clear();
    }
    csArray.clear();
    for(int i = 0; i < numRC; i++)
    {
        for(int j = 0; j < numRC; j++)
        {
            def << Via34Drain2ME3[i][j]->write();
            delete Via34Drain2ME3[i][j];
        }
        Via34Drain2ME3[i].clear();
    }
    Via34Drain2ME3.clear();
    for(int i = 0; i < numSources; i++)
    {
        for(int j = 0; j < 2; j++)
        {
            def << Via34Port2ME3[i][j]->write();
            delete Via34Port2ME3[i][j];
        }
        Via34Port2ME3[i].clear();
    }
    Via34Port2ME3.clear();
    def << "END COMPONENTS\n\n";
    def << "SPECIALNETS " << numSpecialNet << " ;\n";
    for(int i = 0; i < numRC; i++)
    {
        for(int j = 0; j < sqrt(numSources); j++)
        {
            def << ME3SpecialNet[i][j]->write();
            delete ME3SpecialNet[i][j];
        }
        ME3SpecialNet[i].clear();
    }
    ME3SpecialNet.clear();
    for(int i = 0; i < numRC; i++)
    {
        for(int j = 0; j < numRC; j++)
        {
            def << ME4SpecialNetDrain[i][j]->write();
            delete ME4SpecialNetDrain[i][j];
        }
        ME4SpecialNetDrain[i].clear();
    }
    ME4SpecialNetDrain.clear();
    for(int i = 0; i < numRC; i++)
    {
        for(int j = 0; j < numSources / numRC; j++)
        {
            def << ME4SpecialNetPort[i][j]->write();
            delete ME4SpecialNetPort[i][j];
        }
        ME4SpecialNetPort[i].clear();
    }
    ME4SpecialNetPort.clear();
    def << "END SPECIALNETS\n\nEND DESIGN\n";
    def.close();
}