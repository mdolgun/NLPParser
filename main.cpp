#include "stdafx.h"
#include "parser.h"

void test() {
	Parser parser;
	parser.load_grammar("test/trans_case.grm");
	parser.compile();
	try {
		parser.parse("i saw a car");
		auto tree = parser.make_tree();
		print_tree(cout, tree, true, false, false);
		auto tree2 = unify_tree(tree);
		print_tree(cout, tree2, true, true, false);
		auto tree3 = parser.translate_tree(tree2);
		print_tree(cout, tree3, true, true, true);
	}
	catch (UnifyError&) {
		cout << "UnifyError" << nl;
	}
	catch (ParseError&) {
		cout << "ParseError" << nl;
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
	UnitTest test(true);
	test.test_dir("test");
	test.print_result();
}
void test_case() {
	UnitTest test(true);
	test.test_case("test/simple_trans.tst");
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
								 //test_tree("test/trans_case.grm", "i saw a car", "test/trans_case_sh", true);
								 //test_tree("test/trans_case.grm", "i saw a car", "test/trans_case", false);

								 //test_tree("test/simple_trans.grm", "i saw the man in the house with the telescope", "test/simple_trans_sh", true, false, true);
								 //test_tree("test/simple_trans.grm", "i saw the man in the house with the telescope", "test/simple_trans", false, false, true);
								 //test_tree("test/simple_trans_feat.grm", "i saw the man in the house", "test/simple_trans_feat_sh", true, true, true);
								 //test_tree("test/simple_trans_feat.grm", "i saw the man in the house", "test/simple_trans_feat", false, true, true);
	test_tree("test/ambig_trans.grm", "people like heroes like people", "test/ambig_trans_sh", true, true, true);
	test_tree("test/ambig_trans.grm", "people like heroes like people", "test/ambig_trans", false, true, true);
	//test_dir();
	//test_case();
	//debug_mem = 1;
	//if (debug_mem >= 1)
	//	cout << "FeatList count: " << FeatList::count << endl;
	//test.test_case("test/trans_case_norm.tst");
	//if (debug_mem >= 1)
	//	cout << "FeatList count: " << FeatList::count << endl;
	return 0;
}
