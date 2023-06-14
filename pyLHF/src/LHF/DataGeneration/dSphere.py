#!/usr/bin/python3

## This is a simple generator to build spheres for testing TDA tools/techniques. 

## Generate points forming the surface of a sphere in (up to) d-dimensions.  

## generate a point cloud representation for a d-sphere using Muller's method [Muller-59].  this method generates uniform random
## points on a boundary of a d-sphere.  the boundary of points follows a uniform random distribution between radii r1 < r2.

## [Muller-59] Mervin E. Muller, "A note on a method for generating points uniformly on n-dimensional
##             spheres." Communications of the  ACM, 19-20, vol2, no 4, April
##             1959. DOI=http://dx.doi.org/10.1145/377939.377946 

import sys
import numpy as np
from scipy.stats import qmc
import math
from functools import reduce
from scipy.spatial import distance_matrix
import datetime
from ..utilities import run_trials


def dSphere_uniformRandom(dimension=3, numPoints=20000, r1=1.0, r2=1.0, trials=8, selection='min') :
    """
    Generate an optimized point cloud for the boundary of a dSphere that is populated with uniformly distributed points between
    two radii (r1 < r2). 

    Generate an optimized point cloud representation of a dSphere by selecting from multiple trials.  Each trial point cloud is
    generated by the nested function named get_dSphere.  get_dSphere uses Muller's method [Muller-59] to generate a uniform random
    set of points on a boundary of a d-sphere.  The boundary of points follows a uniform random distribution between radii r1 <
    r2.  The candidate point clouds represent d-spheres as a point cloud boundary of uniform random points in a boundary between
    r1 < r2 of a d-sphere.  This is function takes multiple trials for a dSphere and selects the trial that optimizes one of three
    values of the generated point cloud.  The optimization method is based on nearest neighbor distances for the points.  There
    are three options for the optimization method, namely: 'mean' (default), maximize the mean of the nearest neighbor distances;
    'stdev', minimize the variance of the mean nearest neighbor distance; and 'min', maximize the minimum nearest neighbor distance.

    While a d-sphere is technically an n-dimensional manifold that can be embedded in euclidean (d+1)-space, we will use the term
    dimension here to refer to the euclidean space as that is the more commonly used interpretation outside of the math-head
    community.  Thus, if this program will generate a circle at dimension 2, a sphere at dimension 3, and so on....

    [Muller-59] Mervin E. Muller, "A note on a method for generating points uniformly on n-dimensional spheres." Communications of
                the ACM, 19-20, vol2, no 4, April 1959. DOI=http://dx.doi.org/10.1145/377939.377946

    Parameters
    ----------
    dimension : int 
        The number of dimensions for the desired d-sphere (default 3)
    numPoints : int 
        The number of points desired in the output point cloud (default 20000)
    r1 : float 
        The inner radius bounds of the point cloud (default 1.0)
    r2 : float 
        The outer radius bounds of the point cloud (default 1.0)
    trials: int 
        The number of trials used to optimize the distribution of points in the dSphere (default: 8)
    selection: str (min, max, mean, var)
        The selection criteria to select between two candidate point clouds:
            'min' : maximize the minimum nearest neighbor distance of the points
            'mean' : maximize the mean of the nearest neighbor distances.
            'stdev' : minimize the standard deviation of the nearest neighbors

    Returns
    -------
    dShere: [numPoints x dimension] random points on the dSphere

    Notes:
    ------
    1. While it may seem that optimizing the stdev or min 1-NN distance would be the best approach, from the standpoint of getting
       the earliest birth time, maximizing the 1-NN distance will actually produce the best result.
    2. Experimental testing shows that the sobol sequence generator provides the best results to minimize the max 1-NN distance
       between points. 
    """

    #### --------------------------------------------------------------------------------
    #### helper function that builds a trial point cloud 
    #### --------------------------------------------------------------------------------
    def get_dSphere(dimension=3, numPts=1024, r1=1.0, r2=1.0) :
    
        if dimension <= 1 :
            print("Go away kid, don't bother me with 0/1-spheres.  Aborting.`")
            sys.exit(-1)

        ## collect a set of uniform normal deviates for projection to the surface/boundary of the dSphere
        # generating the matrix of normal deviates....
        numPts = numPoints
        covariance = np.zeros((dimension, dimension))
        np.fill_diagonal(covariance,1)
        randomPoints = np.random.multivariate_normal(mean=np.zeros(dimension), cov=covariance, size=(numPts))

        if r1 < r2 :
            #radii = np.random.uniform(low=r1, high=r2, size=numPts)
            radii = np.random.uniform(low=r1**dimension, high=r2**dimension, size=numPts)**(1/dimension)
        elif r1 == r2 :
            radii = np.full(numPts, r1)
        else:
            print("Aborting: r1 must be less than or equal to r2.")
            sys.exit(-1)

        # using the normal deviates to generate cartesian coordinate terms
        for i in range(numPts) :
            r = np.sqrt(np.sum(np.power(randomPoints[i], 2)))
            randomPoints[i] = (randomPoints[i] / r) * float(radii[i])
        
        return randomPoints
    #### --------------------------------------------------------------------------------

    sample_func = lambda: get_dSphere(dimension, numPoints, r1, r2)
    return run_trials(sample_func, trials, selection)

def dSphere_product(dimensions=[2,2], numPoints=20000, r1=1.0, r2=1.0, trials=8, selection='min') :
    '''
    Generates a point cloud for the Cartesian product of spheres of specified dimension. This creates point clouds with
    interesting features in multiple dimensions, as described by
    https://topospaces.subwiki.org/wiki/Homology_of_product_of_spheres.  
    The true Betti numbers are returned by the function and are calculated using the Poincare polynomial of the
    product of the spheres. This generalizes the flat torus. The torus with intrinsic dimension n embedded in 
    Rn can be created using dSphere_product([2 for _ in range(n)])

    Parameters
    ----------
    dimensions : list of ints 
        The number of dimensions for the spheres in the product (default [2, 2])
    numPoints : int 
        The number of points desired in the output point cloud (default 20000)
    r1 : float 
        The inner radius bounds of the point cloud (default 1.0)
    r2 : float 
        The outer radius bounds of the point cloud (default 1.0)
    trials: int 
        The number of trials used to optimize the distribution of points in the dSphere (default: 8)
    selection: str (min, max, mean, var)
        The selection criteria to select between two candidate point clouds:
            'min' : maximize the minimum nearest neighbor distance of the points
            'mean' : maximize the mean of the nearest neighbor distances.
            'stdev' : minimize the standard deviation of the nearest neighbors

    Returns
    -------
    product: [numPoints x dimensions points
    '''

    generateSphere = lambda dimension: dSphere_uniformRandom(dimension, numPoints, r1, r2, trials, selection)
    dSpheres = tuple(generateSphere(dimension) for dimension in dimensions)
    return np.concatenate(dSpheres, axis = 1)

    #### I am preserving this code as it computes the true Betti's; keeping in case we end up wanting this value for some of our testing
    '''  
    polynomials = []
    for dimension in dimensions:
        polynomials.append([0 for _ in range(dimension)])
        polynomials[0] = 1
        polynomials[-1] = 1
    poincarePolynomial = reduce(np.convolve, polynomials)
    trueBettis = dict(enumerate(poincarePolynomial))    

    product = dict()
    product['ptCldObject'] = 'd-Sphere Product'
    product['date'] = datetime.datetime.now()
    product['origin'] = np.zeros(sum(dimensions))
    product['radius'] = (r1, r2)
    product['points'] = points
    product['trueBettis'] = trueBettis
    product['dimensions'] = dimensions
    '''
