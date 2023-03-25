#include <iostream>
#include <fstream>
#include <unordered_map>
#include <sstream>
#include <queue>
#include<unistd.h>
#include <random>

/**
 * Author: Walter Semrau
 * Version: 1
 * Date: December 10, 2022
 *
 * Description:
 * This is a simulator for a simple cpu according to the MARS-REDCODE
 * specifications.  It reads the two hard-coded files with assembly code,
 * assembles the code, writing it to the "core" memory. It has the ability to
 * disassemble a chunk of memory too. Once the two "battle programs" are
 * loaded into the core,  the simulation starts.
 *
 * If the simulation runs for 1000 turns before a player wins, the simulation
 * will stop, and the game will be declared a tie.
 *
 * A player loses if their program attempts to execute an instruction at a data
 * location in memory.
 *
 * I took the "easy path" here. Be sure your assembly code is error free.
 */


/*
 * Declaration of global variables:
 */
const int SIZE = 1024;
unsigned int core[SIZE];

using namespace std;

/*
 * Declaration of functions:
 */

int signExtend(unsigned int);

void assembleFile(string, int, unsigned int);
    unsigned int processLine(string);
        unsigned int processArg(string);

void coreDump(int, int);

string disassemble(int, int);

void runRedCode();

    int getStart(int);

    bool canExecute(int);

    void executeNext(queue<int>&, int);

        struct memoryPair{
            int addr;
            unsigned int *instr;
        };
        memoryPair getRelativeA(memoryPair);
        memoryPair getRelativeB(memoryPair);
        unsigned int getOppA(memoryPair);
        unsigned int getOppB(memoryPair);

    tuple<int,int> showLoc();



int contentOf(memoryPair);

// converts oppcode from string to integer for execution
unordered_map<string,unsigned int> oppCodes = {
        {"MOV", 1},
        {"ADD", 2},
        {"SUB", 3},
        {"JMP", 4},
        {"JMZ", 5},
        {"DMG", 6},
        {"DJZ", 7},
        {"CMP", 8},
        {"SPL", 9},
        {"DAT", 0}
};

// reverses oppcode form executable to text
unordered_map<int,  string> invOppCode = {
        {1,"MOV"},
        {2,"ADD"},
        {3,"SUB"},
        {4,"JMP"},
        {5,"JMZ"},
        {6,"DMG"},
        {7,"DJZ"},
        {8,"CMP"},
        {9,"SPL"},
        {0,"DAT"}
};

//changes address mode from executable to text--used in dissassebly
unordered_map<int,  string> addressMode = {
        {0,"#"},
        {1,""},
        {2,"@"}
};


/*
 * main method assembles the two files into the specified memory locations,
 * prints out the instructions stored, disassemles the core, then runs the
 * simulation.
 */
int main() {

    // randomly generate the starting address for player 2
    random_device rd;
    uniform_int_distribution<int> d(257,768);
    int addr2 = d(rd);

    // assemble files to core
    assembleFile("imp.rc",0,1);
    assembleFile("gemini.rc",addr2,2);

    // 
    coreDump(0,10);
    cout<< endl;
    coreDump(addr2,10);
    cout<< endl;

    cout<<disassemble(0,10);
    cout<<endl;
    cout<<disassemble(addr2,10);

    runRedCode();

    return 0;
}

void assembleFile(string filename, int addr, unsigned int user){
    ifstream filein;
    filein.open(filename);
    string s;
    int i = addr;
    while(getline(filein,s)){
        core[i] = processLine(s) | (user << 30);
        i++;
    }
}

unsigned int processLine(string line){
    unsigned int instr = 0;
    for(int i = 0; i<line.length(); i++){
        if(line[i] == ';'){
            line = line.substr(0,i);
            break;
        }
    }

    //this section splits the line into words
    int len = line.length();
    if(len==0) return 0;
    int i = 0;
    while(line[i] == ' ' && i < len){
        i++;
    }
    int j = i;
    while(line[j] != ' ' && j < len){
        j++;
    }
    string oppCode = line.substr(i,j-i);
    unsigned int biOpp = oppCodes[oppCode];
    instr |= biOpp << 26;
    i = j;
    while(line[i] == ' ' && i < len){
        i++;
    }
    j = i;
    while(line[j] != ' ' && j < len){
        j++;
    }
    string arg1 = line.substr(i,j-i);
    unsigned int biArg = processArg(arg1);
    if(biOpp ==  0){
        instr |= stoi(arg1) & 0x3ff;
        return instr;
    }
    instr |= biArg<<13;
    if(biOpp != 4 && biOpp != 9){
        i = j;
        while(line[i] == ' ' && i < len){
            i++;
        }
        j = i;
        while(line[j] != ' ' && j < len){
            j++;
        }
        string arg2 = line.substr(i,j-i);
        biArg = processArg(arg2);
        instr |= biArg;
    }
    return instr;
}

unsigned int processArg(string arg){
    unsigned int biArg;
    if(arg[0] == '#'){
        arg = arg.substr(1);
        biArg = stoi(arg) & 0x3ff;
        return biArg;
    }
    if(arg[0] == '@'){
        arg = arg.substr(1);
        biArg = stoi(arg) & 0x3ff;
        biArg |= 2 << 10;
        return biArg;
    }
    biArg = stoi(arg) & 0x3ff;
    biArg |= 1 << 10;
    return biArg;

}
void coreDump(int start, int len){
    for (int i = start; i < start + len; i++)
    {
        printf("%03x: 0x%08x\n", i, core[i]);
    }
}
string disassemble(int start, int len){
    stringstream ss;
    for(int i = start; i<len+start; i++){
        unsigned int instr = core[i];
        int subInstr = instr>>26 & 0xf;
        int opp = subInstr;
        ss << invOppCode[subInstr] << ' ';

        if(opp != 0) {
            subInstr = instr >> 23 & 0x3;
            ss << addressMode[subInstr];
            subInstr = instr >> 13 & 0x3ff;
            ss << signExtend(subInstr) << ' ';
        }
        if(opp != 4 && opp != 9) {
            if(opp != 0) {
                subInstr = instr >> 10 & 0x3;
                ss << addressMode[subInstr];
            }
            subInstr = instr & 0x3ff;
            ss << signExtend(subInstr);
        }
        ss << '\n';
    }
    return ss.str();
}
int signExtend(unsigned int num){
    if((num >> 9) & 1){
        num |= 0xfffffc00;
    }
    return num;
}

void runRedCode(){
    queue<int> p1;
    queue<int> p2;;
    p1.push(getStart(1));
    p2.push(getStart(2));
    int numTurns = 0;
    tuple<int,int> playerScores;
    bool valid = true;
    while(valid && numTurns<1000){
        printf("\e[2J");
        printf("\e[2H");
        printf("%sTurn: %d\n","\e[37m",numTurns);
        cout<< "p1- "<<p1.front()<<": "<<disassemble(p1.front(),1)
            << "p2- "<<p2.front()<<": "<<disassemble(p2.front(),1)<<endl;
        if(canExecute(p1.front())) executeNext(p1, 1);
        else valid = false;
        if(canExecute(p2.front())) executeNext(p2, 2);
        else valid = false;
        playerScores = showLoc();
        numTurns++;
        usleep(50000);

    }
    printf("%sTurn: %d\n","\e[37m",numTurns);
    cout<< "p1- "<<p1.front()<<": "<<disassemble(p1.front(),1)
        << "p2- "<<p2.front()<<": "<<disassemble(p2.front(),1)<<endl;

    cout<<"P1 score: "<< get<0>(playerScores)<<endl

    <<"P2 score: "<< get<1>(playerScores)<<endl;


    if(numTurns == 1000){
        cout<< "\nIt's a tie!\n\n";
    }
    else if(((core[p1.front()]>>26)&0xf) != 0){
        cout<< "\nPlayer one wins!\n\n";
    }
    else cout<< "\nPlayer two wins!\n\n";
}
int getStart(int user){
    for(int i = 0; i<SIZE; i++){
        unsigned int instr = core[i];
        if(instr>>30 & user){
            if((instr>>26) & 0xf){
                return i;
            }
        }
    }
    return -1;
}
bool canExecute(int addr){
    if(addr < 0) return false;
    unsigned int instr1 = core[addr];
    if(instr1>>26 & 0xf) return true;
    return false;
}
void executeNext(queue<int>& pc, int user) {
    int addr = pc.front();
    pc.pop();

    memoryPair
            immediate = {addr,&core[addr]},
            argumentA{},
            argumentB{};
    unsigned int
            oppCode = (*immediate.instr >> 26) & 0xf,
            addressModeA = (*immediate.instr >> 23) & 0x3,
            addressModeB = (*immediate.instr >> 10) & 0x3;
//set address to user
    *immediate.instr &= 0x3fffffff;
    *immediate.instr |= (user << 30);

    switch(addressModeA){
        case 0:{
            unsigned int i = getOppA(immediate) | (user<<30);
            unsigned int *arg = &i;
            argumentA = {addr,arg};
            break;
        }
        case 1: {
            argumentA = getRelativeA(immediate);
            break;
        }
        case 2: {
            argumentA = getRelativeB(getRelativeA(immediate));
            break;
        }
        default: {
            pc.push(addr * -1);
            return;
        }
    }

    switch(addressModeB){
        case 0:{
            unsigned int i = getOppB(immediate) | (user<<30);
            unsigned int *arg = &i;
            argumentB = {addr,arg};
            break;
        }
        case 1: {
            argumentB = getRelativeB(immediate);
            break;
        }
        case 2: {
            argumentB = getRelativeB(getRelativeB(immediate));
            break;
        }
        default: {
            pc.push(addr * -1);
            return;
        }
    }

    switch (oppCode) {
        case 1: {
            *argumentB.instr = *argumentA.instr;
            pc.push((addr + 1) % SIZE);
            break;
        }
        case 2: {
            int b = contentOf(argumentB);
            int a = contentOf(argumentA);

            *argumentB.instr &= 0xfffffc00;
            *argumentB.instr |= (b+a) & 0x3ff;
            pc.push((addr + 1) % SIZE);
            break;
        }
        case 3: {
            int b = contentOf(argumentB);
            int a = contentOf(argumentA);
            *argumentB.instr &= 0xfffffc00;
            *argumentB.instr |= (b-a) & 0x3ff;
            pc.push((addr + 1) % SIZE);
            break;
        }
        case 4: {
            pc.push(argumentA.addr);
            break;
        }
        case 5:{
            if(!contentOf(argumentB)) pc.push(argumentA.addr);
            else pc.push((addr+1)%SIZE);
            break;
        }
        case 6:{
            if(contentOf(argumentB)) pc.push(argumentA.addr);
            else pc.push((addr+1)%SIZE);
            break;
        }
        case 7:{
            int b = contentOf(argumentB);
            *argumentB.instr &= 0xfffffc00;
            *argumentB.instr |= (b-1) & 0x3ff;
            if(contentOf(argumentB)) pc.push(argumentA.addr);
            else pc.push((addr+1)%SIZE);
            break;
        }
        case 8:{
            if(*argumentB.instr != *argumentA.instr) pc.push((addr+2)%SIZE);
            else pc.push((addr+1)%SIZE);
            break;
        }
        case 9:{
            pc.push(argumentA.addr);
            pc.push((addr+1)%SIZE);
            break;
        }
        default: pc.push(addr * -1);
    }
}
int contentOf(const memoryPair loc){
    return signExtend(*loc.instr & 0x3ff);
}

tuple<int,int> showLoc(){
    int count1=0, count2=0;
    unsigned int memCol;
    for(int i = 0; i < 16; i++){
        for(int j = 0; j< 64; j++){
            memCol = core[(64 * i) + j] >> 30;
            switch(memCol){
                default: {
                    printf("%s%d","\e[37m",memCol);
                    break;
                }
                case 1: {
                    printf("%s%d","\e[34;1m",memCol);
                    count1++;
                    break;
                }
                case 2: {
                    printf("%s%d","\e[31;1m",memCol);
                    count2++;
                    break;
                }

            }
        }
        cout<<endl;
    }
    cout<<endl;
    return make_tuple(count1,count2);
}
unsigned int getOppA(memoryPair loc){
    return *loc.instr >> 13 & 0x3ff;
}
unsigned int getOppB(memoryPair loc){
    return *loc.instr & 0x3ff;
}
memoryPair getRelativeA(memoryPair loc){
    int newAddr = (loc.addr + signExtend(getOppA(loc))) % SIZE;
    if (newAddr < 0) newAddr += SIZE;
    return {newAddr, &core[newAddr]};
}
memoryPair getRelativeB(memoryPair loc){
    int newAddr = (loc.addr + signExtend(getOppB(loc))) % SIZE;
    if (newAddr < 0) newAddr += SIZE;
    return {newAddr, &core[newAddr]};
}
