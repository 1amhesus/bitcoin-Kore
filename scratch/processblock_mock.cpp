#include <iostream>
#include <string>

/*
    이 코드는 Bitcoin Core의 PeerManagerImpl::ProcessBlock() 흐름을
    아주 단순하게 축소해서 연습하기 위한 임시 mock 코드이다.

    내가 앞에서 학습한 핵심:
    1. ProcessBlock은 블록 검증 자체를 직접 수행하는 핵심 함수라기보다,
       ProcessNewBlock()의 결과를 받아 후처리 분기를 하는 함수에 가깝다.
    2. new_block 이라는 bool 값이 중요하며,
       이 값에 따라 if / else 분기가 갈린다.
    3. &new_block 문법은 변수의 주소를 함수에 넘겨,
       호출된 함수가 그 값을 바꾸게 하는 방식이다.
    4. 원본 코드에서 새 블록이면 block request 추적을 지우고,
       새 블록이 아니면 block source 추적을 지운다.

    이 mock 코드는 위 흐름을 C++ 문법 차원에서 직접 읽고,
    아주 간단히 실행 결과까지 확인하기 위해 작성한다.
*/

struct Block {
    std::string hash;
};

struct Node {
    int last_block_time{0};
};

/*
    ProcessNewBlockMock는 Bitcoin Core의 ProcessNewBlock()을 흉내 낸 축소 함수다.

    핵심 취지:
    - bool* new_block 매개변수를 통해 바깥 변수 값을 바꿀 수 있는지 확인한다.
    - 여기서는 단순히 should_accept 값을 new_block에 넣어준다.
*/
void ProcessNewBlockMock(bool should_accept, bool* new_block)
{
    *new_block = should_accept;
}

/*
    새 블록으로 받아들여졌을 때 호출되는 후처리 mock 함수.
    원본 코드의 RemoveBlockRequest(...) 흐름을 단순하게 흉내 낸다.
*/
void RemoveBlockRequest(const std::string& hash)
{
    std::cout << "RemoveBlockRequest: " << hash << "\n";
}

/*
    새 블록이 아닐 때 호출되는 후처리 mock 함수.
    원본 코드의 mapBlockSource.erase(...) 흐름을 단순하게 흉내 낸다.
*/
void EraseBlockSource(const std::string& hash)
{
    std::cout << "EraseBlockSource: " << hash << "\n";
}

/*
    이 함수는 Bitcoin Core의 PeerManagerImpl::ProcessBlock()을 축소해서 흉내 낸 함수다.

    핵심 흐름:
    1. new_block 지역 변수를 false로 시작한다.
    2. ProcessNewBlockMock(...)에 &new_block을 넘겨 결과를 받는다.
    3. new_block이 true면 새 블록 처리 경로로 들어간다.
    4. new_block이 false면 새 블록이 아닌 경로로 들어간다.

    여기서 중요한 학습 포인트:
    - Type& 는 참조
    - &variable 은 주소 전달
    - if (new_block) 은 bool 분기
*/
void ProcessBlockMock(Node& node, const Block& block, bool should_accept)
{
    bool new_block{false};

    ProcessNewBlockMock(should_accept, &new_block);

    if (new_block) {
        node.last_block_time = 123;
        RemoveBlockRequest(block.hash);
    } else {
        EraseBlockSource(block.hash);
    }
}

int main()
{
    Node node;
    Block block{"20260412"};

    std::cout << "[case 1] should_accept=true\n";
    ProcessBlockMock(node, block, true);

    std::cout << "[case 2] should_accept=false\n";
    ProcessBlockMock(node, block, false);

    return 0;
}
