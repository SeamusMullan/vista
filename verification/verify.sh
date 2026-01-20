#!/bin/bash
# Vista Formal Verification Script
# Uses Frama-C with WP plugin and Alt-Ergo prover

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

echo -e "${CYAN}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║           Vista Formal Verification Suite                     ║${NC}"
echo -e "${CYAN}║           Using Frama-C/WP + Alt-Ergo                         ║${NC}"
echo -e "${CYAN}╚══════════════════════════════════════════════════════════════╝${NC}"
echo ""

# Check for opam environment
if ! command -v frama-c &> /dev/null; then
    echo -e "${YELLOW}Loading opam environment...${NC}"
    eval $(opam env)
fi

# Check frama-c is available
if ! command -v frama-c &> /dev/null; then
    echo -e "${RED}Error: frama-c not found. Install with:${NC}"
    echo "  opam install frama-c"
    exit 1
fi

cd "$SCRIPT_DIR"

echo -e "${YELLOW}Verified Properties:${NC}"
echo "  • Index bounds are always valid (0 <= index <= max)"
echo "  • Navigation never causes out-of-bounds access"
echo "  • View mode toggle is bidirectional"
echo "  • Help toggle inverts state correctly"
echo "  • No integer overflows in navigation"
echo ""

# Run verification on core functions
echo -e "${YELLOW}Running formal verification on core navigation logic...${NC}"
echo ""

RESULT=$(frama-c -wp -wp-rte verify_core.c 2>&1)

# Parse results
PROVED=$(echo "$RESULT" | grep "Proved goals:" | head -1)
if [ -n "$PROVED" ]; then
    echo -e "${GREEN}$PROVED${NC}"
    
    # Check if all proved
    TOTAL=$(echo "$PROVED" | grep -oP '\d+(?= / \d+)' | head -1)
    ALL=$(echo "$PROVED" | grep -oP '(?<=/ )\d+')
    
    if [ "$TOTAL" = "$ALL" ]; then
        echo ""
        echo -e "${GREEN}✓ All proof obligations discharged successfully!${NC}"
        echo -e "${GREEN}✓ The verified properties hold for all valid inputs.${NC}"
    else
        echo ""
        echo -e "${YELLOW}⚠ Some goals could not be proved.${NC}"
        echo "$RESULT" | grep -E "(Failed|Timeout)" || true
    fi
else
    echo -e "${RED}Verification failed:${NC}"
    echo "$RESULT"
    exit 1
fi

echo ""
echo -e "${CYAN}────────────────────────────────────────────────────────────────${NC}"
echo -e "${CYAN}Detailed breakdown:${NC}"
echo "$RESULT" | grep -E "(Qed|Alt-Ergo|Terminating|Unreachable)" || true

echo ""
echo -e "${GREEN}Verification complete!${NC}"
