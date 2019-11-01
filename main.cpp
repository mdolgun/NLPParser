#include "stdafx.h"
#include "parser.h"

void test_ambig() {
	try {
		Parser parser;
		parser.load_grammar("test/ambig.grm"); // load the grammar from the file
		parser.compile(); // create DFA and other internal tables from the grammar
	
		parser.parse("people like heroes like people"); // parse the sentence
		parser.print_parse_dot_all("ambig_parse.dot"); // write parse tables to a file in a GraphViz format
		auto tree = parser.make_tree(true); // generate parse tree from internal tables
		dot_print("ambig_tree.dot", tree); // write parse tree to a file in a GraphViz format
	}
	catch (GrammarError&) { // input grammar cannot be parsed
		cout << "GrammarError" << endl;
	}
	catch (ParseError&) { // sentence cannot be parsed
		cout << "ParseError" << endl;
	}
}
void test_ambig_trans() {
	try {
		Parser parser;
		parser.load_grammar("test/ambig_trans.grm"); // load the grammar from the file
		parser.compile(); // create DFA and other internal tables from the grammar

		parser.parse("people like heroes like people"); // parse the sentence

		auto tree = parser.make_tree(true); // generate parse tree from internal tables
		tree = parser.translate_tree(tree, true); // translate the tree
		dot_print("ambig_trans_tree.dot", tree, false, true); // write translated parse tree to a file in a GraphViz format
		for (auto& s : enumerate(tree)) // enumerate all possible output sentences
			cout << s << endl;
	}
	catch (GrammarError&) { // input grammar cannot be parsed
		cout << "GrammarError" << endl;
	}
	catch (ParseError&) { // sentence cannot be parsed
		cout << "ParseError" << endl;
	}
}

void test_translate(const char* grm,const char* input) {
	try {
		Parser parser;
		parser.load_grammar(grm);
		parser.compile();
		parser.parse(input);
		auto tree = parser.make_tree();
		print_tree(cout, tree, true, false, false);
		auto tree2 = unify_tree(tree);
		print_tree(cout, tree2, true, true, false);
		auto tree3 = parser.translate_tree(tree2);
		print_tree(cout, tree3, true, true, true);
		for (auto& s : enumerate(tree3))
			cout << s << endl;
	}
	catch (GrammarError&) {
		cout << "ParseError" << nl;
	}
	catch (UnifyError&) {
		cout << "UnifyError" << nl;
	}
	catch (ParseError&) {
		cout << "ParseError" << nl;
	}
	catch (exception &e) {
		cout << e.what() << nl;
	}

}
//#define NO_UNIFY
//#define NEW_UNIFY
void test_tree(string grammar_file, string sentence, string out_file, bool shared = false, bool left = true, bool right = true) {
	Parser parser;
	parser.load_grammar(grammar_file.c_str());
	parser.compile();
	try {
		parser.parse(sentence);
		{
			ofstream os(out_file + "_raw.dot");
			parser.print_parse_dot(os);
		}
		{
			ofstream os(out_file + "_rawall.dot");
			parser.print_parse_dot_all(os);
		}
		auto tree = parser.make_tree(shared);
		//print_tree(cout, tree, true, false, false);
		dot_print(out_file + "_parse.dot", tree, left, right);
#ifndef NO_UNIFY
		auto tree2 = unify_tree(tree, shared);
		//print_tree(cout, tree2, true, true, false);
		dot_print(out_file + "_unify.dot", tree2, left, right);

		auto tree3 = parser.translate_tree(tree2, shared);
#else
		auto tree3 = parser.translate_tree(tree, shared);
#endif
		//print_tree(cout, tree3, true, true, true);
		dot_print(out_file + "_trans.dot", tree3, false, right);
		for (auto& s : enumerate(tree3))
			cout << s << '\n';
		cout << nl;
	}
	catch (UnifyError&) {
		cout << "UnifyError" << nl;
	}
	catch (ParseError&) {
		cout << "ParseError" << nl;
	}
}

void test_dir() {
	UnitTest test(false);
	test.test_dir("test");
	test.print_result();
}
void test_case(const char* fname) {
	UnitTest test(false);
	test.test_case(fname);
	test.print_result();
}
int main()
{
	debug = 1;
#ifdef CHANGE_CODEPAGE
	//SetConsoleOutputCP(1254);
	SetConsoleOutputCP(65001);
#endif
	cout.sync_with_stdio(false); // for performance increase
	//test_ambig_trans();
	//test_tree("test/trans_case.grm", "i saw a car", "test/trans_case_sh", true);
	//test_tree("test/trans_case.grm", "i saw a car", "test/trans_case", false);

	//test_tree("test/simple_trans.grm", "i saw the man in the house with the telescope", "test/simple_trans_sh", true, false, true);
	//test_tree("test/simple_trans.grm", "i saw the man in the house with the telescope", "test/simple_trans", false, false, true);
	//test_tree("test/simple_trans_feat.grm", "i saw the man in the house", "test/simple_trans_feat_sh", true, true, true);
	//test_tree("test/simple_trans_feat.grm", "i saw the man in the house", "test/simple_trans_feat", false, true, true);
	//test_tree("test/ambig_trans.grm", "people like heroes like people", "test/ambig_trans_sh", true, true, true);
	//test_tree("test/ambig_trans.grm", "people like heroes like people", "test/ambig_trans", false, true, true);
	//test_dir();
	//test_case("test/feat_param_rename.tst");
	//debug_mem = 1;
	//if (debug_mem >= 1)
	//	cout << "FeatList count: " << FeatList::count << endl;
	//test.test_case("test/trans_case_norm.tst");
	//if (debug_mem >= 1)
	//	cout << "FeatList count: " << FeatList::count << endl;
	test_translate("test/feat_param_rename.grm", "a");
	return 0;
}
