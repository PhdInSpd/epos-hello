#include "TeachData.h"
#include <sstream>
#include <stdexcept>

void TeachData::print() const {
    std::cout << "Name: " << name << std::endl;
    std::cout << "PathAcceleration: " << pathAcceleration << std::endl;
    
    std::cout << "Points:" << std::endl;
    for (const auto& row : points) {
        for (double p : row) {
            std::cout << p << " ";
        }
        std::cout << std::endl;
    }
}

bool TeachData::operator==(const TeachData& other) const {
    return  name == other.name &&
            pathAcceleration == other.pathAcceleration &&
            points == other.points ;
}

bool saveDataToXML(const TeachData& data, const std::string& filename) {
    pugi::xml_document doc;
    pugi::xml_node root = doc.append_child("TeachData");

    root.append_child("name").append_child(pugi::node_pcdata).set_value(data.name.c_str());
    root.append_child("pathAcceleration").append_child(pugi::node_pcdata).set_value(std::to_string( data.pathAcceleration).c_str());
    
    pugi::xml_node pointsNode = root.append_child("points");
    for (const auto& row : data.points) {
        pugi::xml_node rowNode = pointsNode.append_child("row");
        for (double point : row) {
            std::stringstream ss;
            ss << point;
            rowNode.append_child("point").append_child(pugi::node_pcdata).set_value(ss.str().c_str());
        }
    }

    return doc.save_file(filename.c_str());
}

bool loadDataFromXML(TeachData& data, const std::string& filename) {
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(filename.c_str());

    if (!result) {
        std::cerr << "XML file parsing error: " << result.description() << std::endl;
        return false;
    }

    pugi::xml_node root = doc.child("TeachData");
    if (!root) {
        std::cerr << "Error: Root node 'MyData' not found." << std::endl;
        return false;
    }

    data.name = root.child("name").child_value();
    data.pathAcceleration = std::stod(root.child("pathAcceleration").child_value());
    
    
    data.points.clear();
    pugi::xml_node pointsNode = root.child("points");
    if (pointsNode) {
        for (pugi::xml_node rowNode : pointsNode.children("row")) {
            std::vector<double> row;
            for (pugi::xml_node pointNode : rowNode.children("point")) {
                try {
                    row.push_back(std::stod(pointNode.child_value()));
                }
                catch (const std::invalid_argument& e) {
                    std::cerr << "Error converting point to double: " << e.what() << std::endl;
                }
                catch (const std::out_of_range& e) {
                    std::cerr << "Error converting point to double (out of range): " << e.what() << std::endl;
                }
            }
            data.points.push_back(row);
        }
    }

    return true;
}

int xmlWriteReadTest() {
    std::string filename = "teach-data.xml";

    TeachData original;
    original.name = filename;
    original.pathAcceleration = 20;
    original.points = {   {1.0, 2.5, 3.7, 4, 5, 6,1},
                                {4.2, 5.1, 6, 7, 8, 9, 1.5}}; // 2D vector

    if ( !saveDataToXML(original, filename) ) {
        std::cerr << "Error saving data." << std::endl;
        return 1;
    }

    TeachData loaded;
    if ( !loadDataFromXML(loaded, filename) ) {
        std::cerr << "Error loading data." << std::endl;
        return 1;
    }

    std::cout << "Loaded data:" << std::endl;
    loaded.print();

    if (original == loaded) {
        std::cout << "Original and loaded data are identical." << std::endl;
    }
    else {
        std::cout << "Original and loaded data are different." << std::endl;
    }

    return 0;
}
