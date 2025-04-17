#ifndef __PIPE_H__
#define __PIPE_H__

namespace beton
{
class Pipe
{
public:
    Pipe();
    ~Pipe();
    int write(const char *buf, int len);
    int read(char *buf, int len);
    int getReadFd() const { return _pipe_fd[0]; };
    int getWriteFd() const { return _pipe_fd[1]; };
    bool isValid() const { return _pipe_fd[0] != -1 && _pipe_fd[1] != -1; };
    void reset();
private:
    void close();
    void open();
private:
    int _pipe_fd[2] = {-1, -1};
};
    
} // namespace beton
#endif  //__PIPE_H__