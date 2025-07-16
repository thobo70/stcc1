#!/bin/bash

# STCC1 Code Cleanup Script
# This script automatically fixes common code style issues

echo "🧹 Starting STCC1 code cleanup..."

# 1. Remove trailing whitespace from all source files
echo "1. Removing trailing whitespace..."
find src -name "*.c" -o -name "*.h" | xargs sed -i 's/[[:space:]]*$//'

# 2. Fix comment spacing (add space after // if missing)
echo "2. Fixing comment spacing..."
find src -name "*.c" -o -name "*.h" | xargs sed -i 's|//\([^ ].*\)|// \1|g'

# 3. Remove redundant blank lines at start of code blocks
echo "3. Removing redundant blank lines..."
find src -name "*.c" -o -name "*.h" | xargs sed -i '/^[[:space:]]*{[[:space:]]*$/,/^[[:space:]]*$/{
    /^[[:space:]]*{[[:space:]]*$/!{
        /^[[:space:]]*$/d
    }
}'

# 4. Fix excessive line lengths by breaking long parameter lists
echo "4. Breaking long lines (manual intervention needed for complex cases)..."

# 5. Clean up files with most issues first
echo "5. Specific file cleanups..."

# Fix common long line patterns in headers
find src -name "*.h" | xargs sed -i 's/\([,]\) \([a-zA-Z]\)/\1\n                     \2/g'

echo "✅ Automatic cleanup completed!"
echo "📝 Manual fixes still needed for:"
echo "   - Complex function signatures (>80 chars)"
echo "   - Long string literals"
echo "   - Complex expressions"
echo ""
echo "🔍 Run 'make lint' to see remaining issues"
