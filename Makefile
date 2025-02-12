CXX = g++
CXXFLAGS = -std=c++17 -Wall -g -pthread # Added -pthread for threading
LDFLAGS = -lpugixml -lrt -lcanopen # Added -lrt for real-time and -lcanopen
TARGET = epos-hello
OBJDIR = obj
SRCDIR = .

SOURCES = $(SRCDIR)/CatheterRobot.cpp $(SRCDIR)/JodoApplication.cpp $(SRCDIR)/JodoCanopenMotion.cpp $(SRCDIR)/PLC.cpp $(SRCDIR)/RecipeRead.cpp $(SRCDIR)/Scaling.cpp $(SRCDIR)/TeachData.cpp $(SRCDIR)/testgamecontroller.cpp $(SRCDIR)/testutils.cpp
OBJECTS = $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(OBJECTS) $(LDFLAGS) -o $(TARGET)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJDIR)/*.o