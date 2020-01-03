#include "stdafx.h"
#include "parser.h"
#include <chrono>

string UnitTest::get_lines(ifstream& is, stringstream& ref) {
	// get all the lines until next line starting with "###<command>", returns the <command> or "" if EOF
	string line;
	while (getline(is, line) && line.substr(0, 3) != "###") {
		ref << line << '\n';
	}
	if (is)
		return line;
	return "";
}

void UnitTest::diff(string a, string b) {
	// outputs diff of two strings
	cout << "***";
	auto it_a = a.begin(), it_b = b.begin();
	auto end_a = a.end(), end_b = b.end();
	for (; it_a != end_a && it_b != end_b && *it_a == *it_b; ++it_a, ++it_b);
	ostream_iterator<char> it_os(cout);
	copy(a.begin(), it_a, it_os);
	cout << "<<<";
	copy(it_a, end_a, it_os);
	cout << ">>>";
	copy(it_b, end_b, it_os);
	cout << "***" << nl;
}

vector<string> split_strip(const string& s, char delim) {
	vector<string> result;
	split(result, s, delim);
	for (auto& item : result) {
		ltrim(item);
		rtrim(item);
	}
	return result;
}

void UnitTest::test_case(string fname) {
	// loads an executes test cases from file "fname"
	int case_cnt = 0, success_cnt = 0;
	ifstream is(fname);
	if (!is) {
		cout << "Cannot open " << fname << nl;
		return;
	}
	long long total_parse = 0, total_make = 0, total_unify = 0, total_trans = 0, total_post = 0;
	string command;
	unique_ptr<Parser> parser;
	TreeNode *ptree = nullptr, *utree = nullptr, *ttree = nullptr;
	getline(is, command);
	string error;
	while (!command.empty()) {
		if (command == "###grammar") {
			try {
				stringstream grammar;
				command = get_lines(is, grammar);

				parser = make_unique<Parser>();
				GrammarParser gparser(parser.get());

				auto start = std::chrono::system_clock::now();
				gparser.parse_grammar(grammar);
				auto end = std::chrono::system_clock::now();
				if (profile >= 1)
					cout << "ParseGrammar: " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << " mics\n";

				if (debug >= 1)
					parser->print_rules(cout);
				start = std::chrono::system_clock::now();
				parser->compile();
				end = std::chrono::system_clock::now();
				if (profile >= 1)
					cout << "CompileGrammar: " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << " mics\n";
			}
			catch (GrammarError& e) {
				cerr << e.what() << nl;
				return;
			}
		}
		else if (command == "###input") {
			string input;
			getline(is, input);
			getline(is, command);
			if (debug_mem >= 1)
				cout << "Parse-begin FeatList count: " << FeatList::count << endl;
			try {
				auto start = std::chrono::system_clock::now();
				parser->parse(input);
				auto end = std::chrono::system_clock::now();
				cout << "Parse: " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << " mics\n";

				ptree = parser->make_tree(shared);
				utree = unify_tree(ptree, shared);
				ttree = parser->translate_tree(utree, shared);
				error = "";
			}
			catch (UnifyError&) {
				error = "*UnifyError";
			}
			catch (ParseError&) {
				error = "*ParseError";
			}
			if (debug_mem >= 1)
				cout << "Parse-end FeatList count: " << FeatList::count << endl;
		}
		else if (command == "###test") {
			string line;
			while (getline(is, line) && line.substr(0, 3) != "###") {
				ltrim(line);
				if (line.empty() || line[0] == '#')
					continue;
				auto io = split_strip(line, ':');
				assert(io.size() == 2);
				auto expects = split_strip(io[1], '|');
				assert(expects.size() >= 1);
				case_cnt++;

				try {
					cout << io[0] << " : " << io[1] << '\n';
					auto start = std::chrono::system_clock::now();
					parser->parse(io[0]);
					auto end = std::chrono::system_clock::now();
					auto mics_parse = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
					total_parse += mics_parse;
					if (raw_dot) {
						ofstream os(fname + ".raw.dot");
						parser->print_parse_dot(os);
					}
					if (raw_dot) {
						ofstream os(fname + ".rawall.dot");
						parser->print_parse_dot_all(os);
					}

					start = std::chrono::system_clock::now();
					ptree = parser->make_tree(shared);
					end = std::chrono::system_clock::now();
					auto mics_make = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
					total_make += mics_make;
					if (dot)
						dot_print(fname + ".parse.dot", ptree, true, false);

					if (debug >= 1)
						print_tree(cout, ptree, true, true, false);

					start = std::chrono::system_clock::now();
					utree = unify_tree(ptree, shared);
					end = std::chrono::system_clock::now();
					auto mics_unify = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
					total_unify += mics_unify;
					if (dot)
						dot_print(fname + ".unify.dot", utree, true, false);

					if (debug >= 1)
						print_tree(cout, utree, true, true, false);

					start = std::chrono::system_clock::now();
					ttree = parser->translate_tree(utree, shared);
					end = std::chrono::system_clock::now();
					auto mics_trans = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
					total_trans += mics_trans;
					if (dot)
						dot_print(fname + ".trans.dot", ttree, false, true);

					if (debug >= 1)
						print_tree(cout, ttree, true, true, true);

					EnumVec results;
					start = std::chrono::system_clock::now();
					cout << '#'; convert(cout, ttree, parser.get(), results); 
					end = std::chrono::system_clock::now();
					auto mics_post = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
					total_post += mics_post;

					bool found = false;
					for (auto& [s,cost] : results) {
						for (auto& expect : expects)
							if (s == expect) {
								found = true;
								break;
							}
						cout << " *" << s << " {" << cost << "}\n";
					}
					
					if (found) {
						success_cnt++;
						cout << "  OK\n";
					}
					else
						cout << "  NOK\n";

					if (profile >= 2)
						cout << "  *Parse: " << mics_parse << " Make: " << mics_make << " Unify: " << mics_unify << " Trans: " << mics_trans << " Post: " << mics_post << '\n';

				}
				catch (UnifyError& e) {
					cout << "  *UnifyError: " << e.what() << nl;
				}
				catch (ParseError& e) {
					cout << "  *ParseError: " << e.what() << nl;
				}
			}
			if (profile >= 1)
				cout << "  *Parse: " << total_parse << " Make: " << total_make << " Unify: " << total_unify << " Trans: " << total_trans << " Post: " << total_post << '\n';
			if (is)
				command = line;
			else // eof
				command = "";
		}
		else if (command == "###comment") {
			string line;
			while (getline(is, line) && line.substr(0, 3) != "###");
			if (is)
				command = line;
			else // eof
				command = "";
		}
		else if (command == "###params") {
			string line;
			while (getline(is, line) && line.substr(0, 3) != "###") {
				ltrim(line);
				if (line.empty() || line[0] == '#')
					continue;
				auto io = split_strip(line, '=');
				assert(io.size() == 2);
				if (io[0] == "debug") {
					debug = stoi(io[1]);
				}
				else if (io[0] == "profile") {
					profile = stoi(io[1]);
				}
				else if (io[0] == "shared") {
					shared = bool(stoi(io[1]));
				}
				else if (io[0] == "raw_dot") {
					raw_dot = bool(stoi(io[1]));
				}
				else if (io[0] == "dot") {
					dot = bool(stoi(io[1]));
				}
			}
			if (is)
				command = line;
			else // eof
				command = "";
		}
		else {
			stringstream ref, out;
			if (error.empty()) {
				if (command == "###format")
					print_tree(out, ttree, false, false, false);
				else if (command == "###formatr")
					print_tree(out, ttree, false, false, true);
				else if (command == "###pformat")
					print_tree(out, ttree, true, false, false);
				else if (command == "###pformatr")
					print_tree(out, ttree, true, false, true);
				else if (command == "###pformat_ext")
					print_tree(out, ttree, true, true, false);
				else if (command == "###pformatr_ext")
					print_tree(out, ttree, true, true, true);
				else if (command == "###enum")
					for (auto& s : enumerate(parser.get(), ttree))
						out << s << '\n';
				else
				{
					cerr << "Invalid Section:" << command << nl;
					stringstream data;
					command = get_lines(is, data);
				}
			}
			else {
				out << error << nl;
			}
			string new_command = get_lines(is, ref);
			bool result = out.str() == ref.str();
			cout << command.substr(3) << ": " << (result ? "OK" : "NOK") << nl;
			if (!result) {
				if (debug >= 1)
					diff(out.str(), ref.str());
			}
			else
				success_cnt++;
			case_cnt++;
			command = move(new_command);
		}
	}
	cout << fname << ": " << success_cnt << "/" << case_cnt << nl;
	case_total += case_cnt;
	success_total += success_cnt;
}
void UnitTest::test_dir(string dirname) {
	// loads an executes all test cases in a directory with extension ".tst"
	debug = 0;
	debug_mem = 0;
	int case_total = 0, success_total = 0;
	namespace fs = experimental::filesystem;

	for (const auto & entry : fs::directory_iterator(dirname))
		if (entry.path().extension() == ".tst") {
			auto fname = entry.path().u8string();
			cout << "FILE:" << fname << nl;
			test_case(fname.c_str());
		}
}
void UnitTest::print_result() {
	cout << "TOTAL: " << success_total << "/" << case_total << nl;
}


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
		for (auto& s : enumerate(&parser,tree)) // enumerate all possible output sentences
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
		for (auto& s : enumerate(&parser,tree3))
			cout << s << endl;
	}
	catch (GrammarError& e) {
		cout << "ParseError" << e.what() << nl;
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
		for (auto& s : enumerate(&parser,tree3))
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

void test_dir(const char* dirname = "test") {
	UnitTest test(false);
	test.test_dir(dirname);
	test.print_result();
}
void test_case(const char* fname) {
	UnitTest test(false);
	test.test_case(fname);
	test.print_result();
}
int main(int argc, char* argv[])
{
	if (argc <= 1 || strlen(argv[1]) != 2 || argv[1][0] != '-' || !strchr("dtie", argv[1][1])) {
		cerr << "USAGE: -d <test_dir> | -t <test_file> | -i <grammar_file> | -e <grammar_file> \"<Sentence>\"*\n";
		return 1;
	}
#ifdef CHANGE_CODEPAGE
	//SetConsoleOutputCP(1254);
	SetConsoleOutputCP(65001);
#endif
	cout.sync_with_stdio(false); // for performance increase
	switch (argv[1][1]) {
		case 't': 
			if (argc != 3)
				return 1;
			test_case(argv[2]);
			break;
		case 'd':
			if (argc != 3)
				return 1;
			test_dir();
			break;
		default:
			return 1;
	}
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
	//test_case("ng_dic.tst");
	//debug_mem = 1;
	//if (debug_mem >= 1)
	//	cout << "FeatList count: " << FeatList::count << endl;
	//test.test_case("test/trans_case_norm.tst");
	//if (debug_mem >= 1)
	//	cout << "FeatList count: " << FeatList::count << endl;
	//debug = 1;
	//test_translate("test/deep.grm", "she sleeps");
	//test_translate("ng.grm", "house under me");
	return 0;
}
