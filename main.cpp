#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <stdlib.h>
#include <sys/unistd.h>
#include <stdio.h>
#include <cerrno>
#include <string.h>
#include <sys/wait.h>
#include <pwd.h>


using namespace std;


//global variables
string redirectionString = "none";
int pipeNumber;


//Process
struct Process {
    vector<string> arguments;

    void toString();
};

void Process::toString() {
    for (unsigned int j = 0; j < this->arguments.size(); j++) {
        cout << j << ": " << this->arguments.at(j) << "\n";
    }
}


//Job
struct Job {
    Job(string, vector<Process>);

    void toString();

    int pipeNum;
    string command;
    vector<Process> processList;

    string standardEr;
    string standardIn;
    string standardOut;
};

void Job::toString() {
    unsigned int listSize = (unsigned int) processList.size();
    cout << "Job STDIN = " << standardIn << '\n';
    cout << "Job STDOUT = " << standardOut << '\n';
    cout << "Job STDERR = " << standardEr << '\n' << '\n';

    cout << pipeNum << " pipe(s) \n";
    cout << listSize << " process(es) \n\n";

    for (unsigned int i = 0; i < listSize; i++) {
        cout << "Process " << i << " argv:\n";
        Process process = processList.at(i);
        process.toString();
        cout << '\n';
    }
}

Job::Job(string com, vector<Process> processes) {

    standardEr = "STDERR_FILENO";
    standardIn = "STDIN_FILENO";
    standardOut = "STDOUT_FILENO";
    this->command = com;
    this->pipeNum = pipeNumber;
    this->processList = processes;

    if (redirectionString.compare("none") != 0) {
        stringstream redirectionStream(redirectionString);
        for (unsigned int i = 0; i < redirectionString.length(); i++) {
            char ch = redirectionString.at(i);

            if (ch == '<') {
                redirectionStream.ignore(2, '<');
                redirectionStream.ignore(256, ' ');
                redirectionStream >> this->standardIn;
            } else if (ch == '>') {
                if (i == 0) {
                    if (i != (redirectionString.length() - 1)) {
                        if (redirectionString.at(i + 1) == '>') {
                            redirectionStream.ignore(3);
                            redirectionStream >> this->standardOut;
                            this->standardOut += " (append)";
                            i++;
                        } else {
                            redirectionStream.ignore(256, ' ');
                            redirectionStream >> this->standardOut;
                            this->standardOut += " (truncate)";
                        }
                    } else {
                        redirectionStream.ignore(256, ' ');
                        redirectionStream >> this->standardOut;
                        this->standardOut += " (truncate)";
                    }
                } else {
                    if (redirectionString.at(i - 1) == 'e') {
                        if (i != (redirectionString.length() - 1)) {
                            if (redirectionString.at(i + 1) == '>') {
                                redirectionStream.ignore(4);
                                redirectionStream >> this->standardEr;
                                this->standardEr += " (append)";
                                i++;
                            } else {
                                redirectionStream.ignore(256, ' ');
                                redirectionStream >> this->standardEr;
                                this->standardEr += " (truncate)";
                            }
                        } else {
                            redirectionStream.ignore(256, ' ');
                            redirectionStream >> this->standardEr;
                            this->standardEr += " (truncate)";
                        }
                    } else {
                        if (i != (redirectionString.length() - 1)) {
                            if (redirectionString.at(i + 1) == '>') {
                                redirectionStream.ignore(3);
                                redirectionStream >> this->standardOut;
                                this->standardOut += " (append)";
                                i++;
                            } else {
                                redirectionStream.ignore(256, ' ');
                                redirectionStream >> this->standardOut;
                                this->standardOut += " (truncate)";
                            }
                        } else {
                            redirectionStream.ignore(256, ' ');
                            redirectionStream >> this->standardOut;
                            this->standardOut += " (truncate)";
                        }
                    }
                }
            }
        }
    }
}


//method predeclarations
vector<string> divideByPipes(string);

vector<Process> makeProcesses(vector<string>);

void runJob(Job);

string trim(string);

void exitProgram(string);

char *const *devolveArgList(vector<string>);

string prompt();

void help();

int cd(const char *pathname);

void signal();

void deSignal();

void singleProcess(Job);

void multiProcess(Job);


int main() {
    cout << "entering program\n";
    pipeNumber = 0;
    int loopFlag = 0;
    while (loopFlag < 100) {
        signal();

        cout << prompt();
        string command;
        getline(cin , command);

	if(command.compare("exit")==0){
	    break;
	}

        vector<string> argsList = divideByPipes(command);
        vector<Process> processes = makeProcesses(argsList);
        Job job(command, processes);

        runJob(job);
	pipeNumber=0;
	loopFlag++;
    }
}


//utility methods
string prompt() {
    cout << "entering prompt Method\n";
    string prompt = "1730sh:";

    struct passwd *pw = getpwuid(getuid());
    string home = pw->pw_dir;
    cout << "home Directory found to be " << home << '\n';

    string current = get_current_dir_name();
    cout << "Found current to be " << current << '\n' << '\n';

    if (current.substr(0, home.length()).compare(home) == 0) {
        string temp = current;
        current = "~";
        current += temp.substr(home.length(), temp.length() - home.length());
    }
    current += "$ ";
    return current;
}

void signal(){
    signal(SIGINT, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
}

void deSignal(){
    signal(SIGINT, SIG_DFL);
    signal(SIGQUIT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    signal(SIGTTIN, SIG_DFL);
    signal(SIGTTOU, SIG_DFL);
    signal(SIGCHLD, SIG_DFL);
}

void runJob(Job job) {
    cout << "entering runJob with job:\n";
    job.toString();
    cout << '\n';
    
    if (job.processList.size() == 1) {
        singleProcess(job);
    } else {
        multiProcess(job);
    }
}

void singleProcess(Job job){
    cout << "entering single proccess logic\n";

    int pId;
    int status;
    Process process = job.processList.at(0);
    if ((pId = fork()) == -1) {
        exitProgram(strerror(errno));
    }
    if (pId == 0) {
        deSignal();
        cout << "Child says hi :)\n";
        const char *executable = process.arguments.at(0).c_str();
        char *const *arguments = devolveArgList(process.arguments);
        cout << "child execing program " << executable << '\n';
        if (execvp(executable, arguments) == -1) {
            cout << "total failure q" << strerror(errno) << '\n';
            exitProgram(strerror(errno));
        }
    } else {
        cout << "parent says hi \n";
        cout<< waitpid(pId, &status, 0)<<"\n";
        cout << "child process returned\n";
    }
}

void multiProcess(Job job){
    cout << "entered multi process logic\n";
    int pId;
    int **pipeFds = new int *[job.pipeNum];
    int pipeCount = 0;

    for (int i = 0; i < job.pipeNum; i++) {

        int pipeFd[2];
        if (pipe(pipeFd) == -1) {
            exitProgram(strerror(errno));
        }
        cout << "made a pipe\n";
        pipeFds[i] = pipeFd;
    }

    int *pipeFd;
    // int *prevPipeFd;

    for (unsigned int i = 0; i < job.processList.size(); i++) {
        int status;
        Process process = job.processList.at(i);

        if (i == 0 && job.pipeNum != 0) {
            pipeFd = pipeFds[pipeCount];
        }

        if (i == 0) {
            cout << "entering logic for first process\n";
            if ((pId = fork()) == -1) {
                exitProgram(strerror(errno));
            }
            if (pId == 0) {
                deSignal();
                cout << "child process says hi\n";
                const char *executable = process.arguments.at(0).c_str();
                char *const *arguments = devolveArgList(process.arguments);

                if (dup2(pipeFd[1], STDOUT_FILENO) == -1) {
                    cerr << "failure cused by " << strerror(errno) << '\n';
                    exitProgram(strerror(errno));
                }

                if (execvp(executable, arguments) == -1) {
                    cerr << "failure caused by " << strerror(errno) << '\n';
                    exitProgram(strerror(errno));
                }
            } else {
                cout << "parent process says hi\n";
                waitpid(pId, &status, 0);
                close(pipeFd[1]);
                cout << "child process returned\n";
                cout << job.processList.size() << '\n';
            }
        } else if (i == (job.processList.size() - 1)) {
            cout << "entering logic for last process\n";
            if ((pId = fork()) == -1) {
                exitProgram(strerror(errno));
            }
            if (pId == 0) {
                deSignal();
                cout << "child process says hi\n";
                const char *executable = process.arguments.at(0).c_str();
                char *const *arguments = devolveArgList(process.arguments);

                if (dup2(pipeFd[0], STDIN_FILENO) == -1) {
                    cerr << "failure cuased by " << strerror(errno) << "\n";
                    exitProgram(strerror(errno));
                }

                if (execvp(executable, arguments) == -1) {
                    cerr << "failure cuased by " << strerror(errno) << "\n";
                }
            } else {
                cout << "parent process says hi\n";
                waitpid(pId, &status, 0);
                close(pipeFd[0]);
                cout << "child process returned\n";
            }
        } else {
            cout << "entering logic for middle processes\n";
	    int nextPipe[2];
	    pipe(nextPipe);
            if ((pId = fork()) == -1) {
                exitProgram(strerror(errno));
            }
            if (pId == 0) {
                deSignal();
                cout << "child proccess says hi\n";
                const char *executable = process.arguments.at(0).c_str();
                char *const *arguments = devolveArgList(process.arguments);

                if (dup2(pipeFd[0], STDIN_FILENO) == -1) {
                    cout << "failure cuased by 2" << strerror(errno) << '\n';
                    exitProgram(strerror(errno));
                }
                pipeFd= nextPipe;
		cout<< "pipeFD = " << pipeFd[1]<<'\n';
		cout<<"stdout = "<<STDOUT_FILENO<<'\n';
		cout<<"*******\n";
                if (dup2(pipeFd[1], STDOUT_FILENO) == -1) {
                    cout << "failure cuased by 1" << strerror(errno) << '\n';
                    exitProgram(strerror(errno));
                }
		cerr<<"stdout = ********"<<STDOUT_FILENO<<'\n';
                if (execvp(executable, arguments) == -1) {
                    cerr << "failure cuased by " << strerror(errno) << "\n";
                }
            } else {
                cout << "parent proccess says hi\n";
		pipeFd=nextPipe;
                waitpid(pId, &status, 0);
                close(pipeFd[1]);
                cout << "child process returned\n";
            }
        }
    }
}

vector<Process> makeProcesses(vector<string> argsList) {
    vector<Process> processes;

    for (unsigned int i = 0; i < argsList.size(); i++) {
        string arg = argsList.at(i);
        Process p;
        int prev = 0;

        for (unsigned int j = 0; j < arg.length(); j++) {
            char ch = arg.at(j);
            if (j == (arg.length() - 1)) {
                p.arguments.push_back(arg.substr(prev, (arg.length() - prev)));
            }
            if (ch == ' ' || ch == '\t') {
                if (arg.at(j - 1) != ' ' && arg.at(j - 1) != '\t') {
                    p.arguments.push_back(arg.substr(prev, j - prev));
                }
                prev = j + 1;
            }
            if (ch == '"' && arg.at(j - 1) != '\\') {
                int count = 0;
                for (unsigned int k = (j + 1); k < arg.length(); k++, count++) {
                    char ch2 = arg.at(k);
                    if (ch2 == '"' && arg.at(k - 1) != '\\') {
                        p.arguments.push_back(arg.substr(j + 1, count));
                        prev = k;
                        j = k;
                        break;
                    }
                }
            }

        }
        processes.push_back(p);
    }

    return processes;
}

vector<string> divideByPipes(string command) {

    unsigned long prev = 0;
    vector<string> arguments;

    for (unsigned long i = 0; i < command.length(); i++) {

        char ch = command.at(i);

        if (ch == '|') {
            arguments.push_back(command.substr(prev, (i - prev)));
            // cout<<"found process "<<command.substr(prev,(i-prev));
            prev = i + 1;
            pipeNumber++;
        }
        if (i == (command.length() - 1)) {
            arguments.push_back(command.substr(prev, (i - prev) + 1));
            // cout<<"found process "<<command.substr(prev,(i-prev));

        }
        if ((ch == '<') | (ch == '>')) {
            if (command.at(i - 1) == 'e') {
                i -= 1;
            }
            arguments.push_back(command.substr(prev, (i - prev)));
            redirectionString = trim(command.substr(i, command.length() - i));
            break;
        }
    }
    for (unsigned int i = 0; i < arguments.size(); i++) {
        arguments.at(i) = trim(arguments.at(i));
    }
    return arguments;
}

char *const *devolveArgList(vector<string> list) {
    cout << "entering devolve arg method\n";
    char **baseArray = new char *[list.size()];

    for (unsigned int j = 0; j < list.size(); j++) {
        cout << "arg size = " << list.at(j).size() << '\n';
        char *str = new char[list.at(j).size() + 1];
        for (unsigned int i = 0; i < list.at(j).size(); i++) {
            cout << "found char " << list.at(j).at(i) << " at index " << i << '\n';
            str[i] = list.at(j).at(i);
        }
        str[list.at(j).size()] = 0;
        cout << "constructed string " << str << '\n';
        baseArray[j] = str;
    }

    char *const *thing = baseArray;

    cout << "leaving devolve arg method\n";
    return thing;
}

string trim(string str) {
    size_t end = str.find_last_not_of(" \t");
    if (string::npos != end) {
        str = str.substr(0, end + 1);
    }
    size_t start = str.find_first_not_of(" \t");
    if (string::npos != start) {
        str = str.substr(start);
    }
    return str;
}

void exitProgram(string message) {
    perror(message.c_str());
    exit(EXIT_FAILURE);
}

void help() {
    cout << "GNU bash, version 4.1.2(1)-release (x86_64-redhat-linux-gnu)" << endl;
    cout << "These shell commands are defined internally.  Type `help' to see this list." << endl << endl;
    cout << "A star (*) next to a name means that the command is disabled." << endl;
    cout << "cd [dir]" << endl;
    cout << "exit [n]" << endl;
    cout << "help " << endl;

}

int cd(const char *pathname) {
    int x = chdir(pathname);
    return x;
}
