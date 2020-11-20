
# OpenMPI Implementation of various Parallel Sorting Algorithms

Let's begin by looking at some numbers from a small experiment.

## Experiment setup:

I evaluate the parallel quicksort algorithms on a data size of 0.4 billion records distributed across 1 to 6 processors on a single node. For each sorting algorithm and processor count combination, the average time spent by each processor is reported as an average of 10 random test cases. This algorithm uses serial radix sort while sorting the data for a single processor.

The goal is to show how the algorithm scales till 5 processors for a data size of 0.4 billion and will require larger data sizes if needs to be further scaled up.

To view the exact numbers, click [here](https://docs.google.com/spreadsheets/d/1HywrwhCF1CPIMDqe-4hZzqIkq4fygtZB6c42615F0hI/edit?usp=sharing).

![](https://lh5.googleusercontent.com/E6qgpMw2b_8OuXXQxGWyaTgpWRJi0iyjMDQNFJyvfRYqpZutLdcSpYnLq4sPlq-r74rCbsNj_3CRTLxPovyj4wPPh1dwh0YIaUWVGQv3UqRNKI-QJQoyctAUfuzYYkX6q9TNEqKA "Points scored")

### Overview of Algorithms:

1. Quicksort - 
<ul>
<li>  The processors start by proposing a median to a root processor from their set of records. The root processor chooses the global median using the set of proposed local medians. </li>

<li> Each processor after receiving the global median partitions its records around it and reports to the root processor the size of its left and right partition.
</li>

<li>  The root processor compiles this information to decide the left and right group sizes for the next iteration and sends enough meta information to each processos so that they can exchange data between themselves for the next iteration. (Exchange of control information has been done synchronously while that of real data asynchronously) </li>

<li>  The processor recurs within smaller groups after exchanging the data until their group size becomes 1. </li>

<li>  FInally, the data is sorted in place using a sorting algorithm like serial quicksort or serial radix sort. </li>

<li>  The problem with my implementation of parallel quicksort is the unbalanced partition of data. To counter this problem, Hoare's algorithm has been implemented in the code. However, in practice the pros of having a proper median were countered by the cons of the extra time spent on calculating it. So in final submission, the functions for median calculations have not been used. </li>
</ul>

2.  Radix Sort -
<ul>
<li>  This algorithm splits the integer keys into digits each of bit length 4 or 8 and uses a parallel counting sort at each iteration. </li>

<li>  The processors split their datasets into multiple buckets and at each iteration exchanges these buckets with other processors who are known to be responsible for a particular set of buckets. (Note: Global counting sort also need to be stable) </li>

<li>  At the end of the iterations the data at each processor is sorted and each processor is holding the data from the buckets that are assigned to it. </li>

<li>  The problem with the implementation of this algorithm is that in the last iteration, the data tends to accumulate at only a few processors if the input key space is not uniform enough. </li>
</ul>

3. Merge Sort - 
<ul>
<li>  The direct parallelization of serial merge sort leads to a very inefficient parallel algorithm. Hence, I have adopted a variant of bubble sort in the parallel implementation. 
</li>

<li>  Each processor follows a series of odd and even phases. A phase means that each processor will be sharing its data with one other process. It is the processor next to it according to rank if it's an odd phase and the processor behind it if its an even phase. </li>

<li>  During each phase of the algorithm, the two interacting processors exchange their full datasets and merge them to keep only the smallest n or the largest n elements from it.
</li>

<li>  At the end of p iterations, where p is the number of processors, the datasets at each processor are globally sorted. </li>
</ul>

### Major Design Decisions: <br>

1.  Parallel Quicksort:
<ul>
<li>
    Sharing only the meta information from the root processor about the children partitions can reduce the bottleneck of serial code. The root processor does only a minimal amount of calculation on the information received from each processor to return them the global information about the state of all the processors such that they can do the processing on it themselves and know who to communicate with in the next iteration. The root processor tells each child process the sub-group of processors it will belong to in the next iteration, the prefix sum of the left and right partitions and the sizes of partitions of the other processors in the same subgroup.
</li>

<li>
    Apart from the control messages, the actual data transmission and synchronization has been done asynchronously for maximum efficiency.
</li>


<li>
<strong>Further possibilities</strong>:
<ul>
<li> The partitioning scheme could be improved by improving Hoare's algorithm implementation to calculate medians efficiently. </li>
<li> The transfer of data between iterations can be further optimized by transferring more and intelligent metadata from root to child processes. </li>
</ul>
</li>
</ul>

2.  Radix Sort:
<ul>
<li>
The number of buckets during the counting sort and the number of iterations of the radix sort depending on the number of digits has a direct inverse relationship. Reducing the bit length of each digit results in a faster counting sort but also forces radix sort to use more iterations of counting sort. Hence, an optimum bit length for each digit is chosen to be 8 which requires radix sort to go for 4 iterations. For MPI based sorting, this is also a better idea because the computational cost incurred at each iteration is not as much as compared to the message passing cost between different iterations.
</li>

<li>
<strong>Further possibilities</strong>:
<ul>
<li>
The partitioning of buckets between processors is non-adaptive in my implementation due to which the load can become unbalanced during some iterations, more importantly during the last iteration. An adaptive partitioning scheme, which divides buckets between the processors depending on the global load of each bucket can prove to be more efficient with the only cost of extra amounts of metadata.
</li>
</ul>
</li>
</ul>