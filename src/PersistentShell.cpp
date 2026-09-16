/*
 * src/PersistentShell.cpp
 * ════════════════════════════════════════════════════════════════════
 * Implementation of PersistentShell.
 * See common/PersistentShell.hpp for full API documentation.
 *
 * No networking, sockets, or remote communication.
 *
 * Pipe layout
 * ───────────
 *
 *   stdin_pipe[1]  (parent write-end, stored as stdin_fd_)
 *       ──▶  stdin_pipe[0]  (child read-end)  ──▶  dup2 → fd 0
 *
 *   stdout_pipe[0] (parent read-end,  stored as stdout_fd_)
 *       ◀──  stdout_pipe[1] (child write-end) ◀──  dup2 ← fd 1
 *
 *   stderr_pipe[0] (parent read-end,  stored as stderr_fd_)
 *       ◀──  stderr_pipe[1] (child write-end) ◀──  dup2 ← fd 2
 *
 *   After fork(), each side closes the ends it does NOT own.
 *   Failing to do this prevents read() from ever seeing EOF because
 *   the write-end stays open in the parent's fd table.
 */

#include "PersistentShell.hpp"

#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>

#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

/* ── Logging helpers ──────────────────────────────────────────────── */
/*
 * Consistent with the project-wide colour convention used in
 * common/common.hpp and the Python operator console.
 */
#define LOG_INFO(msg)  std::cout << "\033[1;32m[+] \033[0m" << msg << "\n"
#define LOG_WARN(msg)  std::cout << "\033[1;33m[!] \033[0m" << msg << "\n"
#define LOG_ERR(msg)   std::cout << "\033[1;31m[-] \033[0m" << msg << "\n"
#define LOG_DBG(msg)   std::cout << "\033[1;35m[*] \033[0m" << msg << "\n"

/* ══════════════════════════════════════════════════════════════════
 * Internal helpers
 * ══════════════════════════════════════════════════════════════ */

/* set_nonblocking — set O_NONBLOCK on fd so read() returns EAGAIN
 * instead of blocking when the pipe buffer is empty.               */
void PersistentShell::set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("PersistentShell: fcntl F_GETFL");
        return;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
        perror("PersistentShell: fcntl F_SETFL");
}

/* release_fds — close all three pipe fds and mark them -1.
 * Safe to call if some fds are already -1.                        */
void PersistentShell::release_fds() {
    if (stdin_fd_  != -1) { close(stdin_fd_);  stdin_fd_  = -1; }
    if (stdout_fd_ != -1) { close(stdout_fd_); stdout_fd_ = -1; }
    if (stderr_fd_ != -1) { close(stderr_fd_); stderr_fd_ = -1; }
}

/* ══════════════════════════════════════════════════════════════════
 * Move semantics
 * ══════════════════════════════════════════════════════════════ */

PersistentShell::PersistentShell(PersistentShell&& other) noexcept
    : pid_      (other.pid_)
    , stdin_fd_ (other.stdin_fd_)
    , stdout_fd_(other.stdout_fd_)
    , stderr_fd_(other.stderr_fd_)
    , alive_    (other.alive_)
{
    /* Leave other in a safe, stopped state */
    other.pid_       = -1;
    other.stdin_fd_  = -1;
    other.stdout_fd_ = -1;
    other.stderr_fd_ = -1;
    other.alive_     = false;
}

PersistentShell& PersistentShell::operator=(PersistentShell&& other) noexcept {
    if (this != &other) {
        stop();   /* clean up any existing child first */
        pid_       = other.pid_;
        stdin_fd_  = other.stdin_fd_;
        stdout_fd_ = other.stdout_fd_;
        stderr_fd_ = other.stderr_fd_;
        alive_     = other.alive_;
        other.pid_       = -1;
        other.stdin_fd_  = -1;
        other.stdout_fd_ = -1;
        other.stderr_fd_ = -1;
        other.alive_     = false;
    }
    return *this;
}

/* ══════════════════════════════════════════════════════════════════
 * Destructor
 * ══════════════════════════════════════════════════════════════ */

PersistentShell::~PersistentShell() {
    stop();
}

/* ══════════════════════════════════════════════════════════════════
 * start()
 * ══════════════════════════════════════════════════════════════ */

bool PersistentShell::start() {
    if (alive_) {
        LOG_WARN("PersistentShell::start() called on already-running shell.");
        return true;
    }

    /* ── Create three unidirectional pipes ─────────────────────── */
    int stdin_pipe[2], stdout_pipe[2], stderr_pipe[2];

    if (pipe(stdin_pipe) == -1) {
        perror("PersistentShell: pipe(stdin)");
        return false;
    }
    if (pipe(stdout_pipe) == -1) {
        perror("PersistentShell: pipe(stdout)");
        close(stdin_pipe[0]); close(stdin_pipe[1]);
        return false;
    }
    if (pipe(stderr_pipe) == -1) {
        perror("PersistentShell: pipe(stderr)");
        close(stdin_pipe[0]);  close(stdin_pipe[1]);
        close(stdout_pipe[0]); close(stdout_pipe[1]);
        return false;
    }

    LOG_INFO("PersistentShell: 3 pipes created (stdin / stdout / stderr).");

    /* ── fork() ─────────────────────────────────────────────────── */
    pid_t pid = fork();

    if (pid < 0) {
        perror("PersistentShell: fork");
        close(stdin_pipe[0]);  close(stdin_pipe[1]);
        close(stdout_pipe[0]); close(stdout_pipe[1]);
        close(stderr_pipe[0]); close(stderr_pipe[1]);
        return false;
    }

    /* ── Child ──────────────────────────────────────────────────── */
    if (pid == 0) {
        /* Rewire standard streams to pipe ends */
        if (dup2(stdin_pipe[0],  STDIN_FILENO)  == -1 ||
            dup2(stdout_pipe[1], STDOUT_FILENO) == -1 ||
            dup2(stderr_pipe[1], STDERR_FILENO) == -1)
        {
            perror("PersistentShell: dup2");
            _exit(1);
        }

        /* Close ALL raw pipe fds — child only needs the dup2'd streams */
        close(stdin_pipe[0]);  close(stdin_pipe[1]);
        close(stdout_pipe[0]); close(stdout_pipe[1]);
        close(stderr_pipe[0]); close(stderr_pipe[1]);

        /* Replace process image — -s tells bash to read from stdin */
        execl("/bin/bash", "bash", "-s", nullptr);

        /* execl() only returns on failure */
        perror("PersistentShell: execl");
        _exit(1);
    }

    /* ── Parent ─────────────────────────────────────────────────── */
    /* Close the ends that belong exclusively to the child */
    close(stdin_pipe[0]);   /* parent never reads from its own stdin pipe  */
    close(stdout_pipe[1]);  /* parent never writes to the child's stdout   */
    close(stderr_pipe[1]);  /* parent never writes to the child's stderr   */

    pid_       = pid;
    stdin_fd_  = stdin_pipe[1];   /* parent writes commands here  */
    stdout_fd_ = stdout_pipe[0];  /* parent reads stdout from here */
    stderr_fd_ = stderr_pipe[0];  /* parent reads stderr from here */
    alive_     = true;

    /* Make reads non-blocking — required for poll()-based drain */
    set_nonblocking(stdout_fd_);
    set_nonblocking(stderr_fd_);

    LOG_INFO("PersistentShell: child PID " + std::to_string(pid) +
             " launched (/bin/bash -s).");
    return true;
}

/* ══════════════════════════════════════════════════════════════════
 * is_alive()
 * ══════════════════════════════════════════════════════════════ */

bool PersistentShell::is_alive() const {
    return alive_;
}

/* ══════════════════════════════════════════════════════════════════
 * write_input()
 * ══════════════════════════════════════════════════════════════ */

bool PersistentShell::write_input(const std::string& input) {
    if (!alive_) {
        LOG_ERR("PersistentShell::write_input: shell is not alive.");
        return false;
    }

    /* ── Write the caller's command ─────────────────────────────── */
    std::string line = input;
    if (line.empty() || line.back() != '\n') line += '\n';

    const char* ptr = line.data();
    size_t      rem = line.size();
    while (rem > 0) {
        ssize_t n = write(stdin_fd_, ptr, rem);
        if (n > 0) {
            ptr += n;
            rem -= static_cast<size_t>(n);
        } else if (n < 0) {
            if (errno == EINTR) continue;   /* interrupted — retry */
            if (errno == EPIPE) {
                LOG_WARN("PersistentShell::write_input: broken pipe (child exited).");
                alive_ = false;
                return false;
            }
            perror("PersistentShell: write(command)");
            return false;
        }
    }

    /* ── Write the sentinel command ─────────────────────────────── */
    /*
     * After the user's command bash will execute:
     *   echo "__GP_SHELL_DONE__"
     * read_available_output() scans stdout for this exact line
     * and uses it as the definitive end-of-output signal.
     * The sentinel line is stripped before returning to the caller.
     */
    std::string sentinel_cmd =
        std::string("echo \"") + SENTINEL + "\"\n";

    ptr = sentinel_cmd.data();
    rem = sentinel_cmd.size();
    while (rem > 0) {
        ssize_t n = write(stdin_fd_, ptr, rem);
        if (n > 0) {
            ptr += n;
            rem -= static_cast<size_t>(n);
        } else if (n < 0) {
            if (errno == EINTR) continue;
            perror("PersistentShell: write(sentinel)");
            return false;
        }
    }

    LOG_DBG("PersistentShell: wrote command + sentinel to child stdin.");
    return true;
}

/* ══════════════════════════════════════════════════════════════════
 * read_available_output()
 * ══════════════════════════════════════════════════════════════ */

/*
 * How completion is detected
 * ──────────────────────────
 * write_input() appended  echo "__GP_SHELL_DONE__"  after the
 * caller's command.  We accumulate stdout in a string and scan for
 * a line that contains only SENTINEL.  When found:
 *   • the sentinel line is removed from the output string
 *   • the function returns
 *
 * Fallback: if poll() times out POLL_TIMEOUT_MS ms without the
 * sentinel appearing (e.g. a very slow command), we return whatever
 * has accumulated so far — the caller receives partial output.
 *
 * Child exit: detected via waitpid(WNOHANG) at the top of each
 * loop iteration, or via read() returning 0 (EOF on pipe), or via
 * POLLHUP / POLLERR.
 */
std::string PersistentShell::read_available_output() {
    std::string stdout_buf;   /* accumulates raw stdout         */
    std::string stderr_buf;   /* accumulates raw stderr         */
    char        raw[READ_BUF_SIZE];

    const std::string sentinel_line = std::string(SENTINEL) + "\n";

    while (true) {
        /* ── Non-blocking child-exit check ──────────────────────── */
        int    wstatus = 0;
        pid_t  r       = waitpid(pid_, &wstatus, WNOHANG);
        if (r == pid_) {
            alive_ = false;
            LOG_WARN("PersistentShell: child exited (code " +
                     std::to_string(WEXITSTATUS(wstatus)) + ").");
            /* Drain any residual bytes before returning */
            ssize_t n;
            while ((n = read(stdout_fd_, raw, READ_BUF_SIZE)) > 0)
                stdout_buf.append(raw, static_cast<size_t>(n));
            while ((n = read(stderr_fd_, raw, READ_BUF_SIZE)) > 0)
                stderr_buf.append(raw, static_cast<size_t>(n));
            break;
        }

        /* ── Check: is the sentinel already in the buffer? ──────── */
        auto pos = stdout_buf.find(sentinel_line);
        if (pos != std::string::npos) {
            /* Remove everything from the sentinel line onwards */
            stdout_buf.erase(pos);
            LOG_DBG("PersistentShell: sentinel found — output complete.");
            break;
        }

        /* ── poll() on stdout and stderr ────────────────────────── */
        struct pollfd fds[2];
        fds[0].fd = stdout_fd_; fds[0].events = POLLIN; fds[0].revents = 0;
        fds[1].fd = stderr_fd_; fds[1].events = POLLIN; fds[1].revents = 0;

        int ready = poll(fds, 2, POLL_TIMEOUT_MS);

        if (ready < 0) {
            if (errno == EINTR) {
                /* A signal (e.g. SIGCHLD) interrupted poll() — not fatal.
                 * The waitpid check at the top handles child exit. */
                LOG_DBG("PersistentShell: poll() interrupted (EINTR), retrying.");
                continue;
            }
            perror("PersistentShell: poll");
            break;
        }

        if (ready == 0) {
            /* Timeout — sentinel never arrived within POLL_TIMEOUT_MS ms.
             * This is a fallback; the sentinel approach avoids this path
             * under normal circumstances. */
            if (!stdout_buf.empty() || !stderr_buf.empty())
                LOG_WARN("PersistentShell: poll() timed out waiting for sentinel.");
            break;
        }

        /* ── Read stdout — partial-read inner loop ───────────────── */
        if (fds[0].revents & POLLIN) {
            while (true) {
                ssize_t n = read(stdout_fd_, raw, READ_BUF_SIZE);
                if (n > 0) {
                    stdout_buf.append(raw, static_cast<size_t>(n));
                } else if (n == 0) {
                    /* EOF: write-end closed (bash exited) */
                    LOG_WARN("PersistentShell: EOF on stdout — child has exited.");
                    alive_ = false;
                    break;
                } else {
                    /* n < 0 */
                    if (errno == EAGAIN || errno == EWOULDBLOCK) break; /* buffer empty */
                    if (errno == EINTR)  continue;                       /* signal, retry */
                    perror("PersistentShell: read(stdout)");
                    break;
                }
            }
        }
        if (fds[0].revents & (POLLHUP | POLLERR)) {
            LOG_WARN("PersistentShell: stdout pipe HUP/ERR.");
            alive_ = false;
        }

        /* ── Read stderr — partial-read inner loop ───────────────── */
        if (fds[1].revents & POLLIN) {
            while (true) {
                ssize_t n = read(stderr_fd_, raw, READ_BUF_SIZE);
                if (n > 0) {
                    stderr_buf.append(raw, static_cast<size_t>(n));
                } else if (n == 0) {
                    LOG_WARN("PersistentShell: EOF on stderr.");
                    break;
                } else {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                    if (errno == EINTR)  continue;
                    perror("PersistentShell: read(stderr)");
                    break;
                }
            }
        }

        if (!alive_) break;
    }

    /* ── Combine stdout and stderr ──────────────────────────────── */
    /*
     * Prefix every stderr line with "[STDERR] " so the caller can
     * distinguish error output from normal output.
     */
    std::string combined = std::move(stdout_buf);
    if (!stderr_buf.empty()) {
        /* Prefix each line */
        std::string prefixed;
        prefixed.reserve(stderr_buf.size() + 10);
        size_t start = 0;
        while (start < stderr_buf.size()) {
            size_t nl = stderr_buf.find('\n', start);
            if (nl == std::string::npos) nl = stderr_buf.size() - 1;
            prefixed += "[STDERR] ";
            prefixed += stderr_buf.substr(start, nl - start + 1);
            start = nl + 1;
        }
        combined += prefixed;
    }

    return combined;
}

/* ══════════════════════════════════════════════════════════════════
 * stop()
 * ══════════════════════════════════════════════════════════════ */

void PersistentShell::stop() {
    if (!alive_) {
        release_fds();
        return;
    }

    /* ── 1. Polite shutdown via "exit" ──────────────────────────── */
    LOG_INFO("PersistentShell: sending 'exit' to child.");
    {
        const char* cmd = "exit\n";
        ssize_t     n   = write(stdin_fd_, cmd, 5);
        (void)n;   /* best effort; child may have already closed stdin */
    }
    usleep(100'000);   /* 100 ms grace period */

    /* ── 2. SIGTERM ─────────────────────────────────────────────── */
    int wstatus = 0;
    if (waitpid(pid_, &wstatus, WNOHANG) != pid_) {
        LOG_WARN("PersistentShell: child still alive — sending SIGTERM.");
        kill(pid_, SIGTERM);
        usleep(200'000);   /* 200 ms */

        /* ── 3. SIGKILL ─────────────────────────────────────────── */
        if (waitpid(pid_, &wstatus, WNOHANG) != pid_) {
            LOG_WARN("PersistentShell: no response — sending SIGKILL.");
            kill(pid_, SIGKILL);
            waitpid(pid_, nullptr, 0);   /* blocking — must reap zombie */
        }
    }

    alive_ = false;
    pid_   = -1;
    release_fds();
    LOG_INFO("PersistentShell: child process cleaned up.");
}
