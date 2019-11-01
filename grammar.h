#pragma once
#include "common.h"

struct GrammarParser {
	enum AutoDict { None, TermOnly, All };
	AutoDict auto_dict = None;
	TrieNode* root;
	string buf;
	size_t pos, line;
	regex SYMBOL;
	regex FEAT;
	regex INTEGER;
	regex NONTERM;
	bool parse_enabled = true; // enabled parsing for current lines (e.g it is not in a false %ifdef or %else block)
	unordered_set<string> defines; // keeps the %define'd symbols
	vector<bool> if_stack; // keeps %ifdef stack with their boolean values (e.g. parse_enabled is true only if all values in if_stack is true)
	unordered_map<int, vector<reference_wrapper<Rule>>> tail_map; // keeps the vector of tail-transformed rules for an NonTerminal id
	// because we cannot override pointer operator==, we had to implement it as vector of Rule references
	GrammarParser() :
		SYMBOL(R"#(([_A-Z][-_A-Za-z0-9]*'*)|("[^"]*"|[^|{:[_A-Z][\S]*))#"),
		FEAT(R"#(\*?[a-z0-9_]*)#"),
		INTEGER(R"#(-?[1-9][0-9]*)#"),
		NONTERM(R"#([_A-Z][-_A-Za-z0-9]*'*)#")
	{
		pos = 0;
		line = 1;
	}
	void skip_ws();
	bool get_eol(bool ensure = true);
	string_view get_rest();
	bool get_token(const char* token, bool ensure = true, bool skip_ws = true);
	void parse_grammar(istream& is, vector<RulePtr>& rules, TrieNode* root);
	void load_grammar(string fname, vector<RulePtr>& rules, TrieNode* root);
	Symbol* get_symbol(bool ensure = true, bool skip_ws = true);
	bool get_expr(string& out, regex& expr, const char* name, bool ensure = true, bool skip_ws = true);
	int get_integer(bool ensure = true, bool skip_ws = true);
	Symbol* get_nonterm(bool ensure = true, bool skip_ws = true);
	bool get_feat(string& out, bool ensure = true, bool skip_ws = true);
	bool get_char_list(string& out, const char* char_list, bool ensure = true, bool skip_ws = true);
	void parse_feat(dict* fdict, bool param);
	FeatParam* parse_fparam_list();
	FeatPtr parse_feat_list();
	vector<Prod*>* parse_prod();
	void parse_rule(vector<RulePtr>& rules,TrieNode* root);
};

