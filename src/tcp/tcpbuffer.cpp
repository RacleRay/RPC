#include <cstring>
#include <memory>

#include "log.h"
#include "tcp/tcpbuffer.h"


namespace rayrpc {
    
TcpBuffer::TcpBuffer(int size) : m_size(size) {
    m_buffer.resize(size);
}

size_t TcpBuffer::getBufferSize() const noexcept {
    return m_buffer.size();
}

size_t TcpBuffer::readAble() const noexcept {
    return m_write_index - m_read_index;
}

size_t TcpBuffer::writeAble() const noexcept {
    return m_buffer.size() - m_write_index;
}

size_t TcpBuffer::readIndex() const noexcept {
    return m_read_index;
}

size_t TcpBuffer::writeIndex() const noexcept {
    return m_write_index;
}

void TcpBuffer::writeToBuffer(const char* data, size_t len) {
    if (len > writeAble()) {
        size_t new_size = static_cast<size_t>(1.5) * (m_write_index + len);
        resizeBuffer(new_size);
    }
    memcpy(&m_buffer[m_write_index], data, len);
    m_write_index += len;
}

void TcpBuffer::readFromBuffer(std::vector<char>& out, size_t len) {
    if (readAble() == 0) {
        return;
    }

    size_t read_size = readAble() > len ? len : readAble();
    std::vector<char> tmp(read_size);
    memcpy(&tmp[0], &m_buffer[m_read_index], read_size);

    out.swap(tmp);

    adjustBuffer();

    tmp.clear();
}

void TcpBuffer::resizeBuffer(size_t size) {
    std::vector<char> tmp(size);

    size_t count = std::min(size, readAble());
    memcpy(&tmp[0], &m_buffer[m_read_index], count);

    m_buffer.swap(tmp);

    m_read_index = 0;
    m_write_index = count + m_read_index;

    tmp.clear();
}

void TcpBuffer::adjustBuffer() {
    // no need adjust
    if (m_read_index < m_buffer.size() / 3) {
        return;
    }

    // get rid of buffer content had been read
    std::vector<char> tmp(m_buffer.size());
    size_t count = readAble();
    memcpy(&tmp[0], &m_buffer[m_read_index], count);
    
    m_buffer.swap(tmp);

    m_read_index = 0;
    m_write_index = m_read_index + count;

    tmp.clear();
}

void TcpBuffer::moveReadIndex(size_t distance) {
    size_t to = m_read_index + distance;
    if (to > m_buffer.size()) {
        ERRLOG("TcpBuffer::moveReadIndex : invalid move distance %ld, old_read_index %ld, buffer size %ld", distance, m_read_index, m_buffer.size());
        return;
    }
    m_read_index = to;
    adjustBuffer();
}

void TcpBuffer::moveWriteIndex(size_t distance) {
    size_t to = m_write_index + distance;
    if (to > m_buffer.size()) {
        ERRLOG("TcpBuffer::moveWriteIndex : invalid move distance %ld, old_write_index %ld, buffer size %ld", distance, m_write_index, m_buffer.size());
        return;
    }
    m_write_index = to;
    adjustBuffer();
}

}  // namespace rayrpc