#include "stdafx.h"
#include "parser.h"
#include <chrono>

struct UnitTest {
	bool shared, dot, raw_dot;
	int unknown = 0, show_trans_expr = 0;
	int case_total = 0, success_total = 0;
	int case_cnt = 0, total_cnt = 0, success_cnt = 0;
	int line_num = 0;
	long long total_parse = 0, total_make = 0, total_unify = 0, total_trans = 0, total_post = 0;
	unique_ptr<Parser> parser;
	ifstream& get_line(ifstream& is, string& line);
	string get_lines(ifstream& is, stringstream& ref);
	UnitTest(bool _shared = false, bool _dot = false, bool _raw_dot = false) : shared(_shared), dot(_dot), raw_dot(_raw_dot) { }
	static void diff(string a, string b);
	void parse_sent(string sent);
	void test_sent(string line);
	void test_case(string fname);
	void test_dir(string dirname);
	void test_file(string fname);
	void print_result();
};

ifstream& UnitTest::get_line(ifstream& is, string& line) {
	line_num++;
	getline(is, line);
	return is;
}
string UnitTest::get_lines(ifstream& is, stringstream& ref) {
	// get all the lines until next line starting with "###<command>", returns the <command> or "" if EOF
	string line;
	while (get_line(is, line) && line.substr(0, 3) != "###") {
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
Parser* p_parser;
SymbolTable* p_symbol_table;

void UnitTest::parse_sent(string sent) {
	TreeNode* ptree = nullptr, * utree = nullptr, * ttree = nullptr;
	//Parser* parser = p_parser; // CHANGE SOON!!!

	try {
		auto start = std::chrono::system_clock::now();
		parser->parse(sent);
		auto end = std::chrono::system_clock::now();
		auto mics_parse = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
		total_parse += mics_parse;

		start = std::chrono::system_clock::now();
		ptree = parser->make_tree(shared);
		end = std::chrono::system_clock::now();
		auto mics_make = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
		total_make += mics_make;
		//if (dot)
		//	dot_print(fname + ".parse.dot", ptree, true, false);

		if (debug >= 1)
			print_tree(cout, ptree, true, true, false);

		start = std::chrono::system_clock::now();
		utree = unify_tree(ptree, shared);
		end = std::chrono::system_clock::now();
		auto mics_unify = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
		total_unify += mics_unify;
		//if (dot)
		//	dot_print(fname + ".unify.dot", utree, true, false);

		if (debug >= 1)
			print_tree(cout, utree, true, true, false);

		start = std::chrono::system_clock::now();
		ttree = parser->translate_tree(utree, shared);
		end = std::chrono::system_clock::now();
		auto mics_trans = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
		total_trans += mics_trans;
		//if (dot)
		//	dot_print(fname + ".trans.dot", ttree, false, true);
		
		if (debug >= 1)
			print_tree(cout, ttree, true, true, true);
		print_tree(cout, ttree, false, false, true);
		EnumVec results;
		start = std::chrono::system_clock::now();
		convert(cout, ttree, parser.get(), results, show_trans_expr);
		end = std::chrono::system_clock::now();
		auto mics_post = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
		total_post += mics_post;

		cout << results[0].first << nl;

		if (profile >= 2)
			cout << "  *Parse: " << mics_parse << " Make: " << mics_make << " Unify: " << mics_unify << " Trans: " << mics_trans << " Post: " << mics_post << '\n';
	}
	catch (UnifyError& e) {
		cout << "  UnifyError: " << e.what() << nl;
	}
	catch (PostProcessError& e) {
		cout << "  PostProcessError: " << e.what() << nl;
	}
	catch (ParseError& e) {
		cout << "  ParseError: " << e.what() << nl;
	}
}
void UnitTest::test_sent(string line) { // 0:Skip 1:Ignore 2:Error 3:NOK 4:OK 
	TreeNode* ptree = nullptr, * utree = nullptr, * ttree = nullptr;

	ltrim(line);
	if (line.empty() || line[0] == '#')
		return;
	auto io = split_strip(line, '@');
	if (io.size() != 2) {
		cerr << "Invalid test sentence: " << line << nl;
		return;
	}
	auto expects = split_strip(io[1], '|');
	if (!expects.size()) {
		cerr << "Invalid expected sentence: " << line << nl;
		return;
	}
	cout << nl << io[0] << " @ " << io[1] << '\n';
	case_cnt++;

	try {

		auto start = std::chrono::system_clock::now();
		parser->parse(io[0]);
		auto end = std::chrono::system_clock::now();
		auto mics_parse = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
		total_parse += mics_parse;

		start = std::chrono::system_clock::now();
		ptree = parser->make_tree(shared);
		end = std::chrono::system_clock::now();
		auto mics_make = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
		total_make += mics_make;
		//if (dot)
		//	dot_print(fname + ".parse.dot", ptree, true, false);

		//if (debug >= 1)
		//	print_tree(cout, ptree, true, true, false);

		start = std::chrono::system_clock::now();
		utree = unify_tree(ptree, shared);
		end = std::chrono::system_clock::now();
		auto mics_unify = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
		total_unify += mics_unify;
		//if (dot)
		//	dot_print(fname + ".unify.dot", utree, true, false);

		//if (debug >= 1)
		//	print_tree(cout, utree, true, true, false);

		start = std::chrono::system_clock::now();
		ttree = parser->translate_tree(utree, shared);
		end = std::chrono::system_clock::now();
		auto mics_trans = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
		total_trans += mics_trans;
		//if (dot)
		//	dot_print(fname + ".trans.dot", ttree, false, true);

		//if (debug >= 1)
		//	print_tree(cout, ttree, true, true, true);

		EnumVec results;
		start = std::chrono::system_clock::now();
		convert(cout, ttree, parser.get(), results, show_trans_expr);
		end = std::chrono::system_clock::now();
		auto mics_post = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
		total_post += mics_post;

		bool found = false;
		for (auto& [s, cost] : results) {
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
		cout << "  UnifyError: " << e.what() << nl;
	}
	catch (PostProcessError& e) {
		cout << "  PostProcessError: " << e.what() << nl;
	}
	catch (ParseError& e) {
		cout << "  ParseError: " << e.what() << nl;
	}

	//if (raw_dot) {
	//	ofstream os(fname + ".raw.dot");
	//	parser->print_parse_dot(os);
	//}
	//if (raw_dot) {
	//	ofstream os(fname + ".rawall.dot");
	//	parser->print_parse_dot_all(os);
	//}
}
//if (profile >= 1)
//cout << "  *Parse: " << total_parse << " Make: " << total_make << " Unify: " << total_unify << " Trans: " << total_trans << " Post: " << total_post << '\n';

void UnitTest::test_file(string fname) {
	ifstream is(fname);
	string line;
	while (get_line(is, line)) {
		test_sent(line);
	}
}
void UnitTest::test_case(string fname) {
	// loads an executes test cases from file "fname"
	//int case_cnt = 0, success_cnt = 0;
	ifstream is(fname);
	if (!is) {
		cout << "Cannot open " << fname << nl;
		return;
	}
	string line;
	
	TreeNode *ptree = nullptr, *utree = nullptr, *ttree = nullptr;
	get_line(is, line);
	string error;
	while (!line.empty()) {
		vector<string> cmd_params;
		split(cmd_params, line, ' ');
		string& command = cmd_params[0];
		if (command == "###load_grammar") {
			if (cmd_params.size() != 2) {
				cerr << "Invalid command parameter: " << line;
				return;
			}
			try {
				stringstream grammar;
				int start_line_num = line_num;
				line = get_lines(is, grammar);

				parser = make_unique<Parser>();
				GrammarParser gparser(parser.get());
				gparser.file = fname;
				auto start = std::chrono::system_clock::now();
				gparser.load_grammar(cmd_params[1]);
				//gparser.parse_grammar(grammar, 0, start_line_num);
				auto end = std::chrono::system_clock::now();
				if (profile >= 1)
					cout << "ParseGrammar: " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << " mics\n";

				p_parser = parser.get(); // quick & dirty solution
				p_symbol_table = &parser->symbol_table;

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
		else if (command == "###grammar") {
			try {
				stringstream grammar;
				int start_line_num = line_num;
				line = get_lines(is, grammar);

				parser = make_unique<Parser>();
				GrammarParser gparser(parser.get());
				gparser.file = fname;
				auto start = std::chrono::system_clock::now();
				gparser.parse_grammar(grammar, 0, start_line_num);
				auto end = std::chrono::system_clock::now();
				if (profile >= 1)
					cout << "ParseGrammar: " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << " mics\n";

				p_parser = parser.get(); // quick & dirty solution
				p_symbol_table = &parser->symbol_table;

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
		else if (command == "###interact") {
			string sent;
			for (;;) {
				cout << "Enter Sent> ";
				getline(cin, sent);
				rtrim(sent);
				if (sent.starts_with("%")) {
					auto params = split_strip(sent, '=');
					if (params.size() == 2 && params[0] == "%debug") {
						debug = stoi(params[1]);
					}
					else
						cout << "Error: Invalid Format";
					continue;
				}
				if (!sent.size())
					break;
				parse_sent(sent);
				flush(cout);
			}
			return;
		}
		else if (command == "###input") {
			string input;
			get_line(is, input);
			get_line(is, line);
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
		else if (command == "###test_file") {
			if (cmd_params.size() != 2) {
				cerr << "test_file command needs a file name parameter" << endl;
				continue;
			}
			test_file(cmd_params[1]);
			get_line(is, line);
		}
		else if (command == "###test") {
			while (get_line(is, line) && line.substr(0, 3) != "###") {
				test_sent(line);
			}
			if (profile >= 1)
				cout << "  *Parse: " << total_parse << " Make: " << total_make << " Unify: " << total_unify << " Trans: " << total_trans << " Post: " << total_post << '\n';
			if (!is) // eof
				line = "";
		}
		else if (command == "###comment") {
			while (get_line(is, line) && line.substr(0, 3) != "###");
			if (!is) // eof
				line = "";
		}
		else if (command == "###params") {
			while (get_line(is, line) && line.substr(0, 3) != "###") {
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
				else if (io[0] == "unknown") {
					unknown = bool(stoi(io[1]));
				}
				else if (io[0] == "show_trans_expr") {
					unknown = bool(stoi(io[1]));
				}
			}
			if (!is) // eof
				line = "";
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
				else if (command == "###enum") {
					EnumVec results;
					convert(cout, ttree, parser.get(), results, false);
					for (auto&[s, cost] : results)
						out << s << '\n';
				}
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
			line = get_lines(is, ref);
			bool result = out.str() == ref.str();
			cout << command.substr(3) << ": " << (result ? "OK" : "NOK") << nl;
			if (!result) {
				if (debug >= 1)
					diff(out.str(), ref.str());
			}
			else
				success_cnt++;
			case_cnt++;
		}
	}
	cout << fname << ": " << success_cnt << "/" << case_cnt << " (" << (case_cnt ? success_cnt * 100 / case_cnt : 0) << "%)" << nl;
	case_total += case_cnt;
	success_total += success_cnt;
	
}
void UnitTest::test_dir(string dirname) {
	// loads an executes all test cases in a directory with extension ".tst"
	debug = 0;
	debug_mem = 0;
	int case_total = 0, success_total = 0;
	//namespace fs = experimental::filesystem;
	namespace fs = filesystem;

	for (const auto & entry : fs::directory_iterator(dirname))
		if (entry.path().extension() == ".tst") {
			auto fname = reinterpret_cast<const char*>(entry.path().u8string().c_str());
			cout << "FILE:" << fname << nl;
			test_case(fname);
		}
}
void UnitTest::print_result() {
	cout << "TOTAL: " << success_total << "/" << case_total << " (" << (case_total ? success_total * 100 / case_total : 0) << "%)" << nl;
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
	std::cout.imbue(std::locale("")); // for thousands separator ","
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
	return 0;
}
