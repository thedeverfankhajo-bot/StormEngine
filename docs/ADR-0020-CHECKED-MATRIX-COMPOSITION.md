# ADR-0020: Checked 4x4 Matrix Composition

- Status: Accepted
- Date: 2026-11-03

## Context

`Matrix4::operator*` is a non-throwing convenience operator returning a matrix directly. Its intermediate multiplication is performed in `double`, but narrowing a result outside the finite `float` range can produce an infinite matrix. Core's `TransformSystem` previously detected this only after composition, so callers had no checked operation at the math boundary and overflow could be propagated through intermediate calculations.

## Decision

- Add `Matrix4::multiply_checked(lhs, rhs)` returning `Result<Matrix4>`.
- Reject non-finite input matrices before multiplication.
- Accumulate each output coefficient in `double` and verify it is representable as a finite `float` before narrowing.
- Make `TransformSystem::world_matrix` use the checked operation and return its typed error immediately.
- Keep the existing `operator*` for source compatibility and low-level convenience. Its declaration explicitly documents that it is unchecked; runtime-facing code that needs error propagation must use `multiply_checked`.

## Validation

- Unit-test successful parent/local composition.
- Unit-test output overflow rejection.
- Unit-test rejection of a non-finite matrix produced by the unchecked operator.
- Integration-test a Scene hierarchy whose parent and child scales would overflow when composed.

## Consequences

- Core transform composition fails deterministically with `InvalidArgument` instead of carrying a non-finite world matrix.
- Existing callers of `operator*` remain source-compatible, but must inspect `is_finite()` or migrate to `multiply_checked` when inputs may be runtime-controlled.
- No rendering backend, device support, or performance claim is implied by this math hardening change.
