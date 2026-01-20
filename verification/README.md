# Vista Formal Verification

This directory contains formal verification specifications for Vista's core functionality using **Frama-C** with the **WP (Weakest Precondition)** plugin and **Alt-Ergo** theorem prover.

## What is Formal Verification?

Unlike testing which checks specific inputs, formal verification mathematically proves that code satisfies its specification for **all possible inputs**. This provides much stronger guarantees about correctness.

## Verified Properties

The following properties have been **mathematically proven** to hold:

### Index Bounds Safety
- `selected_index` is always ≥ 0
- `selected_index` never exceeds the maximum valid index
- Navigation operations (`prev`, `next`, `up`, `down`) maintain these bounds

### View Mode Toggle
- Toggle always flips between `HORIZONTAL` and `GRID`
- Toggle is an involution (toggling twice returns to original state)

### Help Toggle
- Help state always inverts on toggle
- `show_help` is always a valid boolean

### Memory Safety
- All pointer accesses are valid
- No null pointer dereferences
- No buffer overflows

## Files

| File | Description |
|------|-------------|
| `verify_core.c` | ACSL-annotated core navigation logic (100% verified) |
| `verify_renderer.c` | Extended renderer verification with scroll logic |
| `verify.sh` | Script to run verification |

## Running Verification

### Prerequisites

1. Install opam (OCaml package manager):
   ```bash
   sudo pacman -S opam  # Arch Linux
   # or
   sudo apt install opam  # Debian/Ubuntu
   ```

2. Initialize opam and install Frama-C:
   ```bash
   opam init --auto-setup --yes
   eval $(opam env)
   opam install frama-c --yes
   why3 config detect
   ```

### Run Verification

```bash
./verify.sh
```

Or manually:
```bash
eval $(opam env)
frama-c -wp -wp-rte verify_core.c
```

## Understanding ACSL Annotations

ACSL (ANSI/ISO C Specification Language) annotations are written in special comments:

```c
/*@
  requires \valid(r);                    // Precondition: r is a valid pointer
  requires r->selected_index >= 0;       // Precondition: index is non-negative
  
  assigns r->selected_index;             // Frame condition: only this field modified
  
  ensures r->selected_index >= 0;        // Postcondition: index still non-negative
  ensures r->selected_index <= \old(r->selected_index);  // Index decreased or same
*/
void select_prev(RendererState *r);
```

### Key ACSL Constructs

| Construct | Meaning |
|-----------|---------|
| `requires P` | Precondition - must be true when function is called |
| `ensures Q` | Postcondition - guaranteed true when function returns |
| `assigns L` | Only locations in L may be modified |
| `\valid(p)` | Pointer p is valid for read/write |
| `\old(e)` | Value of expression e at function entry |
| `\result` | Return value of function |

## Results

```
[wp] Proved goals:  106 / 106
  Terminating:       8
  Unreachable:       8
  Qed:              59
  Alt-Ergo 2.6.2:   31
```

✅ **100% of proof obligations verified**

## Why This Matters

For a wallpaper switcher, formal verification ensures:

1. **No crashes from invalid array access** - The selected wallpaper index can never go out of bounds
2. **Predictable UI behavior** - Toggle operations work exactly as specified
3. **No undefined behavior** - Critical for C code that runs with user input

## Further Reading

- [Frama-C Documentation](https://frama-c.com/html/documentation.html)
- [ACSL Specification Language](https://frama-c.com/html/acsl.html)
- [WP Plugin Manual](https://frama-c.com/html/wp-manual.html)
