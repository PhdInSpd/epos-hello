//Code generated by Gemini For the request
/*
Write a C++ sample that Will read And write an instance of MyData  to an XML file using the pugixml library. The read and write operation should be separate Functions. There should be a CPP file in the header file. MyData is defined as

struct MyData {

     std::string name;
    std::vector<std::vector<double>> points;

};

*/
#pragma once

#include <iostream>
#include <string>
#include <vector>
#include "./pugixml/pugixml.hpp"

struct TeachData {
    std::string name;
    double pathAcceleration;
    /// <summary>
    /// there should be MAX_NUM_AXES entries + 1(pathVelocity)
    /// </summary>
    std::vector<std::vector<double>> points;

    void print() const;
    bool operator==(const TeachData& other) const;
    TeachData(const TeachData& other);
    TeachData();
};

bool saveDataToXML(const TeachData& data, const std::string& filename);
bool loadDataFromXML(TeachData& data, const std::string& filename);

int xmlWriteReadTest();