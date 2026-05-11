# Time complexity

In computer science, the time complexity of an algorithm quantifies the amount
of time taken by an algorithm to run as a function of the length of the input.
The time complexity of an algorithm is commonly expressed using **big O**
notation, which excludes coefficients and lower order terms. When expressed this
way, the time complexity is said to be described asymptotically, i.e., as the
input size goes to infinity. For example, if the time required by an algorithm
on all inputs of size `n` is at most ` 5n^3 + 3n`, the
asymptotic time complexity is `O(n^3)`.

Time complexity is commonly estimated by counting the number of elementary
operations performed by the algorithm, where an elementary operation takes a
fixed amount of time to perform. Thus the amount of time taken and the number of
elementary operations performed by the algorithm differ by at most a constant
factor.

Since an algorithm’s performance time may vary with different inputs of the same
size, one commonly uses the worst-case time complexity of an algorithm, denoted
as **T(n)**, which is defined as the maximum amount of time taken on any
input of size `n`. Time complexities are classified by the nature of
the function `T(n)`. For instance, an algorithm with `T(n) =
O(n)` is called a linear time algorithm, and an algorithm with `T(n)
= O(2^n)` is said to be an exponential time algorithm.

__NOTE__:  

    Worst-case time-complexity `T(n)` indicates the longest running time
    performed by an algorithm given any input of size `n`, and thus this
    guarantees that the algorithm finishes on time.

## Big O notation

Big O notation describes the limiting behavior of a function when the argument
tends towards a particular value or infinity, usually in terms of simpler
functions and it is used to classify algorithms by how they respond (e.g., in
their processing time or working space requirements) to changes in input size.

##  Constant time
An algorithm is said to be constant time (also written as `O(1)` time) if the
value of `T(n)` is bounded by a value that does not depend on the size of the
input.

Despite the name *constant time*, the running time does not have to be
independent of the problem size, but an upper bound for the running time has to
be bounded independently of the problem size.

__NOTE__:  

    Constant time effectively means that there is a constant upper bound to
    how long the function will take to run which isn’t affected by any of the
    input argument.

## Nano Kernel time complexity

All Nano Kernel functions are using `constant time O(1)` algorithms. This is
especially important for Real Time applications.

