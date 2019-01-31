# set this variable to the director in which you saved the common files
commondir = ../common/

all : project

project : project.cpp MicroGlut.o loadobj.o GL_utilities.o VectorUtils3.o
	g++ -g -Wall -std=c++17 -o project MicroGlut.o loadobj.o GL_utilities.o VectorUtils3.o -I$(commondir) -I$(commondir)/Linux -DGL_GLEXT_PROTOTYPES project.cpp -lXt -lX11 -lGL -lm -lassimp

MicroGlut.o loadobj.o GL_utilities.o VectorUtils3.o: $(commondir)GL_utilities.c $(commondir)VectorUtils3.c $(commondir)loadobj.c $(commondir)Linux/MicroGlut.c
	gcc -c -g -I$(commondir) -I$(commondir)/Linux -DGL_GLEXT_PROTOTYPES $(commondir)GL_utilities.c $(commondir)loadobj.c $(commondir)VectorUtils3.c $(commondir)Linux/MicroGlut.c

clean :
	rm project MicroGlut.o loadobj.o GL_utilities.o VectorUtils3.o
