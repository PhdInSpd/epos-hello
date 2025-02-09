#include "RecipeRead.h"

/// <summary>
/// precondition: directory exists
/// </summary>
/// <param name="dirPath"></param>
/// <returns></returns>
std::vector< std::string>  readRecipeFiles(std::string dirPath ) {
    std::vector<std::string> recipes;
    try
    {
       /* if ( fs::exists(filePath) ) {
            std::cout << "File exists: " << filePath << std::endl;
            if (fs::is_regular_file(filePath)) {
                std::cout << "It's a regular file." << std::endl;
            }
        }
        else {
            std::cout << "File does not exist: " << filePath << std::endl;
        }*/

        
        namespace fs = std::filesystem; // A common shorthand

        // Option 1: Using a range-based for loop (C++17) - Simpler
        for (const auto& entry : fs::directory_iterator(dirPath)) {
            //std::cout << entry.path() << std::endl; // Full path
            // OR
            // std::cout << entry.path().filename() << std::endl; // Just the filename
            if (entry.is_regular_file()) {
                recipes.push_back( entry.path().string() );
            }
            /*else if (entry.is_directory()) {
            }*/
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl; // Handle potential exceptions
    }
    
    return recipes;
}