# Battleship

![CI](https://github.com/jasonrobertco/Battleship/actions/workflows/ci.yml/badge.svg)

A C++ Battleship engine with four AI player types and an automated test suite.

## Build and run

```sh
g++ -std=c++20 -Wall Board.cpp Game.cpp Player.cpp main.cpp -o battleship
./battleship
```

## Tests

```sh
g++ -std=c++20 -Wall -Wextra Board.cpp Game.cpp Player.cpp test_main.cpp -o tests

./tests          # all 57 checks
./tests --unit   # 52 deterministic checks (these gate CI)
./tests --sim    # 5 statistical match/timing checks
```

Exit code is `0` when every check passes, `1` otherwise.

## Players

| Player | Strategy |
|---|---|
| `AwfulPlayer` | Fixed placement, sequential attacks |
| `HumanPlayer` | Interactive, with input validation |
| `MediocrePlayer` | Recursive backtracking placement, random attacks |
| `GoodPlayer` | Checkerboard-parity hunt, targeted follow-up |

`GoodPlayer` runs a two-state search. In **hunt** mode it fires only on
checkerboard-parity squares, since every ship spans at least two cells — halving
the search space. On a hit it enters **target** mode, probing adjacent cells,
inferring the ship's axis from a second hit, then extending along that axis.
Misses bound the ship's extent and prune the remaining candidates.

Measured: **GoodPlayer beat MediocrePlayer 286 of 350 games** across 7 runs of 50.

## Testing

57 assertions covering ship placement and removal, attack hit/miss/sunk flags,
`block`/`unblock` behavior, display formatting in both full and shots-only modes,
`addShip` input validation, and the player factory — plus match simulations and
timing bounds.

CI runs on every push:

- **test** — builds under both `g++` and `clang++` with `-Wall -Wextra`, runs the
  deterministic suite. Blocking.
- **sanitize** — builds with AddressSanitizer and UndefinedBehaviorSanitizer.
  Blocking.
- **simulation** — runs the statistical match and timing tests. Non-blocking, since
  win counts over 50 randomized games can vary on a correct build.
