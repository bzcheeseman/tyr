## Syntax

Reserved keywords:
```
struct
packed

mutable
repeated
```

Struct definitions should be structured like the following, where a keyword enclosed by `< >`
means that it's optional.
```
// this is a line comment

/*
 * this is a block comment
 */

struct name <packed> {
  <mutable> <repeated> <type> field
}
```
Notes:
 - Fields may be `mutable repeated` or `repeated mutable`. There is no difference.
 - Fields may appear in any order; `tyr` reserves the right to re-order the fields to better pack the 
 structure in memory.