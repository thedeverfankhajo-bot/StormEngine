# ADR-0022: Persistent Physics2D Contact Transitions

- Status: Accepted
- Date: 2026-11-03
- Scope: Deterministic Begin/Stay/End contact events for PhysicsWorld2D

## Context

PhysicsWorld2D already returns the contacts detected during each successful
fixed step. Gameplay code also needs to distinguish a contact that has just
started, persists, or ended. PhysicsWorld1D already exposes this contract, while
Physics2D did not, leaving the two independent physics models inconsistent.

Contact history must preserve the world's transactional step guarantee and
must not treat a destroyed body slot reused by a new generation as the same
body.

## Decision

Add ContactPhase2D and ContactEvent2D to the standalone Physics2D public API.
Each successful StepResult2D contains the current contacts and a transition
stream:

- Begin: a current pair did not exist in the previous successful step.
- Stay: both body handles (including world ID and generation) match the prior
  pair.
- End: a previous pair is absent from the current step; its last contact
  snapshot is retained in the event.

Current and previous contacts are already in deterministic slot-pair order.
Merge those lists rather than allocating a secondary lookup table. If the two
pairs have the same slot indices but different generations, emit End for the
old pair followed by Begin for the new pair.

The world stores contact history from the most recent successful step. It
prepares the next history and all event storage before committing either
history or body state. Allocation or configured-result-capacity failure returns
an error without advancing contact history or mutating live body state. The
world remains single-thread-affine.

Events are polled from StepResult2D. This ADR does not introduce callbacks,
Core EventDispatcher coupling, Scene dependencies, or asynchronous delivery.

## Consequences

Physics2D now offers the same basic contact lifecycle vocabulary as Physics1D.
A destroyed body's final End event is delivered on the next successful step.
Generation-aware matching prevents slot reuse from producing a false Stay.
Event generation requires memory proportional to the current plus previous
contact counts, both bounded by maximum_contacts_per_step.

The API remains a discrete, one-pass contact solver. Begin/Stay/End events do
not imply continuous collision detection, sub-step contact detection, a
multi-iteration solver, sleeping, or device-runtime validation.
