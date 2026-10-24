# DAA Assignment: 0/1 Knapsack using Dynamic Programming (C)

This project solves multiple **0/1 Knapsack** test cases in C, prints an ASCII DP-table visualization to the terminal as each item is processed, and stores the maximum value for every test case in `result.txt`.

## Input format

Each test case begins with `#Test`. The first numeric line contains `n W` (number of items and knapsack capacity). The next `n` lines contain `weight value` pairs.

```text
#Test
4 10
5 60
3 50
4 70
2 30
```

Multiple test cases can be placed in the same input file. The supplied `input.txt` contains all 10 provided test cases.

## Compile and run

### Windows (GCC / MinGW in VS Code terminal)

```powershell
gcc -std=c11 -Wall -Wextra -O2 knapsack.c -o knapsack.exe
.\knapsack.exe input.txt
```

### Linux / macOS

```bash
gcc -std=c11 -Wall -Wextra -O2 knapsack.c -o knapsack
./knapsack input.txt
```

If you run the program without an argument, it attempts to open `input.txt` in the current directory. If that file does not exist, it reads from standard input.

## Terminal visualization

After processing each item, the program prints a snapshot of the DP table built so far. Rows represent the number of items considered, and columns represent capacities from `0` through `W`.

The recurrence is:

\[
DP[i][c] = \begin{cases}
DP[i-1][c], & w_i > c \\
\max(DP[i-1][c],\ v_i + DP[i-1][c-w_i]), & w_i \le c
\end{cases}
\]

Each item can be selected at most once, because the transition uses the previous row `DP[i-1]` when including an item.

## Output file

The program creates/overwrites `result.txt` in the current working directory. It records each test case's `n W` input header and maximum total value, for example:

```text
#Test 1 | Input (n W): 4 10 | Result: 150
```

## Complexity

For `n` items and capacity `W`:

- **Time:** `O(nW)` for the DP computation.
- **Space:** `O(nW)` for the DP table, which is retained to support the table visualization.

## Upload to GitHub

1. Create a new empty repository on GitHub, for example `daa-0-1-knapsack`.
2. Open a terminal in this project folder and run:

```bash
git init
git add knapsack.c input.txt result.txt README.md
git commit -m "Add 0/1 knapsack DP visualization in C"
git branch -M main
git remote add origin https://github.com/YOUR-USERNAME/daa-0-1-knapsack.git
git push -u origin main
```

Replace `YOUR-USERNAME` with your GitHub username. The repository URL will then be `https://github.com/YOUR-USERNAME/daa-0-1-knapsack`.
