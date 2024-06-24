#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <cstring>
#include <sys/wait.h>
#include <iomanip>
#include <fcntl.h>
#include "Commands.h"

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";
const int READ = 0;
const int WRITE = 1;


#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

string _ltrim(const std::string &s) {
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string &s) {
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string &s) {
    return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char *cmd_line, char **args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for (std::string s; iss >> s;) {
        args[i] = (char *) malloc(s.length() + 1);
        memset(args[i], 0, s.length() + 1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    return i;

    FUNC_EXIT()
}

bool _isBackgroundComamnd(const char *cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char *cmd_line) {
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos) {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&') {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

// TODO: Add your implementation for classes in Commands.h 

Job::Job(int _pid, int _jobId, std::string& _command){
    this->pid = _pid;
    this->jobId = _jobId;
    this->command = _command;
}

int Job::get_pid() {
    return this->pid;
}
int Job::get_job_id() {
    return this->jobId;
}

std::string Job::get_command() {
    return this->command;
}




SmallShell::SmallShell() {
// TODO: add your implementation
    internalCommands = {"chprompt", "showpid", "pwd", "cd", "jobs", "fg", "quit", "kill",
        "alias", "unalias"};
    externalCommands = {};

}

SmallShell::~SmallShell() {
// TODO: add your implementation
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command *SmallShell::CreateCommand(const char *cmd_line) {
    // For example:
/*
  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

  if (firstWord.compare("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("showpid") == 0) {
    return new ShowPidCommand(cmd_line);
  }
  else if ...
  .....
  else {
    return new ExternalCommand(cmd_line);
  }
  */
    return nullptr;
}

std::string SmallShell::getCurrentWorkingDirectory() {
  const size_t bufferSize = 4096;
  char buffer[bufferSize];
  return getcwd(buffer, bufferSize);
}

void SmallShell::removeLastCharIfAmpersand(std::string& str) {
    if (!str.empty() && str.back() == '&') {
        str.pop_back();
    }
}

int SmallShell::getMaxJobId() {
    int maxJobId;
    if (this->jobsMap.empty()) {
        maxJobId = 0;
    }
    else {
        maxJobId = this->jobsMap.rbegin()->first;
    }
    return maxJobId;
}

bool is_pid_running(pid_t pid) {

    while(waitpid(-1, 0, WNOHANG) > 0) {
        // Wait for defunct....
    }

    if (0 == kill(pid, 0))
        return true; // Process exists

    return false;
}

void SmallShell::updateFinishedJobs(){
    // for (const int& value : this->readChannels) {
    //     int jobId = 0;
    //     read(value, &jobId, sizeof(jobId));
    //     if (jobId != 0){
    //         this->jobsMap.erase(jobId);
    //         std::cout <<"removed job:" << jobId << std::endl;
    //         close(value);
    //     }
    // }

    for (auto it = this->jobsMap.begin(); it != this->jobsMap.end();) {
        if (!is_pid_running(it->second->get_pid())) {
            // Process does not exist anymore
            delete it->second;
            it = this->jobsMap.erase(it);
        }
        else {
            ++it;
        }
    }
}



void SmallShell::_jobs() {
    this->updateFinishedJobs();
    for (auto it = this->jobsMap.begin(); it != this->jobsMap.end(); ++it) {
        std::cout <<"[" << it->first << "] " << it->second->get_command() << std::endl;
    }
}

void SmallShell::_quit(std::vector<std::string>& args) {
    if (!args.empty() && args[0] == "kill") {
        for (auto it = this->jobsMap.begin(); it != this->jobsMap.end(); ++it) {
            int pid = it->second->get_pid();
            kill(pid, 9);
        }   
    }
    exit(0);
}

void SmallShell::_kill(std::vector<std::string>& args){
    if (args.size() != 2) {
        // error
    }
    if (args[0][0] != '-') {
        // error
    }
    else {
        args[0].erase(args[0].begin());  // Erase the first character
    }

    int jobId;
    int sigNum;
    try {
        sigNum = std::stoi(args[0]); // Convert string to int
        jobId = std::stoi(args[1]); // Convert string to int

    } catch (const std::exception& e) {
        std::cerr << "Conversion error: " << e.what() << std::endl;
        // error
    }

    if (this->jobsMap.count(jobId) == 0) {
        // error
    }

    // kill(this->jobsMap[jobId].get_pid(), sigNum);

}

void SmallShell::_alias(std::vector<std::string> &args) {
    if (args.empty()) {
        for (auto & it : this->aliasMap) {
            std::cout << it.second << std::endl;
        }
    }
    // else if (args.size() != 1) {
    //     // some error
    // }

    // alias fwefwe=quit kill frefr frefref frfe
    else {
        // If "=" is found in the string
        std::string alias, command;
        size_t pos = args[0].find('=');
        if (pos != std::string::npos) {
            alias = args[0].substr(0, pos);
            command = args[0].substr(pos + 1);
            if (this->aliasMap.find(alias) != this->aliasMap.end()) {
                // eror that this alias exists
            }
            else {
                for (int i=1; i < args.size(); i++) {
                    command += " " + args[i];
                }
                this->aliasMap[alias] = command;
            }
        } else {
            std::cout << "The input string does not contain an '=' character." << std::endl;

            return; // Exit with error code
        }
    }
}


std::vector<char*> SmallShell::parseStringCommand(const std::string& command) {
    std::vector<char*> args;
    char* cmd = strdup(command.c_str());  // Duplicate the command string for tokenization
    char* token = std::strtok(cmd, " ");  // Split string by spaces

    while (token != nullptr) {
        args.push_back(token);
        token = std::strtok(nullptr, " ");
    }
    args.push_back(nullptr);  // execvp requires a nullptr at the end

    return args;
}

bool SmallShell::isComplexCommand(std::string &command) {
    return command.find('*') != std::string::npos || command.find('?') != std::string::npos;
}

void SmallShell::runSimpleExternal(std::string &command) {

    std::vector<char*> args = this->parseStringCommand(command);
    // Arguments for the program (first argument is the program name)
    const char* program = args[0];

    execvp(program, args.data());
}

void SmallShell::runComplexCommand(std::string &command) {
    char* commandChar = strdup(command.c_str());
    char *bash = (char *)"/bin/bash";
    char *bashFlag = (char *)"-c";
    char *argv[] = {bash, bashFlag, commandChar, nullptr};
    execvp(bash, argv);}



void SmallShell::executeCommand(const char *cmd_line) {
    this->updateFinishedJobs();
    std::cout << cmd_line << std::endl;
    std::string cmd_line_str = cmd_line;
    this->removeLastCharIfAmpersand(cmd_line_str);
    std::vector<std::string> parsed_cmd = this->splitStringBySpace(cmd_line_str); // should parse the & from the last arg
    if (!parsed_cmd.empty()) {
        std::string command = parsed_cmd.front();
        std::vector<std::string> args(parsed_cmd.begin() + 1, parsed_cmd.end());

        if (command == "quit") {
            this->_quit(args);
        }
        else if (command == "alias") {
            this->_alias(args);
        }
        bool BACKGROUND_FLAG = this->isBackground(cmd_line);
        int pipefd[2];
        pipe(pipefd);
        int flags = fcntl(pipefd[READ], F_GETFL, 0);
        flags |= O_NONBLOCK;
        fcntl(pipefd[0], F_SETFL, flags);
        pid_t pid = fork();
        const int jobId = this->getMaxJobId() + 1;

        if (pid != 0) { // if father
            close(pipefd[WRITE]);
            if (BACKGROUND_FLAG){
                // Job job(pid, jobId, cmd_line_str);
                Job* job = new Job(pid, jobId, cmd_line_str);
                this->readChannels.insert(pipefd[READ]);
                this->jobsMap[jobId] = job;
                // this->jobsMap.emplace(jobId, Job(pid, jobId, cmd_line_str));
            }
            else {
                close(pipefd[READ]); // no background job so we can close the whole pipe
                int status;
                waitpid(pid, &status, 0);
            }
            
        }
        else {
            close(pipefd[READ]);
            if (command == "pwd") {
                std::cout << this->getCurrentWorkingDirectory() << std::endl;
            }
            else if (command == "jobs") {
                this->_jobs();
            }

            else if (command == "kill") {
                this->_kill(args);
            }
            else if (this->aliasMap.find(command) != this->aliasMap.end()) {
                std::string aliasCommand = this->aliasMap[command];
                for (const std::string & arg : args) {
                    aliasCommand += arg;
                }
                char* commandChar = new char[aliasCommand.size() + 1];
                strcpy(commandChar, aliasCommand.c_str());
                this->executeCommand(commandChar);
                delete[] commandChar;
            }

            if (this->isComplexCommand(cmd_line_str)) {
                this->runComplexCommand(cmd_line_str);
            }
            else {
                this->runSimpleExternal(cmd_line_str);

            }

            if (BACKGROUND_FLAG) {
                write(pipefd[WRITE], &jobId, sizeof(jobId));
            }
            close(pipefd[WRITE]);
            
            exit(0);

        }
        

    
    }

    // TODO: Add your implementation here
    // for example:
    // Command* cmd = CreateCommand(cmd_line);
    // cmd->execute();
    // Please note that you must fork smash process for some commands (e.g., external commands....)
}



std::vector<std::string> SmallShell::splitStringBySpace(const std::string& str) {
    std::vector<std::string> result;
    std::string word;
    for (char ch : str) {
        if (ch == ' ') {
            if (!word.empty()) {
                result.push_back(word);
                word.clear();
            }
        } else {
            word += ch;
        }
    }
    if (!word.empty()) {
        result.push_back(word);
    }
    return result;
}

bool  SmallShell::isBackground(const char* str) {
    const char* ptr = str;
    while (*ptr != '\0') {
        ++ptr;
    }
    if (*(ptr - 1) == '&') {
      return true;
    }
    return false;
}