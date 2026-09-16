#pragma once
/*
 * common/shell_process.hpp
 * ════════════════════════════════════════════════════════════════════
 * Reusable POSIX persistent-subprocess management.
 *
 * Provides:
 *   ShellProcess   — POD holding the child PID and pipe fds.
 *   start_shell()  — fork()/execl() /bin/bash once; returns ShellProcess.
 *   send_command() — write one command line to the child's stdin.
 *   drain_output() — poll()-based non-blocking read of stdout + stderr.
 *   stop_shell()   — graceful shutdown (exit → SIGTERM → SIGKILL → waitpid).
 *
 * Design notes
 * ────────────
 *  • The child process is created ONCE and stays alive for the lifetime
 *    of the ShellProcess object.  Shell state (cwd, env vars, shell vars)
 *    is preserved across calls to send_command/drain_output.
 *
 *  • All pipe read-ends are set O_NONBLOCK so that drain_output() never
 *    blocks indefinitely.  poll() with POLL_TIMEOUT_MS provides a
 *    "quiescence" heuristic: if the child is silent for that many
 *    milliseconds we treat the output as complete for this command.
 *
 *  • Partial reads are handled inside drain_output(): one read() may
 *    return fewer bytes than are available; we loop until EAGAIN.
 *
 *  • EINTR is handled in both the poll() call and every read() loop.
 *
 * No networking, no sockets, no remote communication.
 *
 * Pipe layout
 * ───────────
 *   stdin_pipe[1]  (parent write-end)  ──▶  stdin_pipe[0]  (child read-end)
 *                                                │
 *                                           dup2 → STDIN  (fd 0)
 *
 *   stdout_pipe[0] (parent read-end)  ◀──  stdout_pipe[1] (child write-end)
 *                                                │
 *                                           dup2 → STDOUT (fd 1)
 *
 *   stderr_pipe[0] (parent read-end)  ◀──  stderr_pipe[1] (child write-end)
 *                                                │
 *                                           dup2 → STDERR (fd 2)
 *
 *   After fork(), each side closes the ends it doesn't own.
 *   Failing to do so prevents read() from ever seeing EOF.
 */

#include <cerrno>
#include <cstring>
#include <iostream>
#include <string>

#include <fcntl.h>
#include <poll.h>
#include <sys/wait.h>
#include <unistd.h>

/* ── Logging macros (mirror common.hpp / colors.py style) ─────────── */
/*
 * These macros are defined here only if a consumer has not already
 * defined them via common.hpp.  That way shell_process.hpp can be
 * included standalone in the demo OR alongside common.hpp in the agent
 * without causing redefinition errors.
 */
#ifndef LOG_INFO
#  define GP_SH_LOCAL_MACROS
#  define LOG_INFO(msg)  std::cout << "\033[1;32m[+] \033[0m" << msg << "\n"
#  define LOG_WARN(msg)  std::cout << "\033[1;33m[!] \033[0m" << msg << "\n"
#  define LOG_ERR(msg)   std::cout << "\033[1;31m[-] \033[0m" << msg << "\n"
#  define LOG_DATA(msg)  std::cout << "\033[1;36m[>] \033[0m" << msg << "\n"
#  define LOG_DBG(msg)   std::cout << "\033[1;35m[*] \033[0m" << msg << "\n"
#endif

/* ─────────────────────────────────────────────────────────────────── */

/* Milliseconds of child silence treated as "command output complete". */
static constexpr int GP_POLL_TIMEOUT_MS = 200;

/* Read buffer size for each read() call inside drain_output(). */
static constexpr int GP_READ_BUF = 4096;

/* ── ShellProcess ─────────────────────────────────────────────────── */
struct ShellProcess {
    pid_t pid      = -1;   /* child PID                          */
    int   stdin_w  = -1;   /* parent writes commands here        */
    int   stdout_r = -1;   /* parent reads child stdout from here */
    int   stderr_r = -1;   /* parent reads child stderr from here */
    bool  alive    = false;
};

/* ── Internal helper ──────────────────────────────────────────────── */
namespace gp_detail {

inline void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) { perror("fcntl F_GETFL"); return; }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) perror("fcntl F_SETFL");
}

} // namespace gp_detail

/* ── start_shell ──────────────────────────────────────────────────── */
/*
 * Fork a /bin/bash child connected via three pipes.
 * Returns a ShellProcess with alive=false on failure.
 * The child's stdin/stdout/stderr are fully redirected; the child
 * process never inherits a terminal.
 */
inline ShellProcess start_shell() {
    ShellProcess sh;

    int stdin_pipe[2], stdout_pipe[2], stderr_pipe[2];
    if (pipe(stdin_pipe)  == -1) { perror("pipe(stdin)");  return sh; }
    if (pipe(stdout_pipe) == -1) { perror("pipe(stdout)"); return sh; }
    if (pipe(stderr_pipe) == -1) { perror("pipe(stderr)"); return sh; }

    LOG_INFO("Created 3 pipes (stdin / stdout / stderr).");

    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return sh; }

    if (pid == 0) {
        /* ── CHILD ─────────────────────────────────────────────── */
        dup2(stdin_pipe[0],  STDIN_FILENO);
        dup2(stdout_pipe[1], STDOUT_FILENO);
        dup2(stderr_pipe[1], STDERR_FILENO);

        /* Close all raw fds — child uses only the dup2'd streams */
        close(stdin_pipe[0]);  close(stdin_pipe[1]);
        close(stdout_pipe[0]); close(stdout_pipe[1]);
        close(stderr_pipe[0]); close(stderr_pipe[1]);

        execl("/bin/bash", "bash", "-s", nullptr);
        perror("execl");
        _exit(1);
    }

    /* ── PARENT ────────────────────────────────────────────────── */
    close(stdin_pipe[0]);   /* parent never reads from its own stdin pipe  */
    close(stdout_pipe[1]);  /* parent never writes to child stdout         */
    close(stderr_pipe[1]);  /* parent never writes to child stderr         */

    sh.pid      = pid;
    sh.stdin_w  = stdin_pipe[1];
    sh.stdout_r = stdout_pipe[0];
    sh.stderr_r = stderr_pipe[0];
    sh.alive    = true;

    gp_detail::set_nonblocking(sh.stdout_r);
    gp_detail::set_nonblocking(sh.stderr_r);

    LOG_INFO("Child PID " + std::to_string(pid) + " launched (/bin/bash -s).");
    return sh;
}

/* ── send_command ─────────────────────────────────────────────────── */
/*
 * Write one command line to the child's stdin.
 * A trailing newline is appended if absent.
 * Returns false on write error or if the shell is not alive.
 */
inline bool send_command(const ShellProcess& sh, const std::string& cmd) {
    if (!sh.alive) { LOG_ERR("send_command: shell not alive."); return false; }

    std::string line = cmd;
    if (line.empty() || line.back() != '\n') line += '\n';

    ssize_t written = write(sh.stdin_w, line.data(), line.size());
    if (written < 0) { perror("write to child stdin"); return false; }

    LOG_DBG("Wrote " + std::to_string(written) + " bytes to child stdin.");
    return true;
}

/* ── drain_output ─────────────────────────────────────────────────── */
/*
 * Read all currently available output from the child's stdout and
 * stderr, returning it as a single string.
 *
 * Termination conditions (in priority order):
 *   1. waitpid(WNOHANG) reports child has exited   → sh.alive = false, break
 *   2. poll() times out (GP_POLL_TIMEOUT_MS ms of silence) → break
 *   3. poll() returns EINTR                        → retry
 *   4. read() returns EAGAIN/EWOULDBLOCK           → inner loop done
 *   5. read() returns 0 (EOF on pipe)              → sh.alive = false, break
 *
 * stderr lines are prefixed with "[STDERR] " in the returned string.
 */
inline std::string drain_output(ShellProcess& sh) {
    std::string output;
    char buf[GP_READ_BUF];

    while (true) {
        /* Non-blocking exit check — catches SIGCHLD without blocking */
        int wstatus = 0;
        pid_t r = waitpid(sh.pid, &wstatus, WNOHANG);
        if (r == sh.pid) {
            sh.alive = false;
            LOG_WARN("Child exited (code " +
                     std::to_string(WEXITSTATUS(wstatus)) + ").");
            /* Drain residual bytes before returning */
            ssize_t n;
            while ((n = read(sh.stdout_r, buf, GP_READ_BUF)) > 0)
                output.append(buf, static_cast<size_t>(n));
            while ((n = read(sh.stderr_r, buf, GP_READ_BUF)) > 0)
                output.append("[STDERR] ").append(buf, static_cast<size_t>(n));
            break;
        }

        struct pollfd fds[2];
        fds[0].fd = sh.stdout_r; fds[0].events = POLLIN; fds[0].revents = 0;
        fds[1].fd = sh.stderr_r; fds[1].events = POLLIN; fds[1].revents = 0;

        int ready = poll(fds, 2, GP_POLL_TIMEOUT_MS);

        if (ready < 0) {
            if (errno == EINTR) {
                LOG_DBG("poll() interrupted (EINTR) — retrying.");
                continue;
            }
            perror("poll");
            break;
        }

        if (ready == 0) {
            /* Quiescence: no data for GP_POLL_TIMEOUT_MS ms */
            if (!output.empty())
                LOG_DBG("Quiescence timeout — output treated as complete.");
            break;
        }

        /* ── stdout partial-read loop ─────────────────────────── */
        if (fds[0].revents & POLLIN) {
            while (true) {
                ssize_t n = read(sh.stdout_r, buf, GP_READ_BUF);
                if (n > 0) {
                    output.append(buf, static_cast<size_t>(n));
                } else if (n == 0) {
                    LOG_WARN("EOF on stdout — child exited.");
                    sh.alive = false;
                    break;
                } else {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                    if (errno == EINTR)  continue;
                    perror("read stdout");
                    break;
                }
            }
        }
        if (fds[0].revents & (POLLHUP | POLLERR)) {
            LOG_WARN("stdout pipe HUP/ERR.");
            sh.alive = false;
        }

        /* ── stderr partial-read loop ─────────────────────────── */
        if (fds[1].revents & POLLIN) {
            while (true) {
                ssize_t n = read(sh.stderr_r, buf, GP_READ_BUF);
                if (n > 0) {
                    output.append("[STDERR] ").append(buf, static_cast<size_t>(n));
                } else if (n == 0) {
                    LOG_WARN("EOF on stderr.");
                    break;
                } else {
                    if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                    if (errno == EINTR)  continue;
                    perror("read stderr");
                    break;
                }
            }
        }

        if (!sh.alive) break;
    }

    return output;
}

/* ── stop_shell ───────────────────────────────────────────────────── */
/*
 * Graceful shutdown sequence:
 *   1. Send "exit\n" and wait 100 ms for a clean exit.
 *   2. If still alive → SIGTERM + 200 ms wait.
 *   3. If still alive → SIGKILL + blocking waitpid (zombie reap).
 *   4. Close all pipe fds and reset sh to default state.
 */
inline void stop_shell(ShellProcess& sh) {
    if (!sh.alive) return;

    LOG_INFO("Sending 'exit' to child.");
    send_command(sh, "exit");
    usleep(100'000);   /* 100 ms */

    int wstatus = 0;
    if (waitpid(sh.pid, &wstatus, WNOHANG) != sh.pid) {
        LOG_WARN("Child still alive — sending SIGTERM.");
        kill(sh.pid, SIGTERM);
        usleep(200'000);   /* 200 ms */
        if (waitpid(sh.pid, &wstatus, WNOHANG) != sh.pid) {
            LOG_WARN("No response — sending SIGKILL.");
            kill(sh.pid, SIGKILL);
            waitpid(sh.pid, nullptr, 0);   /* blocking — must reap zombie */
        }
    }

    close(sh.stdin_w);
    close(sh.stdout_r);
    close(sh.stderr_r);
    sh = ShellProcess{};   /* reset all fields to defaults */

    LOG_INFO("Shell process cleaned up.");
}
