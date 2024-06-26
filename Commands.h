#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <string>    // for std::string
#include <map>
#include <set>

#define COMMAND_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define MAX_PWD_PATH 1001
#define CONFIG "config1.txt"

int close_wrapper(int fd);
int open_wrapper(const char* __file, int __oflag);
int read_wrapper(int file_fd, void* buffer, size_t nbytes);
int extract_first_int(const char* text);
int read_from_config();

class Job {
private:
    int pid;
    int jobId;
    std::string command;
public:
    Job(int _pid, int _jobId, std::string& _command);
    int get_pid();
    int get_job_id();
    std::string get_command();
};

class Command {
public:
    std::string _real_command;
    std::string _parsed_command;
    Command(std::string& real_command, std::string& parsed_command);
};

class SmallShell {
private:
    // TODO: Add your data members
    SmallShell();
    std::set<std::string> internalCommands;
    std::set<std::string> specialCommands;
    std::set<int> readChannels;
    // std::map<int, std::string> jobsMap;
    std::map<int, Job*> jobsMap;
    std::vector<int> insertionOrderJobs;
    std::map<std::string, Command*> aliasMap;
    std::vector<std::string> insertionOrderAlias;
    std::string shellPromptLine;
    pid_t smashPid;
    char* oldPwd;
    bool need_to_fork;


    void clean_args(std::vector<std::string>& args);
    std::vector<std::string> splitString(const std::string& str);
    void removeLastCharIfAmpersand(std::string& str);
    void _chprompt(std::vector<std::string>& args);
    void _showpid(std::vector<std::string>& args);
    void _pwd();
    void _cd(std::vector<std::string>& args);
    void _jobs();
    void _fg(std::vector<std::string>& args);
    void _quit(std::vector<std::string>& arfgs);
    void _kill(std::vector<std::string>& args);
    void _alias(std::vector<std::string>& args, std::string& real_command, std::string& prefix);
    void _unalias(std::vector<std::string>& args);
    void _getuser(std::vector<std::string>& args);
    void _listdir(std::vector<std::string>& args);


    bool isComplexCommand(std::string& command);
    void runSimpleExternal(std::string& command);
    void runComplexCommand(std::string& command);
    void bringJobForeground(int jobId, bool isArgs);

    int replace_stdout_with_file(int fd);
    std::pair<int, int> handle_redirection(std::vector<std::string>& args, bool redirection_flag, bool double_redirection_flag);


    std::vector<std::string> splitStringBySpace(const std::string& str);
    void updateFinishedJobs();
    int getMaxJobId();
    void changePrevious();

public:
    Command *CreateCommand(const char *cmd_line);

    SmallShell(SmallShell const &) = delete; // disable copy ctor
    void operator=(SmallShell const &) = delete; // disable = operator
    static SmallShell &getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }

    ~SmallShell();

    void executeCommand(const char *cmd_line);
    std::string getPromptLine() { return this->shellPromptLine; }
    void setPromptLine(std::string line) { this->shellPromptLine = std::move(line);}
    pid_t getPid() { return this->smashPid; }
    char* getOldPwd() { return this->oldPwd; }
    void setOldPwd(char* old_pwd) { this->oldPwd = old_pwd; }
    // TODO: add extra methods as needed
};

#endif //SMASH_COMMAND_H_
