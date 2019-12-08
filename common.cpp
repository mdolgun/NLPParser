#include "stdafx.h"
#include "common.h"
#include "morph.h"

int debug, debug_mem, profile;

int TreeNode::id_count = 0;

int FeatList::count = 0;
ostream& FeatList::print(ostream& os) const {
	if (empty())
		return os;
	os << "[";
	bool flag = false;
	for (const auto& feat : *this) {
		if (flag)
			os << ",";
		else
			flag = true;
		os << feat.first << "=" << feat.second;
	}
	os << "]";
	return os;
}

ostream& FeatParam::print(ostream& os) const {
	if (empty())
		return os << "()";
	os << "(";
	bool flag = false;
	for (const auto& feat : *this) {
		if (flag)
			os << ",";
		else
			flag = true;
		os << feat.first;
		if (!feat.second.empty()) {
			os << "=" << feat.second;
		}
	}
	os << ")";
	return os;
}

ostream& Symbol::print(ostream& os) const {
	os << name;
	if (idx != -1)
		os << '<' << idx << '>';
	if (fparam)
		fparam->print(os);
	return os;
}

ostream & Prod::print(ostream& os) const {
	for (const auto& symbol : *this) {
		symbol->print(os) << " ";
	}
	if (cost)
		return os << "{" << cost << "}";
	return os;
}

ostream& Rule::print(ostream& os) const {
	return os << *head << " -> " << *left << " : " << *right << " " << *feat;
}

string Rule::sentence() const {
	// convert the left side (of terminals) to a sentence e.g. NP -> united states => "united states"
	string s;
	for (auto symbol : *left) {
		if (!s.empty())
			s.push_back(' ');
		s.append(symbol->name);
	}
	return s;
}

void print_partial_rule(ostream& os, Rule* rule, int start_pos, int end_pos) {
	auto start = rule->left->begin() + start_pos;
	auto end = end_pos == -1 ? rule->left->end() : rule->left->begin() + end_pos;
	for (auto ptr = start; ptr != end; ++ptr) {
		if (ptr != start)
			os << ' ';
		os << (*ptr)->name;
	}
}
/*
void enumerate(TreeNode* node, vector<string>& out, bool right) {
	// helper function
	vector<string> new_out;
	for (int i = 0; i < node->options.size(); ++i) {
		auto option = node->options[i];
		vector<string> temp;
		if (i == node->options.size() - 1)
			temp = move(out);
		else
			temp = out;
		auto& ref = right ? option->right : option->left;
		for (auto sub_node : ref) {
			if (!sub_node->nonterm)
				for (auto& s : temp)
					if ((*sub_node->name)[0] == '-')
						s += sub_node->name->substr(1);
					else if (!s.empty())
						s += " " + *sub_node->name;
					else
						s += *sub_node->name;
			else
				enumerate(sub_node, temp, right);
		}
		copy(make_move_iterator(temp.begin()), make_move_iterator(temp.end()), back_inserter(new_out));
	}
	out = move(new_out);
}
vector<string> enumerate(TreeNode* node, bool right) {
	// converts all possible inputs/outputs of a parse tree into a vector of strings
	// right=true converts outputs(right trees), right=false converts inputs(left trees)
	vector<string> out = { "" };
	enumerate(node, out, right);
	return out;
}
*/
void enumerate(TreeNode* node, vector<vector<string>>& out, bool right) {
	// helper function
	vector<vector<string>> new_out;
	for (int i = 0; i < node->options.size(); ++i) {
		auto option = node->options[i];
		vector<vector<string>> temp;
		if (i == node->options.size() - 1)
			temp = move(out);
		else
			temp = out;
		auto& ref = right ? option->right : option->left;
		for (auto sub_node : ref) {
			if (!sub_node->nonterm)
				for (auto& item : temp) {
					item.push_back(*sub_node->name);
				}
			else
				enumerate(sub_node, temp, right);
		}
		copy(make_move_iterator(temp.begin()), make_move_iterator(temp.end()), back_inserter(new_out));
	}
	out = move(new_out);
}

string post_process(Grammar* grammar,vector<string>& in) {
	static PostProcessor pp;
	string prev = "";
	vector<string> temp;
	string clipboard;
	temp.reserve(in.size());
	for (auto& item : in) {
		if (item == "+copy") {
			clipboard = item;
		}
		else if (item == "+paste") {
			temp.push_back(clipboard);
		}
		if (item[0] == '+') {
			auto it = grammar->suffixes.find(prev + item);
			if (it == grammar->suffixes.end()) {
				it = grammar->suffixes.find(item);
				if (it == grammar->suffixes.end())
					throw PostProcessError("Cannot find suffix default: " + item);
				temp.push_back(it->second);
			}
			else {
				temp.pop_back();
				temp.push_back(it->second);
			}
		}
		else
			temp.push_back(item);
		prev = item;
	}
	string result;
	bool first = true;
	for (auto& s : temp) {
		assert(s.size());
		if (s == "-")
			continue;
		bool suffix = s[0] == '-';
		if (first)
			first = false;
		else {
			if(!suffix)
				result += ' ';
		}
		if (suffix)
			result += s.substr(1);
		else
			result += s;
	}
	return pp.map_out(pp.process(pp.map_in(result)));
}

vector<string> enumerate(Grammar* grammar,TreeNode* node, bool right) {
	vector<vector<string>> result(1);
	vector<string> out;
	enumerate(node, result, right);
	for (auto& item : result) {
		out.push_back(post_process(grammar, item));
	}
	return out;
}

void print_tree(ostream& os, TreeNode* node, int level, int indent_size, bool extended, bool right) {
	// helper for print_tree
	indent(os, level * indent_size);
	os << *node->name;
	if (!node->nonterm)
		return;

	if (!extended && indent_size && node->options.size() == 1) {
		auto& node_list = right ? node->options[0]->right : node->options[0]->left;
		if (all_of(node_list.begin(), node_list.end(), [](auto& sub_node) { return !sub_node->nonterm; })) // if all symbol are terminals, write them in a single line
			indent_size = 0;
	}
	os << '(';
	bool first = true;
	for (auto option : node->options) {
		auto node_seq = right ? option->right : option->left;
		if (indent_size)
			os << nl;
		if (first) {
			if (extended) {
				indent(os, level * indent_size);
				if (option->rule->id != -1)
					os << '#' << option->rule->id;
				if (option->feat_list)
					os << *option->feat_list;
				if (indent_size && node_seq.size())
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
				if (option->rule->id != -1)
					os << '#' << option->rule->id;
				if (option->feat_list)
					os << *option->feat_list;
				if (indent_size && node_seq.size())
					os << nl;
			}
		}
		bool first2 = true;
		for (auto sub_node : node_seq) {
			if (first2)
				first2 = false;
			else
				if (indent_size)
					os << nl;
				else
					os << ' ';
			print_tree(os, sub_node, level + 1, indent_size, extended, right);
		}
	}
	if (indent_size)
		os << nl;
	indent(os, level * indent_size);
	os << ')';
}
void print_tree(ostream& os, TreeNode* tree, bool indented, bool extended, bool right) {
	/*
	prints a tree in formatted way into an outstream
	indented=true, writes as single line, otherwise multiple indented lines
	extended=true print extra information, like features and rule numbers
	right=true print right branches, right=false converts left branches
	*/
	if (indented)
		print_tree(os, tree, 0, 4, extended, right);
	else
		print_tree(os, tree, 0, 0, false, right);
	os << nl;
}

inline bool is_suffix(const string& s) {
	return strchr("+-", s[0]) != nullptr;
}
bool is_suffix(TreeNode* node) {
	if (!node->nonterm)
		return is_suffix(*node->name);
	for (auto option : node->options) {
		if (option->right.size() > 1 && is_suffix(option->right[0])) {
			return true;
		}
	}
	return false;
}

void normalize(TreeNode* node, vector<TreeNode*>& out) {
	if (!node->nonterm) {
		if (is_suffix(*node->name) && out.size() && out.back()->options.size() > 1) {
			for (auto option : out.back()->options) {
				option->right.push_back(node);
			}
		}
		else
			out.push_back(node);
	}
	else if (node->options.size() == 1) {
		for (auto sub_node : node->options[0]->right) {
			normalize(sub_node, out);
		}
	}
	else {
		bool suffix = false;
		for (auto option : node->options) {
			vector<TreeNode*> new_right;
			for (auto sub_node : option->right) {
				normalize(sub_node, new_right);
			}
			if (new_right.size() && is_suffix(new_right[0]))
				suffix = true;
			option->right = move(new_right);
		}
		if (suffix && out.size() && out.back()->options.size() > 1) {
			vector<OptionNode*> new_options;
			for (auto option : node->options)
				for (auto boption : out.back()->options) {
					OptionNode* new_option = new OptionNode(*boption);
					new_option->right = boption->right;
					new_option->right.insert(new_option->right.end(), option->right.begin(), option->right.end());
					new_options.push_back(new_option);
				}
			out.back()->options = move(new_options);
		}
		else {
			out.push_back(node);
		}
	}
}
inline void append(string& dst, string& src) {
	if (src[0] == '-') {
		dst.insert(dst.end(), src.begin() + 1, src.end()); // append to dst, skipping initial '-'
	} 
	else {
		dst.push_back(' ');
		dst += src;
	}
}

void print_word(ostream& os,vector<TreeNode*>& in,Grammar* grammar) {
	static string clipboard;
	static PostProcessor pp;
	string word;
	bool first = true;
	for (auto node : in) {
		if (node->nonterm) {
			if (first)
				first = false;
			else
				os << ' ';
			if (!word.empty()) {
				os << pp.map_out(pp.process(pp.map_in(word)));
				os << ' ';
			}
			os << '(';
			bool firstopt = true;
			for (auto option : node->options) {
				if (firstopt)
					firstopt = false;
				else
					os << '|';
				print_word(os, option->right, grammar);
			}
			os << ')';
			word = "";
		}
		else {
			string item = *node->name;
			if (item == "+copy") {
				clipboard = word;
			}
			else if (item == "+paste") {
				append(word, clipboard);
			}
			else if (item[0] == '+') {
				auto it = grammar->suffixes.find(word + item);
				if (it == grammar->suffixes.end()) {
					it = grammar->suffixes.find(item);
					if (it == grammar->suffixes.end())
						throw PostProcessError("Cannot find suffix default: " + item);
					append(word, it->second);
				}
				else {
					word = it->second;
				}
			}
			else if (item[0] == '-')
				append(word, item);
			else {
				if (!word.empty()) {
					if (first)
						first = false;
					else
						os << ' ';
					os << pp.map_out(pp.process(pp.map_in(word)));
				}
				word = item;
			}
		}
	}
	if (!word.empty()) {
		if (!first)
			os << ' ';
		os << pp.map_out(pp.process(pp.map_in(word)));
	}
}

void normalize(ostream& os, TreeNode* node, Grammar* grammar) {
	vector<TreeNode*> out;
	normalize(node, out);
	print_word(os, out, grammar);
}

bool unify_feat(shared_ptr<FeatList>& dst, FeatParam* param, shared_ptr<FeatList> src, bool down) {
	/* unification of "src" features into "dst" features, using filtering of "param", if unification fails returns nullptr
	if param is None, src is directly unified into dst
	otherwise param contains a pre - condition dict and a filter set,
	src is checked against pre - condition dict and then filtered with filter set and unified into dst
	returns true if unification is successfull else false
	*/
	if (debug >= 2) {
		cout << (down ? "D:" : "U:");
		dst->print(cout);
		cout << ",";
		if (param)
			param->print(cout);
		cout << ",";
		src->print(cout);
		cout << "->";
	}
	if (param == nullptr) {
		for (auto&[name, val] : *src) {
			auto it = dst->find(name);
			if (it != dst->end()) {
				if (it->second != val) {
					if (debug >= 2)
						cout << '*' << nl;
					return false;
				}
			}
			else {
				if (dst.use_count() > 1) {
					if (debug >= 2)
						cout << '+' << dst.use_count();
					dst = make_shared<FeatList>(*dst);
				}
				(*dst)[name] = val;
			}
		}
	}
	else {
		for (auto [name, val] : *param) {
			if (val.empty() || val[0]=='*') { // empty parameter or renamed parameter, pass-through the src values into dst
				string src_name, dst_name;
				if (val.empty())
					dst_name = src_name = name;
				else if (down) {
					dst_name = name;
					src_name = val.substr(1);
				}
				else {
					dst_name = val.substr(1);
					src_name = name;
				}
				auto sit = src->find(src_name);
				if (sit != src->end()) { // parameter exist in src
					auto dit = dst->find(dst_name);
					if (dit != dst->end()) { // same feat exist in dst too
						if (sit->second != dit->second) { // feat values doesnot match
							if (debug >= 2)
								cout << '*' << nl;
							return false;
						}
					}
					else { // add src feat into dst
						if (dst.use_count() > 1) { // if it is a shared feat, copy before update
							if (debug >= 2)
								cout << '+' << dst.use_count();
							dst = make_shared<FeatList>(*dst);
						}
						(*dst)[dst_name] = sit->second;
					}
				}
			}
			else { // parameter with a value
				if (!down) { // up-propagation
					auto sit = src->find(name);
					if (sit != src->end()) { // if src has the param feat
						if (sit->second != val) { // if param and src values doesn't match, unification fails
							if (debug >= 2)
								cout << '*' << nl;
							return false;
						}
					}
				}
				else { // down-propagation
					auto dit = dst->find(name);
					if (dit != dst->end()) { // if dst has the param feat
						if (dit->second != val) { // if param and dst values doesn't match, unification fails
							if (debug >= 2)
								cout << '*' << nl;
							return false;
						}
					}
					else { // add param feat into dst
						if (dst.use_count() > 1) { // if it is a shared feat, copy before update
							if (debug >= 2)
								cout << '+' << dst.use_count();
							dst = make_shared<FeatList>(*dst);
						}
						(*dst)[name] = val;
					}
				}
			}
		}
	}
	if (debug >= 2)
		cout << *dst << nl;
	return true;
}

inline bool is_equal(FeatPtr a, FeatPtr b) {
	if (a)
		if (b)
			return *a == *b;
		else
			return false;
	else
		if (b)
			return false;
		else
			return true;

}
struct PartitionPred {
	// predicate used for partitioning checks whether it is equal to feat(constructor parameter)
	FeatPtr& feat;
	PartitionPred(FeatPtr& _feat) : feat(_feat) { }
	bool operator()(tuple<FeatPtr, OptionNodePtr>& other) {
		return is_equal(feat, get<0>(other));
	}
};
void partition(TreeNodePtr node,FeatPtr parent_feat, FeatParam* fparam, vector<tuple<FeatPtr, TreeNodePtr>>& parts) {
	// For given node, partitions pairs of options/combined features 
	vector<tuple<FeatPtr, OptionNodePtr>> feats;
	feats.reserve(node->options.size());
	for (auto option : node->options) {
		auto feat_list = parent_feat;
		if (!unify_feat(feat_list, fparam, option->feat_list, false)) {
			feat_list = nullptr;
		}
		feats.emplace_back(feat_list, option);
	}
	auto feat = get<0>(*feats.begin()); // first feature
	auto end = std::partition(feats.begin() + 1, feats.end(), PartitionPred(feat));
	if (end == feats.end()) { // there is only a single partition
		if (!feat) // all items are nullptr
			//throw UnifyError("UnifyError");
			return;
		parts.emplace_back(feat, node);
		return;
	}
	auto begin = feats.begin();
	
	for (;;) {
		if (feat) {
			TreeNode* new_node = new TreeNode(node);
			for (auto it = begin; it != end; ++it) {
				new_node->options.push_back(get<1>(*it));
			}
			parts.emplace_back(feat, new_node);
		}
		if (end == feats.end())
			break;
		begin = end;
		feat = get<0>(*begin);
		end = std::partition(begin + 1, feats.end(), PartitionPred(feat));
	}
}

TreeNodePtr unify_tree(TreeNodePtr node,unordered_set<TreeNode*>* visited) {
	// unifies a tree, if visited is not null, checks if a node is previously visited and it will not visit it again
	if (visited && visited->count(node)) // node has already been visited
		return node;
	string last_error;
	if (!node->nonterm)
		return node;
	vector<OptionNodePtr> new_options;
	for (auto option : node->options) {
		try {
			Prod* body = option->rule->left;
			vector<tuple<vector<TreeNodePtr>, FeatPtr>> worklist = { { vector<TreeNodePtr>{},option->feat_list } };
			for (int rulepos = 0; rulepos < option->left.size(); rulepos++) {
				auto sub_node = option->left[rulepos];
				if (!sub_node->nonterm) {
					for (auto& worklist_item : worklist) {
						get<0>(worklist_item).push_back(sub_node);
					}
					continue;
				}
				FeatParam* fparam = (*body)[rulepos]->fparam;
				sub_node = unify_tree(sub_node, visited);
				vector<tuple<vector<TreeNodePtr>, FeatPtr>> new_worklist;
				for (auto&[work_seq, work_feat] : worklist) {
					vector<tuple<FeatPtr, TreeNodePtr>> parts;
					partition(sub_node, work_feat, fparam, parts);
					for (auto&[par_feat, par_node] : parts) {
						new_worklist.emplace_back(work_seq, par_feat);
						get<0>(new_worklist.back()).push_back(par_node);
					}
				}
				if (!new_worklist.size())
					throw UnifyError(format("Unify {} {}", node->name, sub_node->name));
				worklist = move(new_worklist);
			}
			for (auto[work_seq, work_feat] : worklist) {
				OptionNodePtr new_option = new OptionNode(option->rule, work_feat);
				new_option->left = move(work_seq);
				new_options.push_back(new_option);
			}
		}
		catch (UnifyError& e) {
			last_error = e.what();
		}
	}
	if (!new_options.size())
		throw UnifyError(last_error);
	node->options = move(new_options);
	return node;
}
TreeNodePtr unify_tree(TreeNodePtr node, bool shared) {
	if (shared) {
		unordered_set<TreeNode*> visited;
		return unify_tree(node, &visited);
	}
	else
		return unify_tree(node, nullptr);
}

void dot_print(ostream& os, TreeNode* node, unordered_set<TreeNode*>&visited,bool left,bool right) {
	if (visited.find(node) != visited.end()) // skip already visited nodes
		return;
	visited.insert(node);
#ifdef DEBUG
	if (node->start_pos || node->end_pos)
		os << "\"" << node << "\"[label=\"" << node->id << ' ' << *node->name << "[" << node->start_pos << ":" << node->end_pos << "]\"]\n";
	else
		os << "\"" << node << "\"[label=\"" << node->id << ' ' << *node->name << "\"]\n";

#else
	if (node->start_pos || node->end_pos)
		os << "\"" << node << "\"[label=\"" << *node->name << "[" << node->start_pos << ":" << node->end_pos << "]\"]\n";
	else
		os << "\"" << node << "\"[label=\"" << *node->name << "\"]\n";

#endif
	for (auto& option : node->options) {
		os << "\"" << option << "\"[label=\"#" << option->rule->id << *option->feat_list << "\"];\n";
		os << "\"" << node << "\" -> \"" << option << "\" [label=\"O\"];\n";
		int idx = 0;
		if (left)
			for (auto& sub_node : option->left) {
				os << "\"" << option << "\" -> \"" << sub_node << "\" [label=\"L" << ++idx << "\"];\n";
				dot_print(os, sub_node, visited, left, right);
			}
		idx = 0;
		if (right)
			for (auto& sub_node : option->right) {
				os << "\"" << option << "\" -> \"" << sub_node << "\" [label=\"R" << ++idx << "\"];\n";
				dot_print(os, sub_node, visited, left, right);
			}
	}
}
void dot_print(ostream& os, TreeNode* node, bool left, bool right) {
	// print <node> to <os> in a graphviz format
	//<left>: true if left-branches of the tree is printed
	//<right>: true if right-branches of the tree is printed
	unordered_set<TreeNode*> visited;
	os << "digraph {\n";
	os << "graph[ordering = \"out\"];\n";
	dot_print(os, node, visited, left, right);
	os << "}\n";
}
void dot_print(string fname, TreeNode* node, bool left, bool right) {
	// print <node> to file <fname> in a graphviz format
	//<left>: true if left-branches of the tree is printed
	//<right>: true if right-branches of the tree is printed
	ofstream os(fname);
	if (!os)
		return;
	dot_print(os, node, left, right);
}

void add_trie(TrieNode* node, const string& s, Rule* value) {
	// add a string to a character trie, saving Rule* as value
	if (debug >= 3)
		cout << "Adding To Trie: " << s << '\n';
	const char* str = s.c_str();
	while (*str) {
		auto idx = node->keys.find(*str);
		if (idx == string::npos) {
			TrieNode* new_node = new TrieNode;
			node->keys.push_back(*str);
			node->children.push_back(new_node);
			node = new_node;
		}
		else {
			node = node->children[idx];
		}
		str++;
	}
	node->values.push_back(value);
}
void dump_trie(TrieNode* node, string& buf) {
	// dumps all keys in a trie with associated values (i.e. rules)
	for (auto rule : node->values) {
		cout << buf << ": " << *rule << '\n';
	}
	buf.push_back(' ');
	for (int i = 0; i < node->keys.size(); ++i) {
		buf.back() = node->keys[i];
		dump_trie(node->children[i], buf);
	}
	buf.pop_back();
}
void dump_trie(TrieNode* node) {
	// dumps all keys in a trie with associated values (i.e. rules)
	string buf;
	dump_trie(node, buf);
}
int get_enum(initializer_list<string> list, string& param) {
	// utility func to return index of matching param from an initializer list, -1 if no match occurs
	int idx = 0;
	for (auto& item : list) {
		if (item == param)
			return idx;
		++idx;
	}
	return -1;
}

void search_trie_prefix(TrieNode* node, const char* str, vector<pair<int, Rule*>>& result) {
	int word_pos = 0;
	while (*str) {
		if (*str == ' ') {
			word_pos++;
			for (auto& item : node->values)
				result.emplace_back(word_pos, item);
		}
		auto idx = node->keys.find(*str);
		if (idx == string::npos)
			return;
		node = node->children[idx];
		str++;
	}
	for (auto& item : node->values)
		result.emplace_back(word_pos + 1, item);
}

void split(vector<string>& result, const string &s, char delim) {
	// splits a string into vector of strings using the <delim> character
	stringstream ss(s);
	string item;

	while (getline(ss, item, delim)) {
		result.push_back(item);
	}
}

void indent(ostream& os, int size) {
	// writes <size> spaces to <os>
	for (int i = 0; i < size; i++)
		os << ' ';
}

// trim from start (in place)
void ltrim(std::string &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) { return !isascii(ch) || !std::isspace(ch); } ));
}

// trim from end (in place)
void rtrim(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) { return !isascii(ch) || !std::isspace(ch); }).base(), s.end());
}