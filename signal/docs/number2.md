# #2 프로세스 끼리 돌아가며 카운트 하기

* Related file: [src/count.c](signal/src/count.c) => count

### 목적

어떤 파일을 연다, 없으면 만들어서 열고, 첫 부분에 "0"을 쓴다.

프로세스 셋이서, 2명은 자고 1명은 깨어나서 파일에 string형식으로 쓰여진 숫자를 1증가 시켜서 다시 쓴 후, 다음 프로세스를 깨운다음 자기 자신을 다시 잠든다.

이 과정을 N번 반복하여, 결과적으로 파일에 N이 쓰여지도록 한다.

### 로직

1. 프로세스의 실행 인자로부터 N을 읽는다
2. SIGUSR1 시그널의 핸들러로, work함수를 설정한다.
   1. work함수는 파일을 읽어서 쓰여진 스트링이 N인지 보고,
      1. N이면 root_pid의 프로세스에 SIGUSR2 시그널을 보내고 끝낸다
      2. N이 아니면 N+1을 파일에 쓰고 next_pid의 프로세스에 SIGUSR1 시그널을 보낸다
3. SIGUSR2 시그널의 핸들러로, stop_count함수를 설정한다.
   1. stop_count함수는 자신이 루트면 child1, child2에게 SIGUSR2 시그널을 보내 종료시킨다
   2. 종료된다.
4. argv[2]` 로 들어온 파일경로를 가지고 파일을 열기 시도한다. 없으면 만들고, "0"을 쓴다.
5. 부모 프로세스는 자신의 pid를 root_pid 전역변수에 저장한다
6. 부모 프로세스가 fork해서 child1 프로세스를 만든다. 부모 프로세스는 후에, 자신이 깨울 프로세스(`next_pid`)로 child1을 지정한다.
7. child1 프로세스가 fork해서 child2 프로세스를 만든다. child1 프로세스는 `next_pid` 로 child2의 pid를 가진다.
8. child2 프로세스는 `next_pid` 로 부모 프로세스의 pid를 가진다. 그리고 `next_pid` 의 프로세스 에게 SIGUSR1 시그널을 보낸다.

위의 실행과정을 따라가게 되면, (parent, child1, child2) 프로세스가 만들어지고, child2가 parent에게 자기 자신이 다 만들어졌음을 알리는 동시에 counting의 시작을 알린다. 3개의 각 프로세스는 increment후에 next_pid의 프로세스에게 increment를 시키고. 목적지에 다다르게 되면 멈춘다.