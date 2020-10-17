// Grammar.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "grammar.h"

using namespace std;

void save_suffixes(ostream& os, Grammar& grammar);
void load_suffixes(istream& is, Grammar& grammar);
void save_templates(ostream& os, Grammar& grammar);
void load_templates(istream& is, Grammar& grammar);
//SymbolTable symbol_table;

void GrammarParser::skip_ws() {
	// skip white spaces in the input
	while (buf[pos]!='#' && isascii(buf[pos]) && isspace(buf[pos])) {
		// no need to check end-of-string because buf[size]=='\0'
		pos++;
	}
}

bool GrammarParser::get_eol(bool ensure) {
	// check if eol or a comment is reached
	skip_ws();
	if (pos == buf.size() || buf[pos] == '#')
		return true;
	if (ensure)
		throw GrammarError(
			format("Line:{} Pos:{} EOL expected but found: {}...",
				line, pos, get_rest()
			)
		);
	return false;
}

string_view GrammarParser::get_rest() {
	return string_view(buf.c_str() + pos, 20);
}

bool GrammarParser::get_token(const char* token, bool ensure, bool skip_ws) {
	if (skip_ws) {
		this->skip_ws();
	}
	size_t len = strlen(token);
	if (buf.compare(pos, len, token) == 0)
	{
		pos += len;
		return true;
	}
	if (ensure) {
		throw GrammarError(
			format("File:{} Line:{} Pos:{} '{}' expected but found: {}...", file,
				line, pos, token, get_rest()
			)
		);
	}
	return false;
}

void GrammarParser::parse_grammar(istream& is,int level,int start_line) {
	if (!level) {
		buf = "S' -> S() : S()";
		parse_rule();
	}
	line = start_line - 1;
	while (getline(is, buf)) {
		pos = 0;
		line++;
		skip_ws();
		if (pos == buf.size() || buf[pos] == '#') // skip empty or comment-only lines
			continue;
		if (buf[pos] == '%') {
			buf.erase(0, pos+1); // erase until including %
			vector<string> params;
			split(params, buf, ' ');
			string& directive = params[0];
	
			if (directive == "ifdef") {
				if_stack.push_back(defines.count(params[1]) != 0);
			}
			else if (directive == "ifndef") {
				if_stack.push_back(defines.count(params[1]) == 0);
			}
			else if (directive == "else") {
				if_stack.back() = !if_stack.back();
			}
			else if (directive == "endif") {
				if_stack.pop_back();
			}
			parse_enabled = all_of(if_stack.begin(), if_stack.end(), [](bool val) {return val; });
			if (!parse_enabled)
				continue;
			if (directive == "define")
				for (int i = 1; i < params.size(); i++)
					defines.insert(params[i]);
			else if (directive == "auto_dict") {
				auto_dict = (AutoDict)get_enum({ "false", "true", "all" }, params[1]);
				if (auto_dict == -1) {
					throw GrammarError("Invalid Directive: " + buf);
				}
			}
			else if (directive == "macro") { // %macro <name> <values>
				if (params.size() != 3)
					throw GrammarError("Invalid Directive Parameter Count: " + buf);
				split(macros[params[1]]["$"], params[2], ',');
			}
			else if (directive == "form") { // %form <name> <values>
				if (params.size() != 3)
					throw GrammarError("Invalid Directive Parameter Count: " + buf);
				vector<string> forms;
				split(forms, params[2], ',');
				auto it = macros.find(params[1]);
				if (it == macros.end())
					throw GrammarError("Macro not defined: " + buf);
				if (it->second["$"].size() != forms.size())
					throw GrammarError("Macro Parameter Count doesn't match: " + buf);
				it->second[forms[0]] = move(forms);
			}
			else if (directive == "suffix") { // %suffix <name> <values>
				if (params.size() != 3)
					throw GrammarError("Invalid Directive Parameter Count: " + buf);
				grammar->suffixes[params[1]] = params[2];
			}
			else if (directive == "include") { // %include <file_name>
				if (params.size() != 2)
					throw GrammarError("Invalid Directive Parameter Count: " + buf);
				auto line_bck = line;
				auto file_bck = file;
				defines.insert("include");
				load_grammar(params[1], level+1);
				defines.erase("include");
				file = file_bck;
				line = line_bck;
			}
			else if (directive == "save_dict") { // %save_dic <file_name>
				if (params.size() != 2)
					throw GrammarError("Invalid Directive Parameter Count: " + buf);
				auto start = std::chrono::system_clock::now();
				ofstream os(params[1], ios::binary|ios::out);
				save_suffixes(os, *grammar);
				//save_templates(os, *grammar); // is it necessary?
				grammar->root->save(os);
				auto end = std::chrono::system_clock::now();
				if (profile >= 1)
					cout << "SaveDict " << params[1] << ": " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << " mics\n";
			}
			else if (directive == "load_dict") { // %load_dic <file_name>
				if (params.size() != 2)
					throw GrammarError("Invalid Directive Parameter Count: " + buf);
				auto start = std::chrono::system_clock::now();
				ifstream is(params[1], ios::binary | ios::in);
				load_suffixes(is, *grammar);
				//load_templates(is, *grammar); // is it necessary?
				grammar->root = new TrieNode(is, *grammar);
				auto end = std::chrono::system_clock::now();
				if (profile >= 1)
					cout << "LoadDict " << params[1] << ": " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << " mics\n";

			}
			else if (directive == "major") { // %major NT[,NT]*
				if (params.size() != 2)
					throw GrammarError("Invalid Directive Parameter Count: " + buf);
				for (auto& entry : grammar->symbol_table.table)
					entry.major = false;
				vector<string> items;
				split(items, params[1], ',');
				for (auto& item : items) {
					auto id = grammar->symbol_table.map(item);
					if (id == -1)
						throw GrammarError("Symbol not defined: " + item);
					grammar->symbol_table[id].major = true;
				}
			}
			//else if (directive == "suffix_macro") { // %suffix_macro <name> <values>
			//	if (params.size() != 3)
			//		throw GrammarError("Invalid Directive Parameter Count: " + buf);
			//	split(suffix_macros[params[1]]["$"], params[2], ',');
			//}
			//else if (directive == "suffix_form") { // %suffix_macro <name> <values>
			//	if (params.size() != 3)
			//		throw GrammarError("Invalid Directive Parameter Count: " + buf);
			//	vector<string> forms;
			//	split(forms, params[2], ',');
			//	auto it = suffix_macros.find(params[1]);
			//	if (it == suffix_macros.end())
			//		throw GrammarError("Macro not defined: " + buf);
			//	if (it->second["$"].size() != forms.size())
			//		throw GrammarError("Macro Parameter Count doesn't match: " + buf);
			//	it->second[forms[0]] = move(forms);
			//}
		}
		else if(parse_enabled)
			parse_rule();
	}

	if (level)
		return;

	auto auto_dict_backup = auto_dict;
	auto_dict = None;

	for (auto& item : grammar->templates) { // convert templates to rules to construct associated states
		buf = item.first; pos = 0;
		parse_rule();
		item.second = grammar->rules.back().get();
	}

	auto_dict = auto_dict_backup;
	if (debug >= 2) {
		cout << "Trie Dump:\n";
		dump_trie(grammar->root);
		grammar->print_symbol_table(cout);
	}
	if (debug >= 1) {
		grammar->print_templates(cout);
		grammar->print_rules(cout);
	}
}

void GrammarParser::load_grammar(string fname,int level) {
	ifstream f(fname);
	if (!f)
		throw runtime_error("Cannot open file: " + fname);
	parse_grammar(f, level);
}

bool GrammarParser::get_symbol(vector<PreSymbol>& symbols, bool ensure, bool skip_ws) {
	if (skip_ws) {
		this->skip_ws();
	}
	auto begin = buf.cbegin() + pos, end = buf.cend();
	smatch sm;
	if (regex_search(begin, end, sm, SYMBOL, regex_constants::match_continuous)) {
		pos += sm.length();
		if (sm[1].matched) // NonTerminal
			symbols.emplace_back(sm.str(1), true); 
		else if (sm[2].matched) // Terminal with double-quotes
			symbols.emplace_back(sm.str(2), false);
		else if (sm[3].matched) // Terminal without double-quotes
			symbols.emplace_back(sm.str(3), false);
		return true;
	}
	if (ensure) {
		throw GrammarError(
			format("Line:{} Pos:{} Symbol expected but found: {}...",
				line, pos, get_rest()
			)
		);
	}
	return false;
}

bool GrammarParser::get_expr(string& out, regex& expr,const char* name, bool ensure, bool skip_ws ) {
	if (skip_ws) {
		this->skip_ws();
	}
	auto begin = buf.cbegin() + pos, end = buf.cend();
	smatch sm;
	if (regex_search(begin, end, sm, expr, regex_constants::match_continuous)) {
		pos += sm.length();
		out = sm.str();
		return true;
	}
	if (ensure) {
		throw GrammarError(
			format("Line:{} Pos:{} {} expected but found: {}...",
				line, pos, name, get_rest()
			)
		);
	}
	return false;
}

int GrammarParser::get_integer(bool ensure, bool skip_ws) {
	string val;
	if( !get_expr(val, INTEGER, "INTEGER", ensure, skip_ws) )
		return 0;
	return stoi(val);
}

void GrammarParser::get_head(PreSymbol& head) {
	get_expr(head.name, NONTERM, "NONTERM", true, false);
}

bool GrammarParser::get_feat(string& out, bool ensure, bool skip_ws) {
	return get_expr(out, FEAT, "FEAT", ensure, skip_ws);
}

bool GrammarParser::get_char_list(string& out, const char* char_list, bool ensure, bool skip_ws) {
	if (skip_ws)
		this->skip_ws();
	char c = buf[pos];
	if (strchr(char_list, c)) {
		pos++;
		out = c;
		return true;
	}
	if(ensure)
		throw GrammarError(
			format("Line:{} Pos:{} one of {} expected but found: {}...",
				line, pos, char_list, get_rest()
			)
		);
	return false;
}

void GrammarParser::parse_feat(FeatList* feat_list, FeatList*& check_list) {
	string name, value;
	if (get_char_list(value, "+-?!", false)) {
		get_feat(name, true, false);
		if (value == "?" || value == "!") {
			if (!check_list)
				check_list = new FeatList;
			(*check_list)[name] = value;
			return;
		}
	}
	else {
		get_feat(name);
		get_token("=");
		get_feat(value);
		if (value[0] == '!' || value[0] == '?') {
			if (!check_list)
				check_list = new FeatList;
			(*check_list)[name] = value;
			return;
		}
	}
	(*feat_list)[name] = value;
}

void GrammarParser::parse_fparam(FeatParam& fparam_list) {
	string name, value;
	if (get_char_list(value, "+-", false)) {
		get_feat(name, true, false);
	}
	else {
		get_feat(name);
		if (get_token("=", false))
			get_feat(value);
	}
	(*fparam_list.params)[name] = value;
}

void GrammarParser::parse_fparam_list(FeatParam& fparam_list) {
	if (! get_token("(", false, false))
		return;
	fparam_list.param_type = FeatParamType::only_params;
	fparam_list.params = new map<string, string>;
	if (get_token(")", false))
		return;
	if (get_token("+,", false))
		fparam_list.param_type = FeatParamType::with_params;
	else if (get_token("-,", false))
		fparam_list.param_type = FeatParamType::without_params;
	parse_fparam(fparam_list);
	while (get_token(",", false)) {
		parse_fparam(fparam_list);
	}
	get_token(")");
}

void  GrammarParser::parse_feat_list(FeatPtr& feat_list, FeatList*& check_list) {
	parse_feat(feat_list.get(), check_list);
	while (get_token(",", false)) {
		parse_feat(feat_list.get(), check_list);
	}
	get_token("]");
}

void GrammarParser::parse_prod(vector<PreProd>& prods,const string& macro_name) {
	do {
		prods.emplace_back();
		PreProd& prod = prods.back();
		while (get_symbol(prod, false)) {
			PreSymbol& symbol = prod.back();
			if (symbol.name[0] == '$' && !macro_name.empty()) {
				auto pos = symbol.name.find('$', 1);
				string macro_form;
				if (pos != string::npos) { // e.g. $V -> $V$1 Obj : Obj Case $V$1
					assert(symbol.name.substr(1, pos - 1) == macro_name);
					symbol.macro_suffix = true;
					symbol.name.erase(0, pos + 1); // $V$1 => 1
					macro_form = "$";
				}
				else // e.g. $V -> $go : git
					macro_form = symbol.name.substr(1);
				auto it = macros[macro_name].find(macro_form);
				if (it == macros[macro_name].end())
					throw GrammarError(format("Form {} not defined for macro {}", macro_form, macro_name));
				symbol.macro_values = &it->second;
			}
			if (symbol.nonterminal) {
				parse_fparam_list(symbol.fparam);
			}
		}
		if (get_token("{", false)) {
			prod.cost = get_integer();
			get_token("}");
		}			
		else
			prod.cost = 0;
		if (get_token("!", false))
			prod.cut = true;
	} while (get_token("|",false,true));
}

//bool operator==(const Symbol& a, const Symbol& b) {
//	// check the equivalency of two symbols (their id's and feature parameters should match)
//	if ((a.fparam == nullptr) != (b.fparam == nullptr))
//		return false;
//	if (a.fparam && *a.fparam == *b.fparam)
//		return false;
//	return a.id != b.id;
//}
bool operator==(const Symbol& a, const Symbol& b) {
	// check the equivalency of two symbols (their id's and feature parameters should match)
	if (a.fparam.param_type != b.fparam.param_type)
		return false;
	if (*a.fparam.params == *b.fparam.params)
		return false;
	return a.id != b.id;
}
bool operator==(const Rule& a, const Rule& b) {
	// check the tail-equivalency of two rules
	// LHS of two rules should match except first symbols and features should also match
	if (a.left->size() != b.left->size())
		return false;
	if (*a.feat != *b.feat)
		return false;
	for (int i = 1; i < a.left->size(); i++)
		if (!( *a.left->at(i) == *b.left->at(i) ))
			return false;
	return true;
}

void GrammarParser::create_rule(PreSymbol* head, PreProd* left, PreProd* right, FeatPtr& feat_list, FeatList* check_list, int macro_idx) {
	SymbolTable* symbol_table = &grammar->symbol_table;

	auto p = left->begin(); // dummy initial value for p
	auto flag = auto_dict;
	if (auto_dict != None) {
		p = find_if(left->begin(), left->end(), [](auto& lsymbol) {return lsymbol.nonterminal; }); // find the first nonterminal position
		if (p == left->begin()) // either it is empty or it starts with nonterminals
			flag = None;
		else if (p == left->end()) // all symbols are terminals
			flag = TermOnly;
		else // it starts with terminals
			if (auto_dict == TermOnly)
				flag = None;
	}
	if (flag == None) { // don't add to dict
		//Rule* rule = new Rule(new Symbol(head, symbol_table, true, macro_idx), new Prod(left, symbol_table, true, macro_idx), new Prod(right, symbol_table, false, macro_idx), feat_list, check_list);
		Rule* rule = new Rule(head, left, right, feat_list, check_list, macro_idx, symbol_table, true);
		rule->resolve_references();
		rule->id = grammar->rules.size();
		grammar->rules.emplace_back(rule); // unique_ptr is created for the rule
	}
	else if (flag == TermOnly) { // add whole rule LHS to the dict
		//Rule* rule = new Rule(new Symbol(head, symbol_table, true, macro_idx), new Prod(left, symbol_table, false, macro_idx), new Prod(right, symbol_table, false, macro_idx), feat_list, check_list);
		Rule* rule = new Rule(head, left, right, feat_list, check_list, macro_idx, symbol_table, false);
		add_trie(grammar->root, rule->terminal_prefix(), rule);
	}
	else { // normalize the rule so that it starts with a dummy NonTerminal which contains initial terminals
		//Rule* rule = new Rule(new Symbol(head, symbol_table, true, macro_idx), new Prod(left, symbol_table, false, macro_idx), new Prod(right, symbol_table, false, macro_idx), feat_list, check_list);
		Rule* rule = new Rule(head, left, right, feat_list, check_list, macro_idx, symbol_table, false);
		add_trie(grammar->root, rule->terminal_prefix(), rule);
		rule->resolve_references();
		grammar->templates.emplace(rule->get_template(),nullptr);
		grammar->template_rules.emplace(rule, nullptr);
	}
}

void GrammarParser::parse_rule() {
	// parses a rule, using rule-vector and root node of a dictionary
	string macro_name;
	if (get_eol(false))
		return;
	PreSymbol head;
	get_head(head);
	if (head.name[0] == '$') {
		auto pos = head.name.find('$', 1);
		if (pos != string::npos) { // e.g. $V$0
			head.macro_suffix = true;
			macro_name = head.name.substr(1, pos - 1);
			head.name.erase(0, pos + 1); // $V$1 => 1
		}
		else // e.g. $V
			macro_name = head.name.substr(1);
		auto it = macros.find(macro_name);
		if (it == macros.end())
			throw GrammarError("Macro not defined: " + macro_name);
		head.macro_values = &it->second["$"];
	}
	get_token("->");
	vector<PreProd> llist, rlist;
	parse_prod(llist, macro_name);

	if (get_token(":", false))
		parse_prod(rlist, macro_name);
	else
		rlist.emplace_back();

	FeatPtr feat_list(new FeatList());
	FeatList* check_list = nullptr;
	if (get_token("[", false)) {
		parse_feat_list(feat_list, check_list);
	}

	get_eol();
	
	for (auto& left : llist)
		for (auto& right : rlist) {
			if (macro_name.empty()) {
				//create_rule(new Symbol(&head), new Prod(&left), new Prod(&right), feat);
				create_rule(&head, &left, &right, feat_list, check_list);
			}
			else {
				for (int idx = 0; idx < head.macro_values->size(); ++idx) {
					//create_rule(new Symbol(&head,idx), new Prod(&left,idx), new Prod(&right), feat);
					create_rule(&head, &left, &right, feat_list, check_list, idx);
				}
			}
		}
}
