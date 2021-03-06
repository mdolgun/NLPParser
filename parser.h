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
	RuleDict ruledict;
	vector<StateSet> reduce, ereduce;
	vector<unordered_map<int, int>> dfa;
	vector<string> input;
	unordered_map<Edge, vector<EdgeInfo>> edges;
	unordered_map<BackParam, StateSet> nodes; // maps(pos, state, symbol) to set of(oldpos, oldstate) (i.e adds an arc from(pos, state) to(oldpos, oldstate) labeled with symbol)
	unordered_map<tuple<int, int, int, Rule*>, StateSet> back_nodes;
	unordered_map<tuple<int, int, int>, unordered_set<tuple<Rule*, int>>> expected; // <end_pos,end_state, symbol_id> -> <Rule*,rulepos>*
	vector<unordered_set<int>> act_states; // active set of states for each position
	vector<unordered_set<Edge>> act_edges; // active set of edges for each position

	/**/
	int tail_id, unknown_id;
	/**/

	int n_states, n_symbols;
	Parser() {
		root = new TrieNode;

	}
	Rule* get_rule(int id) {
		// returns the ordinary (non-owned) pointer for the given Symbol id
		return rules[id].get();
	}
	int get_next_state(int state, int symbol) {
		auto it = dfa[state].find(symbol);
		if (it == dfa[state].end())
			return -1;
		return it->second;
	}
	void persist(ofstream& os);
	void load_grammar(const char* fname);
	void closure(StateSet& stateset);
	void print_ruledict(ostream& os);
	void print_stateset(ostream& os, StateSet& stateset);
	void print_rule(ostream& os, Rule* rule, int rulepos);
	void print_state(ostream& os, int ruleno, int rulepos);
	void print_edge(ostream& os, Edge& edge);
	void compile();
	void print_dfa(ostream& os, bool csv = false);
	void print_dfa_dot(ostream& os);
	void add_edge(int start_pos, int start_state, int symbol_id, int end_pos, int end_state);
	void match_and_reduce(unordered_set<int>& active, vector<Edge>& edge_list, vector<Edge>& edge_seq, Rule* rule, int rulepos, const Edge& edge);
	void match_and_reduce(unordered_set<int>& active, vector<Edge>& edge_list, vector<Edge>& edge_seq, Rule* rule, int rulepos, int dic_rulepos, const Edge& edge);
	void parse(string input,int unknown=0);
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
	TreeNode* make_trans_tree(int id, const FeatParam& fparam, FeatPtr parent_feat, unordered_map<TreeNode*, vector<TreeNode*>>* visited);
	TreeNode* translate_tree_shared(unordered_map<TreeNode*, vector<TreeNode*>>& visited, TreeNode* parent_node, const FeatParam& fparam, FeatPtr parent_feat);
	TreeNode* translate_tree(TreeNode* parent_node, bool shared = false);
	TreeNode* translate_tree(TreeNode* parent_node, const FeatParam& fparam, FeatPtr parent_feat);
};


