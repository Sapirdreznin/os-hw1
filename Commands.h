#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <string>    // for std::string
#include <map>
#include <set>

#define COMMAND_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define MAX_PWD_PATH 1001

class Command {
// TODO: Add your data members
public:
    Command(const char *cmd_line);

    virtual ~Command();

    virtual void execute() = 0;
    //virtual void prepare();
    //virtual void cleanup();
    // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
public:
    BuiltInCommand(const char *cmd_line);

    virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
public:
    ExternalCommand(const char *cmd_line);

    virtual ~ExternalCommand() {}

    void execute() override;
};

class PipeCommand : public Command {
    // TODO: Add your data members
public:
    PipeCommand(const char *cmd_line);

    virtual ~PipeCommand() {}

    void execute() override;
};

class WatchCommand : public Command {
    // TODO: Add your data members
public:
    WatchCommand(const char *cmd_line);

    virtual ~WatchCommand() {}

    void execute() override;
};

class RedirectionCommand : public Command {
    // TODO: Add your data members
public:
    explicit RedirectionCommand(const char *cmd_line);

    virtual ~RedirectionCommand() {}

    void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
// TODO: Add your data members public:
    ChangeDirCommand(const char *cmd_line, char **plastPwd);

    virtual ~ChangeDirCommand() {}

    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    GetCurrDirCommand(const char *cmd_line);

    virtual ~GetCurrDirCommand() {}

    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand(const char *cmd_line);

    virtual ~ShowPidCommand() {}

    void execute() override;
};

class JobsList;

class QuitCommand : public BuiltInCommand {
// TODO: Add your data members public:
    QuitCommand(const char *cmd_line, JobsList *jobs);

    virtual ~QuitCommand() {}

    void execute() override;
};


class JobsList {
public:
    class JobEntry {
        // TODO: Add your data members
    };
    // TODO: Add your data members
public:
    JobsList();

    ~JobsList();

    void addJob(Command *cmd, bool isStopped = false);

    void printJobsList();

    void killAllJobs();

    void removeFinishedJobs();

    JobEntry *getJobById(int jobId);

    void removeJobById(int jobId);

    JobEntry *getLastJob(int *lastJobId);

    JobEntry *getLastStoppedJob(int *jobId);
    // TODO: Add extra methods or modify exisitng ones as needed
};

class JobsCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    JobsCommand(const char *cmd_line, JobsList *jobs);

    virtual ~JobsCommand() {}

    void execute() override;
};

class KillCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    KillCommand(const char *cmd_line, JobsList *jobs);

    virtual ~KillCommand() {}

    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    ForegroundCommand(const char *cmd_line, JobsList *jobs);

    virtual ~ForegroundCommand() {}

    void execute() override;
};

class ListDirCommand : public BuiltInCommand {
public:
    ListDirCommand(const char *cmd_line);

    virtual ~ListDirCommand() {}

    void execute() override;
};

class GetUserCommand : public BuiltInCommand {
public:
    GetUserCommand(const char *cmd_line);

    virtual ~GetUserCommand() {}

    void execute() override;
};

class aliasCommand : public BuiltInCommand {
public:
    aliasCommand(const char *cmd_line);

    virtual ~aliasCommand() {}

    void execute() override;
};

class unaliasCommand : public BuiltInCommand {
public:
    unaliasCommand(const char *cmd_line);

    virtual ~unaliasCommand() {}

    void execute() override;
};

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

class SmallShell {
private:
    // TODO: Add your data members
    SmallShell();
    std::set<std::string> internalCommands;
    std::set<std::string> specialCommands;
    std::set<int> readChannels;
    // std::map<int, std::string> jobsMap;
    std::map<int, Job*> jobsMap;
    std::map<std::string, std::string> aliasMap;
    std::string shellPromptLine;
    pid_t smashPid;
    char* oldPwd;


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
    void _alias(std::vector<std::string>& args);
    void _unalias(std::vector<std::string>& args);
    void _getuser(std::vector<std::string>& args);


    bool isComplexCommand(std::string& command);
    void runSimpleExternal(std::string& command);
    void runComplexCommand(std::string& command);

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
