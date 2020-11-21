/*
    * Abstract Syntax Tree as input
    * Convert to 3 Address Code incrementally
    * File generated is a.ir
*/

#include <bits/stdc++.h>
#include "treeNode.hpp"
#include "symbolTable.hpp"
#include "codeGenerator.hpp"
#include "optimizer.hpp"

using namespace std;

long long int label_count = 1ll;
long long int temp_count = 1ll;

extern treeNode* ast;

Expression evaluator;  // from optimizer.hpp

ofstream IR;        // file containing IR

ofstream summary;   // summary of the optimizations
string if_simpl = "";       // answer of if simpl 
map<int, vector<string>> const_folding;  // answer for constant folding
map<int, set<pair<int, pair<string, string>>>> const_prop;  // answer for constant propagation
map<int, vector<int>> str_red;  // strength reduction

symbolTableStack env;    // current symbol table environment
symbolTable final_table;    // used for code generation

unordered_map<int, int> line_map;
int ir_line_count = 0;
set<int> if_line_nos;   // set of line numbers at which if statement appears

int table_offset = 0;
int table_arg_offset = 0;
int depth_counter = 0;

void printAST(treeNode* root, string prefix = "", bool isLast = true) {
    if(root == NULL) {
        return;
    }
    cout << prefix;
    if(isLast) {
        cout << "└───────";
    }
    else {
        cout << "├───────";
    }
    if(root->nodeName == "identifier") {
        cout << root->lexValue << endl;
    }
    else {
        cout << "(" << root->nodeName << " , "  << root->line_no << ")" << endl;
    }
    for(int i = 0; i < root->children.size(); i++) {
        if(i == root->children.size() - 1) {
            printAST(root->children[i], prefix + "|        ", true);
        }
        else {
            printAST(root->children[i], prefix + "|        ", false);
        }
    }
}

string newLabel() {   // returns new label
    string LABEL = ".L";
    string x = to_string(label_count);
    LABEL += x;
    label_count++;
    return LABEL;
}

string newTemp() {  // returns new temp variable for 3 address code
    string TEMP = "$t";
    string x = to_string(temp_count);
    TEMP += x;
    temp_count++;
    return TEMP; 
}

void setInherited(treeNode* root) {
    if(root->nodeName == "func_decl") {
        depth_counter = 0;
        root->depth = depth_counter;
        root->func_name = root->children[1]->lexValue;
    }
    if(root->nodeName == "compound_stmt") {
        depth_counter++;
        for(auto& child : root->children) {
            child->depth = depth_counter;
        }
    }
    else {
        for(auto& child : root->children) {
            child->depth = root->depth;
        }
    }
    for(auto& child : root->children) {
        child->func_name = root->func_name;
    }

}

void traverseAST(treeNode* root) {
    /**
     * Preprocess the current node
     * var_decl
     * local_decl
     * func_decl
     * assign_stmt
     * printf_stmt
     * if_stmt
     * while_stmt
     * return_stmt
     * expr
     * Pexpr
    **/
    setInherited(root);
    string name = root->nodeName;
    if(name == "var_decl") {
        // cout << "I am at " << name << endl;
    }
    if(name == "local_decl") {
        // cout << "I am at " << name << endl;
        // cout << "Local Declaration of " << root->children[1]->lexValue << " at " << root->line_no << endl;
        if(root->children.size() == 3) {    // non-array declaration
            for(auto& child : root->children) {
                traverseAST(child);
            }
            auto child1 = root->children[1];
            child1->width = root->children[0]->width;
            child1->symbolTableName = child1->lexValue + "_" + child1->func_name + "_" + to_string(child1->depth);
            child1->offset = table_offset;
            table_offset += child1->width;
            env.set(child1->lexValue, child1);
            final_table.set(child1->symbolTableName, child1);
        }    
        else {  // array declaration
            for(auto& child : root->children) {
                traverseAST(child);
            }        
            auto child1 = root->children[1];
            string arr_length = root->children[3]->children[0]->children[0]->lexValue;   // assuming that the length of the array is an integer_lit
            child1->width = 4 * to_integer(arr_length);
            child1->symbolTableName = child1->lexValue + "_" + child1->func_name + "_" + to_string(child1->depth);
            child1->offset = table_offset;
            table_offset += child1->width;
            env.set(child1->lexValue, child1);
            final_table.set(child1->symbolTableName, child1);
        }
    }
    else if(name == "func_decl") {
            // cout << "I am at " << name << endl;
            table_offset = 4;
            table_arg_offset = 16;
            symbolTable top;
            env.addTable(top);
            ir_line_count++;
            IR << root->children[1]->lexValue <<  ":" << endl;
            line_map[ir_line_count] = root->line_no;
            // cout << "Function " << root->children[1]->lexValue << " at " << root->line_no << endl;
            for(auto& child : root->children) {
                traverseAST(child);
            }
            root->offset = table_offset;
            root->lexValue = root->children[1]->lexValue;
            final_table.set("@" + root->children[1]->lexValue, root);
            env.removeTop();
            ir_line_count++;
            IR << "end" << endl;  // function code finishes
            line_map[ir_line_count] = root->children[2]->line_no;
    }
    else if(name == "param") {
         if(root->children.size() == 2) {    // non-array paramter
            for(auto& child : root->children) {
                traverseAST(child);
            }
            auto child1 = root->children[1];
            child1->width = root->children[0]->width;
            child1->symbolTableName = child1->lexValue + "_" + child1->func_name + "_" + to_string(child1->depth);
            child1->offset = table_arg_offset;
            child1->isArg = true;
            table_arg_offset += 8;
            env.set(child1->lexValue, child1);
            final_table.set(child1->symbolTableName, child1);
        }       
    }
    else if(name == "assign_stmt") {
        //    cout << "I am at " << name << endl;
        for(auto& child : root->children) {
               traverseAST(child);
        }
        auto op_child = root->children[0];
        // cout << "Assignment to " << op_child->children[0]->lexValue << " at " << root->line_no << endl;
        if(op_child->children.size() == 2) {
            string code = env.get(op_child->children[0]->lexValue) + " = " + op_child->children[1]->addr;
            ir_line_count++;
            IR << code << endl;
            line_map[ir_line_count] = root->line_no;
            if(evaluator.isConstant(op_child->children[1]->addr)) {
                auto node = final_table.getNode(env.get(op_child->children[0]->lexValue));
                node->isConstant = true;
                node->constant_val = op_child->children[1]->addr;  
                if(op_child->children[1]->isFolded) {
                    // summary << "Constant Folding at line " << root->line_no << " value = " << node->constant_val << endl;
                    const_folding[root->line_no].push_back(node->constant_val);
                }         
            }
            else {
                auto node = final_table.getNode(env.get(op_child->children[0]->lexValue));
                node->isConstant = false;
            }
            // cout << root->code << endl;
         }
         else {          // array assignment to be handled
            string code = env.get(op_child->children[0]->lexValue) + "[" + op_child->children[2]->addr + "]" + " = " + op_child->children[4]->addr;
            ir_line_count++;
            IR << code << endl;
            line_map[ir_line_count] = root->line_no;
        }
    }
    else if(name == "compound_stmt") {
        symbolTable top;
        env.addTable(top);
        for(auto& child : root->children) {
            traverseAST(child);
        }
        env.removeTop();
    }
    else if(name == "print_stmt") {
        // cout << "I am at " << name << endl;
        for(auto& child : root->children) {
            traverseAST(child);
        }
        string arg = env.get(root->children[4]->lexValue);
        auto node = final_table.getNode(arg);
        if(node->isConstant) {
            ir_line_count++;
            IR << "printf" << " " << node->constant_val << endl;
            const_prop[root->line_no].insert({node->line_no, {root->children[4]->lexValue, node->constant_val}});
            line_map[ir_line_count] = root->line_no;
        }
        else {
            ir_line_count++;
            IR << "printf" << " " << arg << endl;
            line_map[ir_line_count] = root->line_no;
        }
    }
    else if(name == "scan_stmt") {
        for(auto& child : root->children) {
            traverseAST(child);
        }
        string arg = env.get(root->children[5]->lexValue);
        ir_line_count++;
        IR << "scanf" << " " << arg << endl;
        line_map[ir_line_count] = root->line_no;
        auto node = final_table.getNode(arg);
        node->isConstant = false;
    }
    else if(name == "if_stmt") {
        if(root->children.size() == 5) {   // if but no else
            string l1 = newLabel();
            traverseAST(root->children[2]);  // expression
            /* IF SIMPL */
            if_line_nos.insert(root->line_no);
            if(evaluator.isConstant(root->children[2]->addr)) {
                if(root->children[2]->addr == "0") {
                    // skip code completely, never executes
                    if_simpl = "0";
                }
                else {
                    traverseAST(root->children[4]);  // statement 
                    // statement will always execute
                    if_simpl = "1";
                }
                // summary << "IF Simplification at line " << root->line_no << endl;
                if(root->children[2]->isFolded) {
                    const_folding[root->line_no].push_back(root->children[2]->addr);
                }
            }
            else {
                string code3 = "ifFalse " + root->children[2]->addr + " goto " + l1; 
                ir_line_count++;
                IR << code3 << endl;
                line_map[ir_line_count] = root->line_no;
                traverseAST(root->children[4]);  // statement   
                ir_line_count++;
                string final_code = l1 + ":";
                line_map[ir_line_count] = root->children[0]->line_no - 1;
                IR << final_code << endl;
                for(auto e : final_table.table) {
                    e.second->isConstant = false;
                }
            }
        }
        else {    // if followed by an else
            traverseAST(root->children[2]);  // expression
            /* IF SIMPL */
            if_line_nos.insert(root->line_no);
            if(evaluator.isConstant(root->children[2]->addr)) {
                if(root->children[2]->addr == "0") {
                    if_simpl = "0";
                    traverseAST(root->children[6]);  // execute only else
                }
                else {
                    if_simpl = "1";
                    traverseAST(root->children[4]); // execute only if
                }
                // summary << "IF Simplification at line " << root->line_no << endl;
                if(root->children[2]->isFolded) {
                    const_folding[root->line_no].push_back(root->children[2]->addr);
                }
            }
            else {
                string l1 = newLabel();
                string l2 = newLabel();
                string code4 = "ifFalse " + root->children[2]->addr + " goto " + l1;
                ir_line_count++;
                IR << code4 << endl;
                line_map[ir_line_count] = root->line_no;
                unordered_map<string,pair<bool, string>> save;
                for(auto e : final_table.table) {
                    save[e.first] = {e.second->isConstant, e.second->constant_val};
                }
                traverseAST(root->children[4]);  // if statement
                string code5 = "goto " + l2 + "\n" + l1 + ":";
                ir_line_count++;
                IR << code5 << endl;
                line_map[ir_line_count] = root->children[5]->line_no;
                ir_line_count++;
                line_map[ir_line_count] = root->children[5]->line_no;
                for(auto e : final_table.table) {
                    e.second->isConstant = save[e.first].first;
                    e.second->constant_val = save[e.first].second;
                }
                traverseAST(root->children[6]);  // else statement
                ir_line_count++;
                string code6 = l2 + ":";
                IR << code6 << endl;  
                line_map[ir_line_count] = root->children[0]->line_no - 1;
                for(auto e : final_table.table) {
                    e.second->isConstant = false;
                }
            }
        }
   }
   else if(name == "while_stmt") {
    //    cout << "I am at " << name << endl;
        string l1 = newLabel(); // start of the loop
        string code5 = l1 + ":";
        ir_line_count++;
        IR << code5 << endl;
        line_map[ir_line_count] = root->line_no;
        traverseAST(root->children[2]); // condition expression
        string l2 = newLabel(); // outside the loop
        string code3 = "ifFalse " + root->children[2]->addr + " goto " + l2;
        ir_line_count++;
        IR << code3 << endl;
        line_map[line_count] = root->line_no;
        traverseAST(root->children[4]); // statement
        string code6 = "goto " + l1 + "\n" + l2 + ":";
        ir_line_count++;
        IR << code6 << endl;
        line_map[ir_line_count] = root->children[0]->line_no;
   }
   else if(name == "return_stmt") {
     //    cout << "I am at " << name << endl;
        for(auto& child : root->children) {
            traverseAST(child);
        } 
        if(root->children.size() == 2) {
            ir_line_count++;
            IR << "return " << "0" << endl;
            line_map[ir_line_count] = root->line_no;
        }
        else {
            ir_line_count++;
            IR << "return " << root->children[1]->addr << endl;
            line_map[ir_line_count] = root->line_no;
        }
   }
   else if(name == "incr_stmt") {
       for(auto& child : root->children) {
           traverseAST(child);
       }
       string var = env.get(root->children[0]->lexValue);
       ir_line_count++;
       IR << var << " = " << var << " PLUS " << 1 << endl; 
       line_map[ir_line_count] = root->line_no;
   }
   else if(name == "decr_stmt") {
       for(auto& child : root->children) {
           traverseAST(child);
       }
       string var = env.get(root->children[0]->lexValue);
       ir_line_count++;
       IR << var << " = " << var << " MINUS " << 1 << endl;
       line_map[ir_line_count] = root->line_no;
   }
   else if(name == "expr") {
        // cout << "I am at " << name << endl;
        if(root->children.size() == 1) {   // binary op or Pexpr
            string op = root->children[0]->nodeName;
            auto op_child = root->children[0];
            if(op_child->children.size() == 2) {     // binary op
                if(op == "AND") {
                    root->addr = newTemp();
                    op_child->symbolTableName = root->addr;
                    final_table.set(root->addr, op_child);
                    op_child->width = 4;
                    op_child->offset += table_offset;
                    table_offset += 4;
                    env.set(root->addr, op_child);
                    traverseAST(op_child->children[0]);
                    string L1 = newLabel();
                    string code1 = "ifFalse " + op_child->children[0]->addr + " goto " + L1;
                    ir_line_count++;
                    IR << code1 << endl;
                    line_map[ir_line_count] = root->line_no;
                    traverseAST(op_child->children[1]);
                    string L2 = newLabel();
                    string code2 = "if " + op_child->children[1]->addr + " goto " + L2;
                    ir_line_count++;
                    IR << code2 << endl;
                    line_map[ir_line_count] = root->line_no;
                    string L3 = newLabel();
                    ir_line_count++;
                    IR << L1 << ":" << endl;
                    line_map[ir_line_count] = root->line_no;
                    ir_line_count++;
                    IR << root->addr << " = " << 0 << endl;
                    line_map[ir_line_count] = root->line_no;
                    ir_line_count++;
                    IR << "goto " << L3 << endl;
                    line_map[ir_line_count] = root->line_no;
                    ir_line_count++;
                    IR << L2 << ":" << endl;
                    line_map[ir_line_count] = root->line_no;
                    ir_line_count++;
                    IR << root->addr << " = " << 1 << endl;
                    line_map[ir_line_count] = root->line_no;
                    ir_line_count++;
                    IR << L3 << ":" << endl;
                    line_map[ir_line_count] = root->line_no;
                }
                else if(op == "OR") {
                    root->addr = newTemp();
                    op_child->symbolTableName = root->addr;
                    final_table.set(root->addr, op_child);
                    op_child->width = 4;
                    op_child->offset += table_offset;
                    table_offset += 4;
                    env.set(root->addr, op_child);
                    traverseAST(op_child->children[0]);
                    string L1 = newLabel();
                    string code1 = "if " + op_child->children[0]->addr + " goto " + L1;
                    ir_line_count++;
                    IR << code1 << endl;
                    line_map[ir_line_count] = root->line_no;
                    traverseAST(op_child->children[1]);
                    string L2 = newLabel();
                    string code2 = "ifFalse " + op_child->children[1]->addr + " goto " + L2;
                    ir_line_count++;
                    IR << code2 << endl;
                    line_map[ir_line_count] = root->line_no;
                    string L3 = newLabel();
                    ir_line_count++;
                    IR << L1 << ":" << endl;
                    line_map[ir_line_count] = root->line_no;
                    ir_line_count++;
                    IR << root->addr << " = " << 1 << endl;
                    line_map[ir_line_count] = root->line_no;
                    ir_line_count++;
                    IR << "goto " << L3 << endl;
                    line_map[ir_line_count] = root->line_no;
                    ir_line_count++;
                    IR << L2 << ":" << endl;
                    line_map[ir_line_count] = root->line_no;
                    ir_line_count++;
                    IR << root->addr << " = " << 0 << endl;
                    line_map[ir_line_count] = root->line_no;
                    ir_line_count++;
                    IR << L3 << ":" << endl;
                    line_map[ir_line_count] = root->line_no;
                }
                else {
                    for(auto& child : root->children) {
                       traverseAST(child);
                    }
                    // check if both the 3 address code addresses are constants or hold constants, then
                    if(evaluator.isConstant(op_child->children[0]->addr) and evaluator.isConstant(op_child->children[1]->addr)) {
                        if(op == "DIV" and op_child->children[1]->addr == "0") {
                            /* Dangerous divide by zero, let hardware handle it */
                            root->addr = newTemp();
                            op_child->symbolTableName = root->addr;
                            final_table.set(root->addr, op_child);
                            op_child->width = 4;
                            op_child->offset += table_offset;
                            table_offset += 4;
                            env.set(root->addr, op_child);
                            string code = root->addr +  " = " + op_child->children[0]->addr + " " + op + " " + op_child->children[1]->addr;
                            // string final_code = code1 + code2 + code3 + "\n";
                            ir_line_count++;
                            IR << code << endl;
                            line_map[ir_line_count] = root->line_no;
                        }
                        else {
                            root->addr = evaluator.evaluate(op_child->children[0]->addr, op_child->children[1]->addr, op);
                            root->isFolded = true;
                            // summary << "Constant Folding at line " << root->line_no << " value = " << root->addr << endl;
                            // constant folding
                        }
                    }
                    else {
                        // condition for maximal expression
                        if(op_child->children[0]->isFolded or op_child->children[1]->isFolded) {
                            if(op_child->children[0]->isFolded) {
                                // summary << "Constant Folding at line " << root->line_no << " value = " << op_child->children[0]->addr << endl;
                                const_folding[root->line_no].push_back(op_child->children[0]->addr);
                            }
                            else {
                                // summary << "Constant Folding at line " << root->line_no << " value = " << op_child->children[1]->addr << endl;
                                const_folding[root->line_no].push_back(op_child->children[1]->addr);
                            }
                            root->isFolded = false;
                        }
                        root->addr = newTemp();
                        op_child->symbolTableName = root->addr;
                        final_table.set(root->addr, op_child);
                        op_child->width = 4;
                        op_child->offset += table_offset;
                        table_offset += 4;
                        env.set(root->addr, op_child);
                        if(op != "MULT") {
                            string code = root->addr +  " = " + op_child->children[0]->addr + " " + op + " " + op_child->children[1]->addr;
                            // string final_code = code1 + code2 + code3 + "\n";
                            ir_line_count++;
                            IR << code << endl;
                            line_map[ir_line_count] = root->line_no;
                        }
                        else {
                            // chance of strength reducton
                            if(evaluator.isConstant(op_child->children[0]->addr) and evaluator.check_power_of_2(op_child->children[0]->addr) != -1) {
                                int shift = evaluator.check_power_of_2(op_child->children[0]->addr);
                                string code = root->addr + " = " + to_string(shift) + " SAL1 " + op_child->children[1]->addr;
                                ir_line_count++;
                                IR << code << endl;
                                line_map[ir_line_count] = root->line_no;
                                str_red[root->line_no].push_back(shift);
                            }
                            else if(evaluator.isConstant(op_child->children[1]->addr) and 
                                                                            evaluator.check_power_of_2(op_child->children[1]->addr) != -1) {
                                int shift = evaluator.check_power_of_2(op_child->children[1]->addr);
                                string code = root->addr + " = " + to_string(shift) + " SAL2 " + op_child->children[0]->addr;
                                ir_line_count++;
                                IR << code << endl;
                                line_map[ir_line_count] = root->line_no;
                                str_red[root->line_no].push_back(shift);
                            }
                            else {
                                string code = root->addr +  " = " + op_child->children[0]->addr + " " + op + " " + op_child->children[1]->addr;
                                // string final_code = code1 + code2 + code3 + "\n";
                                ir_line_count++;
                                IR << code << endl;
                                line_map[ir_line_count] = root->line_no;
                            }
                        }
                    }
                }
            }
            else {          // Pexpr
                for(auto& child : root->children) {
                   traverseAST(child);
                }
                root->addr = root->children[0]->addr;
                root->isFolded = root->children[0]->isFolded;
            }
        }
        else if(root->children.size() == 2) {   // unary op
            string op = root->children[0]->nodeName;
            for(auto& child : root->children) {
                    traverseAST(child);
            }
            if(evaluator.isConstant(root->children[1]->addr)) {
                root->addr = evaluator.evaluate(root->children[1]->addr, op);
                root->isFolded = true;  
                // constant folding
                //  summary << "Constant Folding at line " << root->line_no << " value = " << root->addr << endl;
            }
            else {
                root->addr = newTemp();
                root->children[0]->symbolTableName = root->addr;
                root->children[0]->width = 4;
                root->children[0]->offset += table_offset;
                table_offset += 4;
                env.set(root->addr, root->children[0]);
                final_table.set(root->addr, root->children[0]); 
                string code = root->addr + " = " + op + " " + root->children[1]->addr;
                ir_line_count++;
                IR << code << endl;
                line_map[ir_line_count] = root->line_no;
            }
        }
        else {
            // HANDLE sizeof, array reference, function call
            root->addr = newTemp();
            root->children[1]->symbolTableName = root->addr;
            root->children[1]->width = 4;
            root->children[1]->offset += table_offset;
            table_offset += 4;
            env.set(root->addr, root->children[1]);
            final_table.set(root->addr, root->children[1]);
            if(root->children[0]->nodeName == "SIZEOF") {
                for(auto& child : root->children) {
                    traverseAST(child);
                }
                string size;
                if(final_table.isInTable(root->children[2]->addr)) {
                    auto node = final_table.getNode(root->children[2]->addr);
                    size = to_string(node->width);
                }
                else {
                    size = "4";
                } 
                string code = root->addr + " = " + size;
                ir_line_count++;
                IR << code << endl;
                line_map[ir_line_count] = root->line_no;
            }
            else if(root->children[1]->nodeName == "[") {    // array expression
                for(auto& child : root->children) {
                    traverseAST(child);
                }
                string code = root->addr + " = " + env.get(root->children[0]->lexValue) + "[" + root->children[2]->addr + "]";
                ir_line_count++;
                IR << code << endl;
                line_map[ir_line_count] = root->line_no;
            } 
            else {     // function call
                for(auto& child : root->children) {
                    traverseAST(child);
                }
                stack<string> s = root->children[2]->arg_stack;
                int n = s.size();
                while(!s.empty()) {
                    string arg = s.top();
                    s.pop();
                    ir_line_count++;
                    IR << "param " << arg << endl;
                    line_map[ir_line_count] = root->line_no;
                }
                string code = root->addr + " = " + "call function " + root->children[0]->lexValue + " " + to_string(n);
                ir_line_count++;
                IR << code << endl;
                line_map[ir_line_count] = root->line_no;
            }
        }
   }
   else if(name == "args") {
       for(auto& child : root->children) {
           traverseAST(child);
       }
       root->arg_stack = root->children[0]->arg_stack;
   }
   else if(name == "arg_list") {
       for(auto& child : root->children) {
            traverseAST(child);
       }
       if(root->children.size() == 1) {
            root->arg_stack.push(root->children[0]->addr);
       }
       else {
            stack<string> s = root->children[0]->arg_stack;
            s.push(root->children[2]->addr); 
            root->arg_stack = s;
       }
   }
   else if(name == "Pexpr") {
        // cout << "I am at " << name << endl;
        if(root->children.size() == 3) {   // (expr)
            for(auto& child : root->children) {
                traverseAST(child);
            }
            root->addr = root->children[1]->addr;
            root->isFolded = root->children[1]->isFolded;
        }
        else {
            if(root->children[0]->nodeName == "identifier") {
                auto node = final_table.getNode(env.get(root->children[0]->lexValue));
                if(node->isConstant) {
                    /*Constant Propogation*/
                    // summary << "Constant Propogation at line " << root->line_no << " variable " << root->children[0]->lexValue << " value " << node->constant_val << endl; 
                    root->addr = node->constant_val;
                    const_prop[root->line_no].insert({node->line_no, {root->children[0]->lexValue, node->constant_val}});
                }
                else {
                    root->addr = env.get(root->children[0]->lexValue);
                }
            }
            else {
                root->addr = root->children[0]->lexValue;
                root->isConstant = true;
            }
        }
   }
   if(name != "expr" and name != "Pexpr" and name != "assign_stmt" and name != "if_stmt" and name != "while_stmt" and name != "func_decl" and name != "compound_stmt" and 
   name != "print_stmt" and name != "param" and name != "args" and name != "arg_list" and name != "return_stmt" and name != "incr_stmt" and name != "decr_stmt" and name != "scan_stmt") {
        string final_code = "";
        for(auto& child : root->children) {
            traverseAST(child);
            // final_code += child->code;
        }
        // root->code = final_code;
   }
}

int generateIR() {
    // printAST(ast, "", true);
    IR.open("a.ir");
    summary.open("summary.txt");
    traverseAST(ast);
    // cout << "-----------3AC---------" << endl;
    IR.close();
    // final_table.debug();  // final symbol table
    optimzeIR();
    return 0;
}