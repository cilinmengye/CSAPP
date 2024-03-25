/* 
 * tsh - A tiny shell program with job control
 * 
 * <Put your name and login ID here>
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

/* Here are the functions that you will implement
• eval: Main routine that parses and interprets the command line. [70 lines]
• builtin cmd: Recognizes and interprets the built-in commands: quit, fg, bg, and jobs. [25 lines]
• do bgfg: Implements the bg and fg built-in commands. [50 lines]
• waitfg: Waits for a foreground job to complete. [20 lines]
• sigchld handler: Catches SIGCHILD signals. [80 lines]
• sigint handler: Catches SIGINT (ctrl-c) signals. [15 lines]
• sigtstp handler: Catches SIGTSTP (ctrl-z) signals. [15 lines]
 */
void eval(char *cmdline);
int builtin_cmd(char **argv);
void do_bgfg(char **argv);
/*
One of the tricky parts of the assignment is deciding on the allocation of work between the waitfg
and sigchld handler functions. We recommend the following approach:
– In waitfg, use a busy loop around the sleep function.
– In sigchld handler, use exactly one call to waitpid.
*/
void waitfg(pid_t pid);

void sigchld_handler(int sig);
/*Typing ctrl-c (ctrl-z) should cause a SIGINT (SIGTSTP) signal to be sent to the current foreground job,
as well as any descendents of that job (e.g., any child processes that it forked). 
If there is no foreground job, then the signal should have no effect.
*/
/*When you implement your signal handlers, 
be sure to send SIGINT and SIGTSTP signals to the entire foreground process group,
 using ”-pid” instead of ”pid” in the argument to the kill function.
The sdriver.pl program tests for this error
*/
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
/*can't get by pid return NULL*/
struct job_t *getjobpid(struct job_t *jobs, pid_t pid);
/*can't get by jid return NULL*/
struct job_t *getjobjid(struct job_t *jobs, int jid); 
/*can't transform return 0*/
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
 * [70 lines]
*/
/*
* Here is the workaround: After the fork, but before the execve, the child process should call
* setpgid(0, 0), which puts the child in a new process group whose group ID is identical to the
* child’s PID. 
*/
int Fork(void){
    pid_t pid;
    if((pid = fork()) < 0){
        unix_error("Fork error");
    }
    return pid;
}

void Execve(const char *filename, char *const argv[], char *const envp[]){
    char str[MAXLINE];
    /*important! We shouldn't exit if command not found!*/
    if (execve(filename, argv, envp) < 0){
        sprintf(str, "%s: Command not found\n", argv[0]);
        printf("%s",str);
        exit(1);
    }
    return;
}

void Kill(pid_t pid, int sig){
    if (kill(pid, sig) < 0){
        unix_error("Kill error");
    }
    return;
}

void Sigprocmask(int how, const sigset_t *set, sigset_t *oldset){
    if (sigprocmask(how, set, oldset) < 0){
        unix_error("Sigprocmask error");
    }
    return ;
}

void Sigemptyset(sigset_t *set){
    if (sigemptyset(set) < 0){
        unix_error("Sigemptyset error");
    }
    return ;
}

void Sigfillset(sigset_t *set){
    if (sigfillset(set) < 0){
        unix_error("sigefillset error");
    }
    return ;
}

void Sigaddset(sigset_t *set, int signum){
    if (sigaddset(set, signum) < 0){
        unix_error("sigaddset error");
    }
    return ;
}

void Setpgid(pid_t pid, pid_t pgid){
    if (setpgid(pid, pgid) < 0){
        unix_error("setpgid error");
    }
    return ;
}

void eval(char *cmdline) 
{
    char *argv[MAXARGS];
    /*book have this, but I not have
    * Why need buf? because in function parseline, we can't give it cmdline, it will modify cmdline 
    */
    char buf[MAXLINE], str[MAXLINE];
    int bg, jid;
    pid_t pid;
    sigset_t mask_all, mask_one, prev_one;

    strcpy(buf, cmdline);
    bg = parseline(buf, argv);
    
    if (argv[0] == NULL){
        return;     /*Ignore empty lines*/
    }

    if (builtin_cmd(argv)){   
       return;
    }

     /*If not builtin_cmd, then fork and exectue */
    Sigfillset(&mask_all);
    Sigemptyset(&mask_one);
    Sigaddset(&mask_one, SIGCHLD);
    /*Block SIGCHLD, Prevent addjob and deletejob from competing*/
    Sigprocmask(SIG_BLOCK, &mask_one, &prev_one);   
    if ((pid = Fork()) == 0){
        /*
        * Prevent processes other than the foreground process from 
        * being affected by the signal 
        * because they are in the same group after pressing ctrl+c or ctrl+z.
        */
        Setpgid(0, 0);    
        Sigprocmask(SIG_SETMASK, &prev_one, NULL);
        Execve(argv[0], argv, NULL);
    }
    /*we need block some sign for avoid race between addjob and other process*/    
    Sigprocmask(SIG_BLOCK, &mask_all, NULL); 
    if (!bg){
        addjob(jobs, pid, FG, buf);
        Sigprocmask(SIG_SETMASK, &prev_one, NULL);   /*Unblock SIGCHLD*/
        waitfg(pid);
    } else{
        addjob(jobs, pid, BG, buf);   
        jid = pid2jid(pid); 
        sprintf(str, "[%d] (%d) %s", jid, pid, buf);
        printf("%s", str);
        Sigprocmask(SIG_SETMASK, &prev_one, NULL);   /*Unblock SIGCHLD*/
    }
    return;
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
 * [25 lines]
 */
int builtin_cmd(char **argv) 
{
    if (!strcmp(argv[0], "quit")){
        exit(0);
    }
    if (!strcmp(argv[0], "&")){
        return 1;
    }
    if (strcmp(argv[0], "bg") == 0 || strcmp(argv[0], "fg") == 0){
        do_bgfg(argv);
        return 1;
    }
    if (!strcmp(argv[0], "jobs")){
        listjobs(jobs);
        return 1;
    }
    return 0;     /* not a builtin command */
}

/* 
 * do_bgfg - Execute the builtin bg and fg commands
 */
/*
– The bg <job> command restarts <job> by sending it a SIGCONT signal, and then runs it in
the background. The <job> argument can be either a PID or a JID.
– The fg <job> command restarts <job> by sending it a SIGCONT signal, and then runs it in
the foreground. The <job> argument can be either a PID or a JID.
*/
void do_bgfg(char **argv) 
{
    int argc = 0, pid, jid;
    char str[MAXLINE*2];
    struct job_t *job;
    sigset_t mask_all, prev_all;
    
    while (argv[argc] != NULL){
        argc++;
    }
    if (argc < 2){
        sprintf(str,"%s command requires PID or %%jobid argument\n", argv[0]);
        printf("%s",str);
        return;
    }

    /*prevent job from deleting before we modify job status,
    * so I need block some sign
    */
    Sigfillset(&mask_all);
    Sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
    /*get jib or job from command line and then get job from jobs*/
    if (argv[1][0] == '%'){
        if (!sscanf(argv[1], "%%%d", &jid)){
            sprintf(str,"%s: argument must be a PID or %%jobid \n", argv[0]);
            printf("%s",str);
            Sigprocmask(SIG_SETMASK, &prev_all, NULL);
            return;
        }
        job = getjobjid(jobs, jid);
        if (job == NULL){
            sprintf(str,"%%%d: No such job\n", jid);
            printf("%s",str);
            Sigprocmask(SIG_SETMASK, &prev_all, NULL);
            return;
        }
    } else{
        if (!sscanf(argv[1], "%d", &pid)){
            sprintf(str,"%s: argument must be a PID or %%jobid \n", argv[0]);
            printf("%s",str);
            Sigprocmask(SIG_SETMASK, &prev_all, NULL);
            return;
        }
        job = getjobpid(jobs, pid);
        if (job == NULL){
            sprintf(str,"(%d): No such process\n", pid);
            printf("%s",str);
            Sigprocmask(SIG_SETMASK, &prev_all, NULL);
            return;
        }
    } 

    /*sent SIGCONT sign to pid prcesss group*/
    Kill(-(job->pid), SIGCONT);

    if (!strcmp(argv[0], "fg")){
        job->state = FG;
        Sigprocmask(SIG_SETMASK, &prev_all, NULL);
        waitfg(job->pid);
    } else {
        job->state = BG;
        sprintf(str, "[%d] (%d) %s", job->jid, job->pid, job->cmdline);
        printf("%s", str);
        Sigprocmask(SIG_SETMASK, &prev_all, NULL);
    }
    return;
}

/* 
 * waitfg - Block until process pid is no longer the foreground process
 */
/*One of the tricky parts of the assignment is deciding on the allocation of work between the waitfg
and sigchld handler functions. We recommend the following approach:
– In waitfg, use a busy loop around the sleep function.
– In sigchld handler, use exactly one call to waitpid.
*/
/*when have FG job, waitfg will continue wait*/
void waitfg(pid_t pid)
{
    /*wait for SIGCHLD*/
    sigset_t mask_all,prev_all;

    Sigfillset(&mask_all);
    /*because fgpid need access jobs, I need block sign*/
    Sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
    while (fgpid(jobs) != 0){
        sigsuspend(&prev_all);
    }
    Sigprocmask(SIG_SETMASK, &prev_all, NULL);
    
    return;
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
    //wait for solve, when I press ctrl+z, The child process actually sends the SIGCHLD signal to 
    //the parent process
    int olderrno = errno;
    int *statusp = (int *)malloc(sizeof(int));
    pid_t pid;
    sigset_t mask_all, prev_all;
    struct job_t *job;
    char str[MAXLINE];

    Sigfillset(&mask_all);
    while ((pid = waitpid(-1, statusp, WNOHANG | WUNTRACED)) > 0){
        Sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
        job = getjobpid(jobs,pid);
        if (job == NULL) {
            continue;
        }
        if (WIFEXITED(*statusp)){   /*normal end*/
            deletejob(jobs, pid);
        } else if (WIFSIGNALED(*statusp)){  /*ctrl+c*/
            sprintf(str, "Job [%d] (%d) terminated by signal 2\n", job->jid, job->pid);
            printf("%s",str);
            deletejob(jobs, pid);
        } else if (WIFSTOPPED(*statusp)){   /*ctrl+z*/
            job->state = ST;
            sprintf(str, "Job [%d] (%d) stopped by signal 20\n", job->jid, job->pid);
            printf("%s",str);
        } else {
            job->state = UNDEF;
        }
        Sigprocmask(SIG_SETMASK, &prev_all, NULL);
    }
    //if (errno != ECHILD){
    //    unix_error("waitpid error");
    //}
    errno = olderrno;
    return;
}

/* 
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.  
 */
void sigint_handler(int sig) 
{
    int pid, olderrno = errno;
    sigset_t mask_all, prev_all;
    struct job_t *job;

    Sigfillset(&mask_all);
    Sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
    pid = fgpid(jobs);
    job = getjobpid(jobs, pid);
    if (job != NULL){
        Kill(-(job->pid), sig);
    } 
    Sigprocmask(SIG_SETMASK, &prev_all, NULL);
    errno = olderrno; 

    return;
}

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending it a SIGTSTP.  
 */
void sigtstp_handler(int sig) 
{
    int pid, olderrno = errno;
    sigset_t mask_all, prev_all;
    struct job_t *job;

    Sigfillset(&mask_all);
    Sigprocmask(SIG_BLOCK, &mask_all, &prev_all);
    pid = fgpid(jobs);
    job = getjobpid(jobs,pid);
    if (job != NULL){
        Kill(-(job->pid), sig);
    }
    Sigprocmask(SIG_SETMASK, &prev_all, NULL);
    errno = olderrno; 

    return;
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



