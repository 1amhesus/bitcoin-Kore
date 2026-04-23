# PID / pidfile (bitcoind 초기화 흐름)

## PID란?
PID(Process ID)는 운영체제가 실행중인 프로세스(프로그램 인스턴스)에 부여하는 고유한 숫자이다.
- Linux/macOS: `ps`, `top`에서 확인 가능
- Windows: 작업 관리자/프로세스 API로 확인 가능

## pidfile이란?
pidfile은 “현재 실행중인 프로세스의 PID를 파일에 기록”한 것이다.
데몬/서버 프로그램에서 다음 목적에 사용된다.
- 실행 여부 확인(이미 떠 있는지)
- 종료/재시작 시 대상 프로세스 식별
- 외부 관리 도구(systemd, 스크립트 등)가 상태를 추적

bitcoind는 기본적으로 `bitcoind.pid` 파일을 생성할 수 있다.

## 코드 위치
- `BITCOIN_PID_FILENAME = "bitcoind.pid"`
- `GetPidFile(args)` : pidfile 경로 결정
- `CreatePidFile(args)` : pidfile 생성 및 PID 기록
- `g_generated_pid` : “이번 프로세스가 pidfile을 직접 만들었는지” 표시

## GetPidFile(args): 경로 결정 규칙
`GetPidFile`은 `-pid` 옵션을 기준으로 pidfile 경로를 확정한다.
- 사용자가 `-pid=<path>`를 지정하면 그 값을 사용
- 지정하지 않으면 기본 파일명 `bitcoind.pid`를 사용
- 이후 `AbsPathForConfigVal(...)`로 절대경로로 정규화한다

목적: 상대경로/설정값이 섞여도 최종적으로 “실제 파일 시스템상 경로”를 일관되게 얻기 위함.

## CreatePidFile(args): 생성/기록 규칙
### 1) pidfile 비활성화 옵션
`args.IsArgNegated("-pid")`가 true면 pidfile 생성을 건너뛴다.
- “pidfile을 만들지 않겠다”는 사용자 의도 존중

### 2) 파일 생성 및 PID 기록
`std::ofstream`으로 pidfile을 열고(없으면 생성) PID를 한 줄로 기록한다.
- Windows: `GetCurrentProcessId()`
- Unix(Linux/macOS): `getpid()`

기록 포맷은 `%d\n` (정수 PID + 줄바꿈)

### 3) g_generated_pid의 의미(중요)
pidfile 기록에 성공하면 `g_generated_pid = true`로 설정한다.
이 플래그는 종료(shutdown) 시점에
- “내가 만든 pidfile이면 지운다”
- “내가 만든게 아니면 함부로 지우지 않는다”
라는 안전 규칙을 구현하기 위한 장치다.

(예: 다른 프로세스/관리 도구가 만든 pidfile을 잘못 지우는 사고 방지)

### 4) 실패 시 정책
pidfile 생성에 실패하면 `InitError(...)`로 초기화 실패 처리한다.
에러 메시지에는
- pidfile 경로
- errno 기반 시스템 에러 문자열
이 포함된다.

의미: 운영/관리 관점에서 pidfile 생성 실패를 “경고로만 넘기지 않고” 초기화 단계에서 명확히 실패시키는 선택.

## 운영 팁 / 흔한 실패 원인
- 경로 권한 문제(쓰기 불가 디렉토리)
- 데이터 디렉토리/상대경로 해석 착오
- 파일 시스템 오류(디스크 full 등)

## 다음으로 확인할 것(흐름 연결)
- `g_generated_pid`가 실제로 어디서 참조되어 pidfile 삭제 여부를 결정하는지
- shutdown 시 Interrupt/Stop/Flush 순서에서 pidfile 제거 타이밍이 어디인지
