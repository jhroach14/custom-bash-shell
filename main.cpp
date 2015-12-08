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
#include <fcntl.h>
#include <sys/stat.h>

using namespace std;
struct Job;

//global variables
vector<Job> allJobs;
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

    void reset();
    
    bool isRunning;
    bool fg;
    int jid;
    int pipeNum;
    string command;
    vector<Process> processList;

    string standardEr;
    string standardIn;
    string standardOut;

    int errFlag;//value of zero denotes append value of 1 denotes truncate
    int outFlag;
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
  for(unsigned int i=0;i<command.length();i++){
    if(command[i] == '&'){
      cout<<"found the ampersand"<<endl;
      this->fg = false;
    }
  }

    errFlag=0;
    outFlag=0;
    standardEr ="STDERR_FILENO";
    standardIn = "STDIN_FILENO";
    standardOut = "STDOUT_FILENO";
    this->command = com;
    this->pipeNum = pipeNumber;
    this->processList = processes;
    this->fg = true;
    

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
                            //this->standardOut += " (append)";
                            i++;
                        } else {
                            redirectionStream.ignore(256, ' ');
                            redirectionStream >> this->standardOut;
			    outFlag=1;
                            //this->standardOut += " (truncate)";
                        }
                    } else {
                        redirectionStream.ignore(256, ' ');
                        redirectionStream >> this->standardOut;
			outFlag=1;
                        //this->standardOut += " (truncate)";
                    }
                } else {
                    if (redirectionString.at(i - 1) == 'e') {
                        if (i != (redirectionString.length() - 1)) {
                            if (redirectionString.at(i + 1) == '>') {
                                redirectionStream.ignore(4);
                                redirectionStream >> this->standardEr;
				//this->standardEr += " (append)";
                                i++;
                            } else {
                                redirectionStream.ignore(256, ' ');
                                redirectionStream >> this->standardEr;
                                errFlag=1;
				//this->standardEr += " (truncate)";
                            }
                        } else {
                            redirectionStream.ignore(256, ' ');
                            redirectionStream >> this->standardEr;
                            errFlag=1;
			    //this->standardEr += " (truncate)";
                        }
                    } else {
                        if (i != (redirectionString.length() - 1)) {
                            if (redirectionString.at(i + 1) == '>') {
                                redirectionStream.ignore(3);
                                redirectionStream >> this->standardOut;
                                //this->standardOut += " (append)";
                                i++;
                            } else {
				redirectionStream.ignore(1,'>');
                                redirectionStream.ignore(256, ' ');
                                redirectionStream >> this->standardOut;
                                outFlag=1;
				//this->standardOut += " (truncate)";
                            }
                        } else {
                            redirectionStream.ignore(256, ' ');
                            redirectionStream >> this->standardOut;
                            outFlag=1;
			    //this->standardOut += " (truncate)";
                        }
                    }
                }
            }
        }
    }
}

void Job::reset(){
    this->pipeNum=0;
    this->jid=0;
    this->command="";
    this->processList.clear();
    this->standardOut="STDOUT_FILENO";
    this->standardEr="STDERR_FILENO";
    this->standardIn="STDIN_FILENO";
    this->errFlag=0;
    this->outFlag=0;      
}


//method predeclarations
vector<string> divideByPipes(string);

vector<Process> makeProcesses(vector<string>);

void runJob(Job);

int checkForBuiltins(vector<Process>);

string trim(string);

void exitProgram(string);

char *const *devolveArgList(vector<string>);

string prompt();

void help();

void runExit();

void runExit(int n);

void bg(vector<string>);

void fg(vector<string>);

void runExport(vector<string>);

void runJobs();

int cd(const char *pathname);

int cd();

void signal();

void deSignal();

void singleProcess(Job);

void multiProcess(Job);

void outputReDirect(Job);

void errorReDirect(Job);

void inputReDirect(Job);

int main() {
    cout << "entering program\n";
    pipeNumber = 0;
    int loopFlag = 0;
    while (loopFlag < 100) {
        signal();

        cout << prompt();
        string command;
        getline(cin , command);
	if(command.length()==0)continue;
	bool fg = true;
       
	for(unsigned int i=0;i<command.length();i++){
	  if(command[i] == '&'){
	    fg = false;
	  }
	 
	}
        vector<string> argsList = divideByPipes(command);
	
        vector<Process> processes = makeProcesses(argsList);
	int cfb = checkForBuiltins(processes);
        if(cfb==0){
          continue;
        }
	
        Job job(command,processes);
	job.fg = fg;
        if(job.fg){
	  cout<<"job will be in foreground"<<endl;
	}  
	else{
	  cout<<"job will be in background"<<endl;
	}
        runJob(job);
	
	pipeNumber=0;
	loopFlag++;
	job.reset();
	redirectionString = "none";
    }
}


//utility methods
string prompt() {
  //cout << "entering prompt Method\n";
    string prompt = "1730sh:";

    struct passwd *pw = getpwuid(getuid());
    string home = pw->pw_dir;
    //cout << "home Directory found to be " << home << '\n';

    string current = get_current_dir_name();
    //cout << "Found current to be " << current << '\n' << '\n';

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
  //cout << "entering runJob with job:\n";
  //job.toString();
  //cout << '\n';

    int jobLoc = allJobs.size();
    allJobs.push_back(job);
 	
    errorReDirect(job);   
    if(job.fg){
      cout<<"running job in foreground"<<endl;
      if (job.processList.size() == 1) {
          singleProcess(job);
      }   else {
          multiProcess(job);
      }
    }
    else{
      cout<<"running job in background"<<endl;
      int pid;
      int status;
      if((pid=fork())==-1){
	exitProgram(strerror(errno));
      }
      if(pid==0){
	if (job.processList.size() == 1) {
          singleProcess(job);
	}   else {
          multiProcess(job);
	}
	exitProgram("the child process is complete");
      }
      else{
	cout<<"waiting on background process"<<endl;
	int result = waitpid(pid,&status,WNOHANG);
	cout<<"obviously not waiting dawg"<<endl;
      }
    }
    allJobs.erase(allJobs.begin()+jobLoc);
    
}

int checkForBuiltins(vector<Process> input){
  string executable = input.at(0).arguments.at(0);
  
  if(executable.compare("exit")==0){
    if(input.at(0).arguments.size()==1){
      runExit();
    }
    else if(input.at(0).arguments.size()==2){
      stringstream ss(input.at(0).arguments.at(1));
      int n;
      ss >> n;
      runExit(n);
    }
    else{
     
      runExit(EXIT_FAILURE);
    }
  }
  else if(executable.compare("help")==0){
    cout<<"help called"<<endl;
    help();
    return 0;
    
  }
  else if(executable.compare("cd")==0){
    cout<<"change directory called"<<endl;
    int c;
    if(input.at(0).arguments.size()>1){
      cout<<"changing to a specific directory"<<endl;
      c = cd(input.at(0).arguments.at(1).c_str());
    }
    else if(input.at(0).arguments.size()==1){
      cout<<"going to home directory"<<endl;
      c = cd();
    }
    else{
      cout<<"cd must have exactly 1 or 2 arguments"<<endl;
      return -1;
    }
    if(c<0){
      return -1;
    }
    return 0;
  }
  else if(executable.compare("bg")==0){
    bg(input.at(0).arguments);
  }
  else if(executable.compare("fg")==0){
    fg(input.at(0).arguments);
  }
  else if(executable.compare("export")==0){
    runExport(input.at(0).arguments);
    return 0;
  }
  else if(executable.compare("jobs")==0){
    runJobs();
  }


  return 1;

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
        cout << "child execing program '" << executable <<"'\n";

	outputReDirect(job);
	inputReDirect(job);
        
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
		inputReDirect(job);
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
		outputReDirect(job);
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
	bool inQuotes = false;
        for (unsigned int j = 0; j < arg.length(); j++) {
            char ch = arg.at(j);
	    if(ch == '"'){
	      if(!((j>0)&&(arg.at(j-1)=='\\'))){
	        inQuotes = !inQuotes;
	        continue;
	      }
	    }
	    if(ch=='\\'){
	      if((j>0)&&(arg.at(j-1)=='\\')){

	      }
	      else{
		continue;
	      }
	    }
            if (j == (arg.length() - 1)) {
                p.arguments.push_back(arg.substr(prev, (arg.length() - prev)));
            }
            if ((ch == ' ' || ch == '\t') && !inQuotes) {
                if (arg.at(j - 1) != ' ' && arg.at(j - 1) != '\t') {
                    p.arguments.push_back(arg.substr(prev, j - prev));
                }
                prev = j + 1;
            }
	    if(inQuotes){
	      p.arguments.push_back(arg.substr(prev, j - prev));
	      prev = j+1;
	    }
            /*if (ch == '"' && arg.at(j - 1) != '\\') {
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
		}*/

        }
        processes.push_back(p);
    }

    return processes;
}

vector<string> divideByPipes(string command) {
    command = trim(command);
    unsigned long prev = 0;
    vector<string> arguments;
    for(unsigned int i=0;i<command.length();i++){
      if(command[i] == '&'){
        cout<<"found the ampersand"<<endl;
        command.erase(command.begin()+i);
      }
      
    }

    for (unsigned long i = 0; i < command.length(); i++) {

        char ch = command.at(i);

        if (ch == '|') {
            arguments.push_back(command.substr(prev, (i - prev)));
            cerr<<"found process "<<command.substr(prev,(i-prev));
            prev = i + 1;
            pipeNumber++;
        }
	
        if (i == (command.length() - 1)) {
            arguments.push_back(command.substr(prev, (i - prev) + 1));
            cerr<<"found process "<<command.substr(prev,(i-prev)+1);
        }
        if ((ch == '<') | (ch == '>')) {
            if (command.at(i - 1) == 'e') {
                i -= 1;
            }
	    cerr<<"found process "<<command.substr(prev,(i-prev))<<'\n';
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
    char **baseArray = new char *[list.size()+1];
    for (unsigned int j = 0; j < list.size(); j++) {
        cout << "arg size = " << list.at(j).size() << '\n';
        char *str = new char[list.at(j).size() + 1];
        for (unsigned int i = 0; i < list.at(j).size(); i++) {
            
	  cout << "found char " << list.at(j).at(i) << " at index " << i << '\n';
	  
            str[i] = list.at(j).at(i);
        }
        str[list.at(j).size()] = '\0';
        cout << "constructed string " << str << '\n';
        baseArray[j] = str;
    }
    baseArray[list.size()]='\0';
    char *const *thing = baseArray;

    cout << "leaving devolve arg method\n";
    return thing;
}
void inputReDirect(Job job){
    if((job.standardIn.substr(0,12).compare("STDIN_FILENO"))!=0){
        int fd;
        cerr<<"redirecting input to "<<job.standardIn<<'\n';    
	if((fd=open(job.standardIn.c_str(),O_RDONLY))==-1){
	    cerr<<"failure to open cuased by: ";
	    exitProgram(strerror(errno));
        }
        if(dup2(fd,STDIN_FILENO)==-1){
            cerr<<"failure to dup2 cuased by: ";
	    exitProgram(strerror(errno));
        }
    }else{
	cerr<<"Input kept as defualt\n";
    }
}
void outputReDirect(Job job){
    if((job.standardOut.substr(0,13).compare("STDOUT_FILENO"))!=0){    
        int fd;
        cerr<<"redirecting output to "<<job.standardOut<<'\n';
        if(job.outFlag==1){    
	    if((fd = open(job.standardOut.c_str(),O_WRONLY|O_TRUNC|O_CREAT))==-1){
                cerr<<"Failure to open cuased by: ";
	        exitProgram(strerror(errno));
            }
	}else{
	    if((fd = open(job.standardOut.c_str(),O_WRONLY|O_APPEND|O_CREAT))==-1){
	        cerr<<"falure to open cuased by: ";
		exitProgram(strerror(errno));
	    }
	}
        if(dup2(fd,STDOUT_FILENO)==-1){
            cerr<<"fualure to dup2 cuased by :";
            exitProgram(strerror(errno));
        }   
    }else{
	cerr<<"Output kept as defualt\n";
    }
}
void errorReDirect(Job job){
    if((job.standardEr.substr(0,13).compare("STDERR_FILENO"))!=0){
        int fd;
        cerr<<"redirecting error to "<<job.standardEr<<'\n';
        if(job.errFlag==1){
            if((fd = open(job.standardEr.c_str(),O_WRONLY|O_TRUNC|O_CREAT))==-1){
	        cerr<<"failure to open cuased by: ";
 	        exitProgram(strerror(errno));
            }
	}else{
	    if((fd = open(job.standardEr.c_str(),O_WRONLY|O_APPEND|O_CREAT))==-1){
		cerr<<"failure to open cuased by: ";
		exitProgram(strerror(errno));
	    }
	}
        if(dup2(fd,STDERR_FILENO)==-1){
	    cerr<<"faiure to dup2 cuased by :";
	    exitProgram(strerror(errno));
        }
    }else{
	cerr<<"error kept as defualt\n";
    }
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

void runExit() {
  cout<<"Programmed successfully closed by user"<<endl;
  exit(EXIT_SUCCESS);

}

void runExit(int n){
  cout<<"exited with n value"<<endl;
  exit(n);
}
void help() {
    cout << "csci1730sh John Peeples, James Roach, version 1-release" << endl;
    cout << "These shell commands are defined internally.  Type `help' to see this list." << endl << endl;
    cout << "A star (*) next to a name means that the command is disabled." << endl;
    cout << "cd [dir]" << endl;
    cout << "exit [n]" << endl;
    cout << "help " << endl;
    cout << "jobs " << endl;
    cout << "export NAME[=WORD] " << endl;
    cout << "bg JID " << endl;
    cout << "fg JID " << endl;
    

}

void runJobs(){
  if(allJobs.size()==0){
    return;
  }
  cout<<"listing jobs!"<<endl;
  cout<<"JID  STATUS \t COMMAND"<<endl;
  for(unsigned int i=0;i<allJobs.size();i++){
    cout<<allJobs.at(i).jid<<" ";
    if(allJobs.at(i).isRunning){
      cout<<"Running\t"<<endl;
    }
    else{
      cout<<"Stopped\t"<<endl;
    }
    cout<<allJobs.at(i).command;
  }

}
void bg(vector<string> input){
  if(input.size()<=0 || input.size()>2){
    cout<<"error: please enter the prompt as follows"<<endl;
    cout<<"bg JID";
    return;
  }
  

}
void fg(vector<string> input){
  if(input.size()<=0 || input.size()>2){
    cout<<"error: please enter the prompt as follows"<<endl;
    cout<<"fg JID";
    return;
  }
  stringstream stream(input.at(1));
  int jid; 
  stream >> jid;
  int currJob = -1;
  for(unsigned int i=0;i<allJobs.size();i++){
    if(allJobs.at(i).jid == jid){
      currJob = 1;
      return;
    }
  }
  if(currJob==-1){
    cout<<"there is no job with this JID"<<endl;
    return;
  }
  

}
void runExport(vector<string> input){
  cout<<"running export"<<endl;
  for(unsigned int x=1;x<input.size();x++){
    int pre =0;
    int post =0;
    bool before = true;
    bool equals = false;
    for(unsigned int i=0;i<input.at(x).length();i++){
      if(input.at(x)[i] == '='){
	before = false;
	equals = true;
      }
      else if(before){
	pre++;
      }
      else if(!before){
	post++;
      }
    }

   string preVar = "";
   string postVar = "";
    for(int i=0;i<pre;i++){
      preVar+= input.at(x)[i];
    }
    for(int i=0;i<post;i++){
      postVar+= input.at(x)[i+pre+1];
    }
   
    int ev;
    if(equals) ev = setenv(preVar.c_str(), postVar.c_str(),1);
    else ev = setenv(preVar.c_str(), "?",1);
    if(ev<0){
      cout<<"error: "<<strerror(errno)<<endl;
      return;
    }
    cout<<"export worked"<<endl;
  }
}
int cd(const char *pathname) {
    int x = chdir(pathname);
    return x;
}
int cd(){
  struct passwd *pw = getpwuid(getuid());
  string home = pw->pw_dir;
  int x = chdir(home.c_str());
  return x;
}
