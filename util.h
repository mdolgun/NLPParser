#pragma once
#include "stdafx.h"
using namespace std;

inline std::ostream& format(std::ostream& os, const char* s) {
	return os << s;
}
template <typename T, typename... Args>
std::ostream& format(std::ostream& os, const char* fmt, T first, Args&&... rest)
{
	const char* s = strstr(fmt, "{}");
	if (!s) {
		return os << fmt;
	}
	os.write(fmt, s - fmt);
	os << first;
	return format(os, s + 2, std::forward<Args>(rest)...);
}
template <typename... Args>
std::string format(const char* fmt, Args&&... args)
{
	std::ostringstream os;
	format(os, fmt, std::forward<Args>(args)...);
	return os.str();
}

template <class T>
inline void hash_combine(std::size_t& seed, T const& v)
//Combines the hash of the current item with the hashes of the previous ones (boost::hash_combine)
{
	seed ^= hash<T>()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template <typename T, typename U>
struct hash<pair<T, U>> {
	size_t operator()(const pair<T, U>& pair) const
	{
		size_t seed = 0;
		hash_combine(seed, pair.first);
		hash_combine(seed, pair.second);
		return seed;
	}
};

template <typename T>
struct hash<set<T>> {
	size_t operator()(const set<T>& set) const
	{
		size_t seed = 0;
		for (auto& item : set)
			hash_combine(seed, item);
		return seed;
	}
};

//template <typename... T>
//struct hash<tuple<T...>> {
//	size_t operator()(const tuple<T...>& tuple) const
//	{
//		size_t seed = 0;
//		for (int i = 0; i < sizeof...(T); i++)
//			hash_combine(seed, tuple.get<i>());
//		return seed;
//	}
//};

template <class Tuple, size_t Index = std::tuple_size<Tuple>::value - 1>
struct HashValueImpl
{
	static void apply(size_t& seed, Tuple const& tuple)
	{
		HashValueImpl<Tuple, Index - 1>::apply(seed, tuple);
		hash_combine(seed, std::get<Index>(tuple));
	}
};

template <class Tuple>
struct HashValueImpl<Tuple, 0>
{
	static void apply(size_t& seed, Tuple const& tuple)
	{
		hash_combine(seed, std::get<0>(tuple));
	}
};

template <typename ... TT>
struct hash<std::tuple<TT...>>
{
	size_t
		operator()(std::tuple<TT...> const& tt) const
	{
		size_t seed = 0;
		HashValueImpl<std::tuple<TT...> >::apply(seed, tt);
		return seed;
	}

};

template <typename T>
inline ostream& operator<<(ostream& os, const vector<T>& set) {
	os << "{";
	bool empty = true;
	for (auto& item : set) {
		if (empty)
			empty = false;
		else
			os << ",";
		os << item;
	}
	return os << "}";
}

template <typename T>
ostream& join(ostream& os, const T& container, const char*delim) {
	// dumps all elements of the container to "os", separated by "delim" characters
	bool flag = false;
	for (auto& rec : container) {
		if (flag)
			os << delim;
		else
			flag = true;
		os << rec;
	}
	return os;
}
template <typename Iterator>
ostream& join(ostream& os, Iterator start, Iterator end, const char*delim) {
	// dumps elements between "start" and "end" of a container to "os", separated by "delim" characters
	bool flag = false;
	for (auto it = start; it != end; it++) {
		if (flag)
			os << delim;
		else
			flag = true;
		os << **it;
	}
	return os;
}


