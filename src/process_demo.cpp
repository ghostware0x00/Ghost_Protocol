/*
 * src/process_demo.cpp
 * ════════════════════════════════════════════════════════════════════
 * Minimal test driver for PersistentShell.
 *
 * Demonstrates that a single /bin/bash child process remains alive
 * across multiple write_input() / read_available_output() call pairs.
 *
 * Verified state-persistence cases
 * ──────────────────────────────────
 *  A. Working-directory state   : cd /tmp  →  pwd  (expects /tmp)
 *  B. Exported-variable state   : export DEMO_VALUE=hello  →  echo $DEMO_VALUE
 *  C. Shell-variable state      : COUNTER incremented across three calls
 *  D. Built-in arithmetic       : X=7, Y=6  →  echo $((X * Y))  (expects 42)
 *  E. PID identity check        : $$ identical at start and end of all tests
 *
 * No networking, sockets, remote communication, agent/server
 * functionality, or command-and-control functionality.
 *
 * Build (from project root, -I. so "common/" is found):
 *   g++ -std=c++17 -Wall -Wextra -I. \
 *       -o build/process_demo         \
 *       src/process_demo.cpp          \
 *       src/PersistentShell.cpp
 */

#include "PersistentShell.hpp"

#include <iostream>
#include <string>

/* ── ANSI helpers ────────────────────────────────────────────────── */
namespace Color {
    const char* RESET   = "\033[0m";
    const char* RED     = "\033[1;31m";
    const char* GREEN   = "\033[1;32m";
    const char* YELLOW  = "\033[1;33m";
    const char* CYAN    = "\033[1;36m";
    const char* MAGENTA = "\033[1;35m";
    const char* BOLD    = "\033[1m";
}

/* ── Simple test helper ──────────────────────────────────────────── */

static void print_separator(const std::string& title) {
    std::cout << Color::BOLD << Color::MAGENTA
              << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n"
              << "  " << title << "\n"
              << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
              << Color::RESET << "\n";
}

/*
 * run() — send one command, collect output, return trimmed result.
 *
 * The child is NOT restarted.  Shell state from prior calls is still
 * in effect because the same bash process handles every command.
 */
static std::string run(PersistentShell& sh,
                        const std::string& cmd,
                        bool print_output = true)
{
    std::cout << Color::CYAN << "  cmd : " << Color::RESET
              << Color::BOLD << cmd << Color::RESET << "\n";

    if (!sh.write_input(cmd)) {
        std::cout << Color::RED << "  [!] write_input failed\n" << Color::RESET;
        return {};
    }

    std::string out = sh.read_available_output();

    /* Strip trailing newlines for clean single-line display */
    std::string display = out;
    while (!display.empty() && display.back() == '\n') display.pop_back();

    if (print_output) {
        if (!display.empty())
            std::cout << Color::GREEN << "  out : " << Color::RESET << display << "\n";
        else
            std::cout << Color::YELLOW << "  out : (no output)" << Color::RESET << "\n";
    }

    return out;
}

/* ── PASS / FAIL printer ─────────────────────────────────────────── */

static bool check(bool condition,
                   const std::string& pass_msg,
                   const std::string& fail_msg)
{
    if (condition) {
        std::cout << Color::GREEN << "[+] PASS — " << Color::RESET << pass_msg << "\n";
    } else {
        std::cout << Color::RED   << "[-] FAIL — " << Color::RESET << fail_msg << "\n";
    }
    return condition;
}

/* ─────────────────────────────────────────────────────────────────
 * main()
 * ────────────────────────────────────────────────────────────── */
int main() {
    std::cout << Color::BOLD << Color::CYAN
              << "\n  Ghost Protocol — process_demo\n"
              << "  PersistentShell class / POSIX subprocess test\n"
              << Color::RESET << "\n";

    /* ── 1. Construct and start — ONE fork() + execl() ─────────── */
    PersistentShell shell;

    if (!shell.start()) {
        std::cout << Color::RED << "[-] Failed to start PersistentShell.\n"
                  << Color::RESET;
        return 1;
    }

    /* Drain any initial bash output (e.g. MOTD on some systems) */
    shell.write_input(":");   /* colon is a bash no-op */
    shell.read_available_output();

    bool all_passed = true;

    /* ══════════════════════════════════════════════════════════════
     * Setup: capture child PID from inside bash.
     * $$ expands to the PID of the running bash process.
     * We compare this to the PID reported at the end to prove the
     * same process was alive throughout all tests.
     * ════════════════════════════════════════════════════════════ */
    print_separator("Setup — capture child PID from inside bash");
    std::string pid_str = run(shell, "echo $$");
    while (!pid_str.empty() && pid_str.back() == '\n') pid_str.pop_back();

    /* ══════════════════════════════════════════════════════════════
     * Group A — working-directory persistence
     *
     * Call 1:  pwd          → records starting directory
     * Call 2:  cd /tmp      → changes directory (no output)
     * Call 3:  pwd          → MUST report /tmp
     * Call 4:  cd <original> + pwd  → restore and confirm
     *
     * If the child were recreated for each command, call 3 would
     * report the original directory instead of /tmp.
     * ════════════════════════════════════════════════════════════ */
    print_separator("Group A — working-directory persistence");

    std::string initial_cwd = run(shell, "pwd");
    while (!initial_cwd.empty() && initial_cwd.back() == '\n')
        initial_cwd.pop_back();

    run(shell, "cd /tmp", false);

    std::string new_cwd = run(shell, "pwd");
    while (!new_cwd.empty() && new_cwd.back() == '\n') new_cwd.pop_back();

    all_passed &= check(new_cwd == "/tmp",
        "cwd changed from '" + initial_cwd + "' to '/tmp'.",
        "expected /tmp, got '" + new_cwd + "'.");

    run(shell, "cd " + initial_cwd, false);
    std::string restored = run(shell, "pwd");
    while (!restored.empty() && restored.back() == '\n') restored.pop_back();

    all_passed &= check(restored == initial_cwd,
        "directory restored to '" + restored + "'.",
        "restore failed — got '" + restored + "'.");

    /* ══════════════════════════════════════════════════════════════
     * Group B — exported environment-variable persistence
     *
     * Call 1:  echo $DEMO_VALUE   → empty (not set yet)
     * Call 2:  export DEMO_VALUE=hello
     * Call 3:  echo $DEMO_VALUE   → MUST print "hello"
     * Call 4:  unset DEMO_VALUE
     * Call 5:  echo $DEMO_VALUE   → empty again
     * ════════════════════════════════════════════════════════════ */
    print_separator("Group B — environment-variable persistence");

    std::string before = run(shell, "echo $DEMO_VALUE");
    while (!before.empty() && before.back() == '\n') before.pop_back();
    all_passed &= check(before.empty(),
        "DEMO_VALUE is unset before export.",
        "DEMO_VALUE unexpectedly set to '" + before + "' before export.");

    run(shell, "export DEMO_VALUE=hello", false);

    std::string after = run(shell, "echo $DEMO_VALUE");
    while (!after.empty() && after.back() == '\n') after.pop_back();
    all_passed &= check(after == "hello",
        "DEMO_VALUE=\"" + after + "\" persisted across commands.",
        "expected 'hello', got '" + after + "'.");

    run(shell, "unset DEMO_VALUE", false);
    std::string cleared = run(shell, "echo $DEMO_VALUE");
    while (!cleared.empty() && cleared.back() == '\n') cleared.pop_back();
    all_passed &= check(cleared.empty(),
        "DEMO_VALUE is empty after unset.",
        "DEMO_VALUE still set to '" + cleared + "' after unset.");

    /* ══════════════════════════════════════════════════════════════
     * Group C — shell-variable (non-exported) persistence
     *
     * COUNTER is assigned, then incremented in two separate calls,
     * then read.  Expected value: 2.
     * ════════════════════════════════════════════════════════════ */
    print_separator("Group C — shell-variable persistence");

    run(shell, "COUNTER=0",             false);
    run(shell, "COUNTER=$((COUNTER+1))", false);
    run(shell, "COUNTER=$((COUNTER+1))", false);

    std::string counter = run(shell, "echo $COUNTER");
    while (!counter.empty() && counter.back() == '\n') counter.pop_back();
    all_passed &= check(counter == "2",
        "COUNTER=\"" + counter + "\"; increments persisted.",
        "expected '2', got '" + counter + "'.");

    /* ══════════════════════════════════════════════════════════════
     * Group D — built-in arithmetic (no child subprocess needed)
     *
     * X and Y are set in separate calls; $((X * Y)) must see both.
     * Expected: 42.
     * ════════════════════════════════════════════════════════════ */
    print_separator("Group D — built-in arithmetic");

    run(shell, "X=7", false);
    run(shell, "Y=6", false);
    std::string product = run(shell, "echo $((X * Y))");
    while (!product.empty() && product.back() == '\n') product.pop_back();
    all_passed &= check(product == "42",
        "$((X * Y)) = " + product + ".",
        "expected '42', got '" + product + "'.");

    /* ══════════════════════════════════════════════════════════════
     * Group E — PID identity check
     *
     * echo $$ at the end must match the PID captured at the start.
     * A different PID means a new process was spawned somewhere —
     * which would be a bug.
     * ════════════════════════════════════════════════════════════ */
    print_separator("Group E — single child process identity");

    std::string final_pid = run(shell, "echo $$");
    while (!final_pid.empty() && final_pid.back() == '\n') final_pid.pop_back();

    std::cout << "  PID at start  : " << pid_str   << "\n";
    std::cout << "  PID at finish : " << final_pid  << "\n";

    all_passed &= check(final_pid == pid_str,
        "same PID throughout; child was created ONCE.",
        "PID changed — a new process was spawned unexpectedly.");

    /* ── 5. Result ──────────────────────────────────────────────── */
    print_separator(all_passed ? "All tests PASSED" : "Some tests FAILED");
    if (all_passed)
        std::cout << Color::GREEN << Color::BOLD << "  Result: PASS\n" << Color::RESET;
    else
        std::cout << Color::RED   << Color::BOLD << "  Result: FAIL\n" << Color::RESET;

    /* ── 6. stop() — graceful shutdown ──────────────────────────── */
    shell.stop();

    return all_passed ? 0 : 1;
}
