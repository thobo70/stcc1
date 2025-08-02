# STCC1 Compiler Manifest
**Simple Three-Address Code C Compiler**

---

## 🎯 MISSION
Transform C source code into TAC through clean lexical, parsing, and code generation stages.  
**Priority:** Code quality and correctness over speed.

---

## 🛡️ CORE PRINCIPLES

### 1. TEST INTEGRITY
- ❌ **NEVER** weaken tests to make them pass
- ❌ **NEVER** skip failing tests  
- ✅ **FIX** the code, not the test
- ✅ **MAINTAIN** 100% test success rate

### 2. CODE CORRECTNESS
- ❌ **NEVER** commit broken functionality
- ❌ **NEVER** ignore warnings
- ❌ **NEVER** use workarounds instead of proper fixes
- ✅ **FIX** root causes, not symptoms

### 3. PIPELINE INTEGRITY  
- ❌ **NEVER** break cc0 → cc1 → cc2 pipeline
- ❌ **NEVER** mix build artifacts with sources
- ✅ **MAINTAIN** clear stage separation
- ✅ **ENSURE** TAC semantics match C semantics

### 4. MEMORY DISCIPLINE
- Memory is scarce - design accordingly
- Zero leaks, proper cleanup, bounds checking

---

## 🚫 PROHIBITIONS

**Code Quality:**
- No magic numbers, globals, hardcoded paths
- No copy-paste code, TODO comments  
- No fallback patterns masking real bugs

**Testing:**
- No flaky tests, manual steps, disabled tests
- No environment dependencies

**Architecture:**  
- No circular dependencies, stage bypassing
- No platform-specific code

---

## ✅ MANDATORY GATES

**Every Change Must Pass:**
1. `make clean && make` - Zero warnings
2. `make test` - All tests pass  
3. Documentation updated
4. Code review (for architecture changes)

**Quality Standards:**
- C99 compliance, CPPLINT conformance
- Doxygen documentation for public APIs
- Comprehensive error handling

---

## 📊 SUCCESS CRITERIA
- **Test Success:** 100% (zero tolerance)
- **Build Success:** 100% (zero warnings)  
- **Memory Safety:** Zero leaks
- **TAC Correctness:** Semantic equivalence with C

---

## 🎓 EDUCATIONAL VALUE
Demonstrate best practices:
- Clean, readable, well-documented code
- Proper error handling and testing
- Modular architecture
- Professional development workflow

---

**COMMITMENT:** All contributors uphold these principles even under pressure.  
**ACCOUNTABILITY:** Violations result in immediate revert and root cause fix.

*"In code we trust, but we test everything."*
