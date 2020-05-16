LIBS = -lGLEW -lglfw
FWKS = -framework Cocoa -framework OpenGL -framework IOKit

BSpline: Source/glSetup.cpp Source/BSpline.cpp Source/hsv2rgb.cpp Source/glSetup.h Source/hsv2rgb.h
	g++ -o EXE/$@ Source/$@.cpp Source/glSetup.cpp Source/hsv2rgb.cpp $(LIBS) $(FWKS)
