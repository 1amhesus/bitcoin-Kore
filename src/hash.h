// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-present The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_HASH_H
#define BITCOIN_HASH_H

#include <attributes.h>
#include <crypto/common.h>
#include <crypto/ripemd160.h>
#include <crypto/sha256.h>
#include <prevector.h>
#include <serialize.h>
#include <span.h>
#include <uint256.h>

#include <string>
#include <vector>

typedef uint256 ChainCode;

/** A hasher class for Bitcoin's 256-bit hash (double SHA-256). */
class CHash256 {
private:
    CSHA256 sha;
public:
    static const size_t OUTPUT_SIZE = CSHA256::OUTPUT_SIZE;

    void Finalize(std::span<unsigned char> output) {
        assert(output.size() == OUTPUT_SIZE);
        unsigned char buf[CSHA256::OUTPUT_SIZE];
        sha.Finalize(buf);
        sha.Reset().Write(buf, CSHA256::OUTPUT_SIZE).Finalize(output.data());
    }

    CHash256& Write(std::span<const unsigned char> input) {
        sha.Write(input.data(), input.size());
        return *this;
    }

    CHash256& Reset() {
        sha.Reset();
        return *this;
    }
};

/** A hasher class for Bitcoin's 160-bit hash (SHA-256 + RIPEMD-160). */
class CHash160 {
private:
    CSHA256 sha;
public:
    static const size_t OUTPUT_SIZE = CRIPEMD160::OUTPUT_SIZE;

    void Finalize(std::span<unsigned char> output) {
        assert(output.size() == OUTPUT_SIZE);
        unsigned char buf[CSHA256::OUTPUT_SIZE];
        sha.Finalize(buf);
        CRIPEMD160().Write(buf, CSHA256::OUTPUT_SIZE).Finalize(output.data());
    }

    CHash160& Write(std::span<const unsigned char> input) {
        sha.Write(input.data(), input.size());
        return *this;
    }

    CHash160& Reset() {
        sha.Reset();
        return *this;
    }
};

/** Compute the 256-bit hash of an object. */
template<typename T>
inline uint256 Hash(const T& in1)
{
    uint256 result;
    CHash256().Write(MakeUCharSpan(in1)).Finalize(result);
    return result;
}

/** Compute the 256-bit hash of the concatenation of two objects. */
template<typename T1, typename T2>
inline uint256 Hash(const T1& in1, const T2& in2) {
    uint256 result;
    CHash256().Write(MakeUCharSpan(in1)).Write(MakeUCharSpan(in2)).Finalize(result);
    return result;
}

/** Compute the 160-bit hash an object. */
template<typename T1>
inline uint160 Hash160(const T1& in1)
{
    uint160 result;
    CHash160().Write(MakeUCharSpan(in1)).Finalize(result);
    return result;
}

// Kore note:
// HashWriter는 writer-like interface를 제공하지만 실제 목적지는 파일이나 vector가 아니라 SHA256 context다. 
// 따라서 CBlockHeader, CTransaction 같은 객체의 기존 Serialize() 정의를 그대로 사용하면서 
// 그 consensus byte layout을 해시 계산 입력으로 직접 전달할 수 있다.

/** serialization 과정에서 256-bit hash를 계산하는 writer stream. */
class HashWriter
{
private:
    CSHA256 ctx;

public:
    void write(std::span<const std::byte> src)
    {
        ctx.Write(UCharCast(src.data()), src.size());
    }

    /** 이 객체에 쓰여진 모든 데이터의 double-SHA256 hash를 계산한다.
     *
     * 이 객체는 무효화된다.
     */
    uint256 GetHash() {
        uint256 result;
        ctx.Finalize(result.begin());
        ctx.Reset().Write(result.begin(), CSHA256::OUTPUT_SIZE).Finalize(result.begin());
        return result;
    }

    /** 이 객체에 쓰여진 모든 데이터의 SHA256 hash를 계산한다.
     *
     * 이 객체는 무효화된다.
     */
    uint256 GetSHA256() {
        uint256 result;
        ctx.Finalize(result.begin());
        return result;
    }

    /**
     * 결과 hash의 앞 64 bits를 반환한다.
     */
    inline uint64_t GetCheapHash() {
        uint256 result = GetHash();
        return ReadLE64(result.begin());
    }

    template <typename T>
    HashWriter& operator<<(const T& obj)
    {
        ::Serialize(*this, obj);
        return *this;
    }
};

/** Reads data from an underlying stream, while hashing the read data. */
template <typename Source>
class HashVerifier : public HashWriter
{
private:
    Source& m_source;

public:
    explicit HashVerifier(Source& source LIFETIMEBOUND) : m_source{source} {}

    void read(std::span<std::byte> dst)
    {
        m_source.read(dst);
        this->write(dst);
    }

    void ignore(size_t num_bytes)
    {
        std::byte data[1024];
        while (num_bytes > 0) {
            size_t now = std::min<size_t>(num_bytes, 1024);
            read({data, now});
            num_bytes -= now;
        }
    }

    template <typename T>
    HashVerifier<Source>& operator>>(T&& obj)
    {
        ::Unserialize(*this, obj);
        return *this;
    }
};

/** Writes data to an underlying source stream, while hashing the written data. */
template <typename Source>
class HashedSourceWriter : public HashWriter
{
private:
    Source& m_source;

public:
    explicit HashedSourceWriter(Source& source LIFETIMEBOUND) : HashWriter{}, m_source{source} {}

    void write(std::span<const std::byte> src)
    {
        m_source.write(src);
        HashWriter::write(src);
    }

    template <typename T>
    HashedSourceWriter& operator<<(const T& obj)
    {
        ::Serialize(*this, obj);
        return *this;
    }
};

/** Single-SHA256 a 32-byte input (represented as uint256). */
[[nodiscard]] uint256 SHA256Uint256(const uint256& input);

unsigned int MurmurHash3(unsigned int nHashSeed, std::span<const unsigned char> vDataToHash);

void BIP32Hash(const ChainCode &chainCode, unsigned int nChild, unsigned char header, const unsigned char data[32], unsigned char output[64]);

/** Return a HashWriter primed for tagged hashes (as specified in BIP 340).
 *
 * The returned object will have SHA256(tag) written to it twice (= 64 bytes).
 * A tagged hash can be computed by feeding the message into this object, and
 * then calling HashWriter::GetSHA256().
 */
HashWriter TaggedHash(const std::string& tag);

/** Compute the 160-bit RIPEMD-160 hash of an array. */
inline uint160 RIPEMD160(std::span<const unsigned char> data)
{
    uint160 result;
    CRIPEMD160().Write(data.data(), data.size()).Finalize(result.begin());
    return result;
}

#endif // BITCOIN_HASH_H
