#include "stdafx.h"
#include "parser.h"

void Parser::print_ruledict(ostream& os) {
	for (int i = 0; i < ruledict.size(); i++)
		if (ruledict[i].size()) {
			os << symbol_table.get(i) << " : ";
			for (int item : ruledict[i])
				os << item << " ";
			os << nl;
		}
}
void Parser::persist(ofstream& os) {
	
}
void Parser::load_grammar(const char* fname) {
	GrammarParser grammar(this);
	grammar.load_grammar(fname);
	if (debug >= 1) {
		cout << "Rules:" << nl;
		print_rules(cout);
	}

}

void Parser::print_rules(ostream& os) {
	for (int i = 0; i < rules.size(); i++) {
		os << i << ':' << *rules[i] << nl;
	}
}

ostream& operator<<(ostream& os, const State& state) {
	return os << "(" << state.first << "," << state.second << ")";
}
ostream& operator<<(ostream& os, StateSet& stateset) {
	return join(os, stateset, " ");
}

ostream& operator<< (ostream& os, const Edge& edge) {
	return os << '(' << get<0>(edge) << ',' << get<1>(edge) << ',' << symbol_table.get(get<2>(edge)) << ',' << get<3>(edge) << ',' << get<4>(edge) << ')';
}

void Parser::print_stateset(ostream& os,StateSet& stateset) {
// get string repr of "items" in state set in dotted fomat  e.g { S->NP.VP; VP ->.V; VP ->.V NP }

	os << '{';
	bool first = true;
	for (auto& p : stateset) {
		Rule* rule = get_rule(p.first);
		if (first)
			first = false;
		else
			os << ", ";
		os << *rule->head << "->";
		print_partial_rule(os, rule, 0, p.second);
		os << ".";
		print_partial_rule(os, rule, p.second, -1);
	}
	os << "}";
}

void Parser::print_state(ostream& os,int ruleno, int rulepos) {
	Rule* rule = get_rule(ruleno);
	os << symbol_table.get(rule->head->id) << "->";
	print_partial_rule(os, rule, 0, rulepos);
	os << ".";
	print_partial_rule(os, rule, rulepos, -1);
}

void Parser::print_dfa(ostream& os,bool csv) {
	// internal: prints dfa, reduce and e-reduce in a tabular format after compile
	if (csv) { // csv tabular format
		for (int symbolid = 0; symbolid < n_symbols; symbolid++)
			os << "," << symbol_table.get(symbolid);
		os << '\n';
		for (int state = 0; state < n_states; state++) {
			os << state;
			for (int symbolid = 0; symbolid < n_symbols; symbolid++) {
				auto it = dfa[state].find(symbolid);
				if (it != dfa[state].end()) {
					os << "," << it->second << ",";
				}
				else {
					os << ",";
				}
			}
			os << '\n';
		}
		return;
	}
	// non-csv (sparse) format
	for (int state = 0; state < n_states; state++) {
		os << state << ":";
		for (auto it : dfa[state]) {
			os << ' ' << symbol_table.get(it.first) << "(" << it.second << ")";
		}
		os << " {" << reduce[state] << ereduce[state] << "}\n";
	}
}

void Parser::closure(StateSet& stateset) {
	// modifies parameter to add e-closure to existing set of states
	vector<State> todo(stateset.cbegin(), stateset.cend());
	for (int i = 0; i < todo.size(); i++) {
		auto[ruleno, rulepos] = todo[i];
		Prod* prod = rules[ruleno]->left;
		if (rulepos >= prod->size())
			continue;
		Symbol* symbol = prod->at(rulepos);
		if (!symbol->nonterminal)
			continue;
		
		for (int nextno : ruledict[symbol->id]) {
			State nextstate(nextno, 0);
			bool inserted = stateset.insert(nextstate).second;
			if (inserted) {
				todo.push_back(nextstate);
			}
		}
	}
}
void Parser::compile() {
	/*
		compile rule list "rules" to a DFA
		produces dfa, reduce and ereduce tables(dictionaries) from rules
	*/
	n_symbols = symbol_table.size();
	ruledict.resize(n_symbols); // ruledict stores the set of rules a nonterminal can derive (i.e. is head)
	for (int ruleno = 0; ruleno < rules.size(); ruleno++) {
		ruledict[rules[ruleno]->head->id].push_back(ruleno);
	}
	if (debug >= 2) {
		cout << "RuleDict:\n";
		print_ruledict(cout);
	}

	vector<bool> nullable(n_symbols); // nullable stores if a nonterminal can be nullable (i.e. can derive to empty string) for any rule
	bool modified = true;
	while (modified)
	{
		modified = false;
		for (int ruleno = 0; ruleno < rules.size(); ruleno++) {
			Prod* prod = rules[ruleno]->left;
			int head = rules[ruleno]->head->id;
			if (nullable[head])
				continue;
			if (all_of(prod->begin(), prod->end(), [&nullable](Symbol* symbol)->bool {return nullable[symbol->id]; })) {
				nullable[head] = true;
				modified = true;
			}
		}
	}
	if (debug >= 2) {
		cout << "Nullable: ";
		for (int i = 0; i < n_symbols; i++) {
			if (nullable[i]) {
				cout << symbol_table.get(i) << ",";
			}
		}
		cout << '\n';
	}

	StateSetDict statedict; //maps set of nfa states to a single dfa state
	StateSet stateset;
	State initial(0, 0);
	stateset.insert(initial);
	closure(stateset);

	statedict[stateset] = 0; // maps nfa_state->dfa_state
	vector<int> todo = { 0 };
	vector<StateSet> nfa_states = { stateset };
	n_states = 1;

	for (int i = 0; i < todo.size(); i++) {
		unordered_map<int, StateSet> nfa_map; // maps symbol->next_nfa_state for the current state
		unordered_map<int, int> dfa_map;      // maps symbol->next_dfa_state for the current state
		for (auto[ruleno, rulepos] : nfa_states[i]) {
			try {
				auto symbol = rules[ruleno]->left->at(rulepos)->id; // get next symbol id after dot (i.e. A->x.By)
				nfa_map[symbol].emplace(ruleno, rulepos + 1);
			}
			catch (out_of_range&) {
			}
		}
		for (auto&[symbolid, nextstateset] : nfa_map) {
			closure(nextstateset);
			auto[it, inserted] = statedict.insert(make_pair(nextstateset, n_states));
			if (inserted) {
				n_states++;
				nfa_states.push_back(nextstateset);
				todo.push_back(n_states);
			}
			dfa_map[symbolid] = it->second; // next state
		}
		dfa.push_back(move(dfa_map));
	}
	assert(n_states == nfa_states.size());
	assert(n_states == statedict.size());

	if (debug >= 2)
		for (int i = 0; i < nfa_states.size(); i++) {
			cout << i << ": ";
			print_stateset(cout, nfa_states[i]);
			cout << '\n';
		}
	reduce.resize(n_states);
	ereduce.resize(n_states);
	for (int i = 0; i < n_states; i++) {
		for (auto& rule_state : nfa_states[i]) {
			auto[ruleno, rulepos] = rule_state;
			if (ruleno == 0) // we don't reduce S'->S
				continue;
			Prod* body = rules[ruleno]->left;
			if (all_of(body->cbegin() + rulepos, body->cend(), [nullable](Symbol* symbol)->bool {return nullable[symbol->id]; })) {
				if (rulepos == 0) // check if an empty-reducable rule
					ereduce[i].insert(rule_state); // insert (ruleno,0) as e-reducable for state=i
				else
					reduce[i].insert(rule_state); // insert (ruleno,rulepos) as reducable for state=i
			}
		}
	}
	if (debug >= 2) {
		cout << "DFA:\n";
		print_dfa(cout);
	}
}

void Parser::print_parse_dot(ostream& os, unordered_set<Edge>&visited, Edge& edge) {
	if (visited.count(edge))
		return;
	visited.insert(edge);
	int symbol = get<2>(edge);
	os << "\"" << edge << "\"[label=\"" << symbol_table.get(symbol) << "[" << get<0>(edge) << ":" << get<3>(edge) << "]\"]\n";
	if (!symbol_table.nonterminal(symbol)) {
		return;
	}
	auto& edge_seq_list = edges[edge];
	for (int i = 0; i < edge_seq_list.size(); i++) {
		os << "\"" << edge << "_" << i << "\"[label=\"" << symbol_table.get(symbol) << "\",color=red]\n";
		os << "\"" << edge << "\"->\"" << edge << "_" << i << "\"[color=red]\n";
		auto& edge_seq = get<1>(edge_seq_list[i]);
		for (auto& sub_edge : edge_seq) {
			os << "  \"" << edge << "_" << i << "\"->\"" << sub_edge << "\"\n";
			print_parse_dot(os, visited, sub_edge);
		}
	}
}
void Parser::print_parse_dot(ostream& os) { 
	unordered_set<Edge> completed; 
	os << "digraph {\n";
	os << "graph[ordering=out]\n";
	print_parse_dot(os, completed, top_edge);
	os << "}\n";
}

void Parser::print_parse_dot_all(ostream& os) {
	os << "digraph {\n";
	os << "graph[ordering=out]\n";

	for (auto&[edge, edge_seq_list] : edges) {
		int symbol = get<2>(edge);
		if (edge == top_edge)
			os << "\"" << edge << "\"[label=\"" << symbol_table.get(symbol) << "[" << get<0>(edge) << ":" << get<3>(edge) << "]\",style=bold]\n";
		else
			os << "\"" << edge << "\"[label=\"" << symbol_table.get(symbol) << "[" << get<0>(edge) << ":" << get<3>(edge) << "]\"]\n";
		for (int i = 0; i < edge_seq_list.size(); i++) {
			os << "\"" << edge << "_" << i << "\"[label=\"#" << get<0>(edge_seq_list[i])->id << "\"]\n";
			os << "\"" << edge << "\"->\"" << edge << "_" << i << "\"\n";
			auto& edge_seq = get<1>(edge_seq_list[i]);
			for (auto& sub_edge : edge_seq) {
				os << "  \"" << edge << "_" << i << "\"->\"" << sub_edge << "\"\n";
				int sub_symbol = get<2>(sub_edge);
				if (!symbol_table.nonterminal(sub_symbol)) {
					os << "  \"" << sub_edge << "\"[label=\"" << symbol_table.get(sub_symbol) << "[" << get<0>(sub_edge) << "]\"]\n";
				}
			}
		}
	}
	os << "}\n";
}

void Parser::print_parse_dot_all(string fname) {
	ofstream os(fname);
	if (!os)
		throw runtime_error("Cannot write file: " + fname);
	print_parse_dot_all(os);
}

constexpr int indent_size = 4;
void Parser::print_parse(ostream& os, Edge& top_edge, int level,int indent_size,bool extended) {
	indent(os, level * indent_size);
	int symbol = get<2>(top_edge);
	if (!symbol_table.nonterminal(symbol)) {
		bool first = true;
		for (int i = get<0>(top_edge); i < get<3>(top_edge); i++) {
			if (first)
				first = false;
			else
				os << ' ';
			os << (*p_input)[i];
		}
		return;
	}
	auto& edge_seq_list = edges[top_edge];
	if (!extended && indent_size) {
		if (edge_seq_list.size() == 1 && all_of(get<1>(edge_seq_list[0]).begin(), get<1>(edge_seq_list[0]).end(), [](auto& edge) { return !symbol_table.nonterminal(get<2>(edge)); })) {
			indent_size = 0;
		}

	}
	os << symbol_table.get(symbol) << '(';
	bool first = true;
	for (auto& [ruleno, edge_seq] : edge_seq_list) {
		if (indent_size)
			os << nl;
		if (first) {
			if (extended) {
				indent(os, level * indent_size);
				os << '#' << ruleno;
				if (indent_size)
					os << nl;
			}
			first = false;
		}
		else {
			indent(os, level * indent_size);
			os << '|';
			if (indent_size)
				os << nl;
			if (extended) {
				indent(os, level * indent_size);
				os << '#' << ruleno;
				if (indent_size)
					os << nl;
			}
		}
		bool first2 = true;
		for (auto& edge : edge_seq) {
			if (first2)
				first2 = false;
			else
				if (indent_size)
					os << nl;
				else
					os << ' ';

			print_parse(os, edge, level + 1, indent_size, extended);
		}
	}
	if (indent_size)
		os << nl;
	indent(os, level * indent_size);
	os << ")";
}

void Parser::print_dfa_dot(ostream& os) {
	os << "digraph {" << nl;
	for (int state = 0; state < dfa.size(); state++) {
		os << "  " << state << "[label=\"";
		for (auto[ruleno, rulepos] : reduce[state]) {
			print_state(os, ruleno, rulepos);
			os << "\\n";
		}
		for (auto[ruleno, rulepos] : ereduce[state]) {
			print_state(os, ruleno, rulepos);
			os << "\\n";
		}
		os << "\"]\n";
		for (auto[symbol, next_state] : dfa[state]) {
			os << "  " << state << "->" << next_state << "[label=\"" << symbol_table.get(symbol) << "\"]\n";
		}
	}
	os << "}" << nl;
}
void Parser::print_graph(ostream& os,int inlen, vector<unordered_set<int>>& act_states,vector<unordered_set<Edge>>& act_edges) {
	os << "digraph {\n";
	for (int i = 0; i <= inlen; i++) {
		os << "  subgraph cluster_" << i << " {\n";
		for (auto state : act_states[i]) {
			os << "    \"" << i << "," << state << "\"[label=\"";
			for (auto[ruleno, rulepos] : reduce[state]) {
				print_state(os, ruleno, rulepos);
				os << "\\n";
			}
			for (auto[ruleno, rulepos] : ereduce[state]) {
				print_state(os, ruleno, rulepos);
				os << "\\n";
			}
			os << "\"]\n";
		}
		os << "  }\n";
		for (auto& edge : act_edges[i]) {
			os << "  \"" << get<0>(edge) << "," << get<1>(edge) << "\"->\"" << get<3>(edge) << "," << get<4>(edge) << "\"[label=\"" << symbol_table.get(get<2>(edge)) << "\"]\n";
		}
	}
	for (auto&[edge, value] : edges) {
		os << "  \"" << get<0>(edge) << "," << get<1>(edge) << "\"->\"" << get<3>(edge) << "," << get<4>(edge) << "\"[label=\"" << symbol_table.get(get<2>(edge)) << "\"]\n";
	}
	os << "}\n";
}

TreeNode* Parser::make_tree_shared(unordered_map<tuple<int,int,int>,TreeNode*>&visited, Edge& edge) {
	int parent_symbol = get<2>(edge);
	tuple<int, int, int> key{ get<0>(edge), parent_symbol, get<3>(edge) };
	auto it = visited.find(key);
	if (it != visited.end()) // this node is already visited
		return it->second; // return the associated node
	
	TreeNode* node = new TreeNode(&symbol_table.get(parent_symbol), symbol_table.nonterminal(parent_symbol));
	visited[key] = node;
	node->start_pos = get<0>(edge);
	node->end_pos = get<3>(edge);
	if (!node->nonterm)
		return node;
	for (auto& [rule, edge_seq] : edges[edge]) {
		OptionNode* option = new OptionNode(rule, rule->feat);
		if (rule->id == -1) { // this is a dictionary-rule (i.e. consists of only non-terminals), then create a new terminal Node for each word
			for (auto& symbol : *rule->left) {
				option->left.push_back(new TreeNode(&symbol->name, false));
			}
		}
		else { // this is a grammar-rule, make sub-trees recursively
			for (auto sub_edge : edge_seq) {
				option->left.push_back(make_tree_shared(visited, sub_edge));
			}
		}
		node->options.push_back(option);
	}
	
	return node;
}
TreeNode* Parser::make_tree(Edge& edge) {
	// makes a sub-tree from given edge<start_pos,start_state,symbol,end_pos,end_state>
	int parent_symbol = get<2>(edge);
	TreeNode* node = new TreeNode(&symbol_table.get(parent_symbol), symbol_table.nonterminal(parent_symbol));
	node->start_pos = get<0>(edge);
	node->end_pos = get<3>(edge);
	if (!node->nonterm) // if nonterminal no subtree exists under it
		return node;
	for (auto&[rule, edge_seq] : edges[edge]) {
		OptionNode* option = new OptionNode(rule, rule->feat);
		if (rule->id == -1) { // this is a dictionary-rule (i.e. consists of only non-terminals), then create a new terminal Node for each word
			for (auto& symbol : *rule->left) {
				option->left.push_back(new TreeNode(&symbol->name, false));
			}
		}
		else { // this is a grammar-rule, make sub-trees recursively
			for (auto sub_edge : edge_seq) {
				option->left.push_back(make_tree(sub_edge));
			}
		}
		node->options.push_back(option);
	}
	return node;
}

TreeNode* Parser::make_tree(bool shared) {
	if (shared) {
		unordered_map<tuple<int,int,int>, TreeNode*> visited;
		return make_tree_shared(visited, top_edge);
	}
	else
		return make_tree(top_edge);
}

TreeNode* Parser::make_trans_tree(int id,FeatParam* fparam,FeatPtr parent_feat) {
	// make a translated sub tree (i.e. RHS-only subtree) from current id,parent feature,feature parameters
	TreeNode* node = new TreeNode(&symbol_table.get(id), true);
	for (auto ruleno : ruledict[id]) {
		Rule* rule = get_rule(ruleno);
		try {
			FeatPtr feat_list = rule->feat;
			if( !unify_feat(feat_list, fparam, parent_feat, true) )
				throw UnifyError("UnifyError");
			OptionNode* option = new OptionNode(rule, feat_list);
			bool cut = rule->right->cost < 0;
			for (auto symbol : *rule->right) {
				if (symbol->nonterminal) {
					if (symbol->idx == -1) { // a non-referenced nonterminal
						option->right.push_back(make_trans_tree(symbol->id, symbol->fparam, feat_list));
					}
					// else what??
				}
				else { // terminal
					//assert(!symbol->nonterminal); // cannot be a non-referenced nonterminal
					//TreeNode* sub_node = new TreeNode(&symbol->name, false);
					TreeNode* sub_node = new TreeNode(&symbol->name, symbol->nonterminal);
					option->right.push_back(sub_node);
				}
			}
			node->options.push_back(option);
			if (cut)
				break;
		}
		catch (UnifyError&) {
		}
	}
	if (!node->options.size())
		throw UnifyError("make_trans_tree");
	return node;
}
TreeNode* Parser::translate_tree_shared(unordered_set<TreeNode*>& visited,TreeNode* parent_node, FeatParam* fparam, FeatPtr parent_feat) {
	// translate a sub-tree
	if (visited.find(parent_node) != visited.end())
		return parent_node;
	visited.insert(parent_node);
	vector<OptionNode*> new_options;
	for (auto option : parent_node->options) {
		assert(option->right.size() == 0);
		try {
			Prod* right = option->rule->right;
			for (auto symbol : *right) {
				TreeNode* node;
				if (symbol->idx != -1) { // a referenced nonterminal
					node = option->left.at(symbol->idx);
					if (!unify_feat(option->feat_list, fparam, parent_feat, true))
						throw UnifyError("UnifyError");
					node = translate_tree_shared(visited, node, symbol->fparam, option->feat_list);
				}
				else if (!symbol->nonterminal) { // a terminal
					node = new TreeNode(&symbol->name, false);
				}
				else { // a non-referenced non-terminal
					node = make_trans_tree(symbol->id, symbol->fparam, parent_feat);
				}
				option->right.push_back(node);
			}
			new_options.push_back(option);
		}
		catch (UnifyError&) {
		}
	}
	if (!new_options.size())
		throw UnifyError("translate_tree");
	parent_node->options = move(new_options);
	return parent_node;
}
TreeNode* Parser::translate_tree(TreeNode* parent_node,FeatParam* fparam,FeatPtr parent_feat) {
	// translate a sub-tree
	vector<OptionNode*> new_options;
	for (auto option : parent_node->options) {
		try {
			Prod* right = option->rule->right;
			for (auto symbol : *right) {
				TreeNode* node;
				if (symbol->idx != -1) { // a referenced nonterminal
					node = option->left.at(symbol->idx);
					if( !unify_feat(option->feat_list, fparam, parent_feat, true) )
						throw UnifyError("UnifyError");
					node = translate_tree(node, symbol->fparam, option->feat_list);
				}
				else if (!symbol->nonterminal) { // a terminal
					node = new TreeNode(&symbol->name,false);
				}
				else { // a non-referenced non-terminal
					node = make_trans_tree(symbol->id, symbol->fparam, option->feat_list);
				}
				option->right.push_back(node);
			}
			new_options.push_back(option);
		}
		catch (UnifyError&) {
		}
	}
	if (!new_options.size())
		throw UnifyError("translate_tree");
	parent_node->options = move(new_options);
	return parent_node;
}

TreeNode* Parser::translate_tree(TreeNode* parent_node,bool shared) {
	FeatPtr feat_list = FeatPtr(new FeatList);
	if (shared) {
		unordered_set<TreeNode*> visited;
		return translate_tree_shared(visited, parent_node, nullptr, feat_list);
	}
	else {
		return translate_tree(parent_node, nullptr, feat_list);
	}
}

void Parser::parse(string input_str) {
	vector<string> input;
	split(input, input_str, ' ');
	// parses input string using current grammar, throwing ParseError if parsing fails, the parse tree can be later retrieved from "edges"
	p_input = &input;
	edges.clear();

	int inlen = input.size();
	unordered_map<BackParam, StateSet> nodes; // maps(pos, state, symbol) to set of(oldpos, oldstate) (i.e adds an arc from(pos, state) to(oldpos, oldstate) labeled with symbol)
	// <startpos,startstate,symbol,endpos,endstate>
	//unordered_map<Edge, vector<vector<Edge>>> edges; // maps an edge to sub - edges of a production e.g. (p0, s0, S, p2, s2') -> [ (p0,s0,NP,p1,s1), (p1,s1,VP,p2,s2) ] where s0,S->s2'
	int start_symbol = symbol_table.map("S"); // start_symbol == 1
	int final_state = dfa[0].at(start_symbol); 
	top_edge = Edge(0, 0, start_symbol, inlen, final_state);
	vector<unordered_set<int>> act_states(inlen+1); // active set of states for each position
	vector<unordered_set<Edge>> act_edges(inlen+1); // active set of edges for each position
	act_states[0].insert(0); // add initial state to initial position
	int char_pos = 0;
	for (int pos = 0; pos <= inlen; pos++) {

		int token = -1;
		if( pos!=inlen ) // if not end-of-input, which has special token value -1
			token = symbol_table.map(input[pos]);
		vector<Edge> edge_list(act_edges[pos].size()); // list of active edges for current position
		copy(act_edges[pos].cbegin(), act_edges[pos].cend(), edge_list.begin());
		auto& active = act_states[pos]; // active states for current position
		if (debug >= 3) {
			cout << "Position:" << pos << " InputToken:" << ((pos == inlen) ? "$" : input[pos]) << " ActiveStateList:";
			for (auto state : active) {
				cout << ' ' << state;
			}
			cout << " ActiveEdgeList:";
			for (auto edge : edge_list) {
				cout << ' ' << edge;
			}
			cout << '\n';
		}
		for (int edge_idx = 0; edge_idx < edge_list.size(); edge_idx++) { // for each active edge for current position
			auto edge = edge_list[edge_idx];
			if (debug >= 2)
				cout << "Checking edge for reduction:" << edge << '\n';
			auto [start_pos, start_state, edge_symbol, end_pos, end_state] = edge;
			for (auto [ruleno, rulepos] : reduce[end_state]) {
				auto rule = get_rule(ruleno);
				auto head = rule->head->id;
				auto body = rule->left;
				if (debug >= 2) {
					cout << "Reducing:"; print_state(cout, ruleno, rulepos); cout << '\n';
				}
				vector<Edge> edge_seq;
				edge_seq.push_back(edge);
				int state = end_state;
				for (int i = rulepos; i < body->size(); i++) { // iterate and append all right nulled symbols to edge_seq
					int symbol = body->at(i)->id;
					int next_state = dfa[state].at(symbol);
					edge_seq.emplace_back(end_pos, state, symbol, end_pos, next_state);
					state = next_state;
				}
				assert(edge_symbol == body->at(rulepos - 1)->id);
				reverse(edge_seq.begin(), edge_seq.end());
				//vector<vector<Edge>> stack; // special stack used for GLR parsing, where can be multiple Edges, i.e. multiple alternatives a.k.a "parse forest"
				vector<tuple<State, vector<Edge>>> state_edge_seq; // set of states used for backward-iteration of the rule
				state_edge_seq.emplace_back(State{ start_pos,start_state }, edge_seq);
				for (int i = rulepos - 2; i >= 0; i--) { // iterate backward from rule position
					vector<tuple<State, vector<Edge>>> prev_state_edge_seq;
					int symbol = body->at(i)->id;
					if (debug >= 3) {
						cout << "Symbol:" << symbol_table.get(symbol) << " StateEdgeSeq:";
						for (auto& [posstate, edge_seq] : state_edge_seq) {
							cout << "PosState" << posstate << " Edge" << edge << " BackPtr:";
							for (auto prev_posstate : nodes[{posstate.first, posstate.second, symbol}])
								cout << " " << prev_posstate;
							cout << '\n';
						}
					}
					for (auto [iter_posstate, edge_seq] : state_edge_seq) {
						auto [iter_pos,iter_state] = iter_posstate;
						for (auto prev_posstate : nodes[{iter_pos,iter_state, symbol}]) {
							auto [prev_pos, prev_state] = prev_posstate;
							vector<Edge> new_edge_seq(edge_seq);
							new_edge_seq.emplace_back(prev_pos, prev_state, symbol, iter_pos, iter_state);
							prev_state_edge_seq.emplace_back(prev_posstate, new_edge_seq);
						}
					}
					state_edge_seq = move(prev_state_edge_seq);
				}
				for (auto [prev_posstate, edge_seq] : state_edge_seq) {
					auto [prev_pos, prev_state] = prev_posstate;
					int next_state = dfa[prev_state].at(head); 
					if (debug >= 2) {
						cout << "StateTrans:" << prev_state << "," << symbol_table.get(head) << "->" << next_state << nl;
					}
					active.insert(next_state);
					nodes[{pos, next_state, head}].insert(prev_posstate);
					Edge new_edge(prev_pos, prev_state, head, pos, next_state);
					if (debug >= 3) {
						cout << "Inserting BackPtr (" << pos << "," << next_state << "," << symbol_table.get(head) << ")->" << prev_state << nl;
						cout << "Inserting Edges [" << new_edge << "]<-" << edge_seq << nl;
					}
					if (edges.count(new_edge) == 0) {
						edge_list.push_back(new_edge);
						if (debug >= 3)
							cout << "NewEdgeCreated\n";
					}
					reverse(edge_seq.begin(), edge_seq.end());
					edges[new_edge].push_back({ rule, edge_seq});
				}
			}
		}
		vector<int> active_list(active.size());
		copy(active.begin(), active.end(), active_list.begin());
		for (int i = 0; i < active_list.size(); i++) { // for each active state for the current position
			int state = active_list[i];
			for (auto [ruleno, rulepos] : ereduce[state]) {
				auto rule = get_rule(ruleno);
				auto head = rule->head->id;
				auto body = rule->left;
				int end_state = state;
				vector<Edge> edge_seq; // list of sub-edges
				for (auto it = body->begin() + rulepos; it < body->end(); ++it) {
					int symbol = (*it)->id;
					int next_state = dfa[end_state].at(symbol);
					edge_seq.emplace_back(pos, state, symbol, pos, next_state);
					end_state = next_state;
				}
				int next_state = dfa[state].at(head);
				if (active.count(next_state) == 0) { // next_state is not in active states, then add it
					active.insert(next_state);
					active_list.push_back(next_state);
				}
				nodes[{pos, next_state, head}].emplace(pos, state);
				edges[{pos, state, head, pos, next_state}].emplace_back(rule, move(edge_seq));
			}
		}
		if (pos == inlen) { // all the words are consumed
			if (active.find(final_state) != active.end()) {// if final state is in the active states, the parse is successfull
				if (debug >= 1)
					cout << "Parse successfull" << nl;
				if (debug >= 2)
					print_graph(cout, inlen, act_states, act_edges);
				return;
			}
			else {
				for (int i = pos; i >= 0; i--) // find backward position where there was at least one active state (this is most probably where parsing fails)
					if (act_states[i].size()) {
						if (debug >= 1)
							cout << "Parse is not possible at position " << i << nl;
						throw ParseError( format("Parse is not possible at position {}",i) );
					}
			}
		}
		else {
			if (!active.size()) { // there is no active state at current position
				if (debug >= 1)
					cout << "No Active State Left at position " << pos << nl;
				// check next positions for active states (can only happen if a multi-word dictionary search result is found, e.g "et al" is valid but "et" is not valid)
			}
			else {
				vector<pair<int, Rule*>> dict_search; // dictionary search result is a vector of pairs(word_count,rule_ptr)
				// e.g. "turn off the lights", search at pos 0 returns [(1,turn),(2,turn off)] 
				search_trie_prefix(root, input_str.c_str() + char_pos, dict_search);
				if (debug >= 3) {
					cout << "Dict Search result for \"" << input_str.c_str() + char_pos << "\" : " << dict_search.size() << nl;
					for (auto[left_pos, rule] : dict_search) {
						cout << "*Left pos: " << left_pos << "Rule: " << *rule << nl;
					}
				}
				for (auto state : active) {
					auto it = dfa[state].find(token); // check if there is a state transition for current word
					if (it != dfa[state].end()) { // current token results in a next state
						int next_state = it->second;
						nodes[{pos + 1, next_state, token}].insert({ pos,state });
						act_edges[pos + 1].insert({ pos, state, token, pos + 1, next_state });
						act_states[pos + 1].insert(next_state);
					}

					for (auto[len, rule] : dict_search) { // iterate on dictionary search results
						int head = rule->head->id;
						auto it = dfa[state].find(head);
						if (it != dfa[state].end()) { // current token results in a next state
							int next_state = it->second;
							nodes[{pos + len, next_state, head}].insert({ pos,state });
							edges[{pos, state, head, pos + len, next_state}].emplace_back(rule, vector<Edge>{});

							act_edges[pos + len].insert({ pos, state, head, pos + len, next_state });
							act_states[pos + len].insert(next_state);
						}
					}
				}
			}
		}
		char_pos = input_str.find(' ', char_pos);
		if (char_pos == string::npos)
			char_pos = input_str.size();
		else
			char_pos++; // skip space
	}
	throw ParseError("No Active State Left");
}

string UnitTest::get_lines(ifstream& is, stringstream& ref) {
	// get all the lines until next line starting with "###<command>", returns the <command> or "" if EOF
	string line;
	while (getline(is, line) && line.substr(0, 3) != "###") {
		ref << line << '\n';
	}
	if (is)
		return line;
	return "";
}

void UnitTest::diff(string a, string b) {
	// outputs diff of two strings
	cout << "***";
	auto it_a = a.begin(), it_b = b.begin();
	auto end_a = a.end(), end_b = b.end();
	for (; it_a != end_a && it_b != end_b && *it_a == *it_b; ++it_a, ++it_b);
	ostream_iterator<char> it_os(cout);
	copy(a.begin(), it_a, it_os);
	cout << "<<<";
	copy(it_a, end_a, it_os);
	cout << ">>>";
	copy(it_b, end_b, it_os);
	cout << "***" << nl;
}
void enumerate(TreeNode* node, vector<vector<string>>& out, bool right);
string post_process(Grammar* grammar, vector<string>& in);
void UnitTest::test_case(const char* fname) {
	// loads an executes test cases from file "fname"
	int case_cnt = 0, success_cnt = 0;
	ifstream is(fname);
	if (!is) {
		cout << "Cannot open " << fname << nl;
		return;
	}
	string command;
	unique_ptr<Parser> parser;
	TreeNode *ptree = nullptr, *utree = nullptr, *ttree = nullptr;
	getline(is, command);
	string error;
	while (!command.empty()) {
		if (command == "###grammar") {
			try {
				stringstream grammar;
				command = get_lines(is, grammar);

				parser = make_unique<Parser>();
				GrammarParser gparser(parser.get());

				auto start = std::chrono::system_clock::now();
				gparser.parse_grammar(grammar);
				auto end = std::chrono::system_clock::now();
				cout << "ParseGrammar: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " mics\n";

				if (debug >= 1)
					parser->print_rules(cout);
				start = std::chrono::system_clock::now();
				parser->compile();
				end = std::chrono::system_clock::now();
				cout << "CompileGrammar: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " mics\n";
			}
			catch (GrammarError& e) {
				cerr << e.what() << nl;
				return;
			}
		}
		else if (command == "###input") {
			string input;
			getline(is, input);
			getline(is, command);
			if (debug_mem >= 1)
				cout << "Parse-begin FeatList count: " << FeatList::count << endl;
			try {
				auto start = std::chrono::system_clock::now();
				parser->parse(input);
				auto end = std::chrono::system_clock::now();
				cout << "Parse: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " mics\n";

				ptree = parser->make_tree(shared);
				utree = unify_tree(ptree, shared);
				ttree = parser->translate_tree(utree, shared);
				error = "";
			}
			catch (UnifyError&) {
				error = "*UnifyError";
			}
			catch (ParseError&) {
				error = "*ParseError";
			}
			if (debug_mem >= 1)
				cout << "Parse-end FeatList count: " << FeatList::count << endl;
		}
		else if (command == "###test") {
			string line;
			while (getline(is, line) && line.substr(0, 3) != "###") {
				vector<string> io;
				ltrim(line);
				rtrim(line);
				if (line.empty() || line[0] == '#')
					continue;
				split(io, line, ':');
				assert(io.size() == 2);
				rtrim(io[0]);
				ltrim(io[1]);
				case_cnt++;
				try {
					cout << io[0] << " : " << io[1] << nl;
					parser->parse(io[0]);
					ptree = parser->make_tree(shared);
					if (debug >= 1)
						print_tree(cout, ptree, true, true, false);
					utree = unify_tree(ptree, shared);
					if (debug >= 1)
						print_tree(cout, utree, true, true, false);
					ttree = parser->translate_tree(utree, shared);
					if (debug >= 1)
						print_tree(cout, ttree, true, true, true);
					bool found = false;
					for (auto& s : enumerate(parser.get(),ttree)) {
						if (s == io[1]) {
							found = true;
							cout << "  +" << s << nl;
						}
						else
							cout << "  "<< s << nl;
					}
					if (found)
						success_cnt++;
				}
				catch (UnifyError& e) {
					cout << "  *UnifyError: " << e.what() << nl;
				}
				catch (ParseError& e) {
					cout << "  *ParseError: " << e.what() << nl;
				}
			}
			if (is)
				command = line;
			else // eof
				command = "";
		}
		else {
			stringstream ref, out;
			if (error.empty()) {
				if(command == "###format")
					print_tree(out, ttree, false, false, false);
				else if (command == "###formatr")
					print_tree(out, ttree, false, false, true);
				else if(command == "###pformat")
					print_tree(out, ttree, true, false, false);
				else if (command == "###pformatr")
					print_tree(out, ttree, true, false, true);
				else if (command == "###pformat_ext")
					print_tree(out, ttree, true, true, false);
				else if (command == "###pformatr_ext")
					print_tree(out, ttree, true, true, true);
				else if(command == "###enum")
					for (auto& s : enumerate(parser.get(),ttree))
						out << s << '\n';
				else
				{
					cerr << "Invalid Section:" << command << nl;
					stringstream data;
					command = get_lines(is, data);
				}
			}
			else {
				out << error << nl;
			}
			string new_command = get_lines(is, ref);
			bool result = out.str() == ref.str();
			cout << command.substr(3) << ": " << (result ? "OK" : "NOK") << nl;
			if (!result) {
				if(debug >=1 )
					diff(out.str(), ref.str());
			}
			else
				success_cnt++;
			case_cnt++;
			command = move(new_command);
		}
	}
	cout << fname << ": " << success_cnt << "/" << case_cnt << nl;
	case_total += case_cnt;
	success_total += success_cnt;
}
void UnitTest::test_dir(const char* dirname) {
	// loads an executes all test cases in a directory with extension ".tst"
	debug = 0;
	debug_mem = 0;
	int case_total = 0, success_total = 0;
	namespace fs = experimental::filesystem;

	for (const auto & entry : fs::directory_iterator(dirname))
		if (entry.path().extension() == ".tst") {
			auto fname = entry.path().u8string();
			cout << "FILE:" << fname << nl;
			test_case(fname.c_str());
		}
}
void UnitTest::print_result() {
	cout << "TOTAL: " << success_total << "/" << case_total << nl;
}
