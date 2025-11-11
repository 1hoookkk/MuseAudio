#!/bin/bash
# Interactive UI file archiving script
# Usage: bash archive-ui-files.sh

set -e

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}Muse UI File Archiver${NC}"
echo "================================"
echo ""

# Create archive directory
mkdir -p .archive

# Files to keep by default (edit this list)
KEEP_FILES=(
    "source/ui/HalftoneMouth.h"
    "source/ui/MuseColors.h"
)

# Get all UI files
UI_FILES=($(find source/ui -type f \( -name "*.h" -o -name "*.cpp" \) | sort))

echo "Found ${#UI_FILES[@]} UI files"
echo ""
echo "Files marked to KEEP:"
for file in "${KEEP_FILES[@]}"; do
    echo -e "  ${GREEN}✓${NC} $file"
done
echo ""

# Ask for confirmation
echo "Files that will be ARCHIVED:"
for file in "${UI_FILES[@]}"; do
    # Check if file should be kept
    keep=false
    for keep_file in "${KEEP_FILES[@]}"; do
        if [[ "$file" == "$keep_file" ]]; then
            keep=true
            break
        fi
    done

    if [ "$keep" = false ]; then
        echo -e "  ${YELLOW}→${NC} $file"
    fi
done
echo ""

read -p "Proceed with archiving? (y/n) " -n 1 -r
echo ""

if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "Aborted."
    exit 1
fi

# Archive files
archived_count=0
for file in "${UI_FILES[@]}"; do
    # Check if file should be kept
    keep=false
    for keep_file in "${KEEP_FILES[@]}"; do
        if [[ "$file" == "$keep_file" ]]; then
            keep=true
            break
        fi
    done

    if [ "$keep" = false ]; then
        filename=$(basename "$file")
        echo "Archiving: $file → .archive/$filename"
        git mv "$file" ".archive/$filename"
        ((archived_count++))
    fi
done

echo ""
echo -e "${GREEN}✓ Archived $archived_count files${NC}"
echo ""
echo "Next steps:"
echo "  1. Review changes: git status"
echo "  2. Commit: git commit -m 'chore(ui): archive legacy UI components'"
echo "  3. Clean up: rm archive-ui-files.sh"
