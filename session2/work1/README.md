# work1

## Build (CMake)

```pwsh
cmake -S . -B build
cmake --build build
```

## Run

```pwsh
./build/rm
```

## Input Format

Each command starts with a time stamp (seconds), then a command letter and identifiers:

```
<time> <cmd> <team> <id> <params...>
```

Commands:

- `A team id type hpLimit heatLimit`
	- If destroyed robot with same team/id exists and type matches: revive to normal, set hp=hpLimit, heat=0.
	- If normal robot with same team/id exists: do nothing.
	- Otherwise create new robot with level=1, hp=hpLimit, heat=0.
- `F team id hp`
	- Update hp for normal robot; if hp<=0 move to destroyed and output info.
- `H team id heat`
	- If normal robot is infantry, update heat.
- `U team id targetLevel newHpLimit newHeatLimit`
	- If normal robot is infantry and targetLevel > current level, update limits and reset hp to hpLimit.

Time handling:
- Before processing each command, all normal robots tick by $\Delta t$ seconds.
- Each second: heat decreases by 1 (to minimum 0), then if heat > limit, hp decreases by 1.
- If hp reaches 0 due to overheat or `F`, output `D team id`.

Multi-config generators (Visual Studio):

```pwsh
cmake --build build --config Debug
./build/Debug/rm.exe
```
