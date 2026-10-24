# build myshell from all the source files
myshell: command.cc tokenizer.cc command.h tokenizer.h
	g++ -Wall -std=c++11 command.cc tokenizer.cc -o myshell


# remove the built program
clean:
	rm -f myshell 
