#include "stdafx.h"

//using namespace std;

struct CharMapper {
	unordered_map<string, char> in_map;
	unordered_map<char, string> out_map;
	CharMapper();
	string map_in(const string& s);
	string map_out(const string& s);
	void add_uni_map(const u8string& su8, char c) {
		in_map[string(su8.begin(), su8.end())] = c;
	}
	void add_map(const u8string& su8, char c) {
		string s(su8.begin(), su8.end());
		in_map[s] = c;
		out_map[c] = s;
	}
};

struct PostProcessor : public CharMapper {
	static const int _vowel_type = 0x03, _const = 0x04, _vowel = 0x08, _cons = 0x10;
	char char_types[128];
	char H[4] = { 'I' , 'i', 'u',  'U' };
	char A[4] = { 'a', 'e', 'a', 'e' };
	void init_char_types();
	PostProcessor()	{ init_char_types(); }
	bool is_vowel(char c) { return (char_types[c] & _vowel) != 0; }
	bool is_cons(char c) { return (char_types[c] & _cons) != 0; }
	bool is_const(char c) { return (char_types[c] & _const) != 0; }
	char vowel_type(char c) { return char_types[c] & _vowel_type; }
	char last_letter = 'e', last_vowel_type = 1;
/*
	bool is_vowel(const char* s)
	{
		return is_vowel(s[0]) || s[0] == 'N' || s[0] == 'Z' || s[0] == 'Y' && is_vowel(s[1]);
	}
*/
	string process(string s);
};

