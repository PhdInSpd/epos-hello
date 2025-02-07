#include "xml-test.h"

bool saveDataToXML(const MyData& data, const std::string& filename) {
    pugi::xml_document doc;

    // Create the root element
    pugi::xml_node root = doc.append_child("MyData");

    // Add the data elements as children of the root
    pugi::xml_node nameNode = root.append_child("name");
    nameNode.append_child(pugi::node_pcdata).set_value(data.name.c_str());

    pugi::xml_node pointsNode = root.append_child("points");
    for (double point : data.points) {
        std::stringstream ss;
        ss << point;  // Convert double to string
        pugi::xml_node pointNode = pointsNode.append_child("point");
        pointNode.append_child(pugi::node_pcdata).set_value(ss.str().c_str());
    }
    pugi::xml_node ageNode = root.append_child("age");
    ageNode.append_child(pugi::node_pcdata).set_value(std::to_string(data.age).c_str());

    pugi::xml_node addressNode = root.append_child("address");
    addressNode.append_child(pugi::node_pcdata).set_value(data.address.c_str());


    // Save the document to the file
    bool result = doc.save_file(filename.c_str());

    if (result) {
        std::cout << "Data saved to " << filename << std::endl;
    }
    else {
        std::cerr << "Error saving data to " << filename << std::endl;
    }

    return result;
}

bool loadDataFromXML(MyData& data, const std::string& filename) {
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(filename.c_str());

    if (!result) {
        std::cerr << "XML file parsing error: " << result.description() << std::endl;
        return false;
    }


    pugi::xml_node root = doc.child("MyData");
    if (!root) {
        std::cerr << "Error: Root node 'MyData' not found." << std::endl;
        return false;
    }

    data.name = root.child("name").child_value();
    try {
        data.age = std::stoi(root.child("age").child_value());
    }
    catch (const std::invalid_argument& e) {
        std::cerr << "Error converting age to int: " << e.what() << std::endl;
        return false;
    }
    catch (const std::out_of_range& e) {
        std::cerr << "Error converting age to int (out of range): " << e.what() << std::endl;
        return false;
    }

    data.address = root.child("address").child_value();

    data.points.clear();
    pugi::xml_node pointsNode = root.child("points");
    if (pointsNode) {
        for (pugi::xml_node pointNode : pointsNode.children("point")) {
            try {
                data.points.push_back(std::stod(pointNode.child_value()));
            }
            catch (const std::invalid_argument& e) {
                std::cerr << "Error converting point to double: " << e.what() << std::endl;
                // Handle the error as needed (e.g., skip the point)
            }
            catch (const std::out_of_range& e) {
                std::cerr << "Error converting point to double (out of range): " << e.what() << std::endl;
            }
        }
    }

    return true;
}

int xmlWriteReadTest() {
    MyData myData;
    myData.name = "John Doe";
    myData.points = { 1.0, 2.5, 3.7, 4.2 }; // Example points
    myData.age = 30;
    myData.address = "123 Main St, Anytown";

    std::string filename = "myData.xml";

    if (saveDataToXML(myData, filename)) {
        // Data saved successfully
    }
    else {
        // Handle the error as needed
    }
    MyData read;
    loadDataFromXML(read, filename);
    return 0;
}
