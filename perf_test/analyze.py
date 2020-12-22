import statistics
from collections import Counter

with open('times.txt', 'r') as fp:
    times = tuple(map(int, fp.readlines()))
    print('N = ', len(times))
    print('mean: ', statistics.mean(times))
    print('stdev: ', statistics.stdev(times))
    print('median: ', statistics.median(times))
    print('min: ', min(times))
    print('max: ', max(times))
    counter =  Counter(times)
    for i in range(5):
        print(f'{i} count: ', counter[i])
    