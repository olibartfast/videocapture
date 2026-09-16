# Plan

1. Isolate pipeline state and callback userdata; encapsulate frame waiting;
   replace default-context bus watches with per-instance bus polling. R-1/R-3.
2. Make teardown and partial initialization cleanup explicit; add regression
   tests for concurrent pipelines, EOS/error isolation, reopen and destruction.
   R-1/R-2/R-3.
3. Modernize applicable source operations with C++20 without expanding runtime
   requirements; test source classification and shared writer behavior. R-4/R-5.
4. Run focused repeated tests, sanitizer checks where supported, the existing
   backend matrix, capture-only configuration, and downstream build checks.
   Update validation and roadmap with actual outcomes.

Orchestration: single capable implementation context for concurrency/lifetime
reasoning, per orchestrate-ai-coding-workflows routing guidance. No delegated
workers. Writable scope is advisory within harness-enforced workspace boundaries.
