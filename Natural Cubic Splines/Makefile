LIBS = -lGLEW -lglfw
FWKS = -framework Cocoa -framework OpenGL -framework IOKit

NaturalCubicSpline: Source/glSetup.cpp Source/NaturalCubicSpline.cpp Source/glSetup.h
	g++ -o EXE/$@ Source/$@.cpp Source/glSetup.cpp $(LIBS) $(FWKS)
