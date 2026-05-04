


=================================================================================


# BIP102: Block Size Increase to 2MB 구현 분석

## 1. BIP102 문서의 핵심

## 2. Jeff Garzik 구현 커밋 개요

## 3. 코드 변경 위치

### 3.1 src/consensus/consensus.h
### 3.2 src/main.cpp - CheckTransaction()
### 3.3 src/main.cpp - ConnectBlock()
### 3.4 src/main.cpp - CheckBlock()
### 3.5 src/bitcoin-tx.cpp

## 4. MaxBlockSize(nTime)의 의미

## 5. Sigops limit 연동 방식

## 6. 분석: 블록 크기 증가는 단순 바이트 증가가 아니라 검증 비용 조정 문제

## 7. 주의할 점


====================================================================================



# BIP102: Block Size Increase to 2MB 구현 분석

이 문서는 Jeff Garzik이 작성한 BIP102와 그 구현 커밋을 바탕으로

블록크기 제한과 sigops limit이 코드상에서 어떻게 다뤄졌는지 정리한다.


## 1. BIP102 문서의 핵심

BIP102의 문서상 핵심은 블록에 허용되는 트랜잭션 데이터의 총량을

1MB에서 2MB로 단순히 한 번 증가시키는 것이다.

BIP102의 Specification은 다음 세 가지 제안으로 요약 할 수 있다.

1. trigger point에서 `MAX_BLOCK_SIZE`를 `2,000,000 bytes`로 증가
2. maximum block sigops도 비슷한 비율로 증가시키되 `SIZE / 50` 공식 보존
3. activation 조건은 flag day block time과 최근 1,000개 블록 중 95% signaling

즉 문서상 BIP102는 “점진적 증가”보다는 1MB에서 2MB로의 단순한 hard fork 제안에 가깝다.


## 2. 구현 커밋 개요

Jeff Garzik의 BIP102 구현 커밋에서는 문서 설명보다 조금 더 동적인 구조가 있는데

기존의 `MAX_BLOCK_SIZE = 1000000` 고정 상수는 제거되고 `MaxBlockSize(uint64_t nTime)` 함수가 추가된다.

핵심 코드는 다음과 같다.

```cpp
static const uint64_t BIP102_FORK_TIME = 1462406400; // May 5 2016, midnight UTC

inline unsigned int MaxBlockSize(uint64_t nTime) {
    if (nTime < BIP102_FORK_TIME)
        return 1000*1000;

    // cap for tests
    if (nTime > 4113158400)
        nTime = 4113158400;

    return (2*1000*1000) + (20 * ((nTime - BIP102_FORK_TIME) / 600));
}
```
이 함수는 fork time 이전에는 1MB를 반환하고 fork time 이후에는 2MB에 시간 경과에 따른 

증가분을 더한 값을 반환한다.

따라서 BIP102 문서 자체는 2MB로의 단순 1회 증가 제안이지만이 구현 커밋에서는
 
시간에 따라 점진적으로 블록 크기 상한이 증가하는 구조가 포함되어 있다.


## 3. 코드 변경 위치

### 3.1 `src/consensus/consensus.h`

가장 중요한 변경은 `src/consensus/consensus.h`에 있다.

기존에는 블록 크기 hard limit이 다음과 같은 고정 상수로 정의되어 있었다.

```cpp
static const unsigned int MAX_BLOCK_SIZE = 1000000;
```

BIP102 구현 커밋에서는 이 상수가 제거되고, `MaxBlockSize(uint64_t nTime)` 함수로 대체된다.

또한 기존 sigops limit은 다음과 같이 블록 크기 상수에 비례했다.

```cpp
static const unsigned int MAX_BLOCK_SIGOPS = MAX_BLOCK_SIZE / 50;
```

구현 커밋에서는 이것도 다음 함수로 바뀐다.

```cpp
inline unsigned int MaxBlockSigops(uint64_t nTime) {
    return MaxBlockSize(nTime) / 50;
}
```

즉 블록 내 서명검증 연산수 제한인 sigops limit도 블록크기 제한 함수값에 연동되도록 설계된 것이다.

### 3.2 `src/main.cpp` - `CheckTransaction()`

`CheckTransaction()`에서는 트랜잭션 크기 제한에 사용되던 기준이 바뀐다.

기존에는 트랜잭션 직렬화 크기를 `MAX_BLOCK_SIZE`와 비교했지만 구현 커밋에서는 별도의 
`MAX_TRANSACTION_SIZE`를 추가하고 트랜잭션 크기는 이 값과 비교하도록 변경한다.

이 변경은 블록 크기 상한이 동적으로 변하더라도 개별 트랜잭션 크기 제한은 별도로 유지하려는 의도로 볼 수 있다.

### 3.3 `src/main.cpp` - `ConnectBlock()`

`ConnectBlock()`에서는 블록 내 트랜잭션들을 연결하면서 sigops 수를 누적한다.

기존에는 누적된 sigops가 `MAX_BLOCK_SIGOPS`를 넘는지 검사했다.

```cpp
if (nSigOps > MAX_BLOCK_SIGOPS)
    return state.DoS(...);
```

BIP102 구현 커밋에서는 이 비교가 다음 형태로 바뀐다.

```cpp
if (nSigOps > MaxBlockSigops(pindex->nHeight))
    return state.DoS(...);
```

다만 여기서는 주의가 필요하다. `MaxBlockSigops()`의 인자명은 `nTime`인데
이 호출부에서는 `pindex->nHeight`가 전달된다.

따라서 이 부분은 “sigops limit을 블록 크기 함수와 연동하려는 구조”로는 볼 수 있으나
시간 기반 함수에 height를 넘기는 호출이므로 조심하게 볼 필요가 있다.

### 3.4 `src/main.cpp` - `CheckBlock()`

`CheckBlock()`에서는 실제 블록 크기 제한 검사에 `MaxBlockSize(block.GetBlockTime())`가 사용된다.

기존에는 serialized block size를 `MAX_BLOCK_SIZE`와 비교했다.

BIP102 구현 커밋에서는 먼저 다음 값을 계산한다.

```cpp
unsigned int nMaxSize = MaxBlockSize(block.GetBlockTime());
```

그 뒤 블록의 트랜잭션 수와 직렬화 크기를 `nMaxSize`와 비교한다.
이 부분은 BIP102 구현에서 블록 크기 hard limit이 고정 상수에서 block time 기반 
함수값으로 바뀌었음을 가장 직접적으로 보여준다.

### 3.5 `src/bitcoin-tx.cpp`

`src/bitcoin-tx.cpp`에서는 `maxVout` 계산에 사용되던 `MAX_BLOCK_SIZE`가 
`MaxBlockSize(std::numeric_limits<uint64_t>::max())`로 대체된다.

이는 실제 블록 크기를 무제한으로 만든다는 뜻이라기보다 `MAX_BLOCK_SIZE` 고정 상수가 
사라졌기 때문에 가능한 최대 블록 크기 함수값을 사용해 보수적인 상한을 계산하려는 코드로 볼 수 있다.


## 4. 분석

BIP102 문서만 보면 블록 크기 제한을 1MB에서 2MB로 단순히 한 번 늘리는 제안처럼 보인다.

하지만 구현 커밋을 보면 단순히 `MAX_BLOCK_SIZE = 2000000`으로 바꾼 것이 아니라

 블록 크기 제한을 `MaxBlockSize(nTime)`이라는 함수로 추상화했다.

또한 sigops limit도 `MaxBlockSize(nTime) / 50`에 연동했다.

이는 블록 크기 증가는 단순히 바이트 수를 늘리는 문제가 아닌

블록 검증 비용, 특히 서명검증 연산 수 제한과 함께 조정되어야 하는 문제였음을 보여준다.


## 5. 주의할 점

첫째, BIP102 문서상 제안과 구현 커밋의 세부 내용은 구분되어야 한다. 

      문서상 BIP102는 1MB에서 2MB로의 단순한 1회 증가 제안이었으나
 
      구현 커밋에는 시간 기반 증가 함수가 포함되어 있다.

둘째, sigops는 “서명 크기”가 아니라 “서명검증 연산 수”를 의미한다.

셋째, `MaxBlockSigops(pindex->nHeight)`처럼 함수 이름과 인자 의미가 어긋나 보이는 호출부가 있다. 

      따라서 이 구현은 역사적 실험 코드로 분석하되 현재 Bitcoin Core에 반영된 규칙처럼 해석해서는 안 된다.


## 6. 요약

BIP102는 문서상으로는 1MB에서 2MB로의 단순한 block size hard limit 증가 제안이다.

그러나 Jeff Garzik의 구현 커밋에서는 `MAX_BLOCK_SIZE` 고정 상수를 `MaxBlockSize(nTime)` 함수로 대체하고

sigops limit도 `MaxBlockSize(nTime) / 50`에 연동하는 구조가 보인다.

따라서 이 구현은 블록 크기 증가 논쟁이 단순한 바이트 수 조정이 아닌, 

검증 비용과 합의 규칙 변경 방식을 함께 다루는 문제였음을 보여주는 사례로 볼 수 있다. 




