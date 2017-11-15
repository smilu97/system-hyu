# #1 시그널 핸들러 바꿔보기

- Related file: [src/stop.c](/signal/src/stop.c) => stop

### 목적

Ctrl-C가 눌려도 프로세스가 종료되지 않도록 한다.

### [sigaction](http://man7.org/linux/man-pages/man2/sigaction.2.html)

Signal이 Process에 전송되면, 비동기적으로 해당 Signal과 매칭 되는 Signal handler이 호출되게 된다.

이 signal handler는 메모리 영역 어딘가에서 해당되는 signal의 번호와 같이 매칭되어 저장되어있는 것으로 보이는데, `sigaction` 함수를 이용해서 이 테이블을 변경시킬 수 있다.

#### Arguments

- int signum
  - 바꿀 시그널의 번호
- const struct sigaction * act
  - 바꿀 시그널 핸들러를 담고 있는 `struct sigaction` 객체의 주소값
- struct sigaction * oldact
  - 기존에 존재하던 시그널 핸들러에 대한 정보를 기록할 `struct sigaction` 객체의 주소값
  - NULL일 경우 기록되지 않는다.

#### Example

```c
// Prepare struct sigaction
struct sigaction handler_sig_fn;
memset(&handler_sig_fn, 0x00, sizeof(struct sigaction));
handler_sig_fn.sa_handler = &sig_fn;

// Alter SIGINT signal handler to sig_fn
if(sigaction(SIGINT, &handler_sig_fn, NULL) < 0) {
  fprintf(stderr, "Failed to set SIGINT handler\n");
  return -1;
}
```

위 코드를 빌드한 프로그램은, 끊임없이 1초에 하나씩 양을 센다.

그런데 양을 세기 전에, SIGINT에 대한 핸들러를 변경시켰으므로 Ctrl-C 버튼이 눌렸을 때 sig_fn함수가 호출되게 된다.

