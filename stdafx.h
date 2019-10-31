#pragma once

#ifndef __GNUC__
#define CHANGE_CODEPAGE
#endif

#define DEBUG

#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <unordered_set>
#include <set>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string_view>
#include <regex>
#include <stdexcept>
#include <memory>
#if defined(__GNUC__) && __GNU_CC < 8
	#include <experimental/filesystem>
#else
	#include <filesystem>
#endif

#include <cassert>

#ifdef CHANGE_CODEPAGE
#include<windows.h>
#endif

#include "util.h"
