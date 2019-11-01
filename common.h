#pragma once

using namespace std;

extern int debug;
extern int debug_mem;

constexpr char nl = '\n';

class SymbolTable {
	// keeps a unique id for all terminals & nonterminals 
	// keeps a mapping from string representation to symbol id and vice versa
	struct Entry {
		string name;
		bool nonterminal;
	};
	vector<Entry> table;
	unordered_map<string, int> mapper;
public:
	int add(string& str, bool nonterm) {
		auto[it, inserted] = mapper.insert({ str, size() });
		if (inserted)
			table.push_back({ str,nonterm });
		return it->second;
	}
	int map(const string& str) const {
		auto it = mapper.find(str);
		if (it == mapper.end())
			return -1;
		return it->second;
	}
	const string& get(int id) const {
		return table[id].name;
	}
	bool nonterminal(int id) const {
		return table[id].nonterminal;
	}
	int size() const {
		return static_cast<int>(table.size());
	}

};

extern SymbolTable symbol_table;

using dict = map<string, string>;

struct FeatList : public dict {
	static int count;
	FeatList() {
		count++;
	}
	FeatList(const FeatList& other) : dict(other) {
		count++;
	}
	~FeatList() {
		count--;
	}
	ostream& print(ostream& os) const;
};
inline ostream& operator<<(ostream& os, const FeatList& obj) {
	return obj.print(os);
}
using FeatPtr = shared_ptr<FeatList>;
using FeatRef = shared_ptr<FeatList>&;


struct FeatParam : public dict {
	ostream& print(ostream& os) const;
};
inline ostream& operator<<(ostream& os, const FeatParam& obj) {
	return obj.print(os);
}
void split(vector<string>& result, const string &s, char delim);
struct Symbol {
	FeatParam* fparam = nullptr; // feature parameter, nullptr if there is no parameter
	string name; // name of the symbol (for debugging/display purposes)
	int id; // unique id of the nonterminal/terminal (which can be referenced from SymbolTable)
	int idx = -1; // reference index for RHS, shows position of same NT on LHS. e.g.  "VP -> V Obj : Obj V" , Obj(RHS) has ref 1, V(RHS) has ref 0
	bool nonterminal;
	ostream& print(ostream& os) const;
	Symbol(string s, bool nonterm) : name(s) {
		if (nonterm) {
			auto pos = s.find('-');
			if (pos != s.npos)
				s = s.substr(0, pos);
		}
		id = symbol_table.add(s, nonterm);
		nonterminal = nonterm;
	}
	Symbol(Symbol* other) {
		fparam = other->fparam;
		name = other->name;
		id = other->id;
		idx = other->idx;
		nonterminal = other->nonterminal;
	}
};


inline ostream& operator<<(ostream& os, const Symbol& obj) {
	return obj.print(os);
}

struct Prod : public vector<Symbol*> {
	int cost = 0;
	Prod() = default;
	Prod(Prod* other) {
		for (auto psymbol : *other) {
			push_back(new Symbol(psymbol));
		}
		cost = other->cost;
	}
	ostream & print(ostream& os) const;
};

inline ostream& operator<<(ostream& os, const Prod& obj) {
	return obj.print(os);
}

struct Rule {
	int id = -1;
	Symbol* head;
	Prod* left;
	Prod* right;
	FeatPtr feat;
	ostream& print(ostream& os) const;
	string sentence() const;
};

using RulePtr = unique_ptr<Rule>;

inline ostream& operator<<(ostream& os, const Rule& obj) {
	return obj.print(os);
}

void print_partial_rule(ostream& os, Rule* rule, int start_pos, int end_pos);

struct TrieNode {
	using value_type = Rule * ;
	string keys;
	vector<TrieNode*> children;
	vector<value_type> values;
};

struct GrammarError :public runtime_error {
public:
	GrammarError(string msg) :runtime_error(msg) {}
};

struct ParseError :public runtime_error {
public:
	ParseError(string msg) : runtime_error(msg) {}
};

struct UnifyError :public ParseError {
public:
	UnifyError(string msg) : ParseError(msg) {}
};

struct TreeNode;
//using TreeNodePtr = unique_ptr<TreeNode>;
using TreeNodePtr = TreeNode * ;
struct OptionNode {
	Rule* rule = nullptr; // rule associated with this option
	vector<TreeNodePtr> left; // left productions of this option
	vector<TreeNodePtr> right; // right productions of this option
	FeatPtr feat_list; // feature list associated with this option
	OptionNode(Rule* _rule, FeatPtr _feat_list) : rule(_rule),feat_list(_feat_list) { }
};
//using OptionNodePtr = unique_ptr<OptionNode>;
using OptionNodePtr = OptionNode * ;
struct TreeNode {
	static int id_count;
	vector<OptionNodePtr> options;
	const string* name;
	int start_pos = 0, end_pos = 0;
	int id;
	bool nonterm;
	TreeNode(const string* _name,bool _nonterm) : name(_name),nonterm(_nonterm),id(id_count++) { }
	TreeNode(const TreeNode* other) : name(other->name), nonterm(other->nonterm), start_pos(other->start_pos), end_pos(other->end_pos), id(id_count++) { }
};
vector<string> enumerate(TreeNode* node, bool right=true);
void print_tree(ostream& os, TreeNode* tree, bool indented, bool extended, bool right);
TreeNode* unify_tree(TreeNode* parent_node, bool shared=false);
bool unify_feat(shared_ptr<FeatList>& dst, FeatParam* param, shared_ptr<FeatList> src, bool down);
void dot_print(ostream& os, TreeNode* node, bool left = true, bool right = false);
void dot_print(string fname, TreeNode* node, bool left = true, bool right = false);
void search_trie_prefix(TrieNode* node, const char* str, vector<pair<int, Rule*>>& result);
void add_trie(TrieNode* node, const string& s, Rule* value);
void dump_trie(TrieNode* node);
int get_enum(initializer_list<string> list, string& param);
void search_trie_prefix(TrieNode* node, const char* str, vector<pair<int, Rule*>>& result);
void split(vector<string>& result, const string &s, char delim);
void indent(ostream& os, int size);
