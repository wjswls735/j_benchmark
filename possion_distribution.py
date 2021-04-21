import numpy as np

np.random.seed(seed=100000)

rand_pois = np.random.poisson(lam=20, size=100000)

print(rand_pois) 

unique, counts = np.unique(rand_pois, return_counts=True)
print(np.asarray((unique,counts)).T)


