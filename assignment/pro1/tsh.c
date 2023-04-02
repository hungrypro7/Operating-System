/* 
 * Copyright(c) 2023 All rights reserved by Heekuck Oh.
 * 이 프로그램은 한양대학교 ERICA 컴퓨터학부 학생을 위한 교육용으로 제작되었다.
 * 한양대학교 ERICA 학생이 아닌 이는 프로그램을 수정하거나 배포할 수 없다.
 * 프로그램을 수정할 경우 날짜, 학과, 학번, 이름, 수정 내용을 기록한다.
 * --------------한양대학교 ERICA ICT융합학부 2019098068 이찬영------------------
 * 수정 내용 : 표준 입출력 리다이렉션 ('>', '<') 기능과 프로세스 파이프 기능을 추가하였습니다. (CODE_LINE : 26~87, 148~253)
 * 03.19
 * 표준 입출력 리다이렉션("<", ">") 기능을 추가하였습니다.
 * 03.24
 * 파이프("|") 기능을 추가하였습니다.
 * 03.27
 * 다중 파이프 처리와 조합을 위한 구문을 추가하였습니다.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_LINE 80             /* 명령어의 최대 길이 */

/* 
 * redirection() - 리다이렉션을 실행한다.
 * 인자 : *temp[] (포인터 배열), start (탐색 시작 지점), argc (탐색 마지막 지점) 
 * 포인터 배열 *temp[] 위의 start 위치부터 argc 위치까지 문자열을 탐색하고 "<"나 ">" 문자가 있으면 리다이렉션을 수행한다.
 * 만약 중간에 "|" 문자나 NULL을 만나면 탐색을 중지하고 함수를 종료한다.
 * 참고 자료 출처 : https://woorld52.tistory.com/11
 */
void redirection(char *temp[], int start, int argc) {           

    char *input_file = NULL;            // 입력 파일 이름을 저장할 포인터 변수
    char *output_file = NULL;           // 출력 파일 이름을 저장할 포인터 변수
    int input_fd = -1;                  // 입력 파일 디스크립터
    int output_fd = -1;                 // 출력 파일 디스크립터

/* 
 * temp[]배열에서 리다이렉션 연산자를 찾기 위한 반복문 
 * start 인덱스부터 시작하여 리다이렉션 연산자("<", ">")가 나올 때마다 
 * input_file 또는 output_file을 설정하고 temp[] 배열에서 해당 인자를 NULL로 설정한다.
 * 만약 중간에 NULL이나, "|" 을 만나면 반복문을 종료한다.
 */  
    for (int i = start; i < argc; i++) {       
        if (strcmp(temp[i], "<") == 0) {    
            input_file = temp[i+1];
            temp[i] = NULL;                    
        }
        else if (strcmp(temp[i], ">") == 0) {
            output_file = temp[i+1];
            temp[i] = NULL;
        }
        else if (temp[i] == NULL || strcmp(temp[i], "|") == 0) {
            break;
        }
    }

/*
 * 입력 파일이 설정되어 있으면 해당 파일을 열고 파일 디스크립터를 업데이트한다.
 * 파일을 열 수 없으면 오류 메시지를 출력하고 프로그램을 종료한다.
 */
    if (input_file != NULL) {                           
        input_fd = open(input_file, O_RDONLY);
        if (input_fd == -1) {       // 파일을 오픈하는데 실패함
            perror("open");
            exit(EXIT_FAILURE);
        }
        dup2(input_fd, STDIN_FILENO);
        close(input_fd);
    }

/*
 * 출력 파일이 설정되어 있으면 해당 파일을 열고 파일 디스크립터를 업데이트한다. 
 * 파일을 열 수 없으면 오류 메시지를 출력하고 프로그램을 종료한다. 
 */
    if (output_file != NULL) {
        output_fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644); 
        if (output_fd == -1) {          // 파일을 출력하는데 실패함
            perror("open");
            exit(EXIT_FAILURE);
        }
        dup2(output_fd, STDOUT_FILENO);
        close(output_fd);
    }
}

/*
 * cmdexec - 명령어를 파싱해서 실행한다.
 * 스페이스와 탭을 공백문자로 간주하고, 연속된 공백문자는 하나의 공백문자로 축소한다.
 * 작은 따옴표나 큰 따옴표로 이루어진 문자열을 하나의 인자로 처리한다.
 * 기호 '<' 또는 '>'를 사용하여 표준 입출력을 파일로 바꾸거나,
 * 기호 '|'를 사용하여 파이프 명령을 실행하는 것도 여기에서 처리한다.
 */

static void cmdexec(char *cmd)
{
    char *argv[MAX_LINE/2+1];   /* 명령어 인자를 저장하기 위한 배열 */
    int argc = 0;               /* 인자의 개수 */
    char *p, *q;                /* 명령어를 파싱하기 위한 변수 */

    /*
     * 명령어 앞부분 공백문자를 제거하고 인자를 하나씩 꺼내서 argv에 차례로 저장한다.
     * 작은 따옴표나 큰 따옴표로 이루어진 문자열을 하나의 인자로 처리한다.
     */
    p = cmd; p += strspn(p, " \t");
    do {
        /*
         * 공백문자, 큰 따옴표, 작은 따옴표가 있는지 검사한다.
         */
        q = strpbrk(p, " \t\'\"");
        /*
         * 공백문자가 있거나 아무 것도 없으면 공백문자까지 또는 전체를 하나의 인자로 처리한다.
         */
        if (q == NULL || *q == ' ' || *q == '\t') {
            q = strsep(&p, " \t");
            if (*q) argv[argc++] = q;
        }
        /*
         * 작은 따옴표가 있으면 그 위치까지 하나의 인자로 처리하고,
         * 작은 따옴표 위치에서 두 번째 작은 따옴표 위치까지 다음 인자로 처리한다.
         * 두 번째 작은 따옴표가 없으면 나머지 전체를 인자로 처리한다.
         */
        else if (*q == '\'') {
            q = strsep(&p, "\'");
            if (*q) argv[argc++] = q;
            q = strsep(&p, "\'");
            if (*q) argv[argc++] = q;
        }
        /*
         * 큰 따옴표가 있으면 그 위치까지 하나의 인자로 처리하고,
         * 큰 따옴표 위치에서 두 번째 큰 따옴표 위치까지 다음 인자로 처리한다.
         * 두 번째 큰 따옴표가 없으면 나머지 전체를 인자로 처리한다.
         */
        else {
            q = strsep(&p, "\"");
            if (*q) argv[argc++] = q;
            q = strsep(&p, "\"");
            if (*q) argv[argc++] = q;
        }
    } while (p);
    argv[argc] = NULL;
    /*
     * argv에 저장된 명령어를 실행한다.
     */

    pid_t pid, pid2;            // 자식 프로세스 생성을 위한 프로세스 ID
    int state = 0;              // 파이프("|")가 있는 상태와 파이프가 없는 상태를 구분해주기 위한 상태 변수 
    int start_redi = 0;         // 리다이렉션을 시작할 위치를 받는 변수
    int a=0;

    for(int k=0; k<argc; k++) {             // for문을 통해 argv 배열에 파이프("|") 문자가 있는지 확인한다. 만약 존재하면 state == 1, 존재하지 않으면 state == 0 이다.
      if(strcmp(argv[k], "|") == 0) {
        state = 1;
        break;
      }
    }

    if(state == 1) {        // "|" 파이프가 명령어에 존재하는 경우

      for(int i=0 ; i<argc; i++) {
        if(strcmp(argv[i], "|") == 0) {         

          argv[i] = NULL;                       // argv를 탐색하는 중, 파이프("|")가 있으면, 해당 자리를 NULL로 만든다.
          
          int fd[2];                            // 파이프를 만들기 위한 배열 생성

          if (pipe(fd) == -1) {                    // 파이프 생성 중 오류 처리
            printf("pipe error\n");       
            exit(EXIT_FAILURE);
	      }

          pid = fork();                   // 손자 프로세스를 생성해 손자 프로세스는 WRITE, 자식 프로세스는 READ를 수행함

          if (pid == -1) {                // 손자 프로세스를 생성 중 오류 처리
            perror("fork");
            exit(EXIT_FAILURE);
          }
            
          /* 손자 프로세스
           * 자식 프로세스와 파이프를 통해 데이터를 주고받는 프로세스 간 통신을 구현한다.
           * 해당 프로세스에서는 write를 수행하므로 사용하지 않는 fd[0]은 닫고
           * fd[1] 파이프를 표준 출력 파일 디스크립터로 복제한다.
           * 복제한 뒤에는 fd[1] 파이프의 출력 부분을 닫아준다.
           * execvp()를 통해 argv[start_redi] 이후 부분을 실행한다.
           */
          if (pid == 0) {                     
            close(fd[0]);
		    dup2(fd[1], STDOUT_FILENO);     
            close(fd[1]);                                      
            execvp(argv[start_redi], &argv[start_redi]);        // 
          } 

          /* 자식 프로세스
           * wait(NULL)을 통해 손자 프로세스가 종료될 때까지 기다린다.           * 
           * 손자 프로세스와 파이프를 통해 데이터를 주고받는 프로세스 간 통신을 구현한다.
           * 손자 프로세스에서 사용하지 않는 fd[1] 파이프는 닫아주고 fd[0] 파이프를 표준 입력 파일 디스크립터로 복제한다.
           * 복제한 뒤에는 fd[0] 파이프의 입력 부분을 닫아준다.
           * for문을 통해 자식 프로세스에서 "<", ">" 이 있는지를 검사하고 만약 문자가 있으면 리다이렉션을 수행한 뒤 반복문을 빠져나간다.
           * "|" 파이프가 있거나 해당 argv가 NULL 일 때에도 반복문을 빠져나간다.
           */
          else {            
            wait(NULL);
            close(fd[1]);
            dup2(fd[0], STDIN_FILENO);     
            close(fd[0]);

            for(int j=i+1; j<argc; j++) {
              if(strcmp(argv[j], "<") == 0 || strcmp(argv[j], ">") == 0) {
                  redirection(argv, i+1, argc);
                  break;
              }
              
              if(strcmp(argv[j], "|") == 0 || argv[j] == NULL) { break; }

            }

           /* 자식 프로세스에서 다음 for문 수행을 하기 위해 프로세스를 하나 더 복제한다. 
            * execvp() 함수 이후의 코드는 실행되지 않으므로 프로세스를 하나 더 복제해서 실행시켜 주기 위함이다.
            * 자식 프로세스에서는 새로운 프로세스가 끝날 때까지 기다린다.
            */

            pid2 = fork();      
            if(pid2 == 0) {
                execvp(argv[i+1], &argv[i+1]);
                //exit(0);
            }

            else{
                wait(NULL);

                a = start_redi;         // 뒤에 있을 redirection 시작 지점을 a 변수로 설정해 준다.
            } 

          }
        start_redi = i+1;           // start_redi를 업데이트 해 준다.
        }                           // if(strcmp(argv[i], "|") == 0) 문 종료

        else if(strcmp(argv[i], "<") == 0 || strcmp(argv[i], ">") == 0) {           // 손자 프로세스에서 "<", ">" 등의 리다이렉션 문자가 있을 시에 수행을 위한 코드이다.
            redirection(argv, a, argc);
        }

      }   //for 문 종료

    } // if(state == 1) 문 종료

    else {                     // state == 0 인 경우, 즉 파이프("|")가 명령어에 존재하지 않는 경우
        redirection(argv, 0, argc);         // 리다이렉션 수행 ">", "<" 가 명령어에 없더라도 일단 함수를 수행한다. (만약 "<", ">"가 없으면 함수에서 자동으로 걸러짐)
        if(argc > 0) {                      
          execvp(argv[0], &argv[0]);            // execvp() 함수를 통해 새프로그램을 생성해 argv[0]부터 마지막까지 명령어를 순차적으로 수행한다.
        }
    }
}

/*
 * 기능이 간단한 유닉스 셸인 tsh (tiny shell)의 메인 함수이다.
 * tsh은 프로세스 생성과 파이프를 통한 프로세스간 통신을 학습하기 위한 것으로
 * 백그라운드 실행,  파이프 명령, 표준 입출력 리다이렉션 일부만 지원한다.
 */
int main(void)
{
    char cmd[MAX_LINE+1];       /* 명령어를 저장하기 위한 버퍼 */
    int len;                    /* 입력된 명령어의 길이 */
    pid_t pid;                  /* 자식 프로세스 아이디 */
    int background;             /* 백그라운드 실행 유무 */

    /*
     * 종료 명령인 "exit"이 입력될 때까지 루프를 무한 반복한다.
     */
    while (true) {
        /*
         * 좀비 (자식)프로세스가 있으면 제거한다.
         */
        pid = waitpid(-1, NULL, WNOHANG);
        if (pid > 0)
            printf("[%d] + done\n", pid);
        /*
         * 셸 프롬프트를 출력한다. 지연 출력을 방지하기 위해 출력버퍼를 강제로 비운다.
         */
        printf("tsh> "); fflush(stdout);
        /*
         * 표준 입력장치로부터 최대 MAX_LINE까지 명령어를 입력 받는다.
         * 입력된 명령어 끝에 있는 새줄문자를 널문자로 바꿔 C 문자열로 만든다.
         * 입력된 값이 없으면 새 명령어를 받기 위해 루프의 처음으로 간다.
         */
        len = read(STDIN_FILENO, cmd, MAX_LINE);
        if (len == -1) {
            perror("read");
            exit(EXIT_FAILURE);
        }
        cmd[--len] = '\0';
        if (len == 0)
            continue;
        /*
         * 종료 명령이면 루프를 빠져나간다.
         */
        if(!strcasecmp(cmd, "exit"))
            break;
        /*
         * 백그라운드 명령인지 확인하고, '&' 기호를 삭제한다.
         */
        char *p = strchr(cmd, '&');
        if (p != NULL) {
            background = 1;
            *p = '\0';
        }
        else
            background = 0;
        /*
         * 자식 프로세스를 생성하여 입력된 명령어를 실행하게 한다.
         */
        if ((pid = fork()) == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        }
        /*
         * 자식 프로세스는 명령어를 실행하고 종료한다.
         */
        else if (pid == 0) {
            cmdexec(cmd);
            exit(EXIT_SUCCESS);
        }
        /*
         * 포그라운드 실행이면 부모 프로세스는 자식이 끝날 때까지 기다린다.
         * 백그라운드 실행이면 기다리지 않고 다음 명령어를 입력받기 위해 루프의 처음으로 간다.
         */
        else if (!background)
            waitpid(pid, NULL, 0);
    }
    return 0;
}