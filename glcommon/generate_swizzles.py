# generate_swizzles.py
# Run this once: python generate_swizzles.py
# It will print ready-to-paste sections for your rsw_math header.
# Copy the DECLARATIONS into each struct vecN { ... }
# Copy the IMPLEMENTATIONS at the very bottom of the file (after all vec structs are fully defined).

import itertools

def main():
    # Components available per dimension
    vec_components = {
        2: ['x', 'y'],
        3: ['x', 'y', 'z'],
        4: ['x', 'y', 'z', 'w']
    }

    declarations = {}
    implementations = []

    for dim, components in vec_components.items():
        decl_group = []

        for length in range(2, 5):  # 2-, 3-, and 4-component swizzles
            decl_group.append(f"    // {length}-component swizzles")
            for combo in itertools.product(components, repeat=length):
                swizzle_name = ''.join(combo)
                decl_group.append(f"    vec{length} {swizzle_name}() const;")
        
        declarations[dim] = decl_group

        # Build implementations for this dimension (collected in one big list)
        for length in range(2, 5):
            for combo in itertools.product(components, repeat=length):
                swizzle_name = ''.join(combo)
                #args = ', '.join(f'this->{c}' for c in combo)
                args = ', '.join(c for c in combo)  # no this-> (cleaner + identical performance)
                impl = f'inline vec{length} vec{dim}::{swizzle_name}() const {{ return vec{length}{{{args}}}; }}'
                implementations.append(impl)

    # === OUTPUT ===
    print("// ====================== SWIZZLE DECLARATIONS ======================")
    print("// Paste these INSIDE each struct (after the member variables):")
    for dim in [2, 3, 4]:
        print(f"\n// Swizzles for vec{dim}")
        print('\n'.join(declarations[dim]))

    print("\n\n// ====================== SWIZZLE IMPLEMENTATIONS ======================")
    print("// Paste this block ONCE, AFTER all vec structs are fully defined")
    print("// (right before the end of the header or in a .inl file)")
    print('\n'.join(implementations))

    print("\n// Done! These are zero-overhead when inlined.")
    print("// Total functions generated: 481 (all combinations). Modern compilers eliminate them completely.")


if __name__ == "__main__":
    main()
