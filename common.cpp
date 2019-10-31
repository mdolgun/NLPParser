#include "stdafx.h"
#include "common.h"
#include "util.h"

int debug, debug_mem;

int TreeNode::id_count = 0;
#define NEW_UNIFY

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

vector<string> enumerate(TreeNode* node,bool right) {
	// converts all possible inputs/outputs of a parse tree into a vector of strings
	// right=true converts outputs(right trees), right=false converts inputs(left trees)
	vector<string> out = { "" };
	enumerate(node, out, right);
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
		if (all_of(node_list.begin(), node_list.end(), [](auto& sub_node) { return !sub_node->nonterm; }))
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
			if (val.empty()) { // empty parameter, pass-through the src values into dst
				auto sit = src->find(name);
				if (sit != src->end()) { // parameter exist in src
					auto dit = dst->find(name);
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
						(*dst)[name] = sit->second;
					}
				}
			}
			else { // parameter with a value
				if (!down) { // up-propagation
					if (val[0] == '*') {
						auto dit = dst->find(val.substr(1));
						if (dit == dst->end())
							continue;
						val = dit->second;
					}
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
					if (val[0] == '*') {
						auto sit = src->find(val.substr(1));
						if (sit == src->end())
							continue;
						val = sit->second;
					}
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
#ifdef NEW_UNIFY
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
			throw UnifyError("UnifyError");
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
				for (auto&[work_seq, work_feat] : worklist) {
					vector<tuple<FeatPtr, TreeNodePtr>> parts;
					partition(sub_node, work_feat, fparam, parts);
					vector<tuple<vector<TreeNodePtr>, FeatPtr>> new_worklist;
					for (auto&[par_feat, par_node] : parts) {
						new_worklist.emplace_back(work_seq, par_feat);
						get<0>(new_worklist.back()).push_back(par_node);
					}
					worklist = move(new_worklist);
				}
			}
			for (auto[work_seq, work_feat] : worklist) {
				OptionNodePtr new_option = new OptionNode(option->rule, work_feat);
				new_option->left = move(work_seq);
				new_options.push_back(new_option);
			}
		}
		catch (UnifyError&) {
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
#else
TreeNodePtr unify_tree(TreeNodePtr parent_node) {
	string last_error;
	if (!parent_node->nonterm)
		return parent_node;
	vector<OptionNodePtr> new_options;
	for (auto option : parent_node->options) {
		try {
			Prod* body = option->rule->left;
//			FeatList* parent_feat = option->rule->feat; /* or option->feat_list */
			vector<tuple<vector<TreeNodePtr>, FeatPtr>> worklist = { { vector<TreeNodePtr>{},option->feat_list } };
			// make cartesian product of parent & child options and check unification
			for (int rulepos = 0; rulepos < option->left.size(); rulepos++) {
				auto sub_node = option->left[rulepos];
				if (!sub_node->nonterm) {
					for (auto& worklist_item : worklist) {
						get<0>(worklist_item).push_back(sub_node);
					}
					continue;
				}
				FeatParam* fparam = (*body)[rulepos]->fparam;
				sub_node = unify_tree(sub_node);
				vector<tuple<vector<TreeNodePtr>, FeatPtr>> new_worklist;
				for (auto& [work_seq, work_feat] : worklist) {
					vector<tuple<TreeNodePtr, FeatPtr>> sub_worklist;
					bool first = true;
					for (auto sub_option : sub_node->options) {
						FeatPtr out_feat;
						if( !unify_feat(work_feat, fparam, sub_option->feat_list, false) ) {
							last_error = "Unify Error";
							continue;
						}
						out_feat = work_feat;
						bool found = false;
						for (auto[subwork_node, subwork_feat] : sub_worklist) {
							if (*subwork_feat == *out_feat) {
								subwork_node->options.push_back(sub_option);
								found = true;
								break;
							}
						}
						if (!found) {
							TreeNode* new_sub_node = new TreeNode(sub_node);
							new_sub_node->options.push_back(sub_option);
							sub_worklist.emplace_back(new_sub_node, out_feat);
						}
					}
					for (auto[subwork_node, subwork_feat] : sub_worklist) {
						new_worklist.emplace_back(work_seq, subwork_feat);
						get<0>(new_worklist.back()).push_back(subwork_node);
					}
				}
				worklist = move(new_worklist);
			}
			//if (debug >= 1) {
			//	cout << nl;
			//}
			for (auto[work_seq, work_feat] : worklist) {
				OptionNodePtr new_option = new OptionNode(option->rule,work_feat);
				new_option->left = move(work_seq);
				new_options.push_back(new_option);
			}
		}
		catch (UnifyError&) {
		}
	}
	if (!new_options.size())
		throw UnifyError(last_error);
	parent_node->options = move(new_options);
	return parent_node;
}
#endif
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

	unordered_set<TreeNode*> visited;
	os << "digraph {\n";
	os << "graph[ordering = \"out\"];\n";
	dot_print(os, node, visited, left, right);
	os << "}\n";
}
void dot_print(string fname, TreeNode* node, bool left, bool right) {
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
	stringstream ss(s);
	string item;

	while (getline(ss, item, delim)) {
		result.push_back(item);
	}
}

void indent(ostream& os, int size) {
	for (int i = 0; i < size; i++)
		os << ' ';
}