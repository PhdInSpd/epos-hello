#include "./pugixml/pugixml.hpp"
#include <iostream>



struct MyData {

    std::string name;

    int age;

    std::string address;

};


bool saveDataToXML(const MyData& data, const std::string& filename) {
    pugi::xml_document doc;

    // Create the root element
    pugi::xml_node root = doc.append_child("MyData");

    // Add the data elements as children of the root
    pugi::xml_node nameNode = root.append_child("name");
    nameNode.append_child(pugi::node_pcdata).set_value(data.name.c_str());

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

MyData readXmlData(const std::string& filename) {

    pugi::xml_document doc;

    if (!doc.load_file(filename.c_str())) {

        // Handle error

    }

    pugi::xml_node root = doc.child("MyData");

    MyData data;

    data.name = root.child("firstName").text().as_string();

    data.age = std::stoi(root.child("age").text().as_string());

    data.address = root.child("address").text().as_string();

    return data;
}

int xmlWriteReadTest() {
    MyData myData;
    myData.name = "John Doe";
    myData.age = 30;
    myData.address = "123 Main St, Anytown";

    std::string filename = "myData.xml";

    if (saveDataToXML(myData, filename)) {
        // Data saved successfully
    }
    else {
        // Handle the error as needed
    }
    MyData read = readXmlData(filename);
    return 0;
}
