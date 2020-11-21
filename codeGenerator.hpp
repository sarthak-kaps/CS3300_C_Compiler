/*
    Strategy for Code Generation
    *  Expect 3 Address Code as input --> mostly sequential 
    *  Symbol Table --> needs to be shared among lexer, parser, IR generator
*/

/**
 IR statement can be of the following type
 * function label - same name
 * IR labels - start with .
 * Assignment statement, contains =
 * if <variable> goto <label>
 * goto <label>
 * printf and its sole argument
 * return <expr>
 * function call
 * function end
**/

/**
 * Final code is in a.s file
**/

#include <bits/stdc++.h>
#include "symbolTable.hpp"
#define MAXN 1000

extern symbolTable final_table;

extern ofstream summary;
extern string if_simpl;       // answer of if simpl 
extern map<int, vector<string>> const_folding;  // answer for constant folding
extern map<int, set<pair<int, pair<string, string>>>> const_prop;  // answer for constant propagation
extern unordered_map<int, int> line_map;
extern map<pair<int, string>, set<int>> cse;   // common subexpression elimination
extern set<pair<int, string>> unused_var;             // unused variables
extern map<int, vector<int>> str_red;  // strength reduction
extern set<int> if_line_nos;

ofstream AScode;

class x86 {
    public:
    string tab = "      ";
    void movl(string source, string destination) {
        AScode << tab << "movl " << source << " , " << destination << endl;
    }
    void movq(string source, string destination) {
        AScode << tab << "movq " << source << " , " << destination << endl;
    }
    void movzbl(string source, string destination) {
        AScode << tab << "movzbl " << source << " , " << destination << endl;
    }
    void addl(string addend, string destination) {
        AScode << tab << "addl " << addend << " , " << destination << endl;
    }
    void subl(string subend, string destination) {
        AScode << tab << "subl " << subend << " , " << destination << endl;
    }
    void subq(string subend, string destination) {
        AScode << tab << "subq " << subend << " , " << destination << endl;
    }
    void imull(string multiplicand, string destination) {
        AScode << tab << "imull " << multiplicand << " , " << destination << endl;
    }
    void idivl(string divisor) {
        AScode << tab << "idivl " << divisor << endl;
    }
    void sall(string shift, string destination) {
        AScode << tab << "sall " << shift << " , " << destination << endl;
    }
    void salq(string shift, string destination) {
        AScode << tab << "salq " << shift << " , " << destination << endl;
    }
    void sarl(string shift, string destination) {
        AScode << tab << "sarl " << shift << " , " << destination << endl;
    }
    void negl(string arg) {
        AScode << tab << "negl " << arg << endl;
    }
    void notl(string arg) {
        AScode << tab << "notl " << arg << endl;
    }
    void cltd() {
        AScode << tab << "cltd" << endl;
    }
    void cltq() {
        AScode << tab << "cltq" << endl;
    }
    void leaq(string address, string destination) {
        AScode << tab << "leaq " << address << " , " << destination << endl;
    }
    void cmpl(string op2, string op1) {   // compares op1 with op2, yes the parameters are correct
        AScode << tab << "cmpl " << op2 << " , " << op1 << endl;
    }
    void call(string function_name) {
        AScode << tab << "call " << function_name << endl;
    }
    void putLabel(string label) {
        AScode << label << endl;
    }
    void je(string label) {
        AScode << tab << "je " << label << endl;
    }
    void jmp(string label) {
        AScode << tab << "jmp " << label << endl;
    }
    void jne(string label) {
        AScode << tab << "jne " << label << endl;
    }
    void jg(string label) {
        AScode << tab << "jg " << label << endl;
    }
    void jl(string label) {
        AScode << tab << "jl " << label << endl;
    }
    void jge(string label) {
        AScode << tab << "jge " << label << endl;
    }
    void jle(string label) {
        AScode << tab << "jle " << label << endl;
    }
    void leave() {
        AScode << tab << "leave" << endl;
    }
    void begin() {    // basic template of the assembly
        AScode << tab << ".file	\"a.c\"" << endl;
        AScode << tab << ".text" << endl;
        AScode << tab << ".section  .rodata" << endl;
        AScode << ".LC0:" << endl;
        AScode << tab << R"(.string	"%d\n")" << endl;
        AScode << ".LC1:" << endl;
        AScode << tab << R"(.string     "%d")" << endl;
        AScode << tab << ".text" << endl;
        AScode << tab << ".globl	main" << endl;
        AScode << tab << ".type	main, @function" << endl;
    }
    void function_template(int n) {     // n is the number of bytes of space to be allocated in the stack
        while(n % 16) {   // aligning the stack to 16 byte boundary.
            n += 4;
        }
        string space = to_string(n);
        AScode << tab << ".cfi_startproc" << endl;
	    AScode << tab << "pushq    %rbp" << endl;
	    AScode << tab << ".cfi_def_cfa_offset 16" << endl;
 	    AScode << tab << ".cfi_offset 6, -16" << endl;
	    AScode << tab << "movq	%rsp, %rbp" << endl;
	    AScode << tab << ".cfi_def_cfa_register 6" << endl;
	    AScode << tab << "subq  " << "$" << space <<  ", %rsp" << endl;
    }
    void function_return_template() {
        AScode << tab << "leave" << endl;
	    AScode << tab << ".cfi_def_cfa 7, 8" << endl;
	    AScode << tab << "ret" << endl;
	    AScode << tab << ".cfi_endproc" << endl;
    }
    void pushq(string source) {
        AScode << tab << "pushq " << source << endl;
    }
    void popq(string destination) {
        AScode << tab << "popq " << destination << endl;
    }
};

x86 gen;

// movl	-8(%rbp), %eax
// cltd
// idivl	-4(%rbp)
// movl	%eax, -8(%rbp)
// division

// movl	-8(%rbp), %eax
// imull	-4(%rbp), %eax
// movl	%eax, -8(%rbp)
// multiplication

// movl	-4(%rbp), %eax
// 	addl	%eax, -8(%rbp)
// addition

// movl	-4(%rbp), %eax
// subl	%eax, -8(%rbp)
// subtraction

long long int asm_label_count = 0ll;

int to_integer(string s, int base = 10) {   /* Handle hex integers */
        stringstream ss(s);
    if(s.length() >= 2 and s[0] == '0' and s[1] == 'x') {
        int val;
        ss >> hex >> val;
        return val;
    }
    else {
        int val;
        ss >> dec >> val;
        return val;
    }
}

string new_asm_Label() {   // returns new label
    string LABEL = ".LL";
    string x = to_string(asm_label_count);
    LABEL += x;
    asm_label_count++;
    return LABEL;
}

string get_asm_symbol(string var) {   // get symbol in terms of base pointer for variables, or get constants as such
    string op;
    if(final_table.isInTable(var)) {
        auto node = final_table.getNode(var);
        string offset = to_string(node->offset);
        if(!node->isArg) {          // arguments have positive offset
            offset = "-" + offset;
        }
        op = offset + "(%rbp)";
    }
    else {
        op = "$" + var;
    }
    return op;
}

// return -1 on failure or else return i such that 2 ^ i = var
int check_power_of_2(string var) {
    if(final_table.isInTable(var)) {
        return -1;
    }
    int val = to_integer(var);
    for(int i = 0; i < 12; i++) {
        if(val < (1 << i)) {
            return -1;
        }
        if(val == (1 << i)) {
            return i;
        }
    }
    return -1;
}

void binary_assign_code_gen(string op, string res, vector<string>& operands, int line_no) {
    // use 2 registers for 2 operands
    // use the correct instruction corresponding to op
    // store in res
    string op1;
    string op2;
    string res_op;
    // if(final_table.isInTable(operands[0])) {
    //     auto var = final_table.getNode(operands[0]);
    //     string offset = "-" + to_string(var->offset);
    //     op1 = offset + "(%rbp)";    // variable on stack
    // }
    // else {
    //     op1 = "$" + operands[0];   // number
    // }
    // if(final_table.isInTable(operands[1])) {
    //     auto var = final_table.getNode(operands[1]);
    //     string offset = "-" + to_string(var->offset);
    //     op2 = offset + "(%rbp)";    // variable on stack
    // }
    // else {
    //     op2 = "$" + operands[1];    // number
    // }
    // auto var = final_table.getNode(res);
    // string res_offset = "-" + to_string(var->offset);
    // string res_op = res_offset + "(%rbp)";  // variable on stack
    op1 = get_asm_symbol(operands[0]);
    op2 = get_asm_symbol(operands[1]);
    res_op = get_asm_symbol(res);
    if(op == "PLUS") {
        gen.movl(op1, "%eax");
        gen.addl(op2, "%eax");
        gen.movl("%eax", res_op);
    }
    else if(op == "MINUS") {
        gen.movl(op1, "%eax");
        gen.subl(op2, "%eax");
        gen.movl("%eax", res_op);
    }
    else if(op == "MULT") {
        gen.movl(op1, "%eax");
        gen.imull(op2, "%eax");
        gen.movl("%eax", res_op);
    }
    else if(op == "DIV" or op == "MOD") {
        gen.movl("$0", "%edx");
        gen.movl(op1, "%eax");
        gen.movl(op2, "%ebx");
        gen.cltd();
        gen.idivl("%ebx");
        if(op == "DIV") {
            gen.movl("%eax", res_op);   // quotient
        }  
        else {
            gen.movl("%edx", res_op);   // remainder
        }
    }
    else if(op == "SAL1" or op == "SAL2") {
        gen.movl(op2, "%eax");
        gen.sall(op1, "%eax");
        gen.movl("%eax", res_op);
    }
    else {
        // logical operators
        if(op == "AND") {
            gen.movl(op1, "%eax");
            gen.cmpl("$0", "%eax");
            string l1 = new_asm_Label();
            gen.je(l1);
            gen.movl(op2, "%eax");
            gen.cmpl("$0", "%eax");
            gen.je(l1);
            gen.movl("$1", "%eax");
            string l2 = new_asm_Label();
            gen.jmp(l2); 
            gen.putLabel(l1 + ":");
            gen.movl("$0", "%eax");
            gen.putLabel(l2 + ":");
            gen.movl("%eax", res_op);
            return;
        }
        else if(op == "OR") {
            gen.movl(op1, "%eax");
            gen.cmpl("$0", "%eax");
            string l1 = new_asm_Label();
            gen.jne(l1);
            gen.movl(op2, "%eax");
            gen.cmpl("$0", "%eax");
            gen.jne(l1);
            gen.movl("$0", "%eax");
            string l2 = new_asm_Label();
            gen.jmp(l2); 
            gen.putLabel(l1 + ":");
            gen.movl("$1", "%eax");
            gen.putLabel(l2 + ":");
            gen.movl("%eax", res_op);
            return;
        }
        // movl	-12(%rbp), %eax
	    // cmpl	-4(%rbp), %eax
	    // setle	%al
	    // movzbl	%al, %eax

        // relational operators
        gen.movl(op2, "%eax");
        gen.movl(op1, "%ebx");
        gen.cmpl("%eax", "%ebx");
        if(op == "LT") {
            AScode << "      " << "setl " << "%al" << endl;
        }
        else if(op == "GT") {
            AScode << "      " << "setg " << "%al" << endl;
        }
        else if(op == "LEQ") {
            AScode << "      " << "setle " << "%al" << endl;
        }
        else if(op == "GEQ") {
            AScode << "      " << "setge " << "%al" << endl;
        }
        else if(op == "EQEQ") {
            AScode << "      " << "sete " << "%al" << endl;
        }
        else if(op == "NEQ") {
            AScode << "     " << "setne " << "%al" << endl;
        }
        else {
            // IMPLEMENT NEWOP ?
        }
        gen.movzbl("%al", "%eax");    
        gen.movl("%eax", res_op);
    }
}

void unary_assign_code_gen(string op, string res, vector<string>& operands) {
    string op1;
    string res_op;
    // if(final_table.isInTable(operands[0])) {
    //     auto var = final_table.getNode(operands[0]);
    //     string offset = "-" + to_string(var->offset);
    //     op1 = offset + "(%rbp)";    // variable on stack
    // }
    // else {
    //     op1 = "$" + operands[0];   // number
    // }
    // auto var = final_table.getNode(res);
    // string res_offset = "-" + to_string(var->offset);
    // string res_op = res_offset + "(%rbp)";  // variable on stack
    op1 = get_asm_symbol(operands[0]);
    res_op = get_asm_symbol(res);
    if(op == "PLUS") {
        gen.movl(op1, res_op);
    } 
    else if(op == "MINUS") {
        gen.movl(op1, "%eax");
        gen.negl("%eax");
        gen.movl("%eax", res_op);
    }
    else if(op == "NOT") {
        gen.movl(op1, "%eax");
        gen.cmpl("$0", "%eax");
        AScode << "      " << "sete  " << "%al" << endl;
        gen.movzbl("%al", "%eax");
        gen.movl("%eax", res_op); 
    }
}

void assign_code_gen(string res, string rhs) {
    string op1;
    string res_op;
    // if(final_table.isInTable(rhs)) {
    //     auto var = final_table.getNode(rhs);
    //     string offset = "-" + to_string(var->offset);
    //     op1 = offset + "(%rbp)";    // variable on stack
    // }
    // else {
    //     op1 = "$" + rhs;
    // }
    // auto var = final_table.getNode(res);
    // string res_offset = "-" + to_string(var->offset);
    // string res_op = res_offset + "(%rbp)";
    op1 = get_asm_symbol(rhs);
    res_op = get_asm_symbol(res);
    gen.movl(op1, "%eax");
    gen.movl("%eax", res_op);
}

pair<string, string> get_array_and_index(string var) {  // get a and i from a[i]
     string array_name = "", index = "";
    bool flag = false;
    for(auto& x : var) {
        if(x == '[') {
            flag = true;
            continue;
        } 
        if(x == ']') {
            break;
        }
        if(flag) {
            index += x;
        }
        else {
            array_name += x;
        }
    }
    return {array_name, index};
}

void array_assign_code_gen(string res, string rhs) {
    if(res.back() == ']') {
        // array on lhs
        string array_name, index;
        string op1, op2, op3;
        auto xx = get_array_and_index(res);
        array_name = xx.first, index = xx.second; 
        op1 = get_asm_symbol(array_name);
        op2 = get_asm_symbol(index);
        op3 = get_asm_symbol(rhs);
        string array_offset = to_string(final_table.getNode(array_name)->offset); 
        gen.movq("%rbp", "%rdx");
        gen.subq("$" + array_offset, "%rdx");   
        gen.movl(op2, "%eax");
        gen.negl("%eax");
        gen.cltq();
        gen.movl(op3, "%ecx");
        gen.movl("%ecx", "(%rdx, %rax, 4)");
    }
    else {
        //  array on rhs
        string array_name, index;
        string op1, op2, op3;
        auto xx = get_array_and_index(rhs);
        array_name = xx.first, index = xx.second; 
        op1 = get_asm_symbol(array_name);
        op2 = get_asm_symbol(index);
        op3 = get_asm_symbol(res);
        string array_offset = to_string(final_table.getNode(array_name)->offset); 
        gen.movq("%rbp", "%rdx");
        gen.subq("$" + array_offset, "%rdx");   
        gen.movl(op2, "%eax");
        gen.negl("%eax");
        gen.cltq();
        gen.movl("(%rdx, %rax, 4)", "%ecx");
        gen.movl("%ecx", op3);
    }
}

void function_call_code_gen(string res, string func_name, string n_args) {
    gen.call(func_name);
    string op1 = get_asm_symbol(res);
    gen.movl("%eax", op1); // return value
    int n = to_integer(n_args);
    while(n > 0) {
        gen.popq("%rbx");
        n--;
    }
} 

void printf_code_gen(string arg) {
    string op = get_asm_symbol(arg);
    gen.movl(op, "%esi");
    gen.leaq(".LC0(%rip)", "%rdi");
    gen.movl("$0", "%eax");
    gen.call("printf@PLT");
}

void scanf_code_gen(string arg) {
    string op = get_asm_symbol(arg);
    gen.leaq(op, "%rax");
    gen.movq("%rax", "%rsi");
    gen.leaq(".LC1(%rip)", "%rdi");
    gen.movl("$0", "%eax");
    gen.call("scanf@PLT");
}

void parseStatement(string& line, int line_no) {
    stringstream ss(line);
    string s;
    bool if_assignment = false; // check if this is an assignment statement, search for equal
    vector<string> statement;
    while(ss >> s) {
       if(s == "=") {
           if_assignment = true;
       }
       statement.push_back(s);
    }
    if(if_assignment) {
        // handle this statement
        string res; // result of the statement
        string op;  // operator
        res = statement[0];
        vector<string> operands;  // one or two operands
        if(statement.size() == 5) {
            // binary op
            op = statement[3];
            operands.push_back(statement[2]);
            operands.push_back(statement[4]);
            binary_assign_code_gen(op, res, operands, line_no);
        }
        else if(statement.size() == 4) {
            // unary op
            op = statement[2];
            operands.push_back(statement[3]);
            unary_assign_code_gen(op, res, operands);
        }
        else if(statement.size() == 3){
            // simple assignent, array assignment
            op = "=";
            operands.push_back(statement[2]);
            if(res.back() == ']' or statement[2].back() == ']') {
                array_assign_code_gen(res, statement[2]);
            }
            else {
                assign_code_gen(res, operands[0]);
            }
        }
        else {
            // function call and assignment
            function_call_code_gen(res, statement[4], statement[5]);
        }
    }
    else {
        string s = statement[0];
        if(s[0] == '.' and s.back() == ':') {
            // label
            gen.putLabel(s);
        }
        else if(s.back() == ':') {
            // function label
            gen.putLabel(s);
            auto node = final_table.getNode("@" + s.substr(0, s.size() - 1));
            gen.function_template(node->offset);
        }
        else if(s == "if" or s == "ifFalse") {
            // conditional jump
            string op1 = get_asm_symbol(statement[1]);
            string label = statement[3];
            if(s == "if") {
                gen.movl(op1, "%eax");
                gen.cmpl("$0", "%eax");
                gen.jne(label);
            }
            else {
                gen.movl(op1, "%eax");
                gen.cmpl("$0", "%eax");
                gen.je(label);
            }
        } 
        else if(s == "goto") {
            // unconditional jump
            string label = statement[1];
            gen.jmp(label);
        }
        else if(s == "end") {
            // end of function
            gen.function_return_template();
        }
        else if(s == "printf") {
            // printf call
            string arg = statement[1];
            printf_code_gen(arg);            
        }
        else if(s == "scanf") {
            // scanf call
            string arg = statement[1];
            scanf_code_gen(arg);
        }
        else if(s == "return") {
            string op = get_asm_symbol(statement[1]);
            gen.movl(op, "%eax");
    	    AScode << "      " << "leave" << endl;
    	    AScode << "      " << "ret" << endl;
        }
        else if(s == "param") {
            string op = get_asm_symbol(statement[1]);
            gen.pushq(op);
        }
    }
}

void codeGenerator() {
    AScode.open("assembly.s");  // assembly code for x86
    ifstream IRcode("a_opt.ir");
    char* buffer = new char[MAXN];
    gen.begin();    
    int l = 1;
    while(IRcode.getline(buffer, MAXN, '\n')) {
        string s = buffer;
        parseStatement(s, l);
        l++;
    }
    AScode.close();
    summary << "unused-vars" << endl;
    for(auto& e : unused_var) {
        summary << e.second << endl;
    }
    summary << endl;
    summary << "if-simpl" << endl;
    summary << if_simpl << endl;
    if(if_simpl != "") {
        summary << endl;
    }
    summary << "strength-reduction" << endl;
    for(auto p : str_red) {
        if(!if_line_nos.count(p.first) or if_simpl == "") {
            summary << p.first << ' ';
            int64_t mx = -1e18;
            for(auto& e : p.second) {
                mx = max((int64_t)e, mx);
            }
            summary << mx << endl;
        }
    }
    summary << endl;
    summary << "constant-folding" << endl;
    for(auto p : const_folding) {
        if(!if_line_nos.count(p.first) or if_simpl == "") {
            summary << p.first << ' ';
            int64_t mx = -1e18;
            for(auto& x : p.second) {
                mx = max(mx, (int64_t)to_integer(x));
            }
            summary << mx << endl;
        }
    } 
    summary << endl;
    summary << "constant-propagation" << endl;
    for(auto& e : const_prop) {
        if(!if_line_nos.count(e.first) or if_simpl == "") {
            summary << e.first << ' ';
            for(auto& p : e.second) {
                summary << p.second.first << ' ' << p.second.second << ' ';
            }   
            summary << endl;
        }
    }
    summary << endl;
    summary << "cse" << endl;
    for(auto& e : cse) {
        if(!if_line_nos.count(e.first.first) or if_simpl == "") {
            summary << e.first.first << ' ';
            e.second.erase(e.first.first);
            for(auto& x : e.second) {
                if(!if_line_nos.count(x) or if_simpl == "") {
                    summary << x << ' ';
                }
            }
            summary << endl;
        }
    }
}
