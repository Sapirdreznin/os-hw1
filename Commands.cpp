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
#include <pwd.h>
#include <grp.h>
#include "Commands.h"
#include <cmath>
#include <sys/stat.h>
#include <sys/syscall.h>

using namespace std;

#define DIGIT_ASCII_DIFF 48
#define DIGIT_MULTIPLE 10
#define BUF_SIZE 1024
#define PATH_MAX 4096

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

const std::string WHITESPACE = " \n\r\t\f\v";
const int READ = 0;
const int WRITE = 1;

struct linux_dirent {
    unsigned long   d_ino;      /* Inode number */
    unsigned long   d_off;      /* Offset to next dirent */
    unsigned short  d_reclen;   /* Length of this dirent */
    char            d_name[];   /* Filename (null-terminated) */
};

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

// returns the native number from a given string. If the format is invalid, returns -1
int _getNative(const char* str) {
    int num = 0;
    int len = std::string(str).length();
    for (int i = len - 1; i >= 0; --i) {
        if (str[i] < '0' || str[i] > '9') {
            return -1;
        } else {
            num += pow(DIGIT_MULTIPLE, len - i -1) * (int)(str[i] - DIGIT_ASCII_DIFF);
        }
    }
    return num;
}

bool is_digits_only(const std::string& s) {
    return std::all_of(s.begin(), s.end(), ::isdigit);
}

Job::Job(int _pid, int _jobId, std::string& _command){
    this->pid = _pid;
    this->jobId = _jobId;
    this->command = _command;
}

int close_wrapper(int fd) {
    int close_res = close(fd);
    if (close_res == -1) {
        perror("smash error: close failed");
    }
    return close_res;
}

int open_wrapper(const char* __file, int __oflag) {
    int file_fd = open(__file, __oflag, 0644);
    if (file_fd == -1) {
        perror("smash error: open failed");
    }
    return file_fd;
}

int read_wrapper(int file_fd, void* buffer, size_t nbytes) {
    ssize_t bytesRead = read(file_fd, buffer, nbytes);
    if (bytesRead == -1) {
        perror("smash error: read failed");
        close_wrapper(file_fd);
    }
    return bytesRead;
}

int read_from_config() {
    int fd = open_wrapper(CONFIG, O_RDONLY);
    char buffer[128];
    ssize_t bytesRead = read_wrapper(fd, buffer, sizeof(buffer) - 1);
    buffer[bytesRead] = '\0';
    close_wrapper(fd);

    return extract_first_int(buffer);
}

void update_config(int pid) {
    int fd = open_wrapper(CONFIG,O_WRONLY | O_CREAT);

    std::string pidString = "Process ID: " + std::to_string(pid) + "\n";

    ssize_t bytesWritten = write(fd, pidString.c_str(), pidString.size());
    if (bytesWritten == -1) {
        perror("write() error");
    }
    close_wrapper(fd);
}

void reset_config() {
    update_config(0);
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

std::string getCurrentDirectory() {
    char buffer[PATH_MAX];
    if (getcwd(buffer, sizeof(buffer)) != nullptr) {
        return std::string(buffer);
    } else {
        perror("getcwd");
        return std::string();
    }
}


std::string getFileType(const std::string& path) {
    struct stat statbuf;
    if (lstat(path.c_str(), &statbuf) != 0) {
        return "unknown";
    }

    if (S_ISDIR(statbuf.st_mode)) {
        return "directory";
    } else if (S_ISREG(statbuf.st_mode)) {
        return "file";
    } else if (S_ISLNK(statbuf.st_mode)) {
        return "link";
    } else {
        return "other";
    }
}

// Comparator to sort files before directories alphabetically
bool compareEntries(const std::pair<std::string, std::string>& a, const std::pair<std::string, std::string>& b)
{
    if (a.second == b.second)
    {
        return a.first < b.first;
    }
    if (a.second == "file" && b.second != "file")
    {
        return true;
    }
    if (a.second != "file" && b.second == "file")
    {
        return false;
    }
    return a.first < b.first;
}

void processDirectory(const std::string& directoryPath, std::vector<std::pair<std::string, std::string>>& entries) {
    int fd = open(directoryPath.c_str(), O_RDONLY | O_DIRECTORY);
    if (fd == -1) {
        perror("open");
        return;
    }

    char buf[BUF_SIZE];
    int nread;

    while ((nread = syscall(SYS_getdents, fd, buf, BUF_SIZE)) > 0) {
        for (int bpos = 0; bpos < nread;) {
            struct linux_dirent* d = (struct linux_dirent*) (buf + bpos);
            std::string name(d->d_name);

            if (name == "." || name == "..") {
                bpos += d->d_reclen;
                continue;
            }
            std::string path;
            if (directoryPath.back() == '/') {
                path = directoryPath + name;
            } else {
                path = directoryPath + "/" + name;
            }
            std::string type = getFileType(path);
            entries.emplace_back(path, type);

            if (type == "directory") {
                processDirectory(path, entries);
            }

            bpos += d->d_reclen;
        }
    }

    if (nread == -1) {
        perror("getdents");
    }

    close(fd);
}



Command::Command(std::string& real_command, std::string& parsed_command) {
    this->_real_command = real_command;
    this->_parsed_command = parsed_command;
}


SmallShell::SmallShell() {
// TODO: add your implementation
    internalCommands = {"chprompt", "showpid", "pwd", "cd", "jobs", "fg", "quit", "kill",
        "alias", "unalias"};
    specialCommands = {"getuser"};
    this->shellPromptLine = std::string("smash");
    this->smashPid = getpid();
    this->oldPwd = nullptr;
    this->need_to_fork = true;
    reset_config();

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
    char buffer[BUF_SIZE];

    if (getcwd(buffer, sizeof(buffer)) != NULL) {
        std::cout << buffer << std::endl;
    }
    else {
        perror("smash error: getcwd failed");
    }
}

void SmallShell::_cd(std::vector<std::string> &args)
{
    SmallShell& smash = SmallShell::getInstance();
    if (args.size() == 0) {
        return;
    }
    if (args.size() > 1) {
        cerr << "smash error: cd: too many arguments" << endl;
    }
    else if (args[0] == "-") {
        this->changePrevious();
    }
    else {
        char *buff = new char[MAX_PWD_PATH];
        if (getcwd(buff, MAX_PWD_PATH) == nullptr) {
            perror("smash error: getcwd failed");
            delete[] buff;
        } else if (chdir(args[0].c_str()) < 0) {
            perror("smash error: chdir failed");
            delete[] buff;
        } else {
            delete[] smash.getOldPwd();
            smash.setOldPwd(buff);
        }
    }
    return;
}

void SmallShell::changePrevious()
{
    SmallShell& smash = SmallShell::getInstance();
    if (!this->oldPwd) {
        cerr << "smash error: cd: OLDPWD not set" << endl;
    }
        /// IF old PWD exists
    else {
        char *buff = new char[MAX_PWD_PATH];
        if (getcwd(buff, MAX_PWD_PATH) == nullptr) {
            cerr << "smash error: getcwd failed" << endl;
            delete[] buff;

        } else if (chdir(this->oldPwd) < 0) {
            perror("smash error: chdir failed");

        } else {
            delete[] smash.getOldPwd();
            smash.setOldPwd(buff);
        }

    }
}

void SmallShell::bringJobForeground(int jobId) {

    this->updateFinishedJobs();

    if (this->jobsMap.empty()) {
        cerr << "smash error: fg: jobs list is empty" << endl;
        return;
    } else if (jobId == 0)
    {   
        // take maximum job Id.
        jobId = this->jobsMap.rbegin()->first;
    }

    Job* job = this->jobsMap[jobId];
    if (job == nullptr)
    {
        cerr << "smash error: fg: job-id " << jobId << " does not exist" << endl;
        return;
    }
    std::string cmd = job->get_command();
    cout << cmd << "& " << job->get_pid() << endl;

    if (kill(job->get_pid(), SIGCONT) < 0) {
        perror("smash error: kill failed");
    }
    else {
        waitpid(job->get_pid(), nullptr, WUNTRACED);
        this->updateFinishedJobs();
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

void SmallShell::updateFinishedJobs()
{
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

void SmallShell::_fg(std::vector<std::string> &args)
{
    int jobId = 0;

    if (args.size() > 1)
    {
        cerr << "smash error: fg: invalid arguments" << endl;
        return;
    }

    if ((args.size() == 1) && (!is_digits_only(args[0])))
    {
        cerr << "smash error: fg: invalid arguments" << endl;
        return;
    }
    if (args.size() == 1) 
    {
        jobId = _getNative(args[0].c_str());
        if (jobId == -1) {
            cerr << "smash error: fg: invalid arguments" << endl;
            return;
        }
    }
    this->bringJobForeground(jobId);
}

void SmallShell::_quit(std::vector<std::string>& args) {
    if (!args.empty() && args[0] == "kill") {
        std::cout << "smash: sending SIGKILL signal to "<< this->jobsMap.size() <<  " jobs:" << std::endl;
        for (auto it = this->jobsMap.begin(); it != this->jobsMap.end(); ++it) {
            int pid = it->second->get_pid();
            kill(pid, 9);
            std::cout << pid << ": " << it->second->get_command() << std::endl;
        }   
    }
    exit(0);
}

void SmallShell::_kill(std::vector<std::string>& args){
    if (args.empty()) {
        std::cerr << "smash error: kill: invalid arguments" << std::endl;
        return;
    }
    if (args[0][0] != '-') {
        std::cerr << "smash error: kill: invalid arguments" << std::endl;
        return;
    }
    if (args.size() < 2) {
        std::cerr << "smash error: kill: invalid arguments" << std::endl;
        return;
    }

    args[0].erase(args[0].begin());  // Erase the first character
    int jobId;
    int sigNum;
    try {
        sigNum = std::stoi(args[0]); // Convert string to int
        jobId = std::stoi(args[1]); // Convert string to int

    } catch (const std::exception& e) {
        std::cerr << "smash error: kill: job-id " << args[1] << " does not exist" << std::endl;
        // error
        return;
    }

    if (this->jobsMap.count(jobId) == 0) {
        // error
        std::cerr << "smash error: kill: job-id " << args[1] << " does not exist" << std::endl;
    }
    else {
        int pid = this->jobsMap[jobId]->get_pid();
        kill(pid, sigNum);
        std::cout << "signal number " << sigNum << " was sent to pid " << pid << std::endl;
    }

}

void SmallShell::_alias(std::vector<std::string> &args, std::string& real_command) {
    if (args.empty()) {
        for (auto & it : this->aliasMap) {
            std::cout << it.second->_real_command << std::endl;
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
                if ((command[0] == '\'') && (args.back().back() == '\'')) {
                    for (unsigned int i=1; i < args.size(); i++) {
                        command += " " + args[i];
                    }
                    command = command.substr(1, command.length() - 2);
                    Command* command_obj = new Command(real_command, command);
                    this->aliasMap[alias] = command_obj;
                }
                else {
                    std::cerr << "smash error: alias: invalid alias format" << std::endl;

                }

            }
        } else {
            std::cerr << "smash error: alias: invalid alias format" << std::endl;

        }
    }
}

void SmallShell::_unalias(std::vector<std::string> &args) {

    if (args.size() == 0) {
        std::cerr << "smash error: unalias: not enough arguments" << std::endl;
    }
    for (const std::string & arg : args) {
        if (this->aliasMap.count(arg) > 0) {
            this->aliasMap.erase(arg);
            // todo: call delete functiom to avoid memory leaks
        }
        else {
            std::cerr << "smash error: unalias: " << arg << " alias does not exist" << std::endl;
            return;
        }
    }
}

void SmallShell::_chprompt(std::vector<std::string>& args)
{
    if (args.size() > 1 || args.size() < 0) {
        // error
    }
    SmallShell& smash = SmallShell::getInstance();
    if (args.size() == 0 ) {
        smash.setPromptLine(std::string("smash"));
    }
    else if (args.size() == 1) {
        smash.setPromptLine(args[0]);
    }
}

void SmallShell::_showpid(std::vector<std::string>& args)
{
    SmallShell& smash = SmallShell::getInstance();
    cout << "smash pid is "<< smash.getPid() << endl;
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
    // for (auto & arg : argsChar) {
    //     std::cout << arg << std::endl;
    // }

    // std::cout << program << std::endl << argsChar.data() << std::endl;
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
    int file_fd = open_wrapper(path.c_str(),O_WRONLY | O_CREAT | append_flag);
    // int file_fd = open(path.c_str(), O_WRONLY | O_CREAT | append_flag);
    // if (file_fd == -1) {
        // perror("smash error: open failed");
    // }
    args.pop_back();
    args.pop_back();
    int stdOut = this->replace_stdout_with_file(file_fd);
    return std::make_pair(file_fd, stdOut);
}
int extract_first_int(const char* text) {
    const char* end_pointer = text;
    while (*end_pointer && (*end_pointer < '0' || *end_pointer > '9')) {
        ++end_pointer;
    }
    int number = 0;
    while (*end_pointer && (*end_pointer >= '0' && *end_pointer <= '9')) {
        number = number * 10 + (*end_pointer - '0');
        ++end_pointer;
    }
    return number;

    
}

int ConvertToInt(const std::string& str) {
    try {
        int res = std::stoi(str);
        return res;
    } catch (const std::invalid_argument& e) {
        // Thrown if the input is not a valid integer
        return -1;
    } catch (const std::out_of_range& e) {
        // Thrown if the input is out of the range of representable values
        return -1;
    }
}

void SmallShell::_getuser(std::vector<std::string>& args) {
    if (args.empty()) {
        std::cerr << "smash error: getuser: process <pid> does not exist" << std::endl;
    }
    else if (args.size() > 1) {
        std::cerr << "smash error: getuser: too many arguments" << std::endl;
    }

    int pid = ConvertToInt(args[0]);
    if (pid < 0) {
        std::cerr << "smash error: getuser: process <pid> does not exist" << std::endl;
    }
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/status", pid);

    // Open the status file
    int fd = open_wrapper(path, O_RDONLY);

    // Read the contents of the file
    char buffer[4096];
    ssize_t bytesRead = read_wrapper(fd, buffer, sizeof(buffer) - 1);
    // Null-terminate the buffer
    buffer[bytesRead] = '\0';

    // Close the file
    close_wrapper(fd);
    // Parse the buffer to extract Uid and Gid
    const char *uidStr = strstr(buffer, "Uid:");
    const char *gidStr = strstr(buffer, "Gid:");
    char* user_name = getpwuid(extract_first_int(uidStr))->pw_name;
    char* group_name = getgrgid(extract_first_int(gidStr))->gr_name;

    std::cout << "User: " << user_name << std::endl;
    std::cout << "Group: " << group_name << std::endl;
}

void SmallShell::_listdir(std::vector<std::string>& args) 
{
    std::string mainPath;
    if (args.size() > 1) 
    {
        std::cerr << "smash error: listdir: too many arguments" << std::endl;
        return;
    }
    if (args.size() == 0)
    {
        mainPath = getCurrentDirectory();
    } else {
        mainPath = args[0];
    }
    std::vector<std::pair<std::string, std::string>> entries;
    processDirectory(mainPath, entries);

    std::sort(entries.begin(), entries.end(), compareEntries);

    for (const auto& entry : entries) {
        if (entry.second == "link")
        {
            char linkTarget[PATH_MAX];
            ssize_t len = readlink(entry.first.c_str(), linkTarget, sizeof(linkTarget)-1);
            linkTarget[len] = '\0';
            std::cout << entry.second << ": " << entry.first << " -> " << std::string(linkTarget) << std::endl;
        }
        else {
            std::cout << entry.second << ": " << entry.first << std::endl;
        }
    }
}


void SmallShell::executeCommand(const char *cmd_line) {
    this->need_to_fork = true;
    this->updateFinishedJobs();
    std::string copy_cmd_line = cmd_line;
    std::string cmd_line_str = _rtrim(_ltrim(cmd_line));
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
            this->need_to_fork = false;
            this->_pwd();
        }
        else if (command == "jobs") {
            this->need_to_fork = false;
            this->_jobs();
        }
        else if (command == "chprompt") {
            this->need_to_fork = false;
            this->_chprompt(args);
        }
        else if (command == "showpid") {
            this->need_to_fork = false;
            this->_showpid(args);
        }
        else if (command == "cd") {
            this->need_to_fork = false;
            this->_cd(args);
        }
        else if (command == "fg") {
            this->_fg(args);

        }
        else if (command == "quit") {
            this->need_to_fork = false;
            this->_quit(args);
        }
        else if (command == "kill") {
            this->need_to_fork = false;
            this->_kill(args);
        }
        else if (command == "alias") {
            this->need_to_fork = false;
            this->_alias(args, cmd_line_str);
        }
        else if (command == "unalias") {
            this->need_to_fork = false;
            this->_unalias(args);
        }
        else if (command == "getuser") {
            this->need_to_fork = false;
            this->_getuser(args);
        }
        else if (command == "listdir") {
            this->need_to_fork = false;
            this->_listdir(args);
        }

        else if (this->aliasMap.find(command) != this->aliasMap.end()) {
            this->need_to_fork = false;
            std::string aliasCommand = this->aliasMap[command]->_parsed_command;
            for (const std::string & arg : args) {
                aliasCommand += " " + arg;
            }
            char* commandChar = new char[aliasCommand.size() + 1];
            strcpy(commandChar, aliasCommand.c_str());
            this->executeCommand(commandChar);
            delete[] commandChar;
            return;

        }
        bool BACKGROUND_FLAG = (_isBackgroundComamnd(cmd_line)) &&
            (this->internalCommands.count(command) == 0) && (this->specialCommands.count(command) == 0);
        const int jobId = this->getMaxJobId() + 1;

        if (!need_to_fork || *cmd_line == '\0') {
            if (REDIRECTION_FLAG || DOUBLE_REDIRECTION_FLAG) {
                dup2(stdOut, 1);
                close_wrapper(file_fd);
            }
            return;
        }
        pid_t pid = fork();
        if (pid < 0) {
            perror("smash error: fork failed");
        }
        if (pid != 0) { // if father
            if (BACKGROUND_FLAG){
                Job* job = new Job(pid, jobId, copy_cmd_line);
                this->jobsMap[jobId] = job;
            }
            else {
                // int fd = open_wrapper(CONFIG,O_WRONLY | O_CREAT);
                //
                // std::string pidString = "Process ID: " + std::to_string(pid) + "\n";
                //
                // ssize_t bytesWritten = write(fd, pidString.c_str(), pidString.size());
                // if (bytesWritten == -1) {
                //     perror("write() error");
                //     close_wrapper(fd);
                //     return;
                // }
                update_config(pid);
                int status;
                waitpid(pid, &status, 0);
                reset_config();


            }
            if (REDIRECTION_FLAG || DOUBLE_REDIRECTION_FLAG) {
                dup2(stdOut, 1);
                close_wrapper(file_fd);
            }
        }
        else {
            setpgrp();
            if ((this->internalCommands.count(command) == 0) && (this->specialCommands.count(command) == 0)) {
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
