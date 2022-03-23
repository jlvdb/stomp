// Copyright 2010  All Rights Reserved.
// Author: ryan.scranton@gmail.com (Ryan Scranton)

// STOMP is a set of libraries for doing astrostatistical analysis on the
// celestial sphere.  The goal is to enable descriptions of arbitrary regions
// on the sky which may or may not encode futher spatial information (galaxy
// density, CMB temperature, observational depth, etc.) and to do so in such
// a way as to make the analysis of that data as algorithmically efficient as
// possible.
//
// This header file contains the class for calculating angular correlations on
// the sphere.  In general, different methods are more efficient on small vs.
// large angular scales, so this class draws on nearly the entire breadth of
// the STOMP library.

#ifndef STOMP_ANGULAR_CORRELATION_H
#define STOMP_ANGULAR_CORRELATION_H

#include <vector>
#include "stomp_core.h"
#include "stomp_angular_coordinate.h"
#include "stomp_angular_bin.h"

namespace Stomp {

class Map;        // class declaration in stomp_map.h
class ScalarMap;  // class declaration in stomp_scalar_map.h
class TreeMap;    // class declaration in stomp_tree_map.h
class AngularCorrelation;

typedef std::vector<AngularCorrelation> WThetaVector;
typedef WThetaVector::iterator WThetaIterator;

class AngularCorrelation {
  // Class object for calculating auto-correlations and cross-correlations
  // given a set of objects and a Map.  Broadly speaking, this is a
  // container class for a set of AngularBin objects which collectively
  // span some range of angular scales.  Accordingly, the methods are generally
  // intended to package the machinery of the auto-correlation and
  // cross-correlation calculations into simple, one-line calls.

 public:
  AngularCorrelation();
  // The first constructor takes an angular minimum and maximum (in degrees)
  // and constructs a logrithmic binning scheme using the specified number
  // of bins per decade (which can be a non-integer value, obviously).  The
  // bins are such that the minimum angular scale of the first bin will be
  // theta_min and the maximum angular scale of the last bin with be
  // theta_max.  The last boolean argument controls whether or not an
  // pixel resolution will be assigned to the bins.  If it is false, then
  // the resolution values will all be -1.
  AngularCorrelation(double theta_min, double theta_max,
		     double bins_per_decade, bool assign_resolutions = true);

  // The alternate constructor is used for a linear binning scheme.  The
  // relationship between theta_min and theta_max remains the same and the
  // spacing of the bins is determined based on the requested number of bins.
  AngularCorrelation(uint32_t n_bins, double theta_min, double theta_max,
		     bool assign_resolutions = true);
  ~AngularCorrelation() {
    thetabin_.clear();
  };

  // Find the resolution we would use to calculate correlation functions for
  // each of the bins.  If this method is not called, then the resolution
  // for each bin is set to -1, which would indicate that any correlation
  // calculation with that bin should be done using a pair-based estimator.
  void AssignBinResolutions(double lammin = -70.0, double lammax = 70.0,
			    uint32_t max_resolution = MaxPixelResolution);

  // For small angular scales, it's usually faster and more memory
  // efficient to use a pair-based estimator.  To set this scale, we choose
  // a maximum resolution scale we're willing to use our pixel-based estimator
  // on and modify all smaller angular bins to use the pair-based estimator.
  // The boolean indicates to the object whether this break between the two
  // estimators is being set by hand (default) or should be over-ridden if the
  // methods for calculating the correlation functions are called.
  void SetMaxResolution(uint32_t resolution, bool manual_break = true);

  // Additionally, if we are using regions to calculate correlation functions,
  // we need to set the minimum resolution to match the resolution used to
  // divide the total survey area.
  void SetMinResolution(uint32_t resolution);

  // If we haven't set the break manually, this method attempts to find a
  // reasonable place for it, based on the number of objects involved in the
  // correlation function calculation and the area involved.
  void AutoMaxResolution(uint32_t n_obj, double area);

  // If we're going to use regions to find jack-knife errors, then we need
  // to initialize the AngularBins to handle this state of affairs or possibly
  // clear out previous calculations.
  void InitializeRegions(int16_t n_regions);
  void ClearRegions();
  int16_t NRegion();

  // Some wrapper methods for find the auto-correlation and cross-correlations
  void FindAutoCorrelation(Map& stomp_map,
			   WAngularVector& galaxy,
			   uint8_t random_iterations = 1,
			   bool use_weighted_randoms = false);
  void FindCrossCorrelation(Map& stomp_map_a,
  		    Map& stomp_map_b,
			    WAngularVector& galaxy_a,
			    WAngularVector& galaxy_b,
			    uint8_t random_iterations = 1,
			    bool use_weighted_randoms = false);

  // Variation on the wrapper methods that use regions to calculate the
  // cosmic variance on the correlation functions.  If you don't specify the
  // number of regions to use, the code will default to twice the number of
  // angular bins.
  void FindAutoCorrelationWithRegions(Map& stomp_map,
				      WAngularVector& galaxy,
				      uint8_t random_iterations = 1,
				      uint16_t n_regions = 0,
				      bool use_weighted_randoms = false);
  void FindCrossCorrelationWithRegions(Map& stomp_map_a,
  		         Map& stomp_map_b,
				       WAngularVector& galaxy_a,
				       WAngularVector& galaxy_b,
				       uint8_t random_iterations = 1,
				       uint16_t n_regions = 0,
				       bool use_weighted_randoms = false);

  // In general, the code will use a pair-based method for small angular
  // scanes and a pixel-based method for large angular scales.  In the above
  // methods, this happens automatically.  If you want to run these processes
  // separately, these methods allow you to do this.  If the Map used
  // to call these methods has initialized regions, then the estimators will
  // use the region-based methods.
  void FindPixelAutoCorrelation(Map& stomp_map, WAngularVector& galaxy,
  		                          bool use_weighted_randoms = false);
  void FindPixelAutoCorrelation(ScalarMap& stomp_map);
  void FindPixelCrossCorrelation(Map& stomp_map_a, Map& stomp_map_b,
  		   WAngularVector& galaxy_a,
				 WAngularVector& galaxy_b,
				 bool use_weighted_randoms = false);
  void FindPixelCrossCorrelation(ScalarMap& stomp_map_a,
				 ScalarMap& stomp_map_b);
  void FindPairAutoCorrelation(Map& stomp_map, WAngularVector& galaxy,
			       uint8_t random_iterations = 1, bool use_weighted_randoms = false);
  void FindPairCrossCorrelation(Map& stomp_map_a, Map& stomp_map_b,
				WAngularVector& galaxy_a,
				WAngularVector& galaxy_b,
				uint8_t random_iterations = 1,
				bool use_weighted_randoms = false);

  // Once we're done calculating our correlation function, we can write it out
  // to an ASCII file.  The output format will be
  //
  //   THETA   W(THETA)   dW(THETA)
  //
  // where THETA is the angular scale in degrees and dW(THETA) is the jack-knife
  // error based on regionating the data.  If the angular correlation has been
  // calculated without regions, then this column will be omitted.
  bool Write(const std::string& output_file_name);

  // In some cases, we want to default to using either the pair-based or
  // pixel-based estimator for all of our bins, regardless of angular scale.
  // These methods allow us to over-ride the default behavior of the
  // correlation code.
  void UseOnlyPixels();
  void UseOnlyPairs();

  // Now, some accessor methods for finding the angular range of the bins
  // with a given resolution attached to them (the default value returns the
  // results for all angular bins; for pair-based bins, resolution = -1).
  double ThetaMin(uint32_t resolution = 1);
  double ThetaMax(uint32_t resolution = 1);
  double Sin2ThetaMin(uint32_t resolution = 1);
  double Sin2ThetaMax(uint32_t resolution = 1);
  ThetaIterator Begin(uint32_t resolution = 1);
  ThetaIterator End(uint32_t resolution = 1);
  ThetaIterator Find(ThetaIterator begin, ThetaIterator end,
		     double sin2theta);
  ThetaIterator BinIterator(uint8_t bin_idx = 0);
  uint32_t NBins();
  uint32_t MinResolution();
  uint32_t MaxResolution();

  // The AngularBin accessor methods work fine for calculations that can be
  // done using the data from a single AngularBin, but for the covariance
  // matrix for our measurement we need a method that can access
  // multiple bins at once.  This method returns the (theta_a, theta_b) of the
  // covariance matrix.
  double Covariance(uint8_t bin_idx_a, uint8_t bin_idx_b);

  // Alternatively, we can just write the full covariance matrix to a file.
  // The output format will be
  //
  //   THETA_A  THETA_B  Cov(THETA_A, THETA_B)
  //
  bool WriteCovariance(const std::string& output_file_name);


 private:
  ThetaVector thetabin_;
  ThetaIterator theta_pixel_begin_, theta_pixel_end_;
  ThetaIterator theta_pair_begin_, theta_pair_end_;
  double theta_min_, theta_max_, sin2theta_min_, sin2theta_max_;
  uint32_t min_resolution_, max_resolution_, regionation_resolution_;
  int16_t n_region_;
  bool manual_resolution_break_;
};

} // end namespace Stomp

#endif
