#pragma once
#include "common.h"

struct GrammarParser {
	Grammar* grammar;
	enum AutoDict { None, TermOnly, All };
	AutoDict auto_dict = None;
	string buf;
	size_t pos, line;
	regex SYMBOL;
	regex FEAT;
	regex INTEGER;
	regex NONTERM;
	bool parse_enabled = true; // enabled parsing for current lines (e.g it is not in a false %ifdef or %else block)
	unordered_set<string> defines; // keeps the %define'd symbols
	unordered_map<string, unordered_map<string,vector<string>>> macros; // e.g. macros["V"]["go"] = { "go", "goes", "going", "went", "gone" }
	
	vector<bool> if_stack; // keeps %ifdef stack with their boolean values (e.g. parse_enabled is true only if all values in if_stack is true)
	
	// because we cannot override pointer operator==, we had to implement it as vector of Rule references
	GrammarParser(Grammar* _grammar) :
		SYMBOL(R"#((\$?[_A-Z][-_A-Za-z0-9$]*'*)|("[^"]*"|\$?[^|{:[_A-Z#!][\S]*))#"),
		NONTERM(R"#(\$?[_A-Z][-_A-Za-z0-9$]*'*)#"),
		FEAT(R"#(\*?[-_A-Za-z0-9]*)#"),
		INTEGER(R"#(-?[1-9][0-9]*)#")
	{
		pos = 0;
		line = 1;
		grammar = _grammar;
	}
	void skip_ws();
	bool get_eol(bool ensure = true);
	string_view get_rest();
	bool get_token(const char* token, bool ensure = true, bool skip_ws = true);
	void parse_grammar(istream& is);
	void load_grammar(string fname);
	bool get_symbol(vector<PreSymbol>& symbols,bool ensure = true, bool skip_ws = true);
	bool get_expr(string& out, regex& expr, const char* name, bool ensure = true, bool skip_ws = true);
	int get_integer(bool ensure = true, bool skip_ws = true);
	void get_head(PreSymbol& head);
	bool get_feat(string& out, bool ensure = true, bool skip_ws = true);
	bool get_char_list(string& out, const char* char_list, bool ensure = true, bool skip_ws = true);
	void parse_feat(FeatList* feat_list,FeatList*& check_list);
	void parse_fparam(FeatParam* fparam_list);
	FeatParam* parse_fparam_list();
	void parse_feat_list(FeatPtr&,FeatList*&);
	void parse_prod(vector<PreProd>& prods, const string& macro_name);
	void create_rule(PreSymbol* head, PreProd* left, PreProd* right, FeatPtr& feat_list, FeatList* check_list,int macro_idx = -1);
	void parse_rule();
};

