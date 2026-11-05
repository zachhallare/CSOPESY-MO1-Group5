# CSOPESY-MO1-Group5

CSOPESY OS emulator, part 1: command-line interpreter, `screen`-style process multiplexer, and a real-time FCFS / Round-Robin CPU scheduler.

Developers: Hallare, Zach · Ramos, Marga · Tan, Neil

## Build

Requires a C++20 compiler with `<barrier>` (g++ 11+ or Visual Studio 2019 16.10+) and CMake 3.16+.

```bash
cmake -B build -G "MinGW Makefiles"
cmake --build build
```

(With Visual Studio, omit `-G ...` and add `--config Release` to the build step.)

This produces `build/csopesy.exe` and `build/csopesy_tests.exe`, and copies `config.txt` next to the executable.

## Run

Run from the folder that holds `config.txt`. The emulator reads `config.txt` from the working directory on `initialize`.

```bash
cd build
./csopesy.exe
```

| Command | Where | Description |
|---|---|---|
| `initialize` | main menu | Load `config.txt` and start the scheduler (required first) |
| `scheduler-start` / `scheduler-stop` | main menu | Start / stop generating batch processes `p01`, `p02`, … |
| `screen -s <name>` | main menu | Create a process and attach to it |
| `screen -r <name>` | main menu | Re-attach to a running process |
| `screen -ls` | main menu | CPU utilization, running and finished processes |
| `report-util` | main menu | Save the `screen -ls` output to `csopesy-log.txt` |
| `process-smi` | process screen | Process info and logs (`Finished!` when done) |
| `exit` | both | Leave the process screen / quit the emulator |

## config.txt

```
num-cpu 4
scheduler "rr"
quantum-cycles 5
batch-process-freq 1
min-ins 1000
max-ins 2000
delay-per-exec 0
```

`delays-per-exec` is accepted as an alias of `delay-per-exec`. Optional extra keys:

| Key | Default | Meaning |
|---|---|---|
| `tick-delay-ms` | `10` | Real-time length of one CPU tick (0 = as fast as possible) |
| `batch-tick-ms` | `250` | Real-time length of one batch-generator tick; `batch-process-freq` counts these, so generation is paced independently of CPU speed |
| `instruction-pattern` | `"random"` | `"print-add"` makes every program alternate `PRINT("Value from: " +x)` and `ADD(x, x, 1-10)`, with `x` starting at 0 |

## Layout

```
src/
  config/     config.txt parsing and validation
  console/    REPL, main menu, process screen
  process/    instructions, random program generator, process execution
  scheduler/  clock + per-core threads, FCFS / RR
  report/     screen -ls / report-util output
  util/       time, RNG, string helpers
tests/        unit tests for instruction semantics, generator, config
```
