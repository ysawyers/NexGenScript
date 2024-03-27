# TODO

- scoped variables

# Future Test Cases

Conditional Breaking:

```
if 100 > 200 {
    var a = 0;
} else if 200 > 100 {
    if 100 < 1 {
        var a = 0;
    } else if 100 > 1 {
        var b = 1;
    } else {
        var a = 0;
    }

    if 200 > 1 {
        var b = 1;
    } else {
        var a = 0;
    }
} else {
    var a = 0;
}
```

Function calls:

```
fun fib(n) {
    if n <= 1 {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}

fib(30);
```
