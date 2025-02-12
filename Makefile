CXX = g++
CXXFLAGS = -std=c++17 -Wall -g  # Example flags
LDFLAGS = -lpugixml
TARGET = cath-robot
OBJDIR = obj
SRCDIR = .

SOURCES = $(SRCDIR)/CatheterRobot.cpp $(SRCDIR)/JodoApplication.cpp $(SRCDIR)/JodoCanopenMotion.cpp $(SRCDIR)/PLC.cpp $(SRCDIR)/pugixml.cpp $(SRCDIR)/RecipeRead.cpp $(SRCDIR)/RecipeRead.cppScaling $(SRCDIR)/RecipeRead.cppTeachData $(SRCDIR)/RecipeRead.cpp $(SRCDIR)/testgamecontroller.cpp $(SRCDIR)/testutils.cpp
OBJECTS = $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

$(TARGET): $(OBJECTS)
        $(CXX) $(CXXFLAGS) $(OBJECTS) $(LDFLAGS) -o $(TARGET)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp $(SRCDIR)/%.h
        mkdir -p $(OBJDIR) # Make the directory if it doesn't exist
        $(CXX) $(CXXFLAGS) -c $< -o $@

clean:
        rm -f $(TARGET) $(OBJDIR)/*.o