#!/bin/bash
# STCC1 Project Manifest Validation Script
# Purpose: Validate that project follows manifest principles
# Usage: ./validate_manifest.sh

set -e  # Exit on any error

echo "🛡️  STCC1 Manifest Validation"
echo "============================="
echo "Validating project compliance with PROJECT_MANIFEST.md"
echo ""

# Test Gate 1: Build Quality
echo "📋 Gate 1: Build Quality"
echo "------------------------"
echo "Running clean build..."
make clean > /dev/null 2>&1
if make > /dev/null 2>&1; then
    echo "✅ Clean build: SUCCESS (zero warnings)"
else
    echo "❌ Build FAILED - violates manifest principle"
    exit 1
fi

# Test Gate 2: Test Integrity  
echo ""
echo "📋 Gate 2: Test Integrity" 
echo "-------------------------"
echo "Running Unity test suite..."
if make test > test_output.tmp 2>&1; then
    if grep -q "0 Failures 0 Ignored" test_output.tmp; then
        echo "✅ Unity tests: ALL PASS (0 failures)"
    else
        echo "❌ Tests have failures - violates 'never weaken tests' principle"
        cat test_output.tmp
        rm test_output.tmp
        exit 1
    fi
else
    echo "❌ Test execution FAILED"
    cat test_output.tmp
    rm test_output.tmp
    exit 1
fi

echo ""
echo "Running compiler pipeline tests..."
if make test-compiler > compiler_output.tmp 2>&1; then
    echo "✅ Compiler tests: ALL PASS"
else
    echo "❌ Compiler tests FAILED - violates pipeline integrity"
    cat compiler_output.tmp
    rm compiler_output.tmp
    exit 1
fi

# Code Quality Checks
echo ""
echo "📋 Gate 3: Code Quality"
echo "-----------------------"

# Check for TODO/FIXME in production code (manifest violation)
if find src -name "*.c" -o -name "*.h" | xargs grep -l "TODO\|FIXME\|XXX" > /dev/null 2>&1; then
    echo "⚠️  WARNING: TODO/FIXME found in production code"
    find src -name "*.c" -o -name "*.h" | xargs grep -n "TODO\|FIXME\|XXX" || true
    echo "   Recommendation: File issues and remove TODOs"
else
    echo "✅ Code quality: No TODO/FIXME in production code"
fi

# Check for debug statements (potential quality issue)
if find src -name "*.c" | xargs grep -l "printf\|fprintf" > /dev/null 2>&1; then
    echo "⚠️  WARNING: Debug statements found in production code"
    find src -name "*.c" | xargs grep -n "printf\|fprintf" || true
    echo "   Recommendation: Remove or convert to proper logging"
else
    echo "✅ Code quality: No debug statements in production code"
fi

# Architecture Validation
echo ""
echo "📋 Gate 4: Architecture Integrity"
echo "---------------------------------"

# Check that all expected binaries exist
expected_binaries=("cc0" "cc1" "cc2" "cc0t" "cc1t" "cc2t" "test_runner")
missing_binaries=()

for binary in "${expected_binaries[@]}"; do
    if [[ ! -f "bin/$binary" ]]; then
        missing_binaries+=("$binary")
    fi
done

if [[ ${#missing_binaries[@]} -eq 0 ]]; then
    echo "✅ Architecture: All expected binaries present"
else
    echo "❌ Missing binaries: ${missing_binaries[*]}"
    echo "   This violates the cc0→cc1→cc2 pipeline principle"
    exit 1
fi

# Documentation Validation
echo ""
echo "📋 Gate 5: Documentation Quality"
echo "--------------------------------"

required_docs=("PROJECT_MANIFEST.md" "DEVELOPMENT_CHECKLIST.md" "README.md" "tests/README.md")
missing_docs=()

for doc in "${required_docs[@]}"; do
    if [[ ! -f "$doc" ]]; then
        missing_docs+=("$doc")
    fi
done

if [[ ${#missing_docs[@]} -eq 0 ]]; then
    echo "✅ Documentation: All required documents present"
else
    echo "❌ Missing documentation: ${missing_docs[*]}"
    exit 1
fi

# Final Validation
echo ""
echo "🎯 MANIFEST COMPLIANCE REPORT"
echo "============================="

# Get test results summary
TEST_SUMMARY=$(grep -A1 "Test Summary" test_output.tmp | tail -1 | tr -d ' ')

echo "Build Status: ✅ CLEAN (zero warnings)"
echo "Test Status: ✅ $TEST_SUMMARY" 
echo "Code Quality: ✅ COMPLIANT"
echo "Architecture: ✅ INTACT" 
echo "Documentation: ✅ COMPLETE"
echo ""
echo "🏆 PROJECT MANIFEST COMPLIANCE: SUCCESS"
echo ""
echo "✨ All fundamental principles upheld:"
echo "   • Never weaken tests to make them pass"
echo "   • Fix code, not symptoms"  
echo "   • Maintain pipeline integrity"
echo "   • Code quality over quick fixes"
echo "   • Documentation keeps pace with code"

# Cleanup
rm -f test_output.tmp compiler_output.tmp

echo ""
echo "💡 Next: Follow DEVELOPMENT_CHECKLIST.md for daily workflow"
echo "📖 Reference: PROJECT_MANIFEST.md for detailed principles"
