# STCC1 Development Checklist
## Quick Reference for Daily Development

**Based on:** PROJECT_MANIFEST.md  
**Purpose:** Practical checklist for maintaining project standards  
**Usage:** Reference before commits, during reviews, and when adding features

---

## ⚡ **QUICK VALIDATION CHECKLIST**

### Before Every Commit: ✅
```bash
□ make clean && make              # Clean build, zero warnings
□ make test                       # Unity tests: 0 failures
□ make test-compiler             # Pipeline tests: all pass
□ git add . && git status        # Review what's being committed
□ Write meaningful commit message # Why, not just what
```

### When Tests Fail: 🔧
```bash
□ NEVER weaken the test          # Fix code, not test
□ NEVER skip the failing test    # Face the problem directly  
□ NEVER reduce coverage          # Maintain quality standards
□ Fix root cause                 # Not just symptoms
□ Add edge case tests            # Prevent similar issues
```

---

## 🎯 **FEATURE DEVELOPMENT WORKFLOW**

### 1. Planning Phase
```bash
□ Write failing test first (TDD)
□ Define clear success criteria
□ Check architectural fit
□ Plan minimal implementation
□ Document expected behavior
```

### 2. Implementation Phase  
```bash
□ Follow naming conventions
□ Maintain modular design
□ Add comprehensive error handling
□ Document public interfaces
□ Keep functions focused and small
```

### 3. Testing Phase
```bash
□ Unit tests for new functions
□ Integration tests for workflows
□ Edge case and error condition tests
□ Performance regression tests
□ Manual testing of user scenarios
```

### 4. Documentation Phase
```bash
□ Update README if needed
□ Add/update Doxygen comments
□ Document architectural decisions
□ Update relevant design docs
□ Add examples if helpful
```

---

## 🚫 **RED FLAGS CHECKLIST**

### Code Quality Violations
```bash
□ Magic numbers without constants
□ Global variables being added
□ Hardcoded file paths
□ Copy-pasted code blocks
□ TODO comments in production code
□ Compiler warnings being ignored
□ Memory leaks or unsafe operations
```

### Architecture Violations
```bash
□ Circular dependencies
□ Stage bypassing (cc0→cc2 directly)
□ Direct file access without abstraction
□ Mixing concerns in single functions
□ Breaking cc0→cc1→cc2 pipeline
□ Adding external dependencies unnecessarily
```

### Testing Violations
```bash
□ Tests dependent on environment
□ Flaky or non-deterministic tests
□ Tests that don't clean up
□ Commented-out test code
□ Skipped tests without issues filed
□ Reduced test coverage
```

---

## 🎯 **QUALITY GATES**

### Gate 1: Build Quality ✅
```bash
# Command
make clean && make

# Success Criteria
□ Zero compilation errors
□ Zero compiler warnings  
□ All binaries generated (cc0, cc1, cc2, test_runner)
□ No missing dependencies
```

### Gate 2: Test Quality ✅
```bash
# Commands
make test && make test-compiler

# Success Criteria  
□ Unity tests: "X Tests 0 Failures 0 Ignored OK"
□ All compiler pipeline tests pass
□ No memory leaks reported
□ Performance within acceptable bounds
```

### Gate 3: Documentation Quality ✅
```bash
# Manual Review
□ All public functions documented
□ Architecture changes explained
□ README reflects current state
□ Examples work as described
```

---

## 🔧 **DEBUGGING WORKFLOW**

### When Something Breaks
```bash
1. □ Reproduce the issue reliably
2. □ Write failing test that shows the bug
3. □ Use minimal example to isolate cause
4. □ Fix root cause, not symptoms
5. □ Verify test now passes
6. □ Add related edge case tests
7. □ Document the fix
```

### When Tests Are Slow
```bash
□ Profile to find bottlenecks
□ Optimize algorithms, not just disable tests
□ Consider test parallelization
□ Remove unnecessary file I/O
□ Use test fixtures efficiently
```

### When Memory Issues Occur
```bash
□ Run with valgrind if available
□ Check all malloc/free pairs
□ Verify array bounds
□ Test with edge cases (empty input, max size)
□ Add memory safety tests
```

---

## 📊 **HEALTH CHECK COMMANDS**

### Daily Health Check
```bash
# Full validation
make clean && make test && make test-compiler

# Expected output patterns
"5 Tests 0 Failures 0 Ignored OK"      # Unity tests
"✓ All compiler tests passed"           # Pipeline tests
```

### Weekly Deep Check
```bash
# Code quality
find src -name "*.c" -o -name "*.h" | xargs wc -l  # LOC trends
grep -r "TODO\|FIXME\|XXX" src/                    # Technical debt
grep -r "printf\|fprintf" src/                     # Debug statements

# Test coverage
make test-help                                       # Available tests
ls tests/ -la                                       # Test organization
```

### Before Major Changes
```bash
# Baseline establishment
make test > baseline_tests.log 2>&1
make test-compiler > baseline_compiler.log 2>&1
git log --oneline -10                               # Recent changes
git status                                          # Clean state
```

---

## 🎓 **COMMON SCENARIOS**

### Adding a New Feature
```
1. □ Design: What's the minimal viable implementation?
2. □ Test: Write failing tests that define success
3. □ Implement: Code to make tests pass
4. □ Refactor: Clean up and optimize
5. □ Document: Update docs and examples
6. □ Integrate: Full test suite validation
```

### Fixing a Bug
```
1. □ Reproduce: Create minimal failing case
2. □ Test: Write test that demonstrates bug
3. □ Diagnose: Find root cause, not just trigger
4. □ Fix: Minimal change that resolves issue
5. □ Validate: Ensure fix doesn't break anything
6. □ Document: Explain what was wrong and why
```

### Refactoring Code
```
1. □ Baseline: Ensure all tests pass first
2. □ Preserve: Keep same external behavior
3. □ Incremental: Small, verifiable steps
4. □ Test: Validate after each step
5. □ Clean: Remove dead code and comments
6. □ Document: Update architecture notes
```

---

## 🆘 **EMERGENCY PROCEDURES**

### If Build Breaks
```
1. □ Revert to last known good state
2. □ Identify minimal reproduction case
3. □ Fix incrementally with tests
4. □ Validate full pipeline before re-commit
```

### If Tests Start Failing
```
1. □ DO NOT weaken tests to make them pass
2. □ Isolate failing tests to understand scope
3. □ Fix underlying issues systematically
4. □ Add preventive tests for similar issues
```

### If Performance Degrades
```
1. □ Establish baseline measurements
2. □ Profile to identify bottlenecks
3. □ Optimize hot paths
4. □ Validate improvements with benchmarks
```

---

## 🏆 **SUCCESS INDICATORS**

### Green Light (All Good)
- ✅ All tests passing consistently
- ✅ Zero compiler warnings
- ✅ TAC output matches C semantics
- ✅ Clean, readable code
- ✅ Documentation up to date

### Yellow Light (Needs Attention)
- ⚠️ Occasional test flakiness
- ⚠️ Performance degradation
- ⚠️ Increasing technical debt
- ⚠️ Documentation lag
- ⚠️ Accumulating TODOs

### Red Light (Stop and Fix)
- 🔴 Any tests failing
- 🔴 Compiler warnings
- 🔴 Memory leaks
- 🔴 TAC correctness issues
- 🔴 Architecture violations

---

**Remember: This checklist enforces the PROJECT_MANIFEST.md principles. When in doubt, choose quality over speed, correctness over convenience!**
