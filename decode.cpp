/*
    CSE 140 - 03L
    by Duy Do and Tristen Trias

    Single cycle mips cpu simulator
*/

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <math.h>
#include <bitset>

using namespace std;

void fetch(string inst);
char decode(string inst);
void execute(int op, int immediate);            //J-type
void execute(int rs, int rt, int immediate);        //I-type
void execute(int rs, int rt, int rd, int shamt);    //R-type
void mem(string inst, int rt);
void writeback(int rd, int val);
void controlunit(int op);
string shift(string extend);
string signExtend(string imm);

// Initializing global variables
int pc = 0;
int next_pc = pc + 4;
int jump_target = 0;
int branch_target = 0;
int total_clock_cycles = 0;
int alu_zero = 0;
char next_branch = 0;

// Initializing global memory
map<string, int> fields;
string signals[10];
string registerName[32];
int registers[32];
int dmem[32];

// Char opcode
char opcode(int n) {
    if (n == 0) return 'R';
    else if (n == 2 || n == 3) return 'J';
    else return 'I';
}

// Binary opcode
string getOpcode(string bin) {
    string temp = "";
    for (int i = 0; i < 6; i++) 
        temp += bin[i];
    return temp;
}

string getrs(string bin) {
    string temp = "";
    for (int i = 0; i < 5; i++) 
        temp += bin[i + 6];
    return temp;
}

string getrt(string bin) {
    string temp = "";
    for (int i = 0; i < 5; i++) 
        temp += bin[i + 11];
    return temp;
}

string getrd(string bin) {
    string temp = "";
    for (int i = 0; i < 5; i++) 
        temp += bin[i + 16];
    return temp;
}

string getshamt(string bin) {
    string temp = "";
    for (int i = 0; i < 5; i++) 
        temp += bin[i + 21];
    return temp;
}

string getfunct(string bin) {
    string temp = "";
    for (int i = 0; i < 6; i++) 
        temp += bin[i + 26];
    return temp;
}

string getimmidiate16(string bin) {
    string temp = "";
    for (int i = 0; i < 16; i++) 
        temp += bin[i + 16];
    return temp;
}

string getimmidiate26(string bin) {
    string temp = "";
    for (int i = 0; i < 26; i++) 
        temp += bin[i + 6];
    return temp;
}

int getAlu(int funct) {
    if (funct == 8) return 8;

    funct -= 32;
    if (funct == 0) return 2;
    else if (funct == 2) return 6;
    else if (funct == 4) return 0;
    else if (funct == 5) return 1;
    else if (funct == 10) return 7;
    else if (funct == 7) return 12;
    else return funct;
}

// Extends the sign of a number
string signExtend(string imm){
    int n = stoi(imm, nullptr, 2);
    if (n < 0) {
        n = n + pow(2, 16);
        imm = bitset<16>(n).to_string();
    }
    return imm;
}

// Shifts the bits
string shift(string extend){
    extend += "00";
    return extend;
}

//Converts the decimal string to a binary string
string decToBin(int dec) {
    string bin = "";
    int r;
    while (dec > 0) {
        r = dec % 2;
        bin = to_string(r) + bin;
        dec = dec / 2;
    }
    return bin;
}

// Start debug statements

// R Instructions
string rop(int funct) {
    if (funct == 0x20) return "add";
    if (funct == 0x21) return "addu";
    if (funct == 0x24) return "and";
    if (funct == 0x08) return "jr";
    if (funct == 0x27) return "nor";
    if (funct == 0x25) return "or";
    if (funct == 0x2a) return "slt";
    if (funct == 0x2b) return "sltu";
    if (funct == 0x00) return "sll";
    if (funct == 0x02) return "srl";
    if (funct == 0x22) return "sub";
    if (funct == 0x23) return "subu";
}

//I instructions
string iop(int hex) {
    if (hex == 0x8)  return "addi";
    if (hex == 0x9)  return "addiu";
    if (hex == 0xc)  return "andi";
    if (hex == 0x4)  return "beq";
    if (hex == 0x5)  return "bne";
    if (hex == 0x24) return "lbu";
    if (hex == 0x25) return "lhu";
    if (hex == 0x30) return "ll";
    if (hex == 0xf)  return "lul";
    if (hex == 0x23) return "lw";
    if (hex == 0xd)  return "ori";
    if (hex == 0xa)  return "slti";
    if (hex == 0xb)  return "sltiu";
    if (hex == 0x28) return "sb";
    if (hex == 0x38) return "sc";
    if (hex == 0x29) return "sh";
    if (hex == 0x2b) return "sw";
}

//J instructions
string jop(int hex) {
    if (hex == 0x2) return "j";
    if (hex == 0x3) return "jal";
}

void printR() {
    int rs, rt, rd, shamt;
    string funct;

    rs = fields["rs"];
    rt = fields["rt"];
    rd = fields["rd"];
    shamt = fields["shamt"];
    funct = rop(fields["funct"]);

    cout << funct << " " << fields["funct"] << " ";
    cout << registerName[rs] << " ";
    cout << registerName[rt] << " ";
    cout << registerName[rd] << " ";

    if (shamt != 0)
        cout << shamt << " ";
    cout << endl;

}

void printI() {
    string opcode;
    int rs, rt, immidiate;
    opcode = iop(fields["opcode"]);
    rs = fields["rs"];
    rt = fields["rt"];
    immidiate = fields["immediate"];

    cout << opcode << " ";
    cout << registerName[rs] << " ";
    cout << registerName[rt] << " ";
    cout << immidiate << " ";
    cout << endl;
}

void printJ() {
    string opcode;
    string address;

    opcode = jop(fields["opcode"]);
    address = registerName[fields["address"]];

    cout << opcode << " ";
    cout << address << " ";
    cout << endl;
}

// End debug statements

// Start initialize functions

void initRegisters() {
    registerName[0] = "$zero";
    registerName[1] = "$at";
    registerName[2] = "$v0";
    registerName[3] = "$v1";
    registerName[4] = "$a0";
    registerName[5] = "$a1";
    registerName[6] = "$a2";
    registerName[7] = "$a3";
    registerName[8] = "$t0";
    registerName[9] = "$t1";
    registerName[10] = "$t2";
    registerName[11] = "$t3";
    registerName[12] = "$t4";
    registerName[13] = "$t5";
    registerName[14] = "$t6";
    registerName[15] = "$t7";
    registerName[16] = "$s0";
    registerName[17] = "$s1";
    registerName[18] = "$s2";
    registerName[19] = "$s3";
    registerName[20] = "$s4";
    registerName[21] = "$s5";
    registerName[22] = "$s6";
    registerName[23] = "$s7";
    registerName[24] = "$t8";
    registerName[25] = "$t9";
    registerName[26] = "$k0";
    registerName[27] = "$k1";
    registerName[28] = "$gp";
    registerName[29] = "$sp";
    registerName[30] = "$fp";
    registerName[31] = "$ra";

    for (int i = 0; i < 32; i++) {
        registers[i] = 0;
    }

    for (int i = 0; i < 32; i++) {
        dmem[i] = 0;
    }
}

void initFields() {
    fields["opcode"] = 0;
    fields["rs"] = 0;
    fields["rt"] = 0;
    fields["rd"] = 0;
    fields["shamt"] = 0;
    fields["funct"] = 0;
    fields["immediate"] = 0;
    fields["jumpAddress"] = 0;
}

void initSignals() {
    for (int i = 0; i < 10; i++) {
        signals[i] = "0";
    }
}

// End initialize functions


/*

Everything above here are helper functions for below

*/


//Fetches instruction
void fetch(string inst) {      
    if(next_branch == 106)  //j
        pc = jump_target + 4;
    else if(next_branch == 98)  //beq
        pc = branch_target;
    else //everything else
        pc = next_pc;
    next_pc = pc + 4;
    next_branch = 48;
}

// Decodes instruction
char decode(string inst) {
    string str = getOpcode(inst);
    //cout << "opcode is " << str << endl;

    //initializing to base two
    int decOp = stoi(str, nullptr, 2);
    char op = opcode(decOp);

    //cout << "opcode is " << decOp << endl;
    fields["opcode"] = decOp;

    if (op == 'R') {
        //cout << "R type" << endl;
        fields["rs"] = stoi(getrs(inst), nullptr, 2);
        fields["rt"] = stoi(getrt(inst), nullptr, 2);
        fields["rd"] = stoi(getrd(inst), nullptr, 2);
        fields["shamt"] = stoi(getshamt(inst), nullptr, 2);
        fields["funct"] = stoi(getfunct(inst), nullptr, 2);

        //printR();

    } else if (op == 'I') {
        //cout << "I type" << endl;
        fields["rs"] = stoi(getrs(inst), nullptr, 2);
        fields["rt"] = stoi(getrt(inst), nullptr, 2);

        string imm = getimmidiate16(inst);
        fields["immediate"] = stoi(imm, nullptr, 2);

        if (fields["opcode"] == 4) {
            string extend = signExtend(imm);
            string sh = shift(extend);

            int temp = stoi(sh, nullptr, 2);
            branch_target = next_pc + temp;
        }

        //printI();

    } else if (op == 'J') {
        string jumpBin = shift(getimmidiate26(inst));

        fields["jumpAddress"] = stoi(jumpBin, nullptr, 2);
        jump_target = fields["jumpAddress"];
        //cout << jump_target << endl;
        //printJ();

    } else 
        return 'e';

    controlunit(decOp);
    return op;

}

/*
    There are three types of executes for each type of instruction.
    The main handles which execute gets called.
*/


// J-type execute
void execute(int op, int immediate) {
    if (op == 2) { //jump
        pc = jump_target;
        next_branch = 106;
        writeback(0, 0);
    } else if (op == 3) { //jal
        //jump and link
        registers[31] = pc;
        pc = immediate + 8;
        next_branch = 106;
        writeback(31, registers[31]);
    }
}

// I-type execute
void execute(int rs, int rt, int immediate) {
    //writeback(0, 0); //Value is 0 to update clock cycles
    if (fields["opcode"] == 0x23) {
        //Load word
        mem("lw", rt);
    } else if (fields["opcode"] == 0x2b) {
        //Store word
        mem("sw", rt);
    } else if (fields["opcode"] == 0x4) {
        //branch if equal
        if (registers[rs] == registers[rt]) {
            next_branch = 98;
            pc = branch_target - 4;
        }
        writeback(0, 0);
    } else 
        cout << "Error in execute" << endl;
}

// R-type execute
// Shamt is useless, but its there to signify that it is a R type for the overloaded function
void execute(int rs, int rt, int rd, int shamt) {
    //cout << "Alu_op: " << alu_op << endl;
    int val = 0;
    if (signals[9] == "0000")  //and
        val = registers[rs] & registers[rt];

    else if (signals[9] == "0001")  //or
        val = registers[rs] || registers[rt];

    else if (signals[9] == "0010")  //add
        val = registers[rs] + registers[rt];

    else if (signals[9] == "0110")  //sub
        val = registers[rs] - registers[rt];

    else if (signals[9] == "0111") { //slt
        if (registers[rs] < registers[rt]) 
            val = 1;
        else val = 0;

    } else if (signals[9] == "1100")  //nor
        val = !(registers[rs] || registers[rt]);
    else if (signals[9] == "1000") { //jr
        pc = registers[31];
        //cout << "Jumping to " << registers[rs] << endl;
        next_branch = 106;
    } else {
        cout << "alu_op " << signals << " does not exist." << endl;
    }

    writeback(rd, val);
}

// Mem function, handles memory accessq
void mem(string inst, int rt) {
    int val = registers[fields["rs"]] + fields["immediate"];

    if (inst == "lw") {
        writeback(0, 0); // Writeback is called to update clock cycles
        registers[rt] = dmem[val / 4];

        cout << registerName[rt] << " is modified to 0x" << hex << registers[rt] << endl;
    } else if (inst == "sw") {
        writeback(0, 0);
        dmem[val / 4] = registers[rt];

        cout << "memory 0x" << hex << val << " is modified to 0x" << hex << registers[rt] << endl;
    } else 
        cout << "Error in mem" << endl;
    
}

void writeback(int rd, int val) {
    total_clock_cycles++;
    cout << "Total clock cycles: " << total_clock_cycles << endl;

    if (rd != 0) {
        registers[rd] = val;
        cout << registerName[rd] << " is modified to 0x" << hex << registers[rd] << endl;
    }
}

// Control unit function to handle control signals
void controlunit(int op) {
    if (op == 0){// R type
        signals[0] = "0";
        signals[1] = "1";
        signals[2] = "0";
        signals[3] = "0";
        signals[4] = "1";
        signals[5] = "0";
        signals[6] = "0";
        signals[7] = "0";
        signals[8] = "10"; 
        if (fields["funct"] == 0x20) signals[9] = "0010"; //add
        else if (fields["funct"] == 0x22) signals[9] = "0110"; //sub
        else if (fields["funct"] == 0x24) signals[9] = "0000"; //and
        else if (fields["funct"] == 0x25) signals[9] = "0001"; //or
        else if (fields["funct"] == 0x2a) signals[9] = "0111"; //slt
        else if (fields["funct"] == 0x27) signals[9] = "1100"; //nor
        else if (fields["funct"] == 0x08) signals[9] = "1000"; //jr
        
    } else if (op == 2 || op == 3) { // j
        signals[0] = "1";
        signals[4] = "0";
        signals[5] = "0";
        signals[6] = "0";
        signals[7] = "0";
    } else if (op == 0x23) { // lw
        signals[0] = "0";
        signals[1] = "0";
        signals[2] = "1";
        signals[3] = "1";
        signals[4] = "1";
        signals[5] = "1";
        signals[6] = "0";
        signals[7] = "0";
        signals[8] = "00";
        signals[9] = "0010";
    } else if (op == 0x2b) { // sw
        signals[0] = "0";
        signals[1] = "0";
        signals[2] = "1";
        signals[3] = "1";
        signals[4] = "1";
        signals[5] = "1";
        signals[6] = "0";
        signals[7] = "0";
        signals[8] = "00";
        signals[9] = "0010";
    } else if (op == 0x04) { //beq
        signals[0] = "0";
        signals[2] = "0";
        signals[4] = "0";
        signals[5] = "0";
        signals[6] = "0";
        signals[7] = "1";
        signals[8] = "01";
        signals[9] = "0110";
    }
}

int main() {
    initRegisters();

    char op;
    int count = 0, num = 0;
    int size = 10; // Change this to increase number of instructions if needed.
                   // 10 is the value that worked for sample_part1.txt and sample_part2.txt

    string* inst = new string[size];

    // Insert the instructions and given registers here
    ifstream file("sample_part1.txt");
    registers[9] = 0x20;
    registers[10] = 0x5;
    registers[16] = 0x70;

    dmem[0x70 / 4] = 0x5;
    dmem[0x74 / 4] = 0x10;

    // ifstream file("sample_part2.txt");
    // registers[4] = 0x5;
    // registers[5] = 0x2;
    // registers[7] = 0xa;
    // registers[16] = 0x20;
    
    string line;
    while (getline(file, line)) {
        inst[count] = line;
        count++;
    }

    while (count * 4 > pc) {
        initFields();
        initSignals();

        num = pc / 4;
        fetch(inst[num]);
        op = decode(inst[num]);

        if (op == 'R')
            execute(fields["rs"], fields["rt"], fields["rd"], fields["shamt"]);
        else if (op == 'I')
            execute(fields["rs"], fields["rt"], fields["immediate"]);
        else if (op == 'J')     
            execute(fields["opcode"], fields["immediate"]);
        else 
            cout << "Error in decode" << endl;

        cout << "pc is modified to 0x" << hex << pc << endl;
        cout << endl << endl;
    }

    cout << "\nProgram terminated" << endl;
    cout << "Total execution time: " << total_clock_cycles << " cycles" << endl;
    return 0;
}