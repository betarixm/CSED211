/* 
 * tsh - A tiny shell program with job control
 * 
 * 권민재, mzg00
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

/* Misc manifest constants */
#define MAXLINE    1024   /* max line size */
#define MAXARGS     128   /* max args on a command line */
#define MAXJOBS      16   /* max jobs at any point in time */
#define MAXJID    1<<16   /* max job ID */

/* Job states */
#define UNDEF 0 /* undefined */
#define FG 1    /* running in foreground */
#define BG 2    /* running in background */
#define ST 3    /* stopped */

/* 
 * Jobs states: FG (foreground), BG (background), ST (stopped)
 * Job state transitions and enabling actions:
 *     FG -> ST  : ctrl-z
 *     ST -> FG  : fg command
 *     ST -> BG  : bg command
 *     BG -> FG  : fg command
 * At most 1 job can be in the FG state.
 */

/* Global variables */
extern char **environ;      /* defined in libc */
char prompt[] = "tsh> ";    /* command line prompt (DO NOT CHANGE) */
int verbose = 0;            /* if true, print additional output */
int nextjid = 1;            /* next job ID to allocate */
char sbuf[MAXLINE];         /* for composing sprintf messages */

struct job_t {              /* The job struct */
    pid_t pid;              /* job PID */
    int jid;                /* job ID [1, 2, ...] */
    int state;              /* UNDEF, BG, FG, or ST */
    char cmdline[MAXLINE];  /* command line */
};
struct job_t jobs[MAXJOBS]; /* The job list */
/* End global variables */


/* Function prototypes */

/* Here are the functions that you will implement */
void eval(char *cmdline);
int builtin_cmd(char **argv);
void do_bgfg(char **argv);
void waitfg(pid_t pid);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);

/* Here are helper routines that we've provided for you */
int parseline(const char *cmdline, char **argv); 
void sigquit_handler(int sig);

void clearjob(struct job_t *job);
void initjobs(struct job_t *jobs);
int maxjid(struct job_t *jobs); 
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline);
int deletejob(struct job_t *jobs, pid_t pid); 
pid_t fgpid(struct job_t *jobs);
struct job_t *getjobpid(struct job_t *jobs, pid_t pid);
struct job_t *getjobjid(struct job_t *jobs, int jid); 
int pid2jid(pid_t pid); 
void listjobs(struct job_t *jobs);

void usage(void);
void unix_error(char *msg);
void app_error(char *msg);
typedef void handler_t(int);
handler_t *Signal(int signum, handler_t *handler);

/*
 * main - The shell's main routine 
 */
int main(int argc, char **argv) 
{
    char c;
    char cmdline[MAXLINE];
    int emit_prompt = 1; /* emit prompt (default) */

    /* Redirect stderr to stdout (so that driver will get all output
     * on the pipe connected to stdout) */
    dup2(1, 2);

    /* Parse the command line */
    while ((c = getopt(argc, argv, "hvp")) != EOF) {
        switch (c) {
        case 'h':             /* print help message */
            usage();
	    break;
        case 'v':             /* emit additional diagnostic info */
            verbose = 1;
	    break;
        case 'p':             /* don't print a prompt */
            emit_prompt = 0;  /* handy for automatic testing */
	    break;
	default:
            usage();
	}
    }

    /* Install the signal handlers */

    /* These are the ones you will need to implement */
    Signal(SIGINT,  sigint_handler);   /* ctrl-c */
    Signal(SIGTSTP, sigtstp_handler);  /* ctrl-z */
    Signal(SIGCHLD, sigchld_handler);  /* Terminated or stopped child */

    /* This one provides a clean way to kill the shell */
    Signal(SIGQUIT, sigquit_handler); 

    /* Initialize the job list */
    initjobs(jobs);

    /* Execute the shell's read/eval loop */
    while (1) {

	/* Read command line */
	if (emit_prompt) {
	    printf("%s", prompt);
	    fflush(stdout);
	}
	if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
	    app_error("fgets error");
	if (feof(stdin)) { /* End of file (ctrl-d) */
	    fflush(stdout);
	    exit(0);
	}

	/* Evaluate the command line */
	eval(cmdline);
	fflush(stdout);
	fflush(stdout);
    } 

    exit(0); /* control never reaches here */
}
  
/* 
 * eval - Evaluate the command line that the user has just typed in
 * 
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately. Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.  
*/
void eval(char *cmdline) 
{
    char* argv[MAXARGS]; // 아규먼트 리스트를 담을 배열
    sigset_t _sigset; // sigset
    pid_t pidFork; // fork 프로세스 아이디
    int signalValueList[3] = {SIGINT, SIGCHLD, SIGSTOP}; // 핸들링할 시그널 리스트
    int i = 0; // 이터레이터

    // parseline을 통해 argv에 아규먼트 리스트를 세팅하고, 백그라운드 여부를 isBackgroundJob에 저장
    int isBackgroundJob = parseline(cmdline, argv);

    // 올바르지 않은 입력일 경우 종료.
    // i.e. 아무것도 입력되지 않았거나, built-in 커맨드인 경우.
    if(!argv[0] || builtin_cmd(argv)) {
        return;
    }

    // sigset을 세팅한다.
    // 만약 실패한다면, unix_error를 이용하여 에러를 표시한다.

    // sigemptyset을 수행하고, 실패했을 때에는 에러를 표시한다..
    if(sigemptyset(&_sigset) < 0){
        unix_error("sigemptyset error");
    }

    // SIGINT, SIGCHLD, SIGSTOP에 대해 sigaddset을 수행하고, 실패할 경우에는 에러를 표시한다.
    for(i = 0; i < 3; ++i){
        if(sigaddset(&_sigset, signalValueList[i]) != 0){
            unix_error("sigaddset error");
        }
    }

    // sigprocmask를 수행하고, 실패할 경우에는 에러를 표시한다.
    if(sigprocmask(SIG_BLOCK, &_sigset, 0) < 0){
        unix_error("sigprocmask error");
    }

    // fork를 수행하고, 실패할 경우에는 에러를 표시한다.
    if((pidFork = fork()) < 0){
        unix_error("fork error");
    }

    // child process인 경우
    if(pidFork == 0){
        // 우선, 현재 sigset을 unblock한다.
        sigprocmask(SIG_UNBLOCK, &_sigset, 0);

        // 이후, setpgid를 통해 프로세스 그룹을 설정하고, 실패할 경우에는 에러를 띄운다.
        if(setpgid(0, 0) < 0){
            unix_error("setpgid error");
        }

        // execve를 통해 주어진 인자를 실행하고, 실패할 경우에는 커맨드가 존재하지 않는다는 에러를 띄운다.
        if(execve(argv[0], argv, environ) < 0){
            printf("%s: Command not found\n", argv[0]);
            exit(1);
        }
    }

    // addjob을 통해 Job 배열에 포크된 프로세스 아이디를 추가한다.
    addjob(jobs, pidFork, isBackgroundJob ? BG : FG, cmdline);

    // sigprocmask를 통해 unblock을 수행한다.
    if(sigprocmask(SIG_UNBLOCK, &_sigset, 0) < 0){
        unix_error("sigprocmask error");
    }

    // 만약 백그라운드 Job이 아니라면, Job이 종료될 때 까지 대기한다.
    if(!isBackgroundJob){
        waitfg(pidFork);
        return;
    }

    // background job이라면, 실행 정보를 출력한다.
    if(pidFork){
        printf("[%d] (%d) %s", pid2jid(pidFork), pidFork, cmdline);
    }
}

/* 
 * parseline - Parse the command line and build the argv array.
 * 
 * Characters enclosed in single quotes are treated as a single
 * argument.  Return true if the user has requested a BG job, false if
 * the user has requested a FG job.  
 */
int parseline(const char *cmdline, char **argv) 
{
    static char array[MAXLINE]; /* holds local copy of command line */
    char *buf = array;          /* ptr that traverses command line */
    char *delim;                /* points to first space delimiter */
    int argc;                   /* number of args */
    int bg;                     /* background job? */

    strcpy(buf, cmdline);
    buf[strlen(buf)-1] = ' ';  /* replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* ignore leading spaces */
	buf++;

    /* Build the argv list */
    argc = 0;
    if (*buf == '\'') {
	buf++;
	delim = strchr(buf, '\'');
    }
    else {
	delim = strchr(buf, ' ');
    }

    while (delim) {
	argv[argc++] = buf;
	*delim = '\0';
	buf = delim + 1;
	while (*buf && (*buf == ' ')) /* ignore spaces */
	       buf++;

	if (*buf == '\'') {
	    buf++;
	    delim = strchr(buf, '\'');
	}
	else {
	    delim = strchr(buf, ' ');
	}
    }
    argv[argc] = NULL;
    
    if (argc == 0)  /* ignore blank line */
	return 1;

    /* should the job run in the background? */
    if ((bg = (*argv[argc-1] == '&')) != 0) {
	argv[--argc] = NULL;
    }
    return bg;
}

/* 
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.  
 */
int builtin_cmd(char **argv) 
{
    // 인자들 중 첫번째 인자를 Cmd로 설정한다.
    char* cmd = argv[0];

    if(strcmp(cmd, "quit") == 0){
        // cmd가 quit 이라면 쉘을 종료한다.
        exit(0);
    } else if (strcmp(cmd, "fg") == 0 || strcmp(cmd, "bg") == 0) {
        // cmd가 fg나 bg라면 do_bgfg를 호출한다.
        do_bgfg(argv);
    } else if (strcmp(cmd, "jobs") == 0) {
        // cmd가 jobs라면 listjobs를 이용해 출력한다.
        listjobs(jobs);
    } else {
        // cmd가 built-in cmd가 아닌 경우 0을 반환한다.
        return 0;
    }
    // cmd가 built-in cmd인 경우이므로 1을 반환한다.
    return 1;
}

/* 
 * do_bgfg - Execute the builtin bg and fg commands
 */
void do_bgfg(char **argv) 
{
    // argv 파싱 변수들
    char* cmd = argv[0]; // argv의 첫번째 인자
    char* param; // argv의 두번째 인자. 즉, job이나 process ID

    // Job or Process ID
    int jid;
    pid_t pid;

    // Job
    struct job_t* job;

    // kill result
    int killResult;

    // Check Booleans
    int isJobId;
    int isDigit = 1;
    int isFG;

    // Iterator
    int i;

    // 만약 올바르지 않은 입력인데 do_bgfg가 호출되었다면, 에러를 표시하고 종료한다.
    if(!argv[0] || !(strcmp(argv[0], "fg") == 0 || strcmp(argv[0], "bg") == 0)){
        printf("do_bgfg: Internal error");
        exit(0);
    }

    // 만약 두번째 인자가 존재하지 않는다면, 인자가 필요함을 표시하고 종료한다.
    if(!argv[1]){
        printf("%s command requires PID or %%jobid argument\n", argv[0]);
        return;
    }

    // 두번째 인자를 param에 저장한다.
    param = argv[1];

    // 첫번째 글자가 %라면, JobID 이므로 isJobId를 true로 설정하고, 아니라면 false로 설정한다.
    isJobId = (param[0] == '%');

    // 두번째 인자가 올바른 인자인지, 즉 숫자로 이루어져 있는 인자인지 체크하자.
    // Job ID 인 경우에는 인덱스 1부터, Process ID인 경우에는 인덱스 0부터, 각 글자가 숫자인지 체크한다.
    for(i = isJobId; ; i++){
        // null 문자일 경우 종료
        if(param[i] == '\0'){
            break;
        }

        // i번째 인덱스의 글자가 숫자가 아닌 경우
        if(!isdigit(param[i])){
            // isDigit을 false로 만들고 반복 종료.
            isDigit = 0;
            break;
        }
    }

    // param이 올바르지 않은, 즉 숫자로만 이루어져 있지 않은 경우 에러를 띄우고 종료
    if(!isDigit) {
        printf("%s: argument must be a PID or %%jobid\n", cmd);
        return;
    }

    if(isJobId) {
        // Job ID인 경우, ID를 파싱하고 getjobjid를 통해 job 조회
        jid = (int) strtol(param + 1, NULL, 10);
        job = getjobjid(jobs, jid);
    } else {
        // Process ID인 경우, ID를 파싱하고 getjobpid를 통해 job 조회
        pid = strtol(param, NULL, 10);
        job = getjobpid(jobs, pid);
    }

    // 해당하는 job이 존재하지 않는 경우, Job ID 여부에 따라 적절한 에러 메시지 출력
    if(!job){
        if(isJobId){
            printf("%%%d: No such job\n", jid);
        } else {
            printf("(%d): No such process\n", pid);
        }
        return;
    }

    pid = job->pid; // job의 pid 설정
    isFG = (strcmp(cmd, "fg") == 0); // foreground 여부 저장

    killResult = kill(-pid, SIGCONT); // 프로세스에 SIGCONT 전송

    if(isFG){
        // fg인 경우

        // 우선 kill에 실패했을 경우 에러를 띄운다.
        if(killResult < 0) {
            unix_error("kill (fg) error");
        }

        // Job의 상태를 FG로 바꾸고 job이 종료되기를 기다린다.
        job->state = FG;
        waitfg(pid);
    } else {
        // bg인 경우

        // 우선 kill에 실패했을 경우 에러를 띄운다.
        if(killResult < 0) {
            unix_error("kill (bg) error");
        }

        // Job의 상태를 BG로 바꾸고 상태를 출력한다.
        job->state = BG;
        printf("[%d] (%d) %s", job->jid, job->pid, job->cmdline);
    }
}

/* 
 * waitfg - Block until process pid is no longer the foreground process
 */
void waitfg(pid_t pid)
{
    struct job_t* job = getjobpid(jobs, pid); // pid로부터 Job을 조회
    if(job){
        // 만약 job이 존재한다면,
        while(job->state == FG){
            // job의 상태가 FG인 동안 sleep.
            sleep(1);
        }
    }
}

/*****************
 * Signal handlers
 *****************/

/* 
 * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 *     a child job terminates (becomes a zombie), or stops because it
 *     received a SIGSTOP or SIGTSTP signal. The handler reaps all
 *     available zombie children, but doesn't wait for any other
 *     currently running children to terminate.  
 */
void sigchld_handler(int sig) 
{
    pid_t pid; // Process ID
    struct job_t* job; // Job
    int status; // 상태

    // 자식 프로세스 대기 및 상태 회수
    while((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
        if (WIFSTOPPED(status)) { // 자식 프로세스가 정지된 상태일 때
            // 우선 job을 조회한다.
            job = getjobpid(jobs, pid);
            if(!job){ // job이 존재하지 않는 경우, 에러를 표시하고 종료한다.
                printf("Lost track of (%d)\n", pid);
                return;
            }

            job->state = ST; // Job의 상태를 ST로 설정

            // Job 상태 출력
            // WSTOPSIG를 통해 프로세스를 정지시킨 시그널 조회
            printf("Job [%d] (%d) stopped by signal %d\n", pid2jid(pid), pid, WSTOPSIG(status));
        } else if(WIFEXITED(status)){ // 자식 프로세스가 EXIT된 상태일 때
            if(WTERMSIG(status)){ // 자식 프로세스를 종료시킨 시그널이 0이 아닐 때
                // 에러 출력
                unix_error("waitpid error");
            }

            // Job 삭제
            deletejob(jobs, pid);
        } else if (WIFSIGNALED(status)) { // 자식 프로세스가 어떤 신호를 받아서 종료되었을 때
            deletejob(jobs, pid); // job을 우선 삭제

            // Job 상태 출력
            // WTERMSIG를 통해 종료하게 만든 시그널 출력
            printf("Job [%d] (%d) terminated by signal %d\n", pid2jid(pid), pid, WTERMSIG(status));
        }
    }
}

/*
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.
 */
void sigint_handler(int sig)
{
    pid_t pid = fgpid(jobs); // fgpid로 foreground job 조회
    // pid가 존재할 때, kill 시도.
    // 실패할 경우 에러 출력
    if(pid && (kill(-pid, sig) < 0)){
        unix_error("kill (sigint) error");
    }
}

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending it a SIGTSTP.
 */
void sigtstp_handler(int sig)
{
    pid_t pid = fgpid(jobs); // fgpid로 foreground job 조회
    // pid가 존재할 때, kill 시도.
    // 실패할 경우 에러 출력
    if(pid && (kill(-pid, sig) < 0)){
        unix_error("kill (tstp) error");
    }
}

/*********************
 * End signal handlers
 *********************/

/***********************************************
 * Helper routines that manipulate the job list
 **********************************************/

/* clearjob - Clear the entries in a job struct */
void clearjob(struct job_t *job) {
    job->pid = 0;
    job->jid = 0;
    job->state = UNDEF;
    job->cmdline[0] = '\0';
}

/* initjobs - Initialize the job list */
void initjobs(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++)
	clearjob(&jobs[i]);
}

/* maxjid - Returns largest allocated job ID */
int maxjid(struct job_t *jobs) 
{
    int i, max=0;

    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].jid > max)
	    max = jobs[i].jid;
    return max;
}

/* addjob - Add a job to the job list */
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline) 
{
    int i;
    
    if (pid < 1)
	return 0;

    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid == 0) {
	    jobs[i].pid = pid;
	    jobs[i].state = state;
	    jobs[i].jid = nextjid++;
	    if (nextjid > MAXJOBS)
		nextjid = 1;
	    strcpy(jobs[i].cmdline, cmdline);
  	    if(verbose){
	        printf("Added job [%d] %d %s\n", jobs[i].jid, jobs[i].pid, jobs[i].cmdline);
            }
            return 1;
	}
    }
    printf("Tried to create too many jobs\n");
    return 0;
}

/* deletejob - Delete a job whose PID=pid from the job list */
int deletejob(struct job_t *jobs, pid_t pid) 
{
    int i;

    if (pid < 1)
	return 0;

    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid == pid) {
	    clearjob(&jobs[i]);
	    nextjid = maxjid(jobs)+1;
	    return 1;
	}
    }
    return 0;
}

/* fgpid - Return PID of current foreground job, 0 if no such job */
pid_t fgpid(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].state == FG)
	    return jobs[i].pid;
    return 0;
}

/* getjobpid  - Find a job (by PID) on the job list */
struct job_t *getjobpid(struct job_t *jobs, pid_t pid) {
    int i;

    if (pid < 1)
	return NULL;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].pid == pid)
	    return &jobs[i];
    return NULL;
}

/* getjobjid  - Find a job (by JID) on the job list */
struct job_t *getjobjid(struct job_t *jobs, int jid) 
{
    int i;

    if (jid < 1)
	return NULL;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].jid == jid)
	    return &jobs[i];
    return NULL;
}

/* pid2jid - Map process ID to job ID */
int pid2jid(pid_t pid) 
{
    int i;

    if (pid < 1)
	return 0;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].pid == pid) {
            return jobs[i].jid;
        }
    return 0;
}

/* listjobs - Print the job list */
void listjobs(struct job_t *jobs) 
{
    int i;
    
    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid != 0) {
	    printf("[%d] (%d) ", jobs[i].jid, jobs[i].pid);
	    switch (jobs[i].state) {
		case BG: 
		    printf("Running ");
		    break;
		case FG: 
		    printf("Foreground ");
		    break;
		case ST: 
		    printf("Stopped ");
		    break;
	    default:
		    printf("listjobs: Internal error: job[%d].state=%d ", 
			   i, jobs[i].state);
	    }
	    printf("%s", jobs[i].cmdline);
	}
    }
}
/******************************
 * end job list helper routines
 ******************************/


/***********************
 * Other helper routines
 ***********************/

/*
 * usage - print a help message
 */
void usage(void) 
{
    printf("Usage: shell [-hvp]\n");
    printf("   -h   print this message\n");
    printf("   -v   print additional diagnostic information\n");
    printf("   -p   do not emit a command prompt\n");
    exit(1);
}

/*
 * unix_error - unix-style error routine
 */
void unix_error(char *msg)
{
    fprintf(stdout, "%s: %s\n", msg, strerror(errno));
    exit(1);
}

/*
 * app_error - application-style error routine
 */
void app_error(char *msg)
{
    fprintf(stdout, "%s\n", msg);
    exit(1);
}

/*
 * Signal - wrapper for the sigaction function
 */
handler_t *Signal(int signum, handler_t *handler) 
{
    struct sigaction action, old_action;

    action.sa_handler = handler;  
    sigemptyset(&action.sa_mask); /* block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0)
	unix_error("Signal error");
    return (old_action.sa_handler);
}

/*
 * sigquit_handler - The driver program can gracefully terminate the
 *    child shell by sending it a SIGQUIT signal.
 */
void sigquit_handler(int sig) 
{
    printf("Terminating after receipt of SIGQUIT signal\n");
    exit(1);
}



