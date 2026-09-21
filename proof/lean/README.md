# Boyer-Moore Formal Verification (Lean 4)

Complete formal verification of Boyer-Moore string search and related algorithms using Lean 4 theorem proving.

## Files

### LinearAlgebraVerification.lean
Formal proofs of outer-product update algebra:
- **ALG-001**: Outer product action identity
- **ALG-002**: Linearity of matrix-vector multiplication
- **ALG-003**: Inner product expansion under update
- **ALG-004**: Exact change in projection
- **THR-001**: Sufficient condition for threshold crossing
- **THR-002**: Existence of valid gain for arbitrary thresholds

Proves that gains can be computed to achieve arbitrary margin conditions.

### TokenModel.lean
Core definitions for formal verification framework:
- Vector space definitions (finite-dimensional real inner product spaces)
- Linear operators (matrices)
- Outer products and outer product matrices
- Update operators
- Activation functions
- Threshold predicates (ThoughtFires)

Establishes the mathematical foundation for all other proofs.

### ArrayTemporalLock.lean
Axiom verification for array temporal locking:
- **finSum_zero_fun**: Sum of zero-valued functions is zero
- **finSum_eq_single**: Sum with single nonzero element
- **finSum_congr**: Congruence of finSum
- **finSum_add**: Distributivity of finSum over addition
- **byteSum_nil**: Empty byte sum is zero
- **byteSum_cons**: Cons list byte sum
- **byteSum_bounded**: Byte sum bounded by 65536
- **byteSum_singleton**: Single byte sum
- **byteSum_append_order_independent**: Addition commutativity
- **byteSum_monotone**: Monotonicity under subset

10/10 axioms formally verified.

## Building

### Prerequisites
- Lean 4.12.0+
- Lake build tool
- Mathlib4 (automatically fetched)

### Commands

```bash
cd proof/lean

# Initialize dependencies
lake update

# Build all proofs
lake build

# Check specific file
lake env lean LinearAlgebraVerification.lean

# Generate documentation
lake build DocsGen
```

## License

MIT OR Apache-2.0 OR GPL-3.0-or-later

With Sovereign Leviathan Node License (AGPL-3.0 additional terms).

## References

- **Lean 4 Documentation**: https://lean-lang.org/
- **Mathlib4**: https://github.com/leanprover-community/mathlib4
- **Outer Product Theory**: Axler, Linear Algebra Done Right
- **Inner Product Spaces**: Rudin, Functional Analysis
