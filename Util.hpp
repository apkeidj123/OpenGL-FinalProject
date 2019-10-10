#ifndef UTIL_HPP
#define UTIL_HPP

#include "../Externals/Include/Include.h"
#include <sstream>
#include <iomanip>

using namespace glm;
using namespace std;

string to_string(const vec3& vec) {
    ostringstream ss;
    ss << " (";
    ss << fixed << setprecision(2) << vec.x << ", ";
    ss << fixed << setprecision(2) << vec.y << ", ";
    ss << fixed << setprecision(2) << vec.z;
    ss << ") ";
    return ss.str();
}

string to_string(const mat4& mat) {
    ostringstream ss;
    for (int i = 0; i < 4; i++) {
        ss << ((i == 0) ? "[[": " [");
        for (int j = 0; j < 4; j++) {
            ss << setw(7) << fixed << setprecision(2) << mat[i][j] << ",";
        }
        ss << ((i == 3) ? "]]": "]");
        ss << endl;
    }
    return ss.str();
}

#endif