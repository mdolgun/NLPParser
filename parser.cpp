#include "stdafx.h"
#include "parser.h"

ostream& operator<<(ostream& os, const State& state) {
	return os << "(" << state.first << "," << state.second << ")";
}
ostream& operator<<(ostream& os, StateSet& stateset) {
	return join(os, stateset, " ");
}

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

//ostream& operator<< (ostream& os, const Edge& edge) {
//	return os << '(' << get<0>(edge) << ',' << get<1>(edge) << ',' << symbol_table.get(get<2>(edge)) << ',' << get<3>(edge) << ',' << get<4>(edge) << ')';
//}

ostream& operator<< (ostream& os, const Edge& edge) {
	return os << '(' << get<0>(edge) << ',' << get<1>(edge) << ',' << get<2>(edge) << ',' << get<3>(edge) << ',' << get<4>(edge) << ')';
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

void Parser::print_edge(ostream& os, Edge& edge) {
	os << '(' << get<0>(edge) << ',' << get<1>(edge) << ',' << symbol_table.get(get<2>(edge)) << ',' << get<3>(edge) << ',' << get<4>(edge) << ')';
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
			os << input[i];
		}
		return;
	}
	auto& edge_seq_list = edges[top_edge];
	if (!extended && indent_size) {
		if (edge_seq_list.size() == 1 && all_of(get<1>(edge_seq_list[0]).begin(), get<1>(edge_seq_list[0]).end(), [this](auto& edge) { return !symbol_table.nonterminal(get<2>(edge)); })) {
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
	auto[start_pos, start_state, parent_symbol, end_pos, end_state] = edge;

	tuple<int, int, int> key{ start_pos, parent_symbol, end_pos };
	auto it = visited.find(key);
	if (it != visited.end()) // this node is already visited
		return it->second; // return the associated node

	TreeNode* node;
	if (parent_symbol == -1) { // a dictionary-terminal, there is no associated "id" with it
		node = new TreeNode(&input[start_pos], false);
	}
	else {
		node = new TreeNode(&symbol_table.get(parent_symbol), symbol_table.nonterminal(parent_symbol));
	}
	node->start_pos = start_pos;
	node->end_pos = end_pos;
	visited[key] = node;
	if (!node->nonterm) // if terminal no subtree exists under it
		return node;

	for (auto& [rule, edge_seq] : edges[edge]) {
		OptionNode* option = new OptionNode(rule, rule->feat);
		/*if (rule->id == -1) { // this is a dictionary-rule (i.e. consists of only non-terminals), then create a new terminal Node for each word
			for (auto& symbol : *rule->left) {
				option->left.push_back(new TreeNode(&symbol->name, false));
			}
		}
		else */ { // this is a grammar-rule, make sub-trees recursively
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
	auto [start_pos, start_state, parent_symbol, end_pos, end_state] = edge;
	TreeNode* node;
	if (parent_symbol == -1) { // a dictionary-terminal
		node = new TreeNode(&input[start_pos], false);
	}
	else {
		node = new TreeNode(&symbol_table.get(parent_symbol), symbol_table.nonterminal(parent_symbol));
	}
	node->start_pos = start_pos;
	node->end_pos = end_pos;

	if (!node->nonterm) // if terminal no subtree exists under it
		return node;
	for (auto&[rule, edge_seq] : edges[edge]) {
		OptionNode* option = new OptionNode(rule, rule->feat);
		/*if (rule->id == -1) { // this is a dictionary-rule (i.e. consists of only non-terminals), then create a new terminal Node for each word
			for (auto& symbol : *rule->left) {
				option->left.push_back(new TreeNode(&symbol->name, false));
			}
		}
		else */ { // this is a grammar-rule, make sub-trees recursively
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

int get_cost(TreeNode* node) {
	if (!node->options.size())
		return 0;
	int cost = INT_MAX;
	for (auto option : node->options)
		if (option->cost < cost)
			cost = option->cost;
	return cost;
}

TreeNode* Parser::make_trans_tree(int id,FeatParam* fparam,FeatPtr parent_feat) {
	// make a translated sub tree (i.e. RHS-only subtree) from current id,parent feature,feature parameters
	TreeNode* node = new TreeNode(&symbol_table.get(id), true);
	for (auto ruleno : ruledict[id]) {
		Rule* rule = get_rule(ruleno);
		try {
			FeatPtr feat_list = rule->feat;
			if( !unify_feat(feat_list, fparam, parent_feat, true, rule->check_list) )
				throw UnifyError("UnifyError");
			OptionNode* option = new OptionNode(rule, feat_list);
			option->cost = rule->right->cost;
			for (auto symbol : *rule->right) {
				if (symbol->nonterminal) {
					assert(symbol->idx == -1); // a non-referenced nonterminal
					TreeNode* sub_node = make_trans_tree(symbol->id, symbol->fparam, feat_list);
					option->cost += get_cost(sub_node);
					option->right.push_back(sub_node);
				}
				else { // terminal
					//assert(!symbol->nonterminal); // cannot be a non-referenced nonterminal
					TreeNode* sub_node = new TreeNode(&symbol->name, symbol->nonterminal);
					option->right.push_back(sub_node);
				}
			}
			node->options.push_back(option);
			if (rule->right->cut)
				break;
		}
		catch (UnifyError&) {
		}
	}
	if (!node->options.size())
		throw UnifyError(format("make_trans_tree {}", symbol_table.get(id)));
	return node;
}

struct FeatPred {
	// unary predicate used to match "feat_vec" (downward-unified options' features) to a node for "translate_tree_shared"
	vector<pair<FeatPtr, int>> feat_vec; // unified value, and option index
	FeatPred(int size) { feat_vec.reserve(size); }
	bool operator()(TreeNode* node) {
		if (feat_vec.size() != node->options.size())
			return false;
		for (int i = 0; i < feat_vec.size(); ++i) {
			if (feat_vec[i].first != node->options[i]->feat_list)
				return false;
		}
		return true;
	}
};


TreeNode* Parser::translate_tree_shared(unordered_map<TreeNode*, vector<TreeNode*>>& visited,TreeNode* node, FeatParam* fparam, FeatPtr parent_feat) {
	// translate a sub-tree
	auto it = visited.find(node);
	if (it == visited.end()) { // if "node" is visited first time, then initialize an empty vector as value
		it = visited.emplace(node, vector<TreeNode*>{}).first;
	}
	vector<TreeNode*>& node_vec = it->second;

	FeatPred feat_pred(node->options.size());
	for (int i = 0; i < node->options.size(); ++i) {
		FeatPtr feat = node->options[i]->feat_list;
		if (!unify_feat(feat, fparam, parent_feat, true))
			continue;
		feat_pred.feat_vec.emplace_back(move(feat), i);
	}

	auto node_it = find_if(node_vec.begin(), node_vec.end(), feat_pred);
	if (node_it != node_vec.end())
		return *node_it;
	string last_error;
	// if no cached value matches current options' features, then create new options and a new node
	vector<OptionNode*> new_options;
	for (int i = 0; i < feat_pred.feat_vec.size(); ++i) {
		auto& p = feat_pred.feat_vec[i];
		auto option = node->options[p.second];
		try {
			auto new_option = new OptionNode(option->rule, p.first);
			new_option->left = option->left; //??
			option = new_option;
			option->cost = option->rule->right->cost;

			assert(option->right.size() == 0);

			Prod* right = option->rule->right;
			for (auto symbol : *right) {
				TreeNode* sub_node;
				if (symbol->idx != -1) { // a referenced nonterminal
					sub_node = option->left.at(symbol->idx);
					sub_node = translate_tree_shared(visited, sub_node, symbol->fparam, option->feat_list);
				}
				else if (!symbol->nonterminal) { // a terminal
					sub_node = new TreeNode(&symbol->name, false);
				}
				else { // a non-referenced non-terminal
					sub_node = make_trans_tree(symbol->id, symbol->fparam, option->feat_list);
				}
				option->cost += get_cost(sub_node);
				option->right.push_back(sub_node);
			}
			new_options.push_back(option);
		}
		catch (UnifyError& e) {
			last_error = e.what();
		}
	}
	if (!new_options.size())
		throw UnifyError(last_error);

	node = new TreeNode(node->name, node->nonterm);
	node->options = move(new_options);
	node_vec.push_back(node);
	return node;
}

TreeNode* Parser::translate_tree(TreeNode* node,FeatParam* fparam,FeatPtr parent_feat) {
	// translate a sub-tree
	string last_error;
	vector<OptionNode*> new_options;
	for (auto option : node->options) {
		try {
			Prod* right = option->rule->right;
			OptionNode* new_option = new OptionNode(option->rule, option->feat_list);
			if (!unify_feat(new_option->feat_list, fparam, parent_feat, true))
				continue;
			new_option->left = option->left;
			new_option->cost = option->rule->right->cost;
			for (auto symbol : *right) {
				TreeNode* sub_node;
				if (symbol->idx != -1) { // a referenced nonterminal
					sub_node = new_option->left.at(symbol->idx);
					sub_node = translate_tree(sub_node, symbol->fparam, new_option->feat_list);
				}
				else if (!symbol->nonterminal) { // a terminal
					sub_node = new TreeNode(&symbol->name,false);
				}
				else { // a non-referenced non-terminal
					if (!unify_feat(new_option->feat_list, fparam, parent_feat, true))
						throw UnifyError("UnifyError");
					sub_node = make_trans_tree(symbol->id, symbol->fparam, new_option->feat_list);
				}
				new_option->cost += get_cost(sub_node);
				new_option->right.push_back(sub_node);
			}
			new_options.push_back(new_option);
			if (option->rule->right->cut)
				break;
		}
		catch (UnifyError& e) {
			last_error = e.what();
		}
	}
	if (!new_options.size())
		throw UnifyError(last_error);
	TreeNode* new_node = new TreeNode(node);
	new_node->options = move(new_options);
	return new_node;
}

TreeNode* Parser::translate_tree(TreeNode* parent_node,bool shared) {
	FeatPtr feat_list = FeatPtr(new FeatList);
	if (shared) {
		unordered_map<TreeNode*, vector<TreeNode*>> visited;
		return translate_tree_shared(visited, parent_node, nullptr, feat_list);
	}
	else {
		return translate_tree(parent_node, nullptr, feat_list);
	}
}

void split_sentence(string& input, vector<string>& words, vector<int>& word_pos) {
	static std::regex ws_re("\\s+");
	static std::sregex_token_iterator end;
	for (auto it = std::sregex_token_iterator(input.begin(), input.end(), ws_re, -1); it != end; ++it) {
		words.push_back(*it);
		word_pos.push_back(it->first - input.begin());
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

	//int tail_id = symbol_table.map("*tail");
	//if (tail_id != -1)
	//	nullable[tail_id] = true;

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


void Parser::match_and_reduce(unordered_map<BackParam, StateSet>& nodes, unordered_set<int>& active, const char* in, vector<Edge>& edge_seq, Prod* body, int rulepos, int pos, int state) {
	// !!!INCOMPLETE!!!
	int  symbol = body->at(rulepos)->id;
	for (auto[prev_pos, prev_state] : nodes[{pos, state, symbol}]) {
		if (rulepos == 0) {
			for (auto&[rule, sub_edge_seq] : edges[{prev_pos, prev_state, symbol, pos, state}]) {
				auto it = rule->feat->find("tail");
				if (it == rule->feat->end()) {
					edge_seq.emplace_back(prev_pos, prev_state, symbol, pos, state);
				}
				else if (it->second.compare(0, it->second.size(), in) == 0) {
					edge_seq.emplace_back(prev_pos, prev_state, symbol, pos, state);
				}
				else
					continue;
				int head;
				int next_state = dfa[prev_state].at(head);
				if (debug >= 2) {
					cout << "StateTrans:" << prev_state << "," << symbol_table.get(head) << "->" << next_state << nl;
				}
				active.insert(next_state);
				nodes[{pos, next_state, head}].emplace(prev_pos, prev_state);
				Edge new_edge(prev_pos, prev_state, head, pos, next_state);
				if (edges.count(new_edge) == 0) {
					//edge_list.push_back(new_edge);
					if (debug >= 3)
						cout << "NewEdgeCreated\n";
				}
				reverse(edge_seq.begin(), edge_seq.end());
				edges[new_edge].push_back({ rule, edge_seq });
			}
		}
		else {
			auto size = edge_seq.size();
			edge_seq.emplace_back(prev_pos, prev_state, symbol, pos, state);
			match_and_reduce(nodes, active, in, edge_seq, body, rulepos - 1, prev_pos, prev_state);
			edge_seq.resize(size); // truncate the sequence
		}
	}
}

int get_word_count(string& s) {
	// return number of words in a string, where words are seperated by single spaces
	if (s.empty())
		return 0;
	return count(s.begin(), s.end(), ' ') + 1;
}

void Parser::parse(string input_str) {
	// parses input string using current grammar, throwing ParseError if parsing fails, the parse tree can be later retrieved from "edges"
	static regex input_re("'(?=[^t])");
	input_str = regex_replace(input_str, input_re, " '"); // insert space before all "'" except followed by "t"

	input.clear();
	edges.clear();

	vector<int> input_pos;
	split_sentence(input_str, input, input_pos);

	int tail_id = symbol_table.map("*");

	int inlen = input.size();
	unordered_map<BackParam, StateSet> nodes; // maps(pos, state, symbol) to set of(oldpos, oldstate) (i.e adds an arc from(pos, state) to(oldpos, oldstate) labeled with symbol)
	// <startpos,startstate,symbol,endpos,endstate>
	//unordered_map<Edge, vector<vector<Edge>>> edges; // maps an edge to sub - edges of a production e.g. (p0, s0, S, p2, s2') -> [ (p0,s0,NP,p1,s1), (p1,s1,VP,p2,s2) ] where s0,S->s2'
	unordered_map<pair<int,int>, vector<tuple<int,int,int,Rule*,vector<Edge>>>> expected; // <end_pos,nt> -> <start_pos,start_state,end_state,Rule*,EdgeSeq>*
	int start_symbol = symbol_table.map("S"); // start_symbol == 1
	int final_state = dfa[0].at(start_symbol); 
	top_edge = Edge(0, 0, start_symbol, inlen, final_state);
	vector<unordered_set<int>> act_states(inlen+1); // active set of states for each position
	vector<unordered_set<Edge>> act_edges(inlen+1); // active set of edges for each position
	act_states[0].insert(0); // add initial state to initial position
	for (int pos = 0; pos <= inlen; pos++) {
		int token; // current token(word) id, for end of word or not defined, it has value of -1
		int char_pos; // char_pos of the next word in the sentence(used for dictionary prefix search)
		if (pos == inlen) { // end-of-input
			token = -1;
			char_pos = input_str.size();
		}
		else {
			token = symbol_table.map(input[pos]);
			char_pos = input_pos[pos];
		}
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
			if (debug >= 2) {
				cout << "Checking edge for reduction:"; print_edge(cout, edge); cout << nl;
			}
				
			auto [start_pos, start_state, edge_symbol, end_pos, end_state] = edge;
			assert(pos == end_pos);

			for (auto[start_pos, start_state, end_state, rule, edge_seq] : expected[{start_pos, edge_symbol}]) {
				edge_seq.push_back(edge);
				int head = rule->head->id;
				int rulepos = edge_seq.size();
				end_state = get_next_state(end_state, edge_symbol);
				assert(end_state != -1);
				int tail_pos = end_pos;

				for (; rulepos < rule->left->size(); ++rulepos, ++tail_pos) {
					Symbol* symbol = rule->left->at(rulepos);
					if (symbol->nonterminal)
						break;
					if (tail_pos >= inlen) // tail exceeds input length
						goto mismatch;
					if (input[tail_pos] != symbol->name) // tail mismatch
						goto mismatch;
					edge_seq.emplace_back(tail_pos, -1, -1, tail_pos + 1, -1);
				}

				if (tail_pos != end_pos) { // there are matched tail terminals
					end_state = get_next_state(end_state, tail_id);
					if (end_state == -1)
						continue;
				}
				if (rulepos != rule->left->size()) {
					Symbol* symbol = rule->left->at(rulepos);
					if (debug >= 2) {
						cout << "Expected partial match: (" << tail_pos << "," << symbol->name << ") : " << edge_seq << nl;
					}
					expected[{tail_pos, symbol->id}].emplace_back(start_pos, start_state, end_state, rule, move(edge_seq));
					if (debug >= 2) {
						cout << "Inserting * state transition to pos " << tail_pos << " : " << end_state;
					}
					act_states[tail_pos].insert(end_state);
				}
				else {
					end_state = get_next_state(start_state, head);
					assert(end_state != -1);
					active.insert(end_state);
					nodes[{tail_pos, end_state, head}].emplace(start_pos, start_state);
					Edge new_edge(start_pos, start_state, head, tail_pos, end_state);
					if (tail_pos == pos) {
						if (edges.count(new_edge) == 0) {
							edge_list.push_back(new_edge);
						}
					}
					else {
						act_edges[tail_pos].insert(new_edge);
					}
					edges[new_edge].emplace_back(rule, move(edge_seq));
				}
				mismatch:;
			}

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
					edges[new_edge].emplace_back(rule, edge_seq);

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
				if (debug >= 2)
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
					int next_state = get_next_state(state, token); 
					if (next_state != -1) { // check if there is a state transition for current word
						nodes[{pos + 1, next_state, token}].insert({ pos,state });
						act_edges[pos + 1].insert({ pos, state, token, pos + 1, next_state });
						act_states[pos + 1].insert(next_state);
					}

					for (auto[len, rule] : dict_search) { // iterate on dictionary search results
						int head = rule->head->id;
						int next_state = get_next_state(state, head);
						if (next_state == -1)
							continue;
						vector<Edge> edge_seq;
						for (int i = 0; i < len; ++i)
							edge_seq.emplace_back(pos+i, -1, -1, pos+i+1, -1);
						if (len == rule->left->size()) { // full match
							nodes[{pos + len, next_state, head}].insert({ pos,state });
							edges[{pos, state, head, pos + len, next_state}].emplace_back(rule, move(edge_seq));
							act_edges[pos + len].insert({ pos, state, head, pos + len, next_state });
							act_states[pos + len].insert(next_state);
						}
						else { // partial match

							int next_state = get_next_state(state, tail_id);
							if (next_state == -1)
								continue;
							if (debug >= 2) {
								cout << "Expected partial match: (" << (pos + len) << "," << rule->left->at(len)->name << ") : " << edge_seq << nl;
								cout << "Inserting * state transition to pos " << (pos + len) << " : " << next_state << nl;
							}
							act_states[pos + len].insert(next_state);
							expected[{pos + len, rule->left->at(len)->id}].emplace_back(pos, state, next_state, rule, move(edge_seq));
						}
					}
				}
			}
		}
	}
	throw ParseError("No Active State Left");
}

