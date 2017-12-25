# ARCHITECTURE

## Common

``` C
typedef struct Common {
    pid_t server_pid;  // 서버의 pid

    /*
     * 서버와 연결을 수립하거나 해제할 때 얻어야 하는 락
     */
    pthread_mutex_t reg_mutex;
    pid_t waiting;  // 서버와 연결혹은 해제하고 있는 클라이언트의 pid
    RegResult reg_result;  // 서버와의 연결혹은 해제의 결과가 저장되는 곳

    /*
     * 연결된 유저들에 대한 정보를 가리키는 해쉬컨테이너
     * 서버의 힙 공간에 실제 정보가 들어있으므로 클라이언트에서는
     * 접근하지 않는 것을 권장함
     */
    UserLink * users[USER_POOL_SIZE];
    UserLinkNode * first_user;

    /*
     * 전체 메시지가 저장되는 곳
     */
    MessageCont cont;
} Common;
```

Common은 서버와 클라이언트들이 공유하는 공유메모리가 가지고 있는 구조체입니다.

`SHM_ID` 라는 매크로 선언으로 공유메모리의 key값이 `common.h` 에 선언되어있습니다.

서버는 이 SHM_ID를 키 값으로 가지는 공유메모리 공간을 생성해서 자신이 머신에서 살아있음을 클라이언트들이 알 수 있게 해줍니다.

## Connection

### 연결요청

* 클라이언트는 연결요청을 보내기 전에 Common의 reg_mutex 락을 얻습니다.
* 그 후에 Common의 waiting을 자신의 pid값으로 설정합니다.
* 클라이언트에서 서버로 SIGUSR1 시그널을 보내고 자신은 대기합니다.
* 서버에서 SIGUSR1 시그널 핸들러가 동작해서 Common의 waiting을 pid로 가지는 프로세스가 연결요청을 하고 있다는 것을 인식하여 처리합니다.
* 서버에서는 Common의 users 해쉬 테이블과, first_user 링크드 리스트에 새로운 유저를 추가하고 해당 유저와 통신을 비동기적으로 할 수 있도록, 서버에서 보내는 용, 클라이언트에서 보내는 용으로 두 개의 메시지 큐를 생성한 후에 새로운 쓰레드 하나를 생성합니다.
  * 이 쓰레드는 클라이언트에서 보내는 용의 메시지 큐를 계속적으로 검사해서 처리합니다.
* 서버에서 Common의 reg_result에 새로 만든 2개의 메시지 큐의 번호를 저장해줍니다.
  * s_qid는 서버가 보내는 용의 메시지큐의 번호
  * c_qid는 클라이언트가 보내는 용의 메시지큐의 번호
* 그리고 서버에서 클라이언트로 SIGUSR1 시그널을 보내서 서버에서의 처리가 다 끝났음을 알려줍니다.
* 클라이언트의 SIGUSR1 시그널을 받으면 Common의 reg_result에서 서버와 통신하는 용도로 쓸 메시지큐의 번호를 읽어들이고 reg_mutex을 해제하면 연결 요청이 모두 끝이 납니다.

### 연결해제

* 클라이언트는 연결해제요청을 보내기 전에 Common의 reg_mutex 락을 얻습니다.
* 그 후에 Common의 waiting을 자신의 pid값으로 설정합니다.
* 클라이언트에서 서버로 SIGUSR2 시그널을 보내고 자신은 대기합니다.
* 서버에서 SIGUSR2 시그널 핸들러가 동작해서 Common의 waiting을 pid로 가지는 프로세스가 연결해제요청을 하고 있다는 것을 인식하여 처리합니다.
* 서버에서는 Common의 users 해쉬 테이블과, first_user 링크드 리스트에서 해당 유저를 삭제하고 두 개의 메시지 큐를 삭제한 후에 새로운 쓰레드 하나를 생성합니다.
* 그리고 서버에서 클라이언트로 SIGUSR2 시그널을 보내서 서버에서의 처리가 다 끝났음을 알려줍니다.
* 클라이언트의 SIGUSR2 시그널을 받으면 Common의 reg_result에서 서버와 통신하는 용도로 쓸 메시지큐의 번호를 읽어들이고 reg_mutex을 해제하면 연결해제 요청이 모두 끝이 납니다.

## Messaging

클라이언트에서 c_qid(클라이언트에서 보내는 용도의 메시지 큐의 번호)의 메시지큐에 `QMessage` 구조체를 건내줍니다.

표준 메시지 큐 함수에서 사용하는 type인자는 `Common.h` 에 MSG_TYPE으로 선언되어있으며 항상 이 값을 사용합니다.

`QMessage` 구조체는 `long` 타입의 type 변수와 `Message` 구조체로 이루어져있습니다.

#### Message

```C
typedef struct Message {
    int type;
    pid_t from_pid;
    pid_t to_pid;
    char msg[MSG_SIZE];
} Message;
```

메시지는 type과 보내는 사람, 받는 사람, 메시지 내용으로 구성되어있습니다

type은 표준 메시지 큐에서 사용하는 type과는 다릅니다. 여기서는 개인 메시지인지 전체 메시지인지 구분하는 용도로 사용되고 있으며, 그 값은 `MT_BROAD`, `MT_PERSON` 2가지 값중 하나로 됩니다.

`from_pid` 는 메시지를 보내는 클라이언트의 pid값이 들어가야합니다.

`to_pid` 는 이 메시지가 개인 메시지일 경우 누구에게 보내지는지를 담습니다.

`msg` 는 메시지의 내용을 담습니다.

### Broadcast message

* 클라이언트가 Message객체의 type을 MT_BROAD로 해서 c_qid 메시지 큐에 전송합니다.
* 서버는 그 메시지를 받아 Common의 cont에 메시지를 추가하고 연결된 모든 클라이언트들에게 그 내용을 알려줍니다.
* 클라이언트들은 broadcast모드일 경우 cont에 있는 메시지 내용을 재확인해서 콘솔에 업데이트합니다.

### Personal message

* 클라이언트가 Message객체의 type을 MT_PERSON으로 해서 c_qid 메시지 큐에 전송합니다.
* 서버는 그 메시지를 받아 목적지 클라이언트에게 해당 메시지를 전달합니다.
* 받는 사람은 personal 모드일 경우 해당 내용을 `MessageCont personMsg` 전역변수에 저장하고 내용을 재확인해서 콘솔에 업데이트합니다