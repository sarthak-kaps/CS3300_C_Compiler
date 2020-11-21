#include <bits/stdc++.h>
#include "treeNode.hpp"

using namespace std;

#ifndef SYMBOL_TABLE
#define SYMBOL_TABLE

class symbolTable {
    public:
        unordered_map<string, treeNode*> table;  
        symbolTable() {
            table.clear();
        }
        void set(string var, treeNode* node) {
            if(isInTable(var)) {
                cout << "error : redefiniton of variable " << var << endl;
            } 
            table[var] = node;
        }
        bool isInTable(string var) {
            if(table.count(var)) {
                return true;
            }
            else {
                return false;
            }
        }
        string get(string var) {
            return table[var]->symbolTableName;
        }
        treeNode* getNode(string var) {
            return table[var];
        }
        void debug() {
            cout << "Variable Name in IR | Actual Name | Offset on stack | Width" << endl;
            for(auto& e : table) {
                cout << e.first << ' ' << e.second->lexValue << ' ' << e.second->offset << ' ' << e.second->width << endl;
            }
        }
};

class symbolTableStack {
    public:
        vector<symbolTable> s;
        symbolTableStack() {
            s.resize(0);
        }
        void set(string var, treeNode* node) {
            int i = s.size() - 1;
            if(i == -1) {
                cout << "error : no symbol table found" << endl;
                return;
            }
            s[i].set(var, node);
        }
        string get(string var) {
            int i = s.size() - 1;
            while(i >= 0) {
                if(s[i].isInTable(var)) {
                    return s[i].get(var);
                }
                i--;
            }
            cout << "error : variable not declared" << endl;
            return "_";
        }
        void addTable(symbolTable top) {
            s.push_back(top);
        }
        void removeTop() {
            s.pop_back();
        }
};

// int main(void) {
//     treeNode* a = new treeNode("a");
//     a->lexValue = "a";
//     treeNode* b = new treeNode("b");
//     b->lexValue = "b";
//     treeNode* c = new treeNode("c");
//     treeNode* d = new treeNode("d");
//     treeNode* e = new treeNode("e");
//     treeNode* f = new treeNode("f");
//     symbolTable s1, s2;
//     symbolTableStack s;
//     s.addTable(s1);
//     s.set("a", a);
//     cout << s.get("b") << endl;
//     s.addTable(s2);
//     s.set("b", b);
//     cout << s.get("b") << endl;                                                         x
//     return 0;
// }
#endif