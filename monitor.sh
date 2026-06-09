#!/bin/bash
# partly made by GPT-5.5

file=memory0.67
p=4gewinnt
refresh=4

# Auto-quit the solver before it eats the whole machine.
# It sends SIGTERM first, waits a bit, then SIGKILLs only if needed.
auto_quit_on_ram_full=true
quit_at_ram_percent=90
quit_when_free_percent=2
quit_grace=8
quit_log=/tmp/4gewinnt_auto_quit.log
# Duplicate scans are RAM-heavy; pause them before they fight the solver.
dup_pause_at_ram_percent=65
dup_pause_when_free_percent=10

last_lines=0
last_bytes=0
last_time=0

# Exact duplicate scan is expensive on a multi-GB memory file, so it runs in
# the background only every few minutes. It uses a small C helper because
# sort/awk/Python are too slow or too RAM-heavy for live monitoring.
dup_check_interval=30
dup_state=/tmp/4gewinnt_dup_state
dup_pid_file=/tmp/4gewinnt_dup_check.pid
dup_bin=/tmp/4gewinnt_dup_check
last_dup_check=0

build_dup_checker() {
    if [ -x "$dup_bin" ]; then
        return 0
    fi

    cat > /tmp/4gewinnt_dup_check.c <<'C'
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/stat.h>

typedef struct {
    uint64_t lo;
    uint32_t hi;
    unsigned char move;
    unsigned char used;
} Entry;

static uint64_t mix64(uint64_t x) {
    x ^= x >> 30;
    x *= 0xbf58476d1ce4e5b9ULL;
    x ^= x >> 27;
    x *= 0x94d049bb133111ebULL;
    x ^= x >> 31;
    return x;
}

static uint64_t hash_key(uint64_t lo, uint32_t hi, unsigned char move) {
    return mix64(lo ^ ((uint64_t)hi << 1) ^ ((uint64_t)move << 48));
}

int main(int argc, char **argv) {
    if (argc != 2) return 2;

    struct stat st;
    if (stat(argv[1], &st) != 0) return 2;

    uint64_t estimated_lines = (uint64_t)st.st_size / 44 + 1000000;
    uint64_t cap = 1;
    while (cap < estimated_lines * 2) cap <<= 1;

    Entry *table = calloc(cap, sizeof(Entry));
    if (table == NULL) return 3;

    FILE *f = fopen(argv[1], "rb");
    if (f == NULL) return 2;

    char line[256];
    uint64_t duplicates = 0;

    while (fgets(line, sizeof(line), f) != NULL) {
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[--len] = '\0';
        if (len != 43) continue;

        uint64_t lo = 0;
        uint32_t hi = 0;
        for (int i = 0; i < 42; i++) {
            unsigned code;
            if (line[i] == '.') code = 0;
            else if (line[i] == 'M') code = 1;
            else if (line[i] == 'Y') code = 2;
            else code = 3;

            if (i < 32) lo |= ((uint64_t)code) << (2 * i);
            else hi |= ((uint32_t)code) << (2 * (i - 32));
        }

        unsigned char move = (unsigned char)(line[42] - '0');
        uint64_t idx = hash_key(lo, hi, move) & (cap - 1);

        while (table[idx].used) {
            if (table[idx].lo == lo && table[idx].hi == hi && table[idx].move == move) {
                duplicates++;
                goto next_line;
            }
            idx = (idx + 1) & (cap - 1);
        }

        table[idx].used = 1;
        table[idx].lo = lo;
        table[idx].hi = hi;
        table[idx].move = move;

next_line:
        ;
    }

    fclose(f);
    printf("%llu\n", (unsigned long long)duplicates);
    return 0;
}
C

    clang -O3 -march=native /tmp/4gewinnt_dup_check.c -o "$dup_bin" 2>/dev/null
}

quit_task() {
    reason=$1
    targets=$2

    {
        echo "$(date '+%Y-%m-%d %H:%M:%S') auto-quitting $p: $reason"
        echo "pids: $targets"
    } >> "$quit_log"

    kill -TERM $targets 2>/dev/null
    sleep "$quit_grace"

    still_running=""
    for pid in $targets; do
        if kill -0 "$pid" 2>/dev/null; then
            still_running="$still_running $pid"
        fi
    done

    if [ -n "$still_running" ]; then
        echo "$(date '+%Y-%m-%d %H:%M:%S') force-killing:$still_running" >> "$quit_log"
        kill -KILL $still_running 2>/dev/null
    fi
}

start_dup_check() {
    now_for_dup=$1

    if [ "$dup_check_interval" -le 0 ] || [ ! -f "$file" ]; then
        return
    fi

    if [ -f "$dup_pid_file" ] && kill -0 "$(cat "$dup_pid_file")" 2>/dev/null; then
        return
    fi

    if [ -f "$dup_state" ] && [ $((now_for_dup - last_dup_check)) -lt "$dup_check_interval" ]; then
        return
    fi

    last_dup_check=$now_for_dup

    (
        start_time=$(date +%s)
        scan_bytes=$(stat -f%z "$file" 2>/dev/null || echo 0)
        if build_dup_checker; then
            result=$("$dup_bin" "$file" 2>/dev/null)
            rc=$?
            end_time=$(date +%s)
            if [ "$rc" -eq 0 ]; then
                {
                    echo "duplicates=$result"
                    echo "checked_at=$(date '+%H:%M:%S')"
                    echo "duration=$((end_time - start_time))s"
                    echo "file_bytes=$scan_bytes"
                    echo "status=ok"
                } > "$dup_state"
            elif [ "$rc" -eq 3 ]; then
                {
                    echo "duplicates=unknown"
                    echo "checked_at=$(date '+%H:%M:%S')"
                    echo "duration=$((end_time - start_time))s"
                    echo "file_bytes=$scan_bytes"
                    echo "status=not enough RAM for duplicate scan"
                } > "$dup_state"
            else
                {
                    echo "duplicates=unknown"
                    echo "checked_at=$(date '+%H:%M:%S')"
                    echo "duration=$((end_time - start_time))s"
                    echo "file_bytes=$scan_bytes"
                    echo "status=duplicate scan failed"
                } > "$dup_state"
            fi
        else
            {
                echo "duplicates=unknown"
                echo "checked_at=$(date '+%H:%M:%S')"
                echo "duration=0s"
                echo "file_bytes=$scan_bytes"
                echo "status=clang missing or compile failed"
            } > "$dup_state"
        fi
    ) &

    echo $! > "$dup_pid_file"
}

while true
do

    now=$(date +%s)

    if [ -f "$file" ]
    then
        lines=$(wc -l < "$file" | tr -d ' ')
        file_size=$(du -h "$file" | cut -f1)
        bytes=$(stat -f%z "$file")
    else
        lines=0
        file_size="missing"
        bytes=0
    fi

    seconds=$((now - last_time))
    if [ "$last_time" -gt 0 ] && [ "$seconds" -gt 0 ]
    then
        lines_per_sec=$(awk -v d=$((lines - last_lines)) -v s="$seconds" 'BEGIN { printf "%.1f/s", d / s }')
        mb_per_sec=$(awk -v d=$((bytes - last_bytes)) -v s="$seconds" 'BEGIN { printf "%.2f MB/s", d / s / 1000 / 1000 }')
    else
        lines_per_sec="0.0/s"
        mb_per_sec="0.00 MB/s"
    fi

    last_lines=$lines
    last_bytes=$bytes
    last_time=$now

    if [ -f "$dup_state" ]; then
        dupes=$(awk -F= '/^duplicates=/ { print $2 }' "$dup_state")
        dup_checked_at=$(awk -F= '/^checked_at=/ { print $2 }' "$dup_state")
        dup_duration=$(awk -F= '/^duration=/ { print $2 }' "$dup_state")
        dup_file_bytes=$(awk -F= '/^file_bytes=/ { print $2 }' "$dup_state")
        dup_status=$(awk -F= '/^status=/ { print $2 }' "$dup_state")
    else
        dupes="unknown"
        dup_checked_at="never"
        dup_duration="-"
        dup_file_bytes="0"
        dup_status="waiting for first scan"
    fi

    if [ -f "$dup_pid_file" ] && kill -0 "$(cat "$dup_pid_file")" 2>/dev/null; then
        dup_status="checking... last: $dup_status"
    elif [ "$dup_file_bytes" != "$bytes" ]; then
        dup_status="stale: $dup_status"
    fi

    pids=$(pgrep -x "$p")
    total_ram_bytes=$(sysctl -n hw.memsize 2>/dev/null || echo 0)

    if [ -n "$pids" ]
    then
        proc_count=$(echo "$pids" | wc -l | tr -d ' ')
        cpu=$(ps -o pcpu= -p $pids | awk '{ sum += $1 } END { printf "%.1f%%", sum }')
        rss_kib=$(ps -o rss= -p $pids | awk '{ sum += $1 } END { printf "%.0f", sum }')
        rss=$(awk -v kb="$rss_kib" 'BEGIN { printf "%.2f GiB", kb / 1024 / 1024 }')
        proc_phys_bytes=$(footprint -f bytes $pids 2>/dev/null | awk '/phys_footprint:/ { sum += $2 } END { printf "%.0f", sum }')
        if [ -z "$proc_phys_bytes" ] || [ "$proc_phys_bytes" = "0" ]; then
            proc_phys_bytes=$((rss_kib * 1024))
        fi
        ram=$(awk -v bytes="$proc_phys_bytes" 'BEGIN { printf "%.2f GB", bytes / 1000 / 1000 / 1000 }')
        proc_ram_percent=$(awk -v used="$proc_phys_bytes" -v total="$total_ram_bytes" 'BEGIN { if (total > 0) printf "%.1f", used * 100 / total; else printf "0.0" }')
        first_pid=$(echo "$pids" | head -1)
        runtime=$(ps -o etime= -p "$first_pid" | awk '{ gsub(/^ +| +$/, ""); print }')
    else
        proc_count=0
        cpu="0.0%"
        rss="0.00 GiB"
        rss_kib=0
        proc_phys_bytes=0
        ram="0.00 GB"
        proc_ram_percent="0.0"
        runtime="not running"
    fi

    mem_free=$(memory_pressure -Q 2>/dev/null | awk -F': ' '/System-wide memory free percentage/ { print $2 }')
    [ -z "$mem_free" ] && mem_free="unknown"
    mem_free_percent=$(printf "%s" "$mem_free" | tr -cd '0-9.')

    swap_used=$(sysctl -n vm.swapusage 2>/dev/null | awk '{ print $6 }')
    [ -z "$swap_used" ] && swap_used="unknown"

    dup_guard_reason=""
    if [ -n "$pids" ] && awk -v used="$proc_ram_percent" -v limit="$dup_pause_at_ram_percent" 'BEGIN { exit !(used >= limit) }'; then
        dup_guard_reason="task already uses ${proc_ram_percent}% RAM"
    elif [ -n "$mem_free_percent" ] && awk -v free="$mem_free_percent" -v limit="$dup_pause_when_free_percent" 'BEGIN { exit !(free <= limit) }'; then
        dup_guard_reason="only ${mem_free} memory free"
    fi

    if [ -n "$dup_guard_reason" ]; then
        if [ -f "$dup_pid_file" ] && kill -0 "$(cat "$dup_pid_file")" 2>/dev/null; then
            kill -TERM "$(cat "$dup_pid_file")" 2>/dev/null
        fi
        dup_status="paused: $dup_guard_reason; $dup_status"
    else
        start_dup_check "$now"
    fi

    auto_quit_status="armed at ${quit_at_ram_percent}% task RAM or ${quit_when_free_percent}% free"
    if [ "$auto_quit_on_ram_full" = true ] && [ -n "$pids" ]; then
        quit_reason=""

        if awk -v used="$proc_ram_percent" -v limit="$quit_at_ram_percent" 'BEGIN { exit !(used >= limit) }'; then
            quit_reason="task uses ${proc_ram_percent}% of total RAM"
        elif [ -n "$mem_free_percent" ] && \
             awk -v free="$mem_free_percent" -v limit="$quit_when_free_percent" 'BEGIN { exit !(free <= limit) }' && \
             awk -v used="$proc_ram_percent" 'BEGIN { exit !(used >= 50) }'; then
            quit_reason="system memory free is ${mem_free}; task uses ${proc_ram_percent}% of total RAM"
        fi

        if [ -n "$quit_reason" ]; then
            auto_quit_status="TRIGGERED: $quit_reason"
            quit_task "$quit_reason" "$pids"
        fi
    elif [ "$auto_quit_on_ram_full" != true ]; then
        auto_quit_status="disabled"
    fi

    disk_free=$(df -h . | awk 'NR == 2 { print $4 }')

    clear

    echo "-----------------------------------------------"
    echo "total lines:"
    figlet "$lines"
    echo "-----------------------------------------------"
    echo "file size:"
    figlet "$file_size"
    echo "-----------------------------------------------"
    echo "used RAM:"
    printf "\033[31m"
    figlet "$ram"
    printf "\033[0m"
    echo "-----------------------------------------------"
    echo "processes:       $proc_count"
    echo "process runtime: $runtime"
    echo "cpu usage:       $cpu"
    echo "rss/btop RAM:    $rss"
    echo "activity RAM:    $ram"
    echo "task RAM share:  ${proc_ram_percent}%"
    echo "auto quit:       $auto_quit_status"
    printf "line growth:     \033[33m$lines_per_sec\033[0m\n"
    echo "file growth:     $mb_per_sec"
    echo "duplicates:      $dupes"
    echo "dup check:       $dup_status ($dup_checked_at, $dup_duration)"
    echo "disk free:       $disk_free"
    echo "memory free:     $mem_free"
    echo "swap used:       $swap_used"
    echo "refresh:         ${refresh}s"
    echo "-----------------------------------------------"

    sleep "$refresh"

done
