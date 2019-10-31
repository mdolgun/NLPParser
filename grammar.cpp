// Grammar.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "grammar.h"
#include <cctype>

using namespace std;

SymbolTable symbol_table;

void GrammarParser::skip_ws() {
	// skip white spaces in the input
	while (isascii(buf[pos]) && isspace(buf[pos])) {
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
			format("Line:{} Pos:{} '{}' expected but found: {}...",
				line, pos, token, get_rest()
			)
		);
	}
	return false;
}


void GrammarParser::parse_grammar(istream& is, vector<RulePtr>& rules, TrieNode* root) {
	buf = "S' -> S() : S()";
	parse_rule(rules, root);
	line = 0;
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
				auto_dict = (AutoDict)get_enum({ "none", "termonly", "all" }, params[1]);
				if (auto_dict == -1) {
					throw GrammarError("Invalid Directive: " + buf);
				}
			}
		}
		else if(parse_enabled)
			parse_rule(rules, root);
	}
	if (debug >= 2) {
		cout << "Trie Dump:\n";
		dump_trie(root);
	}
}

void GrammarParser::load_grammar(string fname, vector<RulePtr>& rules, TrieNode* root) {
	ifstream f(fname);
	if (!f)
		throw runtime_error("Cannot open file: " + fname);
	parse_grammar(f, rules, root);
}

Symbol* GrammarParser::get_symbol(bool ensure, bool skip_ws) {
	if (skip_ws) {
		this->skip_ws();
	}
	auto begin = buf.cbegin()+pos, end = buf.cend();
	smatch sm;
	if (regex_search(begin, end, sm, SYMBOL, regex_constants::match_continuous)) {
		pos += sm.length();
		if (sm[1].matched)
			return new Symbol( sm.str(1), true); // NonTerminal
		if (sm[2].matched)
			return new Symbol( sm.str(2), false); // Terminal
	}
	if (ensure) {
		throw GrammarError(
			format("Line:{} Pos:{} Symbol expected but found: {}...",
				line, pos, get_rest()
			)
		);
	}
	return nullptr;
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

Symbol* GrammarParser::get_nonterm(bool ensure, bool skip_ws) {
	string name;
	if( !get_expr(name, NONTERM, "NONTERM", ensure, skip_ws) )
		return nullptr;
	return new Symbol(name, true);
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

void GrammarParser::parse_feat(dict* fdict,bool param) {
	string name,value;
	if (get_char_list(value, "+-", false)) {
		get_feat(name, true, false);
	}
	else {
		get_feat(name);
		if (param) {
			if (get_token("=",false))
				get_feat(value);
		}
		else {
			get_token("=");
			get_feat(value);
		}
			
	}
	(*fdict)[name] = value;
}

FeatParam* GrammarParser::parse_fparam_list() {
	if (! get_token("(", false, false))
		return nullptr;
	FeatParam* fparam = new FeatParam();
	if (get_token(")", false))
		return fparam;
	parse_feat(fparam,true);
	while (get_token(",", false)) {
		parse_feat(fparam, true);
	}
	get_token(")");
	return fparam;
}

FeatPtr GrammarParser::parse_feat_list() {
	FeatList* flist = new FeatList();
	parse_feat(flist,false);
	while (get_token(",", false)) {
		parse_feat(flist, false);
	}
	get_token("]");
	return FeatPtr(flist);
}
vector<Prod*>* GrammarParser::parse_prod() {
	vector<Prod*>* prods = new vector<Prod*>();
	do {
		Prod* prod = new Prod();
		Symbol* symbol = get_symbol(false);
		while (symbol) {
			if (symbol->nonterminal) {
				symbol->fparam = parse_fparam_list();
			}
			prod->push_back(symbol);
			symbol = get_symbol(false);
		}
		if (get_token("{", false)) {
			prod->cost = get_integer();
			get_token("}");
		}			
		else
			prod -> cost = 0;
		prods->push_back(prod);
	} while (get_token("|",false,true));
	return prods;
}


bool operator==(const Symbol& a, const Symbol& b) {
	// check the equivalency of two symbols (their id's and feature parameters should match)
	if ((a.fparam == nullptr) != (b.fparam == nullptr))
		return false;
	if (a.fparam && *a.fparam == *b.fparam)
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
void resolve_reference(Prod* left, Prod* right) {
	// given LHS and RHS of a rule, for nonterminals on both sides, RHS symbol is added an reference index for the LHS position of the symbol
	for (auto rsymbol : *right) {
		auto p = find_if(left->begin(), left->end(), [&rsymbol](auto lsymbol) {return lsymbol->name == rsymbol->name; }); // find the first symbol on the left, where it matches the right symbol
		if (p != left->end()) {
			rsymbol->idx = p - left->begin();
		}
	}
}
void GrammarParser::parse_rule(vector<RulePtr>& rules, TrieNode* root) {
	// parses a rule, using rule-vector and root node of a dictionary
	if (get_eol(false))
		return;
	Symbol* head = get_nonterm();
	get_token("->");
	vector<Prod*>* llist = parse_prod();
	vector<Prod*>* rlist;
	if (get_token(":", false))
		rlist = parse_prod();
	else {
		rlist = new vector<Prod*>();
		rlist->push_back(new Prod());
	}

	FeatPtr feat;
	if (get_token("[", false))
		feat = parse_feat_list();
	else
		feat = FeatPtr(new FeatList()); // or use a singleton for empty feature list

	get_eol();
	for (auto left : *llist)
		for (int i = 0; i < rlist->size(); ++i) {
			auto right = (*rlist)[i];
			Rule* rule = new Rule();
			rule->head = head;
			rule->left = new Prod(left);
			//if (i == rlist->size() - 1)
			//	rule->right = right; // use current production, if it is last alternative
			//else
			rule->right = new Prod(right); // else copy the current production (because they may have not same references)
			rule->feat = feat;

			auto p = rule->left->begin(); // dummy initial value for p
			auto flag = auto_dict;
			if (auto_dict != None) {
				p = find_if(rule->left->begin(), rule->left->end(), [](auto& lsymbol) {return lsymbol->nonterminal; }); // find the first nonterminal position
				if (p == rule->left->begin()) // either it is empty or it starts with nonterminals
					flag = None;
				else if (p == rule->left->end()) // all symbols are terminals
					flag = TermOnly;
				else // it starts with terminals
					if (auto_dict == TermOnly)
						flag = None;
			}
			if (flag == None) { // don't add to dict
				resolve_reference(rule->left, rule->right);
				rule->id = rules.size();
				rules.emplace_back(rule); // unique_ptr is created for the rule
			} else if (flag == TermOnly) { // add whole rule LHS to the dict
				add_trie(root, rule->sentence(), rule);
			}
			else { // normalize the rule so that it starts with a dummy NonTerminal which contains initial terminals
				Rule* new_rule = new Rule;
				new_rule->left = new Prod;
				new_rule->right = new Prod;
				new_rule->feat = FeatPtr(new FeatList);
				copy(rule->left->begin(), p, back_inserter(*new_rule->left));
				rule->left->erase(rule->left->begin(), p);
				rule->left->insert(rule->left->begin(), nullptr); // insert place-holder nullptr, for the terminal-only portion of the rule

				auto& entries = tail_map[head->id]; // get all transformed rules having same head e.g.  V -> V$0,  V -> V$1 Obj,  V -> V$2 Obj Obj
				auto q = find(entries.begin(), entries.end(), *new_rule); // search if an equivalent rule exists 
				// two rules are equivalent if all parameters except initial left terminals are the same
				// e.g (V -> watch Obj , V -> look after Obj) are equivalent, can be reduced to V -> V$0 Obj , V$0 -> watch , V$0 -> look after
				string name;
				if (q == entries.end()) { // if no equivalent rule is found, then create the parent rule (eg V -> V$0 Obj )
					name = rule->head->name + '$' + to_string(entries.size());
					(*rule->left)[0] = new Symbol(name, true);
					entries.push_back(ref(*rule));
					rule->id = rules.size();
					resolve_reference(rule->left, rule->right);
					rules.emplace_back(rule); // unique_ptr is created for the transformed rule
				}
				else
				{
					name = q->get().left->at(0)->name;
				}
				new_rule->head = new Symbol(name, true);

				if (debug >= 3)
					cout << "***" << *rule << '\n';
				add_trie(root, new_rule->sentence(), new_rule);
			}
		}
}
