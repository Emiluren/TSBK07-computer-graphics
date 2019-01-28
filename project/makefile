# set this variable to the director in which you saved the common files
commondir = ../common/

all : project

project : project.c $(commondir)GL_utilities.c $(commondir)VectorUtils3.c $(commondir)loadobj.c $(commondir)Linux/MicroGlut.c
	gcc -Wall -std=c11 -o project -I$(commondir) -I$(commondir)/Linux -DGL_GLEXT_PROTOTYPES project.c $(commondir)GL_utilities.c $(commondir)loadobj.c $(commondir)VectorUtils3.c $(commondir)Linux/MicroGlut.c -lXt -lX11 -lGL -lm

clean :
	rm project
