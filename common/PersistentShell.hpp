#pragma once
/*
 * common/PersistentShell.hpp
 * ════════════════════════════════════════════════════════════════════
 * Declaration of PersistentShell — a reusable POSIX persistent child
 * process manager.
 *
 * The class creates exactly one /bin/bash child process via fork() +
 * execl() and keeps it alive while the caller supplies multiple inputs.
 * Shell state (cwd, exported variables, shell variables) is preserved
 * across calls because the same process handles every command.
 *
 * Completion detection
 * ─────────────────────
 * A fixed-timeout heuristic ("200 ms of silence") is unreliable for
 * commands that pause or produce output slowly.  Instead, after writing
 * the caller's command this class appends an extra sentinel command:
 *
 *   echo "PERSISTENT_SHELL_DONE_<marker>"
 *
 * read_available_output() accumulates stdout until the sentinel line
 * appears, then strips it before returning the result.  The sentinel is
 * invisible to the caller.
 *
 * No networking, sockets, remote communication, agent/server
 * functionality, or command-and-control functionality.
 *
 * POSIX primitives used
 * ──────────────────────
 *   pipe(), fork(), dup2(), execl(), fcntl(O_NONBLOCK),
 *   poll(), read(), write(), waitpid(), kill(), close()
 *
 * Error handling
 * ───────────────
 *   All system calls are checked.  EINTR is retried.
 *   EAGAIN / EWOULDBLOCK signals "no more data right now".
 *   EOF (read returns 0) marks the child as no longer alive.
 *   POLLHUP / POLLERR is treated as child exit.
 *   stop() escalates: "exit" → SIGTERM → SIGKILL → waitpid.
 *   ~PersistentShell() calls stop() if the child is still alive.
 */

#include <string>
#include <sys/types.h>   /* pid_t */

class PersistentShell {
public:
    /* ── Construction / destruction ─────────────────────────────── */

    PersistentShell()  = default;
    ~PersistentShell();

    /* No copy — file descriptors and a child PID are unique resources */
    PersistentShell(const PersistentShell&)            = delete;
    PersistentShell& operator=(const PersistentShell&) = delete;

    /* Move is allowed; moved-from object is left in stopped state */
    PersistentShell(PersistentShell&& other) noexcept;
    PersistentShell& operator=(PersistentShell&& other) noexcept;

    /* ── Lifecycle ──────────────────────────────────────────────── */

    /*
     * start()
     * ───────
     * Fork /bin/bash -s, connect three pipes, set read-ends non-blocking.
     * Must be called exactly once before any other method.
     * Returns true on success, false if any system call fails.
     */
    bool start();

    /*
     * stop()
     * ──────
     * Graceful shutdown:
     *   1. Write "exit\n" and wait 100 ms.
     *   2. SIGTERM + 200 ms wait.
     *   3. SIGKILL + blocking waitpid.
     *   4. Close all pipe fds.
     * Safe to call multiple times.
     */
    void stop();

    /* ── I/O ────────────────────────────────────────────────────── */

    /*
     * write_input(input)
     * ──────────────────
     * Write one command line to the child's stdin.
     * A trailing newline is appended if absent.
     * Then writes a sentinel echo command so read_available_output()
     * knows when the command's output has ended.
     * Returns false on write error or if the shell is not alive.
     */
    bool write_input(const std::string& input);

    /*
     * read_available_output()
     * ───────────────────────
     * Poll stdout and stderr.  Accumulate data until:
     *   (a) the sentinel marker appears in stdout    — primary path
     *   (b) poll() times out (POLL_TIMEOUT_MS ms)    — fallback
     *   (c) child exits (waitpid or EOF)             — error path
     *
     * The sentinel line is removed from the returned string.
     * Stderr lines are prefixed with "[STDERR] ".
     * Returns the combined output string.
     */
    std::string read_available_output();

    /*
     * is_alive()
     * ──────────
     * Returns true if the child process is believed to be running.
     * Does NOT call waitpid; use read_available_output() to detect exit
     * during normal I/O.
     */
    bool is_alive() const;

private:
    /* ── Pipe file descriptors ──────────────────────────────────── */
    /*
     *  stdin_fd_  — parent writes commands here (write-end of stdin pipe)
     *  stdout_fd_ — parent reads child stdout   (read-end of stdout pipe)
     *  stderr_fd_ — parent reads child stderr   (read-end of stderr pipe)
     */
    pid_t pid_       = -1;
    int   stdin_fd_  = -1;
    int   stdout_fd_ = -1;
    int   stderr_fd_ = -1;
    bool  alive_     = false;

    /* ── Completion sentinel ─────────────────────────────────────── */
    /*
     * Written to the child as:  echo "SENTINEL_VALUE"
     * Detected in stdout stream; stripped before returning to caller.
     */
    static constexpr const char* SENTINEL = "__GP_SHELL_DONE__";

    /* Milliseconds to wait in poll() before treating output as finished */
    static constexpr int POLL_TIMEOUT_MS = 3000;

    /* Read buffer size per read() call */
    static constexpr int READ_BUF_SIZE = 4096;

    /* ── Internal helpers ───────────────────────────────────────── */
    static void set_nonblocking(int fd);
    void        release_fds();
};
