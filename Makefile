CXX = g++
CXXFLAGS = -std=c++17 -Wall -g -pthread -I./SDL2/include  # Added -I./SDL2/include
# LDFLAGS = -lEposCmd -lpugixml -lrt -lcanopen # Added -lrt for real-time and -lcanopen
LDFLAGS = -lEposCmd 
TARGET = cather-robot
OBJDIR = obj
SRCDIR = .

# Define macros here:
DEBUG_MACRO = -DDEBUG_MODE=1 -DEXPORT_DLL
RELEASE_MACRO = -DRELEASE_MODE=1

SOURCES = $(SRCDIR)/CatheterRobot.cpp $(SRCDIR)/JodoApplication.cpp $(SRCDIR)/JodoCanopenMotion.cpp $(SRCDIR)/PLC.cpp $(SRCDIR)/RecipeRead.cpp $(SRCDIR)/Scaling.cpp $(SRCDIR)/TeachData.cpp $(SRCDIR)/testgamecontroller.cpp $(SRCDIR)/pugixml/pugixml.cpp 
OBJECTS = $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

# Example: Debug build
debug: CXXFLAGS += $(DEBUG_MACRO)  # Add DEBUG_MACRO to CXXFLAGS
debug: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) $(LDFLAGS) -o $(TARGET)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJDIR)/*.o