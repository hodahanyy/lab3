
/*
 * 
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <dirent.h>
#include <vector>
#include <string>
#include <iostream>
#include "command.h"
#include "tokenizer.h"

bool matchesPattern(const char *name, const char *pattern)
{
    if (*pattern == '\0')
        return *name == '\0';

    if (*pattern == '*')
    {
        return matchesPattern(name, pattern + 1) ||
               (*name != '\0' && matchesPattern(name + 1, pattern));
    }

    if (*pattern == '?' || *pattern == *name)
    {
        return *name != '\0' &&
               matchesPattern(name + 1, pattern + 1);
    }

    return false;
}


void parse(std::vector<Token> &tokens)
{
    Command &currentCommand = Command::_currentCommand;
    SimpleCommand *currentSimpleCommand = new SimpleCommand();

    for (int i = 0; i < tokens.size(); i++)
    {
        Token token = tokens[i];

        // Stop when we reach the end of the command
        if (token.type == TOKEN_EOF)
        {
            break;
        }

if (token.type == TOKEN_COMMAND || token.type == TOKEN_ARGUMENT)
{
    // Exit the shell when the user types exit
    if (token.value == "exit")
    {
        exit(0);
    }

    // Change directory when the user types cd
    if (token.value == "cd")
    {
        if (i + 1 < tokens.size())
        {
            i++;
            chdir(tokens[i].value.c_str());
        }
        else
        {
            chdir(getenv("HOME"));
        }

        continue;
    }

    if (token.value.find('*') != std::string::npos ||
    token.value.find('?') != std::string::npos)
{
    DIR *dir = opendir(".");
    
    if (dir != NULL)
    {
        struct dirent *entry;
        bool found = false;

        while ((entry = readdir(dir)) != NULL)
        {
            if (entry->d_name[0] == '.')
                continue;

            if (matchesPattern(entry->d_name, token.value.c_str()))
            {
                char *argument = strdup(entry->d_name);
                currentSimpleCommand->insertArgument(argument);
                found = true;
            }
        }

        closedir(dir);

        if (!found)
        {
            char *argument = strdup(token.value.c_str());
            currentSimpleCommand->insertArgument(argument);
        }
    }
}
else
{
    char *argument = strdup(token.value.c_str());
    currentSimpleCommand->insertArgument(argument);
}
}

        // Pipe: save the current simple command
        else if (token.type == TOKEN_PIPE)
        {
            currentCommand.insertSimpleCommand(currentSimpleCommand);
            currentSimpleCommand = new SimpleCommand();
        }

        // Output redirection >
        else if (token.type == TOKEN_REDIRECT)
        {
            i++;

            if (i < tokens.size())
            {
                currentCommand._outFile = strdup(tokens[i].value.c_str());
            }
        }

        // Append output >>
        else if (token.type == TOKEN_APPEND)
        {
            i++;

            if (i < tokens.size())
            {
                currentCommand._outFile = strdup(tokens[i].value.c_str());
                currentCommand._append = 1;
            }
        }

        // Input redirection <
        else if (token.type == TOKEN_INPUT)
        {
            i++;

            if (i < tokens.size())
            {
                currentCommand._inputFile = strdup(tokens[i].value.c_str());
            }
        }

        // Error redirection 2>
        else if (token.type == TOKEN_ERROR)
        {
            i++;

            if (i < tokens.size())
            {
                currentCommand._errFile = strdup(tokens[i].value.c_str());
            }
        }
// Redirect output and errors to the same file
else if (token.type == TOKEN_REDIRECT_AND_ERROR)
{
    i++;

    if (i < tokens.size())
    {
        currentCommand._outFile = strdup(tokens[i].value.c_str());
        currentCommand._errFile = strdup(tokens[i].value.c_str());
        currentCommand._append = 1;
        currentCommand._out_error = 1;
    }
}

        // Background &
        else if (token.type == TOKEN_BACKGROUND)
        {
            currentCommand._background = 1;
        }
    }

    // Add the last simple command
    if (currentSimpleCommand->_numberOfArguments > 0)
    {
        currentCommand.insertSimpleCommand(currentSimpleCommand);
    }
    else
    {
        delete currentSimpleCommand;
    }

    currentCommand.execute();
}

SimpleCommand::SimpleCommand()
{
    _numberOfAvailableArguments = 5;
    _numberOfArguments = 0;
    _arguments = (char **)malloc(_numberOfAvailableArguments * sizeof(char *));
}

void SimpleCommand::insertArgument(char *argument)
{
    if (_numberOfAvailableArguments == _numberOfArguments + 1)
    {
        _numberOfAvailableArguments *= 2;
        _arguments = (char **)realloc(_arguments,
                                      _numberOfAvailableArguments * sizeof(char *));
    }

    _arguments[_numberOfArguments] = argument;
    _arguments[_numberOfArguments + 1] = NULL;

    _numberOfArguments++;
}

Command::Command()
{
    _numberOfAvailableSimpleCommands = 1;

    _simpleCommands = (SimpleCommand **)
        malloc(_numberOfAvailableSimpleCommands* sizeof(SimpleCommand *));

    _numberOfSimpleCommands = 0;

    _outFile = 0;
    _inputFile = 0;
    _errFile = 0;
    _background = 0;
    _append = 0;
    _out_error = 0;
    

}

void Command::insertSimpleCommand(SimpleCommand *simpleCommand)
{
    if (_numberOfAvailableSimpleCommands == _numberOfSimpleCommands)
    {
        _numberOfAvailableSimpleCommands *= 2;
        _simpleCommands = (SimpleCommand **)realloc(_simpleCommands,
                                                    _numberOfAvailableSimpleCommands * sizeof(SimpleCommand *));
    }

    _simpleCommands[_numberOfSimpleCommands] = simpleCommand;
    _numberOfSimpleCommands++;
}

void Command::clear()
{
    for (int i = 0; i < _numberOfSimpleCommands; i++)
    {
        for (int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++)
        {
            free(_simpleCommands[i]->_arguments[j]);
        }

        free(_simpleCommands[i]->_arguments);
        free(_simpleCommands[i]);
    }

    if (_outFile)
    {
        free(_outFile);
    }

    if (_inputFile)
    {
        free(_inputFile);
    }

    if (_errFile)
    {
        free(_errFile);
    }

    _numberOfSimpleCommands = 0;
    _outFile = 0;
    _inputFile = 0;
    _errFile = 0;
    _background = 0;
    _append = 0;
    _out_error = 0;
}

void Command::print()
{
    printf("\n\n");
    printf("              COMMAND TABLE                \n");
    printf("\n");
    printf("  #   Simple Commands\n");
    printf("  --- ----------------------------------------------------------\n");

    for (int i = 0; i < _numberOfSimpleCommands; i++)
    {
        printf("  %-3d ", i);
        for (int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++)
        {
            printf("\"%s\" \t", _simpleCommands[i]->_arguments[j]);
        }
    }

    printf("\n\n");
    printf("  Output       Input        Error        Err&Out       Background\n");
    printf("  ------------ ------------ ------------ ------------ ------------\n");
    printf("  %-12s %-12s %-12s %-12s %-12s\n", _outFile ? _outFile : "default",
           _inputFile ? _inputFile : "default", _errFile ? _errFile : "default", _out_error == 1 ? _errFile : "default"
           ,_background ? "YES" : "NO");
    printf("\n\n");
}
void Command::execute()
{
    print();

int previousPipe = -1;
std::vector<pid_t> childPids;

    // Run each simple command
    for (int i = 0; i < _numberOfSimpleCommands; i++)
    {
        int pipefd[2];

if (i < _numberOfSimpleCommands - 1)
{
    if (pipe(pipefd) < 0)
    {
        perror("pipe");
        continue;
    }
}

        pid_t pid = fork();

        // Check if fork failed
        if (pid < 0)
        {
            perror("fork");
            continue;
        }

// Child process
if (pid == 0)
{
    // Let the child process respond normally to Ctrl+C
      signal(SIGINT, SIG_DFL);

    // Read from the previous pipe
    if (previousPipe != -1)
    {
        dup2(previousPipe, 0);
        close(previousPipe);
    }

    // Send output to the next pipe
    if (i < _numberOfSimpleCommands - 1)
    {
        dup2(pipefd[1], 1);
        close(pipefd[0]);
        close(pipefd[1]);
    }

    // Redirect output and errors to the same file with >>&
    if (_out_error)
    {
        int fd = open(_outFile, O_WRONLY | O_CREAT | O_APPEND, 0644);

        if (fd < 0)
        {
            perror("open");
            exit(1);
        }

        dup2(fd, 1);
        dup2(fd, 2);

        close(fd);
    }

    // Redirect output to a file if > or >> was used
    else if (_outFile != 0)
    {
        int fd;

        if (_append)
        {
            fd = open(_outFile, O_WRONLY | O_CREAT | O_APPEND, 0644);
        }
        else
        {
            fd = open(_outFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        }

        if (fd < 0)
        {
            perror("open");
            exit(1);
        }

        dup2(fd, 1);
        close(fd);
    }

    // Read input from a file if < was used
    if (_inputFile != 0)
    {
        int fd = open(_inputFile, O_RDONLY);

        if (fd < 0)
        {
            perror("open");
            exit(1);
        }

        dup2(fd, 0);
        close(fd);
    }

    // Redirect errors to a file if 2> was used
    if (_errFile != 0 && !_out_error)
    {
        int fd = open(_errFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);

        if (fd < 0)
        {
            perror("open");
            exit(1);
        }

        dup2(fd, 2);
        close(fd);
    }

    execvp(
        _simpleCommands[i]->_arguments[0],
        _simpleCommands[i]->_arguments
    );

    perror("execvp");
    exit(1);
}
        // Parent process
        else
{
    // Close the previous pipe because the parent does not need it anymore
    if (previousPipe != -1)
    {
        close(previousPipe);
    }

    // Save the new pipe for the next command
    if (i < _numberOfSimpleCommands - 1)
    {
        close(pipefd[1]);
        previousPipe = pipefd[0];
    }
    else
    {
        previousPipe = -1;
    }

    // Save the child process ID
childPids.push_back(pid);
} }

// Wait for all child processes to finish
if (!_background)
{
    for (int i = 0; i < childPids.size(); i++)
    {
        waitpid(childPids[i], NULL, 0);
    }
}
    
    clear();
    prompt();
}

void Command::prompt()
{
    printf("myshell>");
    fflush(stdout);
    std::string input;
    while (true)
    {
        std::getline(std::cin, input); 
        std::vector<Token> tokens = tokenize(input);
        parse(tokens);
    }
}

Command Command::_currentCommand;
SimpleCommand *Command::_currentSimpleCommand;

void handleSigchld(int signal)
{
    // Clean up finished child processes
    while (waitpid(-1, NULL, WNOHANG) > 0)
    {
    }
}

int main()
{   signal(SIGINT, SIG_IGN); //handles Ctrl+C
    signal(SIGCHLD, handleSigchld); //handles finished children

    Command::_currentCommand.prompt();
    return 0;
}
