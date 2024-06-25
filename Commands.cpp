#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <cstring>
#include <sys/wait.h>
#include <iomanip>
#include <regex>
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
    specialCommands = {};

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

void SmallShell::_pwd() {
    const size_t bufferSize = 1024;
    char buffer[bufferSize];

    if (getcwd(buffer, sizeof(buffer)) != NULL) {
        std::cout << buffer << std::endl;
    }
    else {
        perror("smash error: getcwd failed");
    }
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
        std::cout << "sending SIGKILL signal to "<< this->jobsMap.size() <<  " jobs:" << std::endl;
        for (auto it = this->jobsMap.begin(); it != this->jobsMap.end(); ++it) {
            int pid = it->second->get_pid();
            kill(pid, 9);
            std::cout << pid << ": " << it->second->get_command() << std::endl;
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
        return;
    }

    if (this->jobsMap.count(jobId) == 0) {
        // error
    }

    kill(this->jobsMap[jobId]->get_pid(), sigNum);

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
            std::regex pattern("^[a-zA-Z0-9_]*$");
            if (this->aliasMap.find(alias) != this->aliasMap.end() ||
                this->internalCommands.count(alias) > 0 || this->specialCommands.count(alias) > 0) {
                // eror that this alias exists
                std::cerr << "smash error: alias: " << alias << " already exists or is a reserved command" << std::endl;
            }
            else if (!std::regex_match(alias, pattern)) {
                std::cerr << "smash error: alias: invalid alias format" << std::endl;
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

bool SmallShell::isComplexCommand(std::string &command) {
    return command.find('*') != std::string::npos || command.find('?') != std::string::npos;
}

void SmallShell::runSimpleExternal(std::string &command) {

    std::vector<string> args = this->splitStringBySpace(command);
    std::vector<char*> argsChar;
    for (const std::string & arg : args) {
        const char* cstr = arg.c_str();
        size_t len = std::strlen(cstr);
        char* charPtr = new char[len + 1];
        std::strcpy(charPtr, cstr);
        argsChar.push_back(charPtr);

    }
    const char* program = argsChar[0];

    if (execvp(program, argsChar.data()) == -1) {
        perror("smash error: execvp failed");
    }
}

void SmallShell::runComplexCommand(std::string &command) {
    char* commandChar = strdup(command.c_str());
    char *bash = (char *)"/bin/bash";
    char *bashFlag = (char *)"-c";
    char *argv[] = {bash, bashFlag, commandChar, nullptr};
    if (execvp(bash, argv) == -1) {
        perror("smash error: execvp failed");
    }
}


int SmallShell::replace_stdout_with_file(int fd) {
    int saved_stdout = dup(1); // Save current stdout
    // Replace stdout with your file descriptor
    dup2(fd, 1);
    return saved_stdout;
}

std::pair<int, int> SmallShell::handle_redirection(std::vector<std::string> &args, bool redirection_flag, bool double_redirection_flag) {
    int append_flag = O_TRUNC;
    if (double_redirection_flag) {
        append_flag = O_APPEND;
    }
    std::string& path = args[args.size() - 1];
    int file_fd = open(path.c_str(), O_WRONLY | O_CREAT | append_flag, 0644);
    args.pop_back();
    args.pop_back();
    int stdOut = this->replace_stdout_with_file(file_fd);
    return std::make_pair(file_fd, stdOut);
}



void SmallShell::executeCommand(const char *cmd_line) {
    this->updateFinishedJobs();
    std::string cmd_line_str = cmd_line;
    this->removeLastCharIfAmpersand(cmd_line_str);
    std::vector<std::string> parsed_cmd = this->splitStringBySpace(cmd_line_str); // should parse the & from the last arg
    if (!parsed_cmd.empty()) {
        std::string command = parsed_cmd.front();
        std::vector<std::string> args(parsed_cmd.begin() + 1, parsed_cmd.end());
        bool REDIRECTION_FLAG = std::find(args.begin(), args.end(), "<") != args.end();
        bool DOUBLE_REDIRECTION_FLAG = std::find(args.begin(), args.end(), "<<") != args.end();
        int stdOut = 0;
        int file_fd = 0;
        if (DOUBLE_REDIRECTION_FLAG || REDIRECTION_FLAG) {
            auto res = this->handle_redirection(args, REDIRECTION_FLAG, DOUBLE_REDIRECTION_FLAG);
            file_fd = res.first;
            stdOut = res.second;

        }


        if (command == "pwd") {
            this->_pwd();
        }
        else if (command == "jobs") {
            this->_jobs();
        }

        else if (command == "quit") {
            this->_quit(args);
        }
        else if (command == "kill") {
            this->_kill(args);
        }
        else if (command == "alias") {
            this->_alias(args);
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
        bool BACKGROUND_FLAG = (_isBackgroundComamnd(cmd_line)) &&
            (this->internalCommands.count(command) == 0) && (this->specialCommands.count(command) == 0);
        pid_t pid = fork();
        if (pid < 0) {
            perror("smash error: fork failed");
        }
        const int jobId = this->getMaxJobId() + 1;
        if (pid != 0) { // if father
            if (BACKGROUND_FLAG){
                Job* job = new Job(pid, jobId, cmd_line_str);
                this->jobsMap[jobId] = job;
            }
            else {
                int status;
                waitpid(pid, &status, 0);
            }
            if (REDIRECTION_FLAG || DOUBLE_REDIRECTION_FLAG) {
                dup2(stdOut, 1);
                close(file_fd);
            }
        }
        else {
            auto it = this->internalCommands.find(command);
            if (it == this->internalCommands.end()) {
                if (this->isComplexCommand(cmd_line_str)) {
                    this->runComplexCommand(cmd_line_str);
                }
                else {
                    this->runSimpleExternal(cmd_line_str);

                }
            }
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
    // Regular expression to split by any whitespace characters
    std::regex ws_re("[\\s]+"); // \s matches any whitespace character

    // Splitting the string using regex_token_iterator
    std::vector<std::string> results(
        std::sregex_token_iterator(str.begin(), str.end(), ws_re, -1),
        std::sregex_token_iterator()
    );
    return results;
}
