# OpenMP Implementation of various Parallel Sorting Algorithms

Let's begin by looking at some numbers from a small experiment.

## Experiment setup:

I evaluate the parallel quicksort algorithms on different numbers of records distributed across 1 to 20 processors on a single node. These algorithms use serial radix sort while sorting the data for a single processor/thread.

The goal is to show how the algorithm scales till 20 processors.

To view the exact numbers, click [here](https://docs.google.com/spreadsheets/d/1jhhLp6YrGt-3emgvbolQy4HrQezcImjoTdy51AEOLxE/edit?usp=sharing).

### Comments:

#### <strong>Quick sort</strong>: <br>
![](https://lh3.googleusercontent.com/AzKhMRikmUb5PIDdONSaPcVM8rRBRFu6YveYJ5A_FtNt7g-zRU6gCjVBCYIAkvTTtc-hOw4eBVrSMDpLQeyxJPEGDYmL5RU3t3Ju5eKx9tAxqh6jprfmr03eBE16PEeFFhdV9-C2)

The graph reported above is from my personal laptop instead of HPC unlike all other graphs. My quicksort algorithm was slow and couldn't scale on HPC. The reason I believe for this is the use of barriers which are apparently very costly on HPC and with increasing number of processors on HPC, the cost of omp barriers subdue the benefit from more processors. However, my own laptop quicksort is definitely weekly scaling for at least 6 processors since the top most series in the graph above shows positive benefits of increasing the processors. To make the implementation even more efficient I have made sure that if a particular thread that gets less number of elements and finished earlier doesn't sit idle but is used again and again. <br>

#### <strong>Merge sort</strong>: <br>
![](https://lh6.googleusercontent.com/PgH79tsRdS8uvtSAQR_0Ig7RcQGLV7ugXDGz2YMcF7zFNJNgfZ4xnZOdZUbO6btP6ZGU4VM07euL4hw3hZjKrSapk0mmEBHLBUHlq_s53aBoLPvNaPN7x8bw3Xr1e98z0k4rh9MN)

Merge sort is performing very well for smaller data sizes but may not scale to a very large number of processors. The reason for this being that the work of merging two sorted arrays during the end iteration falls upon just 1 or 2 processors and ultimately becomes a bottleneck for a very large number of records on a large number of processors. <br>

#### <strong>Radix sort</strong>: <br>
![](https://lh3.googleusercontent.com/EOAdjpf15oaFaHtJmg0VLQ5Gl7f45V0GvaHBsrJ4xs39J96aYbzYORSfGXcqAi9T8ON-YFejXAlKP3InBWnHRtfWsEF9KAnUhEGe4dRe3Q6C8sIvm8hB0_EYNoDBopVuasYspDKo)

Just like the message passing assignment done earlier, radix sort beats all other algorithms in this case. It is very fast like merge sort as well as scales very well (the graph above shows it to be even strongly scaling for the processor counts we are worried about). It has another benefit over the radix sort in the message passing implementation that the imbalance in the data distribution during the iterations doesn't hamper the performance because it doesn't cause any large overheads from the network effects unlike the message passing case. <br>

#### <strong>Bubble sort</strong>: <br>
![](https://lh6.googleusercontent.com/ZP18qSRMkVsMuAXWIT-LHhB9-0Fm4SWGQMhq8t-W2C-vJG6T3wPISy7uGNngnKo01LAW0W22ztoHh3KNtG1HaK6kLaxDCfndNKx3mSQEzwE8VVpxNzV940f9zT70p4F6joSXVSZZ)

To my surprise, unlike the standard performance results we know of serial bubble sort compared to other algorithms, parallel bubble sort in the shared memory setting performs very well and is at least weakly scalable and can be said as strongly scalable around the number of processors we care about. (1-20). In the message passing setting, parallel bubble sort incurs huge overheads from the network because a lot of data needs to be passed around but in shared memory even those overheads are not incurred.