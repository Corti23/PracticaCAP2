# Nombre del ejecutable
TARGET = raytracing

# Compilador y flags
CXX = g++
CXXFLAGS = -O3 -fopenmp
SRC = main.cpp utils.cpp random.cpp Scene.cpp Sphere.cpp Metallic.cpp Crystalline.cpp
OBJ = $(SRC:.cpp=.o)

# Regla por defecto (compilar todo)
all: $(TARGET)

# Cómo construir el ejecutable a partir de los objetos
$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Cómo construir los objetos a partir de los .cpp
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

# Limpiar objetos y ejecutable
clean:
	rm -f $(TARGET) *.o