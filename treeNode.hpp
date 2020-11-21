#include <bits/stdc++.h>

using namespace std;

extern int line_count;
extern int yylineno;

#ifndef TREE_NODE
#define TREE_NODE
class treeNode {
    public:
        vector<treeNode*> children;   // children
        int maxRootedLength;          // maximum path length passing through this node
        int maxSubtreeLength;         // maximum path length in the subtree rooted at this node  
        int maxDirectLength;          // maximum path length ending at this node
        string nodeName;              // name of the node
        int level;                    // for printing
        int line_no;                  // line number where the item is found in the source program

        /*****************
         * START IR FIELDS
        *****************/

        string code;                  // IR code for the subtree rooted at current node
                                      // by default EMPTY                   
        
        string next;                  // label of the next block of code, not meaningful for
                                      // all constructs

        string TRUE;                  // for boolean expressions

        string FALSE;                 // for boolean expressions


        string lexValue;              // lexical value, name of identifier, value of constant etc

        int width;                    // width of the variable in bytes (if applicable)

        string addr;                  // for expression

        string func_name;             // name of the function which contains the current node, by default global

        int depth;                    // depth of the current compound statement - for symbol table by default 0

        int offset;                   // offset of the variable on the stack (not applicable to all) start from 0

        string symbolTableName;       // name for the final symbol table entry

        bool isArg;                   // whether the given variable is an argument (default false) 

        stack<string> arg_stack;      // stack of arguments for function call

        bool isConstant;              // if the given node holds a constant

        string constant_val;          // constant value of the identifier (if applicable)

        bool isFolded;                // true if the current expression has been folded into a constant

       /*****************
        * END IR FIELDS
       *****************/

        treeNode(string nodeName, vector<treeNode*> children) {  // for non-terminals, assume valid pointers are passed
            this->nodeName = nodeName;
            this->children = children;
            maxRootedLength = 1;
            maxSubtreeLength = 1;
            maxDirectLength = 1;
            code = "";
            func_name = "$global";
            depth = 0;
            offset = 0;
            isArg = false;
            isConstant = false;
            isFolded = false;
            compute();
        }
        treeNode(string nodeName) {     // for epsilon and terminals, no need to call compute
            this->nodeName = nodeName;
            children.assign(0, NULL);
            maxRootedLength = 1;
            maxSubtreeLength = 1;
            maxDirectLength = 1;
            func_name = "$global";
            depth = 0;
            offset = 0;
            isArg = false;
            isConstant = false;
            isFolded = false;
            line_no = line_count;
            code = "";
        }
        int max_path_in_children() {
            vector<int> x;
            for(auto& child : children) {
                x.push_back(child->maxSubtreeLength);
            }
            sort(x.rbegin(), x.rend());
            return x[0];
        }
        pair<int, int> max_and_smax_from_children() {
            vector<int> x;
            for(auto& child : children) {
                if(child) {
                    x.push_back(child->maxDirectLength);
                }
            }
            sort(x.rbegin(), x.rend());
            return {x[0], x[1]};
        }
        void compute() {
            if(children.size() == 0) {
    
            }
            else if(children.size() == 1) {
                maxDirectLength = 1 + children[0]->maxDirectLength;
                maxRootedLength = maxDirectLength;
                maxSubtreeLength = max(maxRootedLength, children[0]->maxSubtreeLength);
                line_no = children[0]->line_no;
            }
            else {
                pair<int, int> max_and_smax = max_and_smax_from_children();
                maxDirectLength = 1 + max_and_smax.first;
                maxRootedLength = 1 + max_and_smax.first + max_and_smax.second;
                line_no = children[0]->line_no;
                vector<int> x;
                x.push_back(maxRootedLength);
                for(auto& child : children) {
                    x.push_back(child->maxSubtreeLength);
                }            
                sort(x.rbegin(), x.rend());
                maxSubtreeLength = x[0];
            }
        }
        void debug() {
            cout << nodeName << endl;
            for(auto& child : children) {
                if(child) {
                    cout << child->nodeName << ' ';
                }
            }
            cout << endl;
            cout << maxDirectLength << ' ' << maxRootedLength << ' ' << maxSubtreeLength << endl;
            cout.flush();
        }
};
#endif