

bitcoind 샘플 초기화 스크립트(init script)와 서비스 설정(service configuration)
===============================================================================

systemd, Upstart, OpenRC를 위한 샘플 스크립트와 설정 파일은 contrib/init 폴더
에서 찾을 수 있습니다.

    contrib/init/bitcoind.service:    systemd 서비스 유닛 설정
    contrib/init/bitcoind.openrc:     OpenRC 호환 SysV 스타일 init 스크립트
    contrib/init/bitcoind.openrcconf: OpenRC conf.d 파일
    contrib/init/bitcoind.conf:       Upstart 서비스 설정 파일
    contrib/init/bitcoind.init:       CentOS 호환 SysV 스타일 init 스크립트

서비스 사용자(Servicee User))
-------------------------------------------------------------------------------

세 가지 Linux 시작 설정은 모두 "bitcoin" 사용자와 그룹이 존재한다고 가정합니다.
이 스크립트의 사용을 위한 시도하는 것이전에 해당 사용자와 그룹을 생성해야 합니다.
macOS 설정은 bitcoind가 현재 사용자를 위해 설정될 것이라고 가정합니다.

설정(Configuration)
-------------------------------------------------------------------------------

bitcoind를 데몬으로 실행하는 것에는 별도의 수동설정이 필요하지 않습니다. 인증을 
위해 특수한 쿠키를 사용하는 기본 동작을 덮어쓰기 위해 `bitcoin.conf` 설정 파일
에서 `rpcauth` 설정을 지정할 수 있습니다.

이 비밀번호는 반드시 기억하거나 직접 입력할 필요는 없습니다. 이는 주로 bitcoind
와 클라이언트 프로그램들이 설정 파일에서 읽어들이는 고정 토큰으로 사용되기 때문
입니이다. 그러나 지갑이 활성화되어 있는 경우 이 비밀번호는 지갑을 보호하는 것에 
 보안상 매우 중요하므로 강력하고 안전한 비밀번호를 사용하는 것이 권장됩니다.

bitcoind가 `-server` 플래그와 함께 실행되고(기본적으로 설정됨) `rpcpassword`가 
설정되어 있지 않다면 인증을 위해 특수한 쿠키 파일을 사용합니다. 이 쿠키는 데몬이 
시작될 때 무작위 내용으로 생성되며 데몬이 종료될 때 삭제됩다. 이 파일에 대한 읽기
접근 권한이 RPC를 통해 누가 접근할 수 있는지를 제어합니다.

기본적으로 쿠키는 데이터 디렉터리에 저장되지만 `-rpccookiefile` 옵션을 통해 그
위치를 덮어쓸 수 있습니다. 쿠키의 기본 파일 권한은 애플리케이션 전체에 적용되는 
기본 파일 `umask` 값인 `0077`을 통해 “소유자”, 즉 사용자가 읽고 쓸 수 있는 권한
으로 설정됩니다. 하지만 이 권한은 `-rpccookieperms` 옵션으로 덮어쓸 수 있습니다.

이를 통해 별도의 수동 설정 없이 bitcoind를 실행할 수 있습니다.

`conf`, `pid`, `wallet`은 상대 경로를 받을 수 있으며 이 상대 경로는 데이터 디렉
터리를 기준으로 해석됩니다. `wallet`은 *오직* 상대 경로만 지원합니다.

설정 항목들을 설명하는 예시 설정 파일을 생성하려면 
[contrib/devtools/README.md](../contrib/devtools/README.md#gen-bitcoin-confsh)
를 참고하세요.

경로(Paths)
-------------------------------------------------------------------------------

### Linux

세 가지 설정은 모두 조정이 필요할 수 있는 몇 가지 경로를 가정합니다.

    바이너리:            /usr/bin/bitcoind
    설정 파일:           /etc/bitcoin/bitcoin.conf
    데이터 디렉터리:      /var/lib/bitcoind
    PID 파일:            /var/run/bitcoind/bitcoind.pid (OpenRC 및 Upstart) 또는
                         /run/bitcoind/bitcoind.pid (systemd)
    Lock 파일:           /var/lock/subsys/bitcoind (CentOS)

PID 디렉터리(해당되는 경우)와 데이터 디렉터리는 모두 bitcoin 사용자와 그룹이 소유
해야 합니다. 보안상의 이유로 설정 파일과 데이터 디렉터리는 bitcoin 사용자와 그룹만 
읽을 수 있도록 하는 것이 권장됩니다. 그러면 bitcoin-cli 및 다른 bitcoind RPC 클라
이언트에 대한 접근은 그룹 멤버십을 통해 제어할 수 있습니다.

참고: systemd .service 파일을 사용할 경우, 앞서 언급한 디렉터리 생성과 권한 설정은
systemd가 자동으로 처리합니다. 디렉터리에는 710 권한이 부여되며 해당 디렉터리 아래
의 파일 자체가 bitcoin 그룹에 접근 권한을 부여하는 경우에 한해 bitcoin 그룹이 그 
파일에 접근할 수 있습니다. 하지만 이 권한은 디렉터리 아래의 파일 목록을 나열하는 
것은 허용하지 않습니다.

참고: 현재 systemd, OpenRC, Upstart init 파일을 그대로 사용할 경우, 
`/etc/bitcoin/bitcoin.conf`에서 `datadir`를 덮어쓰는 것은 가능하지 않습니다. 
이는 init 파일에 지정된 명령줄 옵션이 `/etc/bitcoin/bitcoin.conf`의 설정값보다 
우선하기 때문입니다. 그러나 일부 init 시스템은 init 파일에 지정된 명령줄 옵션을 
덮어쓸 수 있는 자체 설정 메커니즘을 제공합니다. 예를 들어 OpenRC에서는 
`BITCOIND_DATADIR`를 설정할 수 있습니다.

### macOS

    바이너리:            /usr/local/bin/bitcoind
    설정 파일:           ~/Library/Application Support/Bitcoin/bitcoin.conf
    데이터 디렉터리:      ~/Library/Application Support/Bitcoin
    Lock 파일:           ~/Library/Application Support/Bitcoin/.lock


서비스 설정 설치(Installing Service Configuration))
---------------------------------------------------------------------------------

### systemd

이 .service 파일을 설치하는 과정은 단순히 해당 파일을 `/usr/lib/systemd/system` 
디렉터리에 복사한 뒤 실행 중인 systemd 설정을 갱신하기 위해 `systemctl daemon-reload` 
명령을 실행하는 것으로 이루어집니다.

테스트하려면 `systemctl start bitcoind`를 실행하고 시스템 시작 시 자동으로 실행되도록
활성화하려면 `systemctl enable bitcoind`를 실행합니다.

참고: Debian/Ubuntu에서 systemd용으로 설치할 경우 .service 파일은 대신 
`/lib/systemd/system` 디렉터리에 복사해야 합니다.

### OpenRC

`bitcoind.openrc`의 이름을 `bitcoind`로 변경한 뒤 `/etc/init.d`에 넣습니다다. 소유권
과 권한을 다시 확인하고 실행 가능하도록 만듭니다. `/etc/init.d/bitcoind start`로 
테스트하고 시작 시 자동으로 실행되도록 설정하려면 `rc-update add bitcoind`를 실행합니다.

### Upstart (Debian/Ubuntu 기반 배포판용)

Upstart는 15.04보다 오래된 Debian/Ubuntu 버전의 기본 init 시스템입니다. 15.04 이상 
버전을 사용하고 있고 Upstart를 수동으로 설정하지 않았다면 대신 systemd 지침을 따라야 
합니다.

`bitcoind.conf`를 `/etc/init`에 넣습니다. `service bitcoind start`를 실행하여 테스트
합니다. 재부팅 시에는 자동으로 시작됩니다.

참고: 이 스크립트는 CentOS 5 및 Amazon Linux 2014와 호환되지 않습니다. 이들은 오래된 
버전의 Upstart를 사용하며 `start-stop-daemon` 유틸리티를 제공하지 않기 때문입니다.

### CentOS

`bitcoind.init`을 `/etc/init.d/bitcoind`로 복사합니다. `service bitcoind start`를 
실행하여 테스트합니다.

이 스크립트를 사용할 경우 `/etc/sysconfig/bitcoind` 파일에서 `BITCOIND` 및 `FLAGS` 
환경 변수를 설정하여 bitcoind 프로그램의 경로와 플래그를 조정할 수 있습니다다. 
여기에서 `DAEMONOPTS` 환경 변수도 사용할 수 있습니다.

### macOS

`org.bitcoin.bitcoind.plist`를 `~/Library/LaunchAgents`에 복사합니다. 다음 명령을 
실행하여 launch agent를 로드합니다.

    launchctl load ~/Library/LaunchAgents/org.bitcoin.bitcoind.plist

이 Launch Agent는 사용자가 로그인할 때마다 bitcoind가 시작되도록 합니다.

참고: 이 방식은 현재 사용자로 bitcoind를 실행하려는 경우를 위한 것입니다. 전용 
bitcoin 사용자와 함께 Launch Daemon으로 사용하려는 경우에는 
`org.bitcoin.bitcoind.plist`를 수정해야 합니다.


자동 재시작(Auto-respawn))
-------------------------------------------------------------------------------------

자동 재시작은 현재 Upstart와 systemd에 대해서만 설정되어 있습니다. 합리적인 기본값이 
선택되어 있지만 사용자의 환경에 따라 결과는 달라질 수 있습니다.
