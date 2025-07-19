import math

for x in range(16, 400):
    s = 1024/math.sqrt(x)
    print(int(s), end=", ")
