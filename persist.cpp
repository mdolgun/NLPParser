#include "stdafx.h"
#include "common.h"

// !!! TODO: check_list

void save(ostream& os, int val) {
	os.write((const char*)&val, sizeof(val));
}
void load(istream& is, int& val) {
	is.read((char*)&val, sizeof(val));
}
void save(ostream& os, bool val) {
	os.write((const char*)&val, sizeof(val));
}
void load(istream& is, bool& val) {
	is.read((char*)&val, sizeof(val));
}
void save(ostream& os, const string& s) {
	save(os, (int)s.size());
	os.write(s.c_str(), s.size());
}
void load(istream& is, string& s) {
	int size;
	load(is, size);
	s.resize(size);
	is.read(&s[0], size);
}

void save_suffixes(ostream& os, Grammar& grammar) {
	save(os, static_cast<int>(grammar.suffixes.size()));
	for (auto item : grammar.suffixes) {
		save(os, item.first);
		save(os, item.second);
	}
}
void load_suffixes(istream& is, Grammar& grammar) {
	int size;
	load(is, size);
	for (int i = 0; i < size; ++i) {
		string key, val;
		load(is, key);
		load(is, val);
		grammar.suffixes[key] = val;
	}
}

void save_templates(ostream& os, Grammar& grammar) {
	save(os, static_cast<int>(grammar.templates.size()));
	for (auto item : grammar.templates)
		save(os, item.first);
}

void load_templates(istream& is, Grammar& grammar) {
	int size;
	load(is, size);
	for (int i = 0; i < size; ++i) {
		string s;
		load(is, s);
		grammar.templates.emplace(s, nullptr);
	}
}

void save(ostream& os, Grammar& grammar) {
	save(os, (int)grammar.rules.size());
	for (auto& rule : grammar.rules) {
		rule->save(os);
	}
}
void load(istream& is, Grammar& grammar) {
	int size;
	load(is, size);
	grammar.rules.reserve(size);
	for (int i = 0; i<size; ++i) {
		Rule* rule = new Rule(is, grammar);
		grammar.rules.emplace_back(rule);
		rule->id = i;
	}
}
void Rule::save(ostream& os) {
	head->save(os);
	left->save(os);
	right->save(os);
	feat->save(os);
}

Rule::Rule(istream& is,Grammar& grammar) : 
	head(new Symbol(is,&grammar.symbol_table)), 
	left(new Prod(is, &grammar.symbol_table)),
	right(new Prod(is, &grammar.symbol_table)),
	feat(new FeatList(is)) {
	grammar.templates.emplace(get_template(), nullptr);
	grammar.template_rules.emplace(this, nullptr);
}

void Prod::save(ostream& os) {
	::save(os, cost);
	::save(os, cut);
	::save(os, (int)size());
	for (auto symbol : *this) {
		symbol->save(os);
	}
}
Prod::Prod(istream& is, SymbolTable* symbol_table) {
	load(is, cost);
	load(is, cut);
	int size;
	load(is, size);
	reserve(size);
	for (int i = 0; i<size; ++i) {
		push_back(new Symbol(is, symbol_table));
	}
}

void FeatList::save(ostream& os) {
	::save(os, (int)size());
	for (auto&[name, val] : *this) {
		::save(os, name);
		::save(os, get<string>(val));
	}
}

FeatList::FeatList(istream& is) {
	int size;
	load(is, size);
	for (int i = 0; i<size; ++i) {
		string name, val;
		load(is, name);
		load(is, val);
		(*this)[name] = val;
	}
}

FeatParam::FeatParam(istream& is) {
	int enum_param_type, size;
	load(is, enum_param_type);
	param_type = (FeatParamType)enum_param_type;
	if (param_type == FeatParamType::all)
		return;
	params = new map<string, string>;
	load(is, size);
	for (int i = 0; i < size; ++i) {
		string name, val;
		load(is, name);
		load(is, val);
		(*params)[move(name)] = move(val);
	}
}
void FeatParam::save(ostream& os) {
	::save(os, (int)param_type);
	if (param_type != FeatParamType::all) {
		::save(os, (int)params->size());
		for (auto& [name, val] : *params) {
			::save(os, name);
			::save(os, val);
		}
	}
}

void Symbol::save(ostream& os) {
	fparam.save(os);
	::save(os, name);
	//::save(os, id);
	::save(os, nonterminal);
	::save(os, idx);
	//save_fparam(os, fparam);
}

Symbol::Symbol(istream& is,SymbolTable* symbol_table) : fparam(is) {
	load(is, name);
	//load(is, id);
	load(is, nonterminal);
	if (symbol_table && nonterminal)
		id = symbol_table->add(name, true);
	load(is, idx);
	//fparam = load_fparam(is);
}

void SymbolTable::save(ostream& os) {
	::save(os, size());
	for (auto& item : table) {
		::save(os, item.name);
		::save(os, item.nonterminal);
	}
}

SymbolTable::SymbolTable(istream& is) {
	int size;
	load(is, size);
	table.reserve(size);
	for (int i = 0; i < size; ++i) {
		string name;
		bool nonterm;
		load(is, name);
		load(is, nonterm);
		add(name, nonterm);
	}
}

void TrieNode::save(ostream& os) {
	::save(os, keys);
	::save(os, (int)values.size());
	for (auto value : values)
		value->save(os);
	::save(os, (int)children.size());
	for (auto child : children)
		child->save(os);
}
TrieNode::TrieNode(istream& is, Grammar& grammar) {
	load(is, keys);
	int value_size, child_size;
	load(is, value_size);
	values.reserve(value_size);
	for (int i = 0; i < value_size; ++i)
		values.push_back(new Rule(is, grammar));
	load(is, child_size);
	children.reserve(child_size);
	for (int i = 0; i < child_size; ++i)
		children.push_back(new TrieNode(is, grammar));
}
