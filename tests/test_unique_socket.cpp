
#include "unique_socket.hpp"

#include <iostream>
#include <utility>
#include <sys/socket.h>
#include <fcntl.h>
#include <cerrno>
#include <unistd.h>

// Simple test reporting
void check(bool condition, const char *test_name)
{
    if (condition)
        std::cout << "[PASS] " << test_name << '\n';
    else
        std::cerr << "[FAIL] " << test_name << '\n';
}

// Check whether an FD is still open
bool is_fd_open(int fd)
{
    errno = 0;
    return fcntl(fd, F_GETFD) != -1 || errno != EBADF;
}

int create_test_socket()
{
    return socket(AF_INET, SOCK_STREAM, 0);
}

int main()
{
    std::cout << "===== UniqueSocket Tests =====\n\n";

    // Test 1: Constructor and get()
    int fd1 = create_test_socket();

    {
        UniqueSocket socket1(fd1);

        check(socket1.get() == fd1,
              "Constructor and get()");

        check(is_fd_open(fd1),
              "Socket remains open while owned");
    }

    // Test 2: Destructor
    check(!is_fd_open(fd1),
          "Destructor closes socket");

    // Test 3: Move constructor
    int fd2 = create_test_socket();

    {
        UniqueSocket socket1(fd2);
        UniqueSocket socket2(std::move(socket1));

        check(socket1.get() == -1,
              "Move constructor invalidates source");

        check(socket2.get() == fd2,
              "Move constructor transfers ownership");

        check(is_fd_open(fd2),
              "Moved socket remains open");
    }

    check(!is_fd_open(fd2),
          "Moved socket closes exactly when owner is destroyed");

    // Test 4: Move assignment
    int fd3 = create_test_socket();
    int fd4 = create_test_socket();

    {
        UniqueSocket socket1(fd3);
        UniqueSocket socket2(fd4);

        socket2 = std::move(socket1);

        check(socket1.get() == -1,
              "Move assignment invalidates source");

        check(socket2.get() == fd3,
              "Move assignment transfers ownership");

        check(!is_fd_open(fd4),
              "Move assignment closes destination's old socket");
    }

    check(!is_fd_open(fd3),
          "Move-assigned socket closes on destruction");

    // Test 5: release()
    int fd5 = create_test_socket();

    {
        UniqueSocket socket1(fd5);

        int released_fd = socket1.release();

        check(released_fd == fd5,
              "release() returns original FD");

        check(socket1.get() == -1,
              "release() invalidates owner");

        check(is_fd_open(fd5),
              "release() does not close socket");

        close(released_fd);
    }

    check(!is_fd_open(fd5),
          "Released FD can be closed manually");

    // Test 6: reset()
    int fd6 = create_test_socket();
    int fd7 = create_test_socket();

    {
        UniqueSocket socket1(fd6);

        socket1.reset(fd7);

        check(socket1.get() == fd7,
              "reset() adopts new FD");

        check(!is_fd_open(fd6),
              "reset() closes previous FD");

        check(is_fd_open(fd7),
              "reset() keeps new FD open");

        socket1.reset();

        check(socket1.get() == -1,
              "reset() without argument empties owner");

        check(!is_fd_open(fd7),
              "reset() without argument closes current FD");
    }

    // Test 7: Self move assignment
    int fd8 = create_test_socket();

    {
        UniqueSocket socket1(fd8);

        socket1 = std::move(socket1);

        check(socket1.get() == fd8,
              "Self move assignment preserves ownership");

        check(is_fd_open(fd8),
              "Self move assignment keeps FD open");
    }

    check(!is_fd_open(fd8),
          "Self move-assigned socket closes on destruction");

    std::cout << "\n===== Tests Completed =====\n";

    return 0;
}
