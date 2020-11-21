#include "symbolTable.hpp"

extern symbolTable final_table;

#ifndef OPT
#define OPT

ofstream optIRcode;


map<pair<int, string>, set<int>> cse;   // common subexpression elimination
set<pair<int, string>> unused_var;             // unused variables

extern unordered_map<int, int> line_map;
extern map<int, vector<string>> const_folding;  // answer for constant folding
extern map<int, set<pair<int, pair<string, string>>>> const_prop;  // answer for constant propagation
extern map<int, vector<int>> str_red;  // strength reduction
extern pair<string, string> get_array_and_index(string var);

constexpr int maxn = 1000;

class Expression {
    public :
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
    string evaluate(string a, string b, string op) {    // binary operator
        int x = to_integer(a);
        int y = to_integer(b);
        if(op == "MULT") {
            return to_string(x * y);
        }
        if(op == "PLUS") {
            return to_string(x + y);
        }
        if(op == "MINUS") {
            return to_string(x - y);
        }
        if(op == "DIV") {
            return to_string(x / y);
        }
        if(op == "MOD") {
            return to_string(x % y);
        }
        if(op == "LT") {
            return to_string(x < y);
        }
        if(op == "GT") {
            return to_string(x > y);
        }
        if(op == "LEQ") {
            return to_string(x <= y);
        }
        if(op == "GEQ") {
            return to_string(x >= y);
        }
        if(op == "EQEQ") {
            return to_string(x == y);
        }
        if(op == "NEQ") {
            return to_string(x != y);
        }
        if(op == "AND") {
            return to_string(x && y);
        }
        if(op == "OR") {
            return to_string(x || y);
        }
        if(op == "SAL1" or op == "SAL2") {
            return to_string(y << x);
        }
        return "";
    }
    string evaluate(string a, string op) {   // unary operator
        int x = to_integer(a);
        if(op == "MINUS") {
            return to_string(-x);
        }
        if(op == "PLUS") {
            return to_string(x);
        }
        if(op == "NOT") {
            return to_string(!x);
        }
        return "";
    }
    bool isConstant(string s) {
        if(!final_table.isInTable(s)) {
            return true;
        }
        return false;
        // auto node = final_table.getNode(s);
        // return node->isConstant;
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
};

class ExpressionCell {    // binary operators for now, like quadruple sort of representation
    public:
        vector<string> cell;
        int line_no;
        ExpressionCell(vector<string> v, int line_no) {
            cell = v;
            this->line_no = line_no;
        }
        bool match(ExpressionCell x) {    // matches the RHS of 2 expressions, checks if the same symbols occur in the same order
                                          // starts with index 1 to not consider LHS
            if(x.size() != cell.size()) {
                return false;
            }
            for(int i = 1; i < cell.size(); i++) {
                if(x[i] != cell[i]) {
                    return false;
                }
            }
            return true;
        }
        bool complete_match(ExpressionCell x) {    // checks if 2 expressions are completely identical
            if(x.size() != cell.size()) {
                return false;
            }
            for(int i = 0; i < cell.size(); i++) {
                if(x[i] != cell[i]) {
                    return false;
                }
            }
            return true;
        }
        int size() {
            return cell.size();
        }
        string operator[](int i) {
            return cell[i];
        }
        bool isInCell(string var) {
            for(auto& e : cell) {
                if(e == var) {
                    return true;
                }
            }
            return false;
        }
};

class ExpressionTable {
    // some data structure of expression cells
    public :
    vector<ExpressionCell> table;  // keeps all active/live expressions in the block
                                   // assignment of a variable in the block kill alls previous expressions involving that variable
    Expression evaluator;   // to handle expression tasks
    
    void kill(string var) {
        // remove all cells that refer to var
        vector<ExpressionCell> new_table;
        for(auto cell : table) {
            if(!cell.isInCell(var)) {
                new_table.push_back(cell);
            }
        }
        table = new_table;
    }
    void insert(ExpressionCell x) {
        table.push_back(x);
    }
    bool check_matching_subexpression(ExpressionCell x) {  /* For Common Subexpression Elimination */
        for(auto cell : table) {
            if(cell.match(x)) {
                return true;
            }
        }
        return false;
    }
    bool check_complete_match(ExpressionCell x) {
        for(auto cell : table) {
            if(cell.complete_match(x)) {
                return true;
            }
        }
        return false;
    }
    ExpressionCell find_matching_subexpression(ExpressionCell x) {   /* For Common Subexpression Elimination */
        for(auto cell : table) {
            if(cell.match(x)) {
                return cell;
            }
        }
        cout << "An Issue here : Expression Table" << endl;
        return ExpressionCell({""}, 0);  /* return some nonsense expression cell, NEVER REACH HERE */
    }

    bool check_for_assignment(string var) {  /* For Constant Propogation */
        for(auto cell : table) {
            if(cell[0] == var and cell.size() == 2) {
                return true;
            }
        }
        return false;
    }

    string find_assignment(string var) {  /* For Constant Propogation */
        for(auto cell : table) {
            if(cell[0] == var and cell.size() == 2) {
                return cell[1];
            }
        }
        return "";  /* return some nonsense expression, NEVER REACH HERE */
    }

    void merge(ExpressionTable x) {  /* FOR NOW MERGING ONLY ASSIGNMENTS, THAT TOO IF THEY ARE EQUAL, some issues still */
        if(table.size() == 0) {
            table = x.table;
        }
        else {
            vector<ExpressionCell> new_table;
            for(auto cell : x.table) {
                if(check_complete_match(cell)) {
                    new_table.push_back(cell);
                }
            }
            table = new_table;
        }
    }
};

class BasicBlock {
    public:
    vector<string> statements;
    string leader;   // leader of the basic block
    int leader_line_no;   // line number of the leader in the source code
    ExpressionTable table;   // some sort of expression DAG of the block
    BasicBlock(int line_no = 1) {
        leader_line_no = line_no;
    }
    BasicBlock(string leader_statement, int line_no = 1) {
        leader = leader_statement;
        leader_line_no = line_no;
        statements.push_back(leader_statement);
    }
    int size() {
        return statements.size();
    }
    void push(string statement) {
        statements.push_back(statement);
    }
    void clear() {
        statements.clear();
        leader = "";
    }
    void debug() {
        int block_line_no = 0;
        for(auto& line : statements) {
            cout << line << ' ' << "[" << leader_line_no + block_line_no << " , " << line_map[leader_line_no + block_line_no] << "]" << endl;  
            block_line_no++;
        }   
    }

    void generate() {
        for(auto& line : statements) {
            optIRcode << line << endl;
        }
    }

    string optimize_line(string& line, int line_no = 1) {   /* returns optimized line if possible, or returns the same line otherwise */     
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
                string op1 = operands[0];
                string op2 = operands[1];
                if(table.check_for_assignment(op1)) {
                    op1 = table.find_assignment(op1);
                    if(table.evaluator.isConstant(op1)) {
                        //CP
                        if(operands[0][0] != '$') {
                            // summary << "Constant Propogation at line " << line_map[line_no] << " variable " << operands[0] << " = " << op1 << endl;
                            auto node = final_table.getNode(operands[0]);
                            const_prop[line_map[line_no]].insert({node->line_no, {node->lexValue, op1}});
                        }
                    }
                }
                if(table.check_for_assignment(op2)) {
                    op2 = table.find_assignment(op2);
                    if(table.evaluator.isConstant(op2)) {
                        //CP
                        if(operands[1][0] != '$') {
                            // summary << "Constant Propogation at line " << line_map[line_no] << " variable " << operands[1] << " = " << op2 << endl;
                            auto node = final_table.getNode(operands[1]);
                            const_prop[line_map[line_no]].insert({node->line_no, {node->lexValue, op2}});
                        }
                    }
                }
                ExpressionCell cell({res, op1, op, op2}, line_no);
                bool flag = true;
                if(table.evaluator.isConstant(op1) and table.evaluator.isConstant(op2)) {
                    flag = false;
                }
                else if(op == "MULT") {
                    /*Possiblilty Of Strength reduction */
                    if(table.evaluator.isConstant(op1) and table.evaluator.check_power_of_2(op1) != -1) {
                        int shift = table.evaluator.check_power_of_2(op1);
                        cell = * new ExpressionCell({res, to_string(shift), "SAL1", op2}, line_no);
                        str_red[line_map[line_no]].push_back(shift);
                    }
                    else if(table.evaluator.isConstant(op2) and table.evaluator.check_power_of_2(op2) != -1) {
                        int shift = table.evaluator.check_power_of_2(op2);
                        cell = * new ExpressionCell({res, to_string(shift), "SAL2", op1}, line_no);
                        str_red[line_map[line_no]].push_back(shift);
                    }
                }
                
                if(table.check_matching_subexpression(cell) and flag) {
                    // CSE
                    ExpressionCell same_cell = table.find_matching_subexpression(cell);
                    string new_line = res + " = " + same_cell[0];
                    table.kill(res);
                    ExpressionCell new_cell({res, same_cell[0]}, line_no);
                    table.insert(new_cell);
                    // summary << "Common Subexpression elimination " << line_map[same_cell.line_no] << ' ' << line_map[line_no] << endl; 
                    cse[{line_map[same_cell.line_no], same_cell[0]}].insert(line_map[line_no]);
                    if(res[0] == '$' and same_cell[0][0] == '$') {
                        return "";
                    }
                    return new_line;
                }
                else {
                    if(!flag) {
                        // constant folding
                        string val = table.evaluator.evaluate(op1, op2, op);
                        string new_line = res + " = " + val;
                        // summary << "Constant folding at line " << line_map[line_no] << " value = " << val << endl;
                        const_folding[line_map[line_no]].push_back(val);
                        table.kill(res);
                        ExpressionCell new_cell({res, val}, line_no);
                        table.insert(new_cell);
                        return new_line;
                    }
                    else {
                        string new_line = res + " = " + op1 + " " + op + " " + op2;
                        table.kill(res);
                        table.insert(cell);
                        return new_line; 
                    }
                }
            }
            else if(statement.size() == 4) {
                // unary op
                op = statement[2];
                operands.push_back(statement[3]);
                string op1 = operands[0];
                if(table.check_for_assignment(op1)) {
                    op1 = table.find_assignment(op1);
                    if(table.evaluator.isConstant(op1)) {
                        //CP
                        if(operands[0][0] != '$') {
                            // summary << "Constant Propogation at line " << line_map[line_no] << " variable " << operands[0] << " = " << op1 << endl;
                            auto node = final_table.getNode(operands[0]);
                            const_prop[line_map[line_no]].insert({node->line_no, {node->lexValue, op1}});
                        }
                    }
                }
                ExpressionCell cell({res, op, op1}, line_no);
                bool flag = true;
                if(table.evaluator.isConstant(op1)) {
                    flag = false;
                }
                if(table.check_matching_subexpression(cell) and flag) {
                    // CSE
                    ExpressionCell same_cell = table.find_matching_subexpression(cell);
                    string new_line = res + " = " + same_cell[0];
                    table.kill(res);
                    ExpressionCell new_cell({res, same_cell[0]}, line_no);
                    table.insert(new_cell);
                    // summary << "Common Subexpression elimination " << line_map[same_cell.line_no] << ' ' << line_map[line_no] << endl;
                    cse[{line_map[same_cell.line_no], same_cell[0]}].insert(line_map[line_no]);
                    return new_line;
                }
                else {
                    if(flag) {
                        string new_line = res + " = " + op + " " + op1;
                        table.kill(res);
                        table.insert(cell);
                        return new_line;
                    }
                    else {
                        // constant folding
                        string val = table.evaluator.evaluate(op1, op);
                        string new_line = res + " = " + val;
                        // summary << "Constant folding at line " << line_map[line_no] << " value = " << val << endl;
                        const_folding[line_map[line_no]].push_back(val);
                        table.kill(res);
                        ExpressionCell new_cell({res, val}, line_no);
                        table.insert(new_cell);
                        return new_line;
                    }
                }
            }
            else if(statement.size() == 3){
                // simple assignent, array assignment
                op = "=";
                operands.push_back(statement[2]);
                if(res.back() == ']' or statement[2].back() == ']') {
                    // array assign 
                    // risky, lets leave it for now
                }
                else {
                    // assign_code_gen(res, operands[0]);
                    string op1 = statement[2];
                    if(table.check_for_assignment(op1)) {
                        op1 = table.find_assignment(op1);
                        if(table.evaluator.isConstant(op1)) {
                            //CP
                            if(operands[0][0] != '$') {
                                // summary << "Constant Propogation at line " << line_map[line_no] << " variable " << operands[0] << " = " << op1 << endl;
                                auto node = final_table.getNode(operands[0]);
                                const_prop[line_map[line_no]].insert({node->line_no, {node->lexValue, op1}});
                            }
                        }
                    }
                    ExpressionCell cell({res, op1}, line_no);
                    bool flag = true;
                    if(table.evaluator.isConstant(op1)) {
                        flag = false;
                    }
                    string new_line = res + " = " + op1;
                    table.kill(res);
                    table.insert(cell);
                    return new_line;
                }
            }
            else {
                // function call and assignment
            }
        }
        else {
            string s = statement[0];
            if(s[0] == '.' and s.back() == ':') {  
                // label
                
            }
            else if(s.back() == ':') {
                // function label
    
            }
            else if(s == "if" or s == "ifFalse") {  /*A conditional/ unconditional jump is the last statement of the block*/
                // conditional jump
        
            } 
            else if(s == "goto") {
                // unconditional jump
    
            }
            else if(s == "end") {
                // end of function
            }
            else if(s == "printf") {
                // printf call
                string arg = statement[1];
                if(table.check_for_assignment(arg)) {
                    string val = table.find_assignment(arg);
                    if(table.evaluator.isConstant(val)) {
                        auto node = final_table.getNode(arg);
                        string new_line = "printf " + val;
                        const_prop[line_map[line_no]].insert({node->line_no, {node->lexValue, val}});
                        return new_line;
                    }
                }
                
            }
            else if(s == "scanf") {
                // scanf call
                string arg = statement[1];
                table.kill(arg);
            }
            else if(s == "return") {
                // string op = get_asm_symbol(statement[1]);
            }
            else if(s == "param") {
                // string op = get_asm_symbol(statement[1]);
            }
        }
        return line;
    }
    void optimize_block() {
       /* RUN THROUGH THE STATEMENTS, do constant propogation and common subexpression elimination */ 
        vector<string> new_statements;
        int block_line_no = 0;
        for(auto line : statements) {
            string new_line = optimize_line(line, leader_line_no + block_line_no);
            if(new_line != "") {
                new_statements.push_back(new_line);
            }
            block_line_no++;
        }
        statements = new_statements;
    }
};  

/**
 * Optimization Strategy -
 * Constant folding ---> done at IR generation stage
 * Unused Variables ---> Analysis on IR code
 * Constant Propogation ---> Done after generating IR
 * Common Subexpression Elimination ---> Done after Generating IR
 * Strength reduction ---> Done at Code Generation
 * if simplification ---> Not sure, not to mess with while loops (mostly done at IR generation)
**/

ifstream IRcode;

set<string> all_variables;

BasicBlock curr_block;
vector<BasicBlock> blocks;
unordered_map<string, int> label_block;   // tells in which block the current label belongs to
vector<vector<int>> CFG;                  // control flow graph of the program ---> construct after if-simpl
vector<vector<string>> jump_targets;      // jump targets for goto from current block
vector<set<int>> parent;               // parents of the current block

/* ALMOST A COPY OF THAT IN CODE GENERATOR */ 
/* MAKES BASIC BLOCKS FOR FURTHER ANALYSIS AND FINDS UNUSED VARIABLES */
void constructBasicBlocks(string& line, int line_no) {     
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
            all_variables.erase(res);
            all_variables.erase(operands[0]);
            all_variables.erase(operands[1]);
        }
        else if(statement.size() == 4) {
            // unary op
            op = statement[2];
            operands.push_back(statement[3]);
            all_variables.erase(res);
            all_variables.erase(operands[0]);
        }
        else if(statement.size() == 3){
            // simple assignent, array assignment
            op = "=";
            operands.push_back(statement[2]);
            if(res.back() == ']' or statement[2].back() == ']') {
                // array assign 
                if(res.back() == ']') {
                    auto xx = get_array_and_index(res);
                    all_variables.erase(xx.first);
                    all_variables.erase(xx.second);
                }
                else {
                    auto xx = get_array_and_index(statement[2]);
                    all_variables.erase(xx.first);
                    all_variables.erase(xx.second);
                }
            }
            else {
                // assign_code_gen(res, operands[0]);
                all_variables.erase(res);
                all_variables.erase(operands[0]);
            }
        }
        else {
            // function call and assignment
            all_variables.erase(res);
        }
        curr_block.push(line);
    }
    else {
        string s = statement[0];
        if(s[0] == '.' and s.back() == ':') {  /* EVERY LABEL is a leader */
            // label
            if(curr_block.size() > 0) {
                blocks.push_back(curr_block);
                curr_block = * new BasicBlock(line, line_no);
                label_block[s.substr(0, s.length() - 1)] = blocks.size();
                parent[blocks.size()].insert(blocks.size() - 1);
                CFG[blocks.size() - 1].push_back(blocks.size());
            }
            else {
                curr_block.push(line);
                label_block[s.substr(0, s.length() - 1)] = blocks.size();
            }
        }
        else if(s.back() == ':') {
            // function label
            all_variables.erase("@" + s.substr(0, s.size() - 1));
            if(curr_block.size() > 0) {
                blocks.push_back(curr_block);
                curr_block = * new BasicBlock(line, line_no);
                label_block[s.substr(0, s.length() - 1)] = blocks.size();
                parent[blocks.size()].insert(blocks.size() - 1);
                CFG[blocks.size() - 1].push_back(blocks.size());
            }
            else {
                curr_block.push(line);
                label_block[s.substr(0, s.length() - 1)] = blocks.size();
            }
        }
        else if(s == "if" or s == "ifFalse") {  /*A conditional/ unconditional jump is the last statement of the block*/
            // conditional jump
            all_variables.erase(statement[1]);
            curr_block.push(line);
            blocks.push_back(curr_block);
            curr_block = * new BasicBlock(line_no + 1);
            CFG[blocks.size() - 1].push_back(blocks.size());
            parent[blocks.size()].insert(blocks.size() - 1);
            jump_targets[blocks.size() - 1].push_back(statement[3]);
        } 
        else if(s == "goto") {
            // unconditional jump
            curr_block.push(line);
            blocks.push_back(curr_block);
            curr_block = * new BasicBlock(line_no + 1);
            jump_targets[blocks.size() - 1].push_back(statement[1]);
        }
        else if(s == "end") {
            // end of function
            // gen.function_return_template();
            curr_block.push(line);
        }
        else if(s == "printf") {
            // printf call
            string arg = statement[1];
            // printf_code_gen(arg);         
            all_variables.erase(arg);  
            curr_block.push(line);
        }
        else if(s == "scanf") {
            // scanf call
            string arg = statement[1];
            // scanf_code_gen(arg);
            all_variables.erase(arg);
            curr_block.push(line);
        }
        else if(s == "return") {
            // string op = get_asm_symbol(statement[1]);
            all_variables.erase(statement[1]);
            curr_block.push(line);
            blocks.push_back(curr_block);
            curr_block = * new BasicBlock();
            /*No interprocedural analysis, so leave it*/
        }
        else if(s == "param") {
            // string op = get_asm_symbol(statement[1]);
            all_variables.erase(statement[1]);
            curr_block.push(line);
        }
    }
}

void construct_CFG() {
    for(int i = 0; i < blocks.size(); i++) {
        for(auto label : jump_targets[i]) {
            CFG[i].push_back(label_block[label]);
            parent[label_block[label]].insert(i);
        }
    }
}

void print_CFG() {
    for(int i = 0; i < blocks.size(); i++) {
        cout << i + 1 << ' ';
        for(auto j : CFG[i]) {
            cout << j + 1 << ' ';
        }
        cout << endl;
    }
}

void data_flow_analysis() {
    vector<int> in_degree(blocks.size());
    for(int i = 0; i < blocks.size(); i++) {
        in_degree[i] = parent[i].size();
        // cout << i + 1 << ' ';
        // for(auto& e : parent[i]) {
        //     cout << e + 1 << ' ';
        // }
        // cout << endl;
    }
    queue<int> q;   // top sort
    for(int i = 0; i < in_degree.size(); i++) {
        if(in_degree[i] == 0) {
            q.push(i);
        }
    }
    while(!q.empty()) {
        int curr = q.front();
        q.pop();
        for(auto par : parent[curr]) {
            blocks[curr].table.merge(blocks[par].table);   /* Merge DAGS of parents to get reaching definitions */
        }
        blocks[curr].optimize_block();
        blocks[curr].generate();
        for(auto child : CFG[curr]) {
            in_degree[child]--;
            if(in_degree[child] == 0) {
                q.push(child);
            }
        }
    }
}

void optimzeIR() {
    IRcode.open("a.ir");
    for(auto& e : final_table.table) {
        all_variables.insert(e.first);
    }   
    for(auto& e : all_variables) {
        auto node = final_table.getNode(e);
        // cout << e << ' ' << node->lexValue << " , " << node->line_no << endl;
    }
    char* buffer = new char[maxn];   
    int l = 1;
    CFG.resize(100);   // rough estimate of the number of blocks
    parent.resize(100);   
    jump_targets.resize(100);

    while(IRcode.getline(buffer, maxn, '\n')) {
        string s = buffer;
        constructBasicBlocks(s, l);
        l++;
    }
    IRcode.close();
    // summary << "The following variables are unused" << endl;
    for(auto& e : all_variables) {
        // summary << final_table.getNode(e)->lexValue << endl;
        auto node = final_table.getNode(e);
        unused_var.insert({node->line_no, node->lexValue});
    }
    if(curr_block.size() > 0) {
        blocks.push_back(curr_block);
        // end block, can be ignored
        if(blocks.size() >= 2) {
            CFG[blocks.size() - 2].push_back(blocks.size() - 1);
            parent[blocks.size() - 1].insert(blocks.size() - 2);
        }
    }
    construct_CFG();
    // print_CFG();
    optIRcode.open("a_opt.ir");
    // for(int i = 0; i < blocks.size(); i++) {
    //     blocks[i].optimize_block();
    //     cout << "Start Block " << i + 1 << " ---------- " << endl;
    //     blocks[i].debug();
    //     cout << "End Block " << i + 1 << " ---------- " << endl;
    //     blocks[i].generate();
    // }
    data_flow_analysis();
    optIRcode.close();
}

#endif

