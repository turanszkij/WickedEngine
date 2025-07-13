#pragma once
#include <string>
#include <cassert>
#include <cmath>
#include <deque>
#include <limits>
#include <mutex>
#include <fstream>
#include <istream>
#include <streambuf>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <sstream>

#include "WickedEngine.h"
#include "Editor.h"

// Linked externally from EmbeddedResources.cpp:
extern const uint8_t font_awesome_v6[];
extern const size_t font_awesome_v6_size;
