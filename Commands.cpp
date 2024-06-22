#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
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

SmallShell::SmallShell() {
// TODO: add your implementation
    allowedCommands = {"pwd"};

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
  sleep(7);
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


void SmallShell::updateFinishedJobs(){
    for (const int& value : this->readChannels) {
        int jobId = 0;
        read(value, &jobId, sizeof(jobId));
        if (jobId != 0){
            this->jobsMap.erase(jobId);
            std::cout <<"removed job:" << jobId << std::endl;
            close(value);
        }
    }
}


void SmallShell::_jobs(){
    this->updateFinishedJobs();
    for (auto it = this->jobsMap.begin(); it != this->jobsMap.end(); ++it) {
        std::cout <<"[" << it->first << "] " << it->second << std::endl;
    }
}


void SmallShell::executeCommand(const char *cmd_line) {
    this->updateFinishedJobs();
    std::cout << cmd_line << std::endl;
    std::string cmd_line_str = cmd_line;
    this->removeLastCharIfAmpersand(cmd_line_str);
    std::vector<std::string> parsed_cmd = this->splitStringBySpace(cmd_line_str); // should parse the & from the last arg
    if (!parsed_cmd.empty()) {
        std::string command = parsed_cmd.front();
        std::vector<std::string> args(parsed_cmd.begin() + 1, parsed_cmd.end());
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
                this->readChannels.insert(pipefd[READ]);
                this->jobsMap[jobId] = cmd_line;
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
            if (BACKGROUND_FLAG) {
                std::string jobIdStr = std::to_string(jobId);
                std::cout << jobIdStr.c_str() << std::endl;
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