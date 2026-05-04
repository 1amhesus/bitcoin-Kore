# `MAX_BLOCK_SERIALIZED_SIZE` (버퍼 상한) vs `MAX_BLOCK_WEIGHT` (합의 규칙)

## 문제의식

`src/consensus/consensus.h`에는(v30 기준) 다음 상수가 함께 정의되어 있다.

- `MAX_BLOCK_SERIALIZED_SIZE = 4,000,000`
- `MAX_BLOCK_WEIGHT = 4,000,000`

겉보기에는 둘 다 “4MB”처럼 보이지만, **의미와 역할이 다르다.**

이 차이를 구분하지 않으면 “세그윗 이후 블록 크기가 4MB로 늘었다” 같은 오해가 생길 수 있다.

---

## 문서화 해결 방안

`MAX_BLOCK_SERIALIZED_SIZE = 4000000`는 ‘네트워크가 허용하는 블록의 최대치(합의 규칙)’를 정의하는 값이 아니라

프로그램이 블록 데이터를 메모리/버퍼로 읽고 처리할 때 너무 큰 입력을 미리 잘라내기 위한 “버퍼(안전) 제한”에만 쓰인다는 의미

임을 아래와 같이 더 명시적으로 설명해야 한다. 


- 합의 규칙(Consensus rule): 

  모든 노드가 “이건 유효한 블록 / 이건 무효한 블록”이라고 같이 동의해야 하는 규칙.

- 버퍼 크기 제한(Buffer size limit): 

  네트워크에서 들어오는 데이터(블록)를 “일단 읽어 들이는 과정”에서 메모리 과소비/DoS(큰 데이터로 죽이기)

  를 막기 위해 두는 로컬 방어으로 이것은 “합의 규칙”이라기보다 안전장치에 가까움.


## 원문 주석

```cpp
/** The maximum allowed size for a serialized block, in bytes (only for buffer size limits) */
static const unsigned int MAX_BLOCK_SERIALIZED_SIZE = 4000000;

/** The maximum allowed weight for a block, see BIP 141 (network rule) */
static const unsigned int MAX_BLOCK_WEIGHT = 4000000;
