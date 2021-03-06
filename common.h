#pragma once

using namespace std;

extern int debug;
extern int debug_mem;
extern int profile;

constexpr char nl = '\n';

struct SymbolEntry {
	string name;
	bool nonterminal, left = false, right = false, nullable = false, major = true;
	SymbolEntry(string p_name, bool p_nonterminal) : name(p_name), nonterminal(p_nonterminal) { }
};
struct SymbolTable {
	// keeps a unique id for all terminals & nonterminals 
	// keeps a mapping from string representation to symbol id and vice versa
	vector<SymbolEntry> table;
	unordered_map<string, int> mapper;
	int add(string& str, bool nonterm) {
		auto[it, inserted] = mapper.insert({ str, size() });
		if (inserted)
			table.emplace_back(str,nonterm);
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
	void set_right(int id) {
		table[id].right = true;
	}
	void set_left(int id) {
		table[id].left = true;
	}
	void set_nullable(int id) {
		table[id].nullable = true;
	}
	bool is_right(int id) {
		return table[id].right;
	}
	bool is_left(int id) {
		return table[id].left;
	}
	bool is_nullable(int id) {
		return table[id].nullable;
	}
	int size() const {
		return static_cast<int>(table.size());
	}
	SymbolTable() = default;
	SymbolTable(istream& is);
	void save(ostream& os);
	SymbolEntry& operator[](int id) { return table[id]; }
};

struct TreeNode;
using FeatVal = variant<string, int, TreeNode*>;
/*
string: normal features 
int: left index for parsed rules 
TreeNode*: the left tree node for a parse tree
*/

inline ostream& operator<<(ostream& os, const FeatVal& val) {
	if (holds_alternative<string>(val))
		os << get<string>(val);
	else if (holds_alternative<int>(val))
		os << '<' << get<int>(val) << '>';
	else
		os << '*';
	return os;
}

struct FeatList : public map<string, FeatVal> {
	static int count;
	FeatList() {
		count++;
	}
	FeatList(const FeatList& other) : map<string, FeatVal>(other) {
		count++;
	}
	~FeatList() {
		count--;
	}
	FeatList(istream& is);
	void save(ostream& os);
	ostream& print(ostream& os) const;
};
inline ostream& operator<<(ostream& os, const FeatList& obj) {
	return obj.print(os);
}
using FeatPtr = shared_ptr<FeatList>;
using FeatRef = shared_ptr<FeatList>&;

enum class FeatParamType : unsigned char {
	all,			// All features are passed through								e.g. V -> PPS
	only_params,	// Only parameters are passed through							e.g. V -> PPS(+gap) PPS(gap)
	with_params,	// Parameters and remaining features are passed through			e.g. V -> PPS(+, +gap) 		Note: PPS(+, gap) == PPS
	without_params	// Except parameters, remaining features are passed through		e.g. V -> PPS(-, gap)		Note: PPS(-, +gap) == PPS(-, gap)
};
struct FeatParam {
	map<string, string>* params = nullptr;
	FeatParamType param_type = FeatParamType::all;
	FeatParam() = default;
	FeatParam(istream& is);
	void save(ostream& os);
	ostream& print(ostream& os) const;
};

inline ostream& operator<<(ostream& os, const FeatParam& obj) {
	return obj.print(os);
}
struct PreSymbol {
	string name;
	FeatParam fparam;
	const vector<string>* macro_values = nullptr;
	bool macro_suffix = false;
	bool nonterminal = true;
	PreSymbol() = default;
	PreSymbol(string s,bool nonterm) : name(s), nonterminal(nonterm) { }
};

struct PreProd : public vector<PreSymbol> {
	int cost = 0;
	bool cut = false;
};

struct Symbol {
	FeatParam fparam;
	string name; // name of the symbol (for debugging/display purposes)
	int id = -1; // unique id of the nonterminal/terminal (which can be referenced from SymbolTable)
	int idx = -1; // reference index for RHS, shows position of same NT on LHS. e.g.  "VP -> V Obj : Obj V" , Obj(RHS) has ref 1, V(RHS) has ref 0
	bool nonterminal;
	ostream& print(ostream& os) const;

	Symbol(string _name, bool _nonterminal,int _id) : name(_name), nonterminal(_nonterminal), id(_id) {
	}

	Symbol(Symbol* other) {
		fparam = other->fparam;
		name = other->name;
		id = other->id;
		idx = other->idx;
		nonterminal = other->nonterminal;
	}
	Symbol(PreSymbol* other, SymbolTable* symbol_table, bool terminal_symbol, int macro_idx) {
		fparam = other->fparam;
		nonterminal = other->nonterminal;
		if (macro_idx != -1 && other->macro_values != nullptr)
			if (other->macro_suffix)
				name = other->macro_values->at(macro_idx) + other->name;
			else
				name = other->macro_values->at(macro_idx);
		else
			name = other->name;
		if (symbol_table) {
			string s = name;
			if (nonterminal) {
				auto pos = s.find('-');
				if (pos != s.npos)
					s = s.substr(0, pos);
			}
			if (nonterminal || terminal_symbol)
				id = symbol_table->add(s, other->nonterminal);
		}
	}
	Symbol(istream& is, SymbolTable* symbol_table = nullptr);
	void save(ostream& os);
};


inline ostream& operator<<(ostream& os, const Symbol& obj) {
	return obj.print(os);
}

struct Prod : public vector<Symbol*> {
	int cost = 0;
	bool cut = false;
	Prod() = default;
	Prod(PreProd* other, SymbolTable* symbol_table,bool terminal_symbol,int macro_idx) {
		reserve(other->size());
		for (auto& psymbol : *other) {
			push_back(new Symbol(&psymbol, symbol_table, terminal_symbol, macro_idx));
		}
		cost = other->cost;
		cut = other->cut;
	}
	Prod(istream& is, SymbolTable* symbol_table = nullptr);
	void save(ostream& os);
	ostream& print(ostream& os) const;
};

inline ostream& operator<<(ostream& os, const Prod& obj) {
	return obj.print(os);
}
struct Grammar;
struct Rule {
	int id = -1;
	Symbol* head;
	Prod* left;
	Prod* right;
	FeatPtr feat;
	FeatList* check_list = nullptr;
	Rule() {
		head = nullptr;
		left = new Prod;
		right = new Prod;
		feat = FeatPtr(new FeatList);
	}
	Rule(PreSymbol* head, PreProd* left, PreProd* right, FeatPtr& feat_list, FeatList* check_list, int macro_idx, SymbolTable* symbol_table,bool terminal_symbol);
	Rule(Symbol* _head, Prod* _left, Prod* _right, FeatPtr& _feat, FeatList* _check_list) : head(_head),left(_left),right(_right),feat(_feat), check_list(_check_list) { }
	Rule(istream& is, Grammar& grammar);
	void save(ostream& os);
	ostream& print(ostream& os) const;
	string terminal_prefix() const;
	string get_template() const;
	void resolve_references();
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
	TrieNode() = default;
	TrieNode(istream& is, Grammar& grammar);
	void save(ostream& os);
};

struct GrammarError :public runtime_error {
public:
	//GrammarError(const string& file_name,int line_num,int pos,const string& msg) : runtime_error(format())
	//{}
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

struct PostProcessError :public ParseError {
public:
	PostProcessError(string msg) : ParseError(msg) {}
};

struct TreeNode;
//using TreeNodePtr = unique_ptr<TreeNode>;
using TreeNodePtr = TreeNode * ;
struct OptionNode {
	Rule* rule = nullptr; // rule associated with this option
	vector<TreeNodePtr> left; // left productions of this option
	vector<TreeNodePtr> right; // right productions of this option
	FeatPtr feat_list; // feature list associated with this option
	int cost = 0;
	int term_cnt = 0;
	OptionNode(Rule* _rule, FeatPtr _feat_list) : rule(_rule),feat_list(_feat_list) { }
};
//using OptionNodePtr = unique_ptr<OptionNode>;
using OptionNodePtr = OptionNode * ;
struct TreeNode {
	static int node_count;
	vector<OptionNodePtr> options;
	SymbolEntry* head = nullptr;
	const string* name;
	int start_pos = 0, end_pos = 0;
	int node_id;
	bool nonterm;
	TreeNode(const string* _name,bool _nonterm) : name(_name),nonterm(_nonterm),node_id(node_count++) { }
	TreeNode(SymbolEntry* _head) : name(&_head->name), nonterm(_head->nonterminal),head(_head),node_id(node_count++) { }
	TreeNode(const TreeNode* other) : name(other->name), nonterm(other->nonterm), start_pos(other->start_pos), end_pos(other->end_pos), head(other->head), node_id(node_count++) { }
};

struct Grammar {
	unordered_map<string, string> suffixes; // e.g. suffix["ben+e"] = "bana"
	vector<RulePtr> rules;
	TrieNode* root;
	SymbolTable symbol_table;
	unordered_map<string,Rule*> templates; // templates for matching dictionary entries e.g. "turn Obj on" and "think Obj over" has a template "* Obj *"
	unordered_map<Rule*, Rule*> template_rules;
	void print_rules(ostream& os);
	void print_symbol_table(ostream& os);
	void print_dict(ostream& os);
	void print_templates(ostream& os);
};

struct Node;
using Option = pair<vector<Node>, int>;
using OptionVec = vector<Option>;

struct Node : public variant<string, vector<Option>> {
	Node(const string& name) : variant<string, vector<Option>>(name) { }
	Node(OptionVec&& option_vec) : variant<string, vector<Option>>(option_vec) { }
};

using EnumVec = vector<pair<string, int>>;

vector<string> enumerate(Grammar* grammar, TreeNode* node, bool right = true);
void print_tree(ostream& os, TreeNode* tree, bool indented, bool extended, bool right);
void convert(ostream& os, TreeNode* node, Grammar* grammar, EnumVec& enums, bool show_trans_expr);
TreeNode* unify_tree(TreeNode* parent_node, bool shared=false);
bool unify_feat(shared_ptr<FeatList>& dst, const FeatParam& param, shared_ptr<FeatList> src, bool down, FeatList* check_list=nullptr);
void dot_print(ostream& os, TreeNode* node, bool left = true, bool right = false);
void dot_print(string fname, TreeNode* node, bool left = true, bool right = false);
void search_trie_prefix(TrieNode* node, const char* str, vector<pair<int, Rule*>>& result);
void add_trie(TrieNode* node, const string& s, Rule* value);
void dump_trie(TrieNode* node);
int get_enum(initializer_list<string> list, string& param);
void search_trie_prefix(TrieNode* node, const char* str, vector<pair<int, Rule*>>& result);
void split(vector<string>& result, const string &s, char delim);
void indent(ostream& os, int size);
void ltrim(string &s);
void rtrim(string &s);
void to_lower(string& s);
