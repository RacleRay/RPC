#pragma once

#include <memory>
#include <vector>

namespace rayrpc {

class TcpBuffer {
  public:
    using s_ptr = std::shared_ptr<TcpBuffer>;

    explicit TcpBuffer(int size);
    ~TcpBuffer() = default;
    
    size_t getBufferSize() const noexcept;

    size_t readAble() const noexcept;
    size_t writeAble() const noexcept;

    size_t readIndex() const noexcept;  // 读指针，指向可读的起始index
    size_t writeIndex() const noexcept; // 写指针，指向可写的起始index

    void writeToBuffer(const char *data, size_t len);
    void readFromBuffer(std::vector<char> &out, size_t len);

    void resizeBuffer(size_t size);
    void adjustBuffer();

    void moveReadIndex(size_t distance);
    void moveWriteIndex(size_t distance);

  private:
    size_t m_read_index{0};
    size_t m_write_index{0};
    size_t m_size{0};
  
  public:
    std::vector<char> m_buffer;
};

} // namespace rayrpc