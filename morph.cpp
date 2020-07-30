#include "stdafx.h"
#include "morph.h"

using namespace std;

CharMapper::CharMapper() {
	u8string temp = u8" ";
	for (int c = 32; c <= 127; ++c) {
		temp[0] = c;
		add_map(temp, c);
	}
	add_map(u8"ğ", 'G');
	add_uni_map(u8"Ğ", 'G');
	add_map(u8"ş", 'S');
	add_uni_map(u8"Ş", 'S');
	add_map(u8"ç", 'C');
	add_uni_map(u8"Ç", 'G');
	add_map(u8"ö", 'O');
	add_uni_map(u8"Ö", 'O');
	add_map(u8"ü", 'U');
	add_uni_map(u8"Ü", 'U');
	add_map(u8"ı", 'I');
	add_uni_map(u8"İ", 'i');
}

string CharMapper::map_in(const string& s) {
	string result;

	auto it = s.cbegin();
	auto end = s.cend();
	while (it != end) {
		string cp;

		unsigned char lb = *it;
		int len = 0;
		if ((lb & 0x80) == 0) // lead bit is zero, must be a single ascii
			len = 1;
		else if ((lb & 0xE0) == 0xC0)  // 110x xxxx
			len = 2;
		else if ((lb & 0xF0) == 0xE0) // 1110 xxxx
			len = 3;
		else if ((lb & 0xF8) == 0xF0) // 1111 0xxx
			len = 4;
		else
			throw runtime_error("Unrecognized lead byte: " + lb);
		for (; len > 0 && it != end; --len, ++it) {
			cp.push_back(*it);
		}
		if (len > 0)
			throw runtime_error("Unfinished unicode char: " + cp);

		if (in_map.count(cp) == 0)
			throw runtime_error("Undefined unicode string: " + cp);
		result.push_back(in_map[cp]);
	}
	return result;
}

string CharMapper::map_out(const string& s) {
	string result;
	for (char c : s) {
		result += out_map[c];
	}
	return result;
}

void PostProcessor::init_char_types() {
	memset(char_types, 0, sizeof(char_types));
	for (auto c : "aI")
		char_types[c] |= _vowel;

	for (auto c : "ei")
		char_types[c] |= _vowel | 1;

	for (auto c : "ou")
		char_types[c] |= _vowel | 2;

	for (auto c : "OU")
		char_types[c] |= _vowel | 3;

	for (auto c : "CfhkpsSt")
		char_types[c] |= _cons | _const;

	for (auto c : "bcdgGjlmnrvyz")
		char_types[c] |= _cons;

	for (auto c : "AHV")
		char_types[c] |= _vowel;

	char_types[0] = 0;
}
/*
string PostProcessor::process(string& s) {
	string out;
	const char* in = s.c_str();
	char last_letter = 'e', last_vowel_type = 1, prev_vowel_type = 1;
	while (*in) {
		char c = *in;
		int next_letter = in[1];
		bool is_next_vowel = is_vowel(in + 1);
		switch (c) {
			case 'Y': if (is_vowel(last_letter)) out.push_back('y'); break;
			case 'N': 
				if (in[1] == 0 || in[1] == ' ' || in[1] == 'N' || in[1] == 'Y' && !is_vowel(in[2]))
					break;
				if (is_vowel(last_letter)) out.push_back('n'); break;
			case 'Z': if (is_vowel(last_letter)) out.push_back('s'); break;
			case '?': if ( is_next_vowel && out.size() >= 1) {
				c = out.back();
				out.pop_back();
				switch (c) {
					case 'k':
						if (out.size() >= 1 && out.back() == 'n')
							out.push_back('g');
						else
							out.push_back('G');
						break;
					case 'g':
						out.push_back('G');
						break;
					case 'p':
						out.push_back('b');
						break;
					case 't':
						out.push_back('d');
						break;
					case 'C':
						out.push_back('c');
						break;
				}
			} break;
			case 'H':
				if (!is_vowel(last_letter))
					out.push_back(H[last_vowel_type]);
				break;
			case 'A':
				out.push_back(A[last_vowel_type]);
				break;
			case 'V':
				if (is_vowel(last_letter)) {
					out.pop_back();
					out.push_back(H[prev_vowel_type]);
				}
				else
					out.push_back(H[last_vowel_type]);
				break;
			case 'D':
				out.push_back(is_const(last_letter) ? 't' : 'd');
				break;
			case '+':
				if (is_next_vowel && out.size() >= 1)
					out.push_back(out.back());
				break;
			case '^':
				last_vowel_type |= 1;
				in++;
				continue;
			case '@':
				if (is_vowel(in + 3))
					in++;
				break;
			default:
				out.push_back(c);
		}
		prev_vowel_type = last_vowel_type;
		last_letter = out.back();
		if (is_vowel(last_letter))
			last_vowel_type = vowel_type(last_letter);
		in++;
	}
	return out;
}
*/
string PostProcessor::process(string s) {
	//regex regex_N("N(?= |$|N|Y[^aeIioOuUHA])");
	//regex regex_YZNH("([^aeIioOuUHA])(?:Y|Z|N)|([aeIioOuUHA])H");
	//regex regex_V("[aeIioOuUHA]V");
	//s = regex_replace(s, regex_N, "");
	//s = regex_replace(s, regex_YZNH, "$1$2");
	//s = regex_replace(s, regex_V, "H");
	string out;
	const char* in = s.c_str();
	bool is_vowel_last_letter = true;
	while (*in) {
		switch (*in) {
			case 'Y': 
				if (is_vowel_last_letter)
					out.push_back('y'); 
				break;
			case 'N':
				if (in[1] == 0 || in[1] == ' ' || in[1] == 'N' || in[1] == 'Y' && !is_vowel(in[2]))
					break;
				if (is_vowel_last_letter) 
					out.push_back('n'); 
				break;
			case 'Z': 
				if (is_vowel_last_letter)
					out.push_back('s'); 
				break;
			case 'H':
				if (!is_vowel_last_letter)
					out.push_back('H');
				break;
			case 'V':
				if (is_vowel_last_letter)
					out.pop_back();
				out.push_back('H');
				break;
			default:
				out.push_back(*in);
		}
		if (out.size() >= 1)
			is_vowel_last_letter = is_vowel(out.back());
		in++;
	}
	s = move(out);
	in = s.c_str();
	//char last_letter = 'e', last_vowel_type = 1;
	while (*in) {
		char c = *in;
		bool is_vowel_next_letter = (in[1] == '+') ? is_vowel(in[2]) : is_vowel(in[1]);
		switch (c) {
			case '?':
				if (is_vowel_next_letter && out.size() >= 1) {
					c = out.back();
					out.pop_back();
					switch (c) {
					case 'k':
						if (out.size() >= 1 && out.back() == 'n')
							out.push_back('g');
						else
							out.push_back('G');
						break;
					case 'g':
						out.push_back('G');
						break;
					case 'p':
						out.push_back('b');
						break;
					case 't':
						out.push_back('d');
						break;
					case 'C':
						out.push_back('c');
						break;
					case 'y':
						out.push_back('y');
					}
				}
				else if (out.back() == 'y')
					out.pop_back();
				break;
			case 'H':
				out.push_back(H[last_vowel_type]);
				break;
			case 'A':
				out.push_back(A[last_vowel_type]);
				break;
			case 'D':
				out.push_back(is_const(last_letter) ? 't' : 'd');
				break;
			case '+':
				if (is_vowel_next_letter && out.size() >= 1)
					out.push_back(out.back());
				break;
			case '^':
				last_vowel_type |= 1;
				in++;
				continue;
			case '@':
				if (is_vowel(in[3]))
					in++;
				break;
			default:
				out.push_back(c);
		}
		if (out.size() >= 1)
			last_letter = out.back();
		if (is_vowel(last_letter))
			last_vowel_type = vowel_type(last_letter);
		in++;
	}
	return out;
}

