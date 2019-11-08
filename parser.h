#pragma once

#include "grammar.h"

using State = pair<int, int>; // pair(ruleno,rulepos)
using StateSet = set<State>; // set of (ruleno,rulepos)
using StateSetDict = unordered_map<StateSet, int>; // maps from set of (ruleno,rulepos) (NFA) to int (DFA)
using RuleSet = vector<int>;
using RuleDict = vector<RuleSet>;


using Edge = tuple<int, int, int, int, int>; // <start_pos,start_state,symbol,end_pos,end_state>
using EdgeSeq = vector<Edge>;
using EdgeInfo = tuple<Rule*, EdgeSeq>;

using BackParam = tuple<int, int, int>; // <pos,state,symbol>
struct Parser : public Grammar{
	Edge top_edge;
	//vector<RulePtr> rules;
	//TrieNode* root;
	RuleDict ruledict;
	vector<StateSet> reduce, ereduce;
	vector<unordered_map<int, int>> dfa;
	vector<string>* p_input;
	unordered_map<Edge, vector<EdgeInfo>> edges;
	int n_states, n_symbols;
	Parser() {
		root = new TrieNode;
	}
	Rule* get_rule(int id) {
		// returns the ordinary (non-owned) pointer for the given Symbol id
		return rules[id].get();
	}
	void persist(ofstream& os);
	void load_grammar(const char* fname);
	void closure(StateSet& stateset);
	void print_rules(ostream& os);
	void print_stateset(ostream& os, StateSet& stateset);
	void print_state(ostream& os, int ruleno, int rulepos);
	void compile();
	void print_dfa(ostream& os, bool csv = false);
	void print_dfa_dot(ostream& os);
	void parse(string input);
	void print_parse(ostream& os, Edge& parent_edge, int level = 0, int indent_size = 0, bool extended = false);
	void print_parse_dot(ostream& os, unordered_set<Edge>&completed, Edge& parent_edge);
	void print_parse_dot(ostream& os);
	void print_parse_dot_all(ostream& os);
	void print_parse_dot_all(string fname);
	void print_graph(ostream& os, int inlen, vector<unordered_set<int>>& act_states, vector<unordered_set<Edge>>& act_edges);
	void print_parse_tree(Edge& top_edge);
	TreeNode* make_tree_shared(unordered_map<tuple<int, int, int>, TreeNode*>&visited, Edge& edge);
	TreeNode* make_tree(Edge& parent_edge);
	TreeNode* make_tree(bool shared = false);
	TreeNode* make_trans_tree(int id, FeatParam* fparam, FeatPtr parent_feat);
	TreeNode* translate_tree_shared(unordered_set<TreeNode*>& visited, TreeNode* parent_node, FeatParam* fparam, FeatPtr parent_feat);
	TreeNode* translate_tree(TreeNode* parent_node, bool shared = false);
	TreeNode* translate_tree(TreeNode* parent_node, FeatParam* fparam, FeatPtr parent_feat);
};

struct UnitTest {
	bool shared;
	int case_total = 0, success_total = 0;
	static string get_lines(ifstream& is, stringstream& ref);
	UnitTest(bool _shared = false) : shared(_shared) { }
	static void diff(string a, string b);
	void test_case(const char* fname);
	void test_dir(const char* dirname);
	void print_result();
};
