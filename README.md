# TODO

- scoped variables
- loops

# Approaches

- condition-breaking counter + backpatching jump offsets
- NaN boxing

# Future Test Cases

Conditional Breaking:

```
if (100 > 200) {
    var a = 0;
} else if (200 > 100) {
    if (100 < 1) {
        var a = 0;
    } else if (100 > 1) {
        var b = 1;
    } else {
        var a = 0;
    }

    if (200 > 1) {
        var b = 1;
    } else {
        var a = 0;
    }
} else {
    var a = 0;
}
```

# Edge Cases

make sure compiler checks that return is being specified within functionDecl context
