#pragma once
#include "./pugixml/pugixml.hpp"
#include <iostream>
#include <vector>
#include <sstream> // For converting doubles to strings



struct MyData {

    std::string name;
    std::vector<double> points;
    int age;

    std::string address;

};
int xmlWriteReadTest();