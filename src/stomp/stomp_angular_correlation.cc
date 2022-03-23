// Copyright 2010  All Rights Reserved.
// Author: ryan.scranton@gmail.com (Ryan Scranton)

// STOMP is a set of libraries for doing astrostatistical analysis on the
// celestial sphere.  The goal is to enable descriptions of arbitrary regions
// on the sky which may or may not encode futher spatial information (galaxy
// density, CMB temperature, observational depth, etc.) and to do so in such
// a way as to make the analysis of that data as algorithmically efficient as
// possible.
//
// This file contains the class for calculating angular correlations on
// the sphere.  In general, different methods are more efficient on small vs.
// large angular scales, so this class draws on nearly the entire breadth of
// the STOMP library.

#include "stomp_core.h"
#include "stomp_angular_correlation.h"
#include "stomp_map.h"
#include "stomp_scalar_map.h"
#include "stomp_tree_map.h"

namespace Stomp {

AngularCorrelation::AngularCorrelation() {
  theta_min_ = theta_max_ = sin2theta_min_ = sin2theta_max_ = 0.0;
  min_resolution_ = HPixResolution;
  max_resolution_ = HPixResolution;
  regionation_resolution_ = 0;
  n_region_ = -1;
  manual_resolution_break_ = false;
}

AngularCorrelation::AngularCorrelation(double theta_min, double theta_max,
				       double bins_per_decade,
				       bool assign_resolutions) {
  double unit_double = floor(log10(theta_min))*bins_per_decade;
  double theta = pow(10.0, unit_double/bins_per_decade);

  while (theta < theta_max) {
    if (DoubleGE(theta, theta_min) && (theta < theta_max)) {
      AngularBin thetabin;
      thetabin.SetThetaMin(theta);
      thetabin.SetThetaMax(pow(10.0,(unit_double+1.0)/bins_per_decade));
      thetabin.SetTheta(pow(10.0,0.5*(log10(thetabin.ThetaMin())+
				      log10(thetabin.ThetaMax()))));
      thetabin_.push_back(thetabin);
    }
    unit_double += 1.0;
    theta = pow(10.0,unit_double/bins_per_decade);
  }

  theta_min_ = thetabin_[0].ThetaMin();
  sin2theta_min_ = thetabin_[0].Sin2ThetaMin();
  theta_max_ = thetabin_[thetabin_.size()-1].ThetaMax();
  sin2theta_max_ = thetabin_[thetabin_.size()-1].Sin2ThetaMax();

  regionation_resolution_ = 0;

  if (assign_resolutions) {
    AssignBinResolutions();
    theta_pixel_begin_ = thetabin_.begin();
    theta_pixel_end_ = thetabin_.end();

    theta_pair_begin_ = thetabin_.begin();
    theta_pair_end_ = thetabin_.begin();
  } else {
    min_resolution_ = HPixResolution;
    max_resolution_ = HPixResolution;
    theta_pixel_begin_ = thetabin_.end();
    theta_pixel_end_ = thetabin_.end();

    theta_pair_begin_ = thetabin_.begin();
    theta_pair_end_ = thetabin_.end();
  }

  manual_resolution_break_ = false;
}

AngularCorrelation::AngularCorrelation(uint32_t n_bins,
				       double theta_min, double theta_max,
				       bool assign_resolutions) {
  double dtheta = (theta_max - theta_min)/n_bins;

  for (uint32_t i=0;i<n_bins;i++) {
    AngularBin thetabin;
    thetabin.SetThetaMin(theta_min + i*dtheta);
    thetabin.SetThetaMin(theta_min + (i+1)*dtheta);
    thetabin.SetTheta(0.5*(thetabin.ThetaMin()+thetabin.ThetaMax()));
    thetabin_.push_back(thetabin);
  }

  theta_min_ = thetabin_[0].ThetaMin();
  sin2theta_min_ = thetabin_[0].Sin2ThetaMin();
  theta_max_ = thetabin_[n_bins-1].ThetaMax();
  sin2theta_max_ = thetabin_[n_bins-1].Sin2ThetaMax();

  regionation_resolution_ = 0;

  if (assign_resolutions) {
    AssignBinResolutions();
    theta_pixel_begin_ = thetabin_.begin();
    theta_pixel_end_ = thetabin_.end();

    theta_pair_begin_ = thetabin_.begin();
    theta_pair_end_ = thetabin_.begin();
  } else {
    min_resolution_ = HPixResolution;
    max_resolution_ = HPixResolution;
    theta_pixel_begin_ = thetabin_.end();
    theta_pixel_end_ = thetabin_.end();

    theta_pair_begin_ = thetabin_.begin();
    theta_pair_end_ = thetabin_.end();
  }

  manual_resolution_break_ = false;
}

void AngularCorrelation::AssignBinResolutions(double lammin, double lammax,
					      uint32_t max_resolution) {
  min_resolution_ = MaxPixelResolution;
  max_resolution_ = HPixResolution;

  for (ThetaIterator iter=thetabin_.begin();iter!=thetabin_.end();++iter) {
    iter->CalculateResolution(lammin, lammax, max_resolution);

    if (iter->Resolution() < min_resolution_)
      min_resolution_ = iter->Resolution();
    if (iter->Resolution() > max_resolution_)
      max_resolution_ = iter->Resolution();
  }
}

void AngularCorrelation::SetMaxResolution(uint32_t resolution,
					  bool manual_break) {
  max_resolution_ = resolution;

  // By default every bin is calculated with the pixel-based estimator
  theta_pair_begin_ = thetabin_.begin();
  theta_pair_end_ = thetabin_.begin();

  theta_pixel_begin_ = thetabin_.begin();
  theta_pixel_end_ = thetabin_.end();

  for (ThetaIterator iter=thetabin_.begin();iter!=thetabin_.end();++iter) {
    iter->CalculateResolution();
    if (iter->Resolution() > max_resolution_) {
      iter->SetResolution(0);
      ++theta_pixel_begin_;
      ++theta_pair_end_;
    }
  }

  if (manual_break) manual_resolution_break_ = true;
}

void AngularCorrelation::SetMinResolution(uint32_t resolution) {
  min_resolution_ = resolution;
  for (ThetaIterator iter=theta_pixel_begin_;iter!=theta_pixel_end_;++iter) {
    if (iter->Resolution() < min_resolution_) {
      iter->SetResolution(min_resolution_);
    }
  }
}

void AngularCorrelation::AutoMaxResolution(uint32_t n_obj, double area) {
  uint32_t max_resolution = 2048;

  if (area > 500.0) {
    // large survey limit
    max_resolution = 512;
    if (n_obj < 500000) max_resolution = 64;
    if ((n_obj > 500000) && (n_obj < 2000000)) max_resolution = 128;
    if ((n_obj > 2000000) && (n_obj < 10000000)) max_resolution = 256;
  } else {
    // small survey limit
    if (n_obj < 500000) max_resolution = 256;
    if ((n_obj > 500000) && (n_obj < 2000000)) max_resolution = 512;
    if ((n_obj > 2000000) && (n_obj < 10000000)) max_resolution = 1024;
  }

  std::cout << "Stomp::AngularCorrelation::AutoMaxResolution - " <<
    "Setting maximum resolution to " <<
    static_cast<int>(max_resolution) << "...\n";

  SetMaxResolution(max_resolution, false);
}

void AngularCorrelation::InitializeRegions(int16_t n_regions) {
  n_region_ = n_regions;
  for (ThetaIterator iter=Begin();iter!=End();++iter)
    iter->InitializeRegions(n_region_);
}

void AngularCorrelation::ClearRegions() {
  n_region_ = 0;
  for (ThetaIterator iter=Begin();iter!=End();++iter)
    iter->ClearRegions();
  regionation_resolution_ = 0;
}

int16_t AngularCorrelation::NRegion() {
  return n_region_;
}

void AngularCorrelation::FindAutoCorrelation(Map& stomp_map,
					     WAngularVector& galaxy,
					     uint8_t random_iterations,
					     bool use_weighted_randoms) {
  if (!manual_resolution_break_)
    AutoMaxResolution(galaxy.size(), stomp_map.Area());

  if (theta_pixel_begin_ != theta_pixel_end_)
    FindPixelAutoCorrelation(stomp_map, galaxy, use_weighted_randoms);

  if (theta_pair_begin_ != theta_pair_end_)
    FindPairAutoCorrelation(stomp_map, galaxy, random_iterations,
    		                    use_weighted_randoms);
}

void AngularCorrelation::FindCrossCorrelation(Map& stomp_map_a,
		            Map& stomp_map_b,
					      WAngularVector& galaxy_a,
					      WAngularVector& galaxy_b,
					      uint8_t random_iterations,
					      bool use_weighted_randoms) {
  if (!manual_resolution_break_) {
    uint32_t n_obj =
      static_cast<uint32_t>(sqrt(1.0*galaxy_a.size()*galaxy_b.size()));
    double area = stomp_map_a.Area();
    if (stomp_map_b.Area() < area) area = stomp_map_b.Area();
    AutoMaxResolution(n_obj, area);
  }

  if (theta_pixel_begin_ != theta_pixel_end_)
    FindPixelCrossCorrelation(stomp_map_a, stomp_map_b, galaxy_a, galaxy_b,
    		                      use_weighted_randoms);

  if (theta_pair_begin_ != theta_pair_end_)
    FindPairCrossCorrelation(stomp_map_a, stomp_map_b, galaxy_a, galaxy_b,
    		                     random_iterations, use_weighted_randoms);
}

void AngularCorrelation::FindAutoCorrelationWithRegions(Map& stomp_map,
							WAngularVector& gal,
							uint8_t random_iter,
							uint16_t n_regions,
							bool use_weighted_randoms) {
  if (!manual_resolution_break_)
    AutoMaxResolution(gal.size(), stomp_map.Area());

  if (n_regions == 0) n_regions = static_cast<uint16_t>(2*thetabin_.size());
  std::cout << "Stomp::AngularCorrelation::FindAutoCorrelationWithRegions - " <<
    "Regionating with " << n_regions << " regions...\n";
  uint16_t n_true_regions = stomp_map.NRegion();
  if (n_true_regions == 0)
    n_true_regions = stomp_map.InitializeRegions(n_regions);

  if (n_true_regions != n_regions) {
    std::cout << "Stomp::AngularCorrelation::" <<
      "FindAutoCorrelationWithRegions - Splitting into " << n_true_regions <<
      " rather than " << n_regions << "...\n";
    n_regions = n_true_regions;
  }

  regionation_resolution_ = stomp_map.RegionResolution();

  std::cout << "Stomp::AngularCorrelation::FindAutoCorrelationWithRegions - " <<
    "Regionated at " << regionation_resolution_ << "...\n";
  InitializeRegions(n_regions);
  if (regionation_resolution_ > min_resolution_)
    SetMinResolution(regionation_resolution_);

  if (regionation_resolution_ > max_resolution_) {
    std::cout << "Stomp::AngularCorrelation::FindAutoCorrelationWithRegions " <<
      " - regionation resolution (" << regionation_resolution_ <<
      ") exceeds maximum resolution (" << max_resolution_ << ")\n" <<
      "\tReseting to use pair-based estimator only\n";
    UseOnlyPairs();
  }

  if (theta_pixel_begin_ != theta_pixel_end_)
    FindPixelAutoCorrelation(stomp_map, gal, use_weighted_randoms);

  if (theta_pair_begin_ != theta_pair_end_)
    FindPairAutoCorrelation(stomp_map, gal, random_iter,
    		                    use_weighted_randoms);
}

void AngularCorrelation::FindCrossCorrelationWithRegions(Map& stomp_map_a,
		           Map& stomp_map_b,
							 WAngularVector& gal_a,
							 WAngularVector& gal_b,
							 uint8_t random_iter,
							 uint16_t n_regions,
							 bool use_weighted_randoms) {
  if (!manual_resolution_break_) {
    uint32_t n_obj =
      static_cast<uint32_t>(sqrt(1.0*gal_a.size()*gal_b.size()));
    AutoMaxResolution(n_obj, stomp_map_a.Area());
  }

  if (n_regions == 0) n_regions = static_cast<uint16_t>(2*thetabin_.size());
  uint16_t n_true_regions = stomp_map_a.NRegion();
  if (n_true_regions == 0)
    n_true_regions = stomp_map_a.InitializeRegions(n_regions);
  if (n_true_regions != n_regions) {
    std::cout << "Stomp::AngularCorrelation::FindCrossCorrelationWithRegions" <<
      " - Splitting into " << n_true_regions << " rather than " <<
      n_regions << "...\n";
    n_regions = n_true_regions;
  }

  regionation_resolution_ = stomp_map_a.RegionResolution();

  std::cout << "Stomp::AngularCorrelation::FindCrossCorrelationWithRegions" <<
    " - Regionated at " << regionation_resolution_ << "...\n";
  InitializeRegions(n_regions);
  if (regionation_resolution_ > min_resolution_)
    SetMinResolution(regionation_resolution_);

  if (regionation_resolution_ > max_resolution_) {
    std::cout << "Stomp::AngularCorrelation::FindAutoCorrelationWithRegions " <<
      " - regionation resolution (" << regionation_resolution_ <<
      ") exceeds maximum resolution (" << max_resolution_ << ")\n" <<
      "\tReseting to use pair-based estimator only\n";
    UseOnlyPairs();
  }

  if (theta_pixel_begin_ != theta_pixel_end_)
    FindPixelCrossCorrelation(stomp_map_a, stomp_map_b, gal_a, gal_b,
    		                      use_weighted_randoms);

  if (theta_pair_begin_ != theta_pair_end_)
    FindPairCrossCorrelation(stomp_map_a, stomp_map_b, gal_a, gal_b,
    		                     random_iter, use_weighted_randoms);
}

void AngularCorrelation::FindPixelAutoCorrelation(Map& stomp_map,
						  WAngularVector& galaxy, bool use_weighted_randoms) {

  std::cout << "Stomp::AngularCorrelation::FindPixelAutoCorrelation - " <<
    "Initializing ScalarMap at " << max_resolution_ << "...\n";
  ScalarMap* scalar_map = new ScalarMap(stomp_map, max_resolution_,
					ScalarMap::DensityField, false, use_weighted_randoms);
  if (stomp_map.NRegion() > 0) {
    std::cout << "Stomp::AngularCorrelation::FindPixelAutoCorrelation - " <<
      "Intializing regions...\n";
    scalar_map->InitializeRegions(stomp_map);
  }

  std::cout << "Stomp::AngularCorrelation::FindPixelAutoCorrelation - " <<
    "Adding points to ScalarMap...\n";
  uint32_t n_filtered = 0;
  uint32_t n_kept = 0;
  for (WAngularIterator iter=galaxy.begin();iter!=galaxy.end();++iter) {
    if (stomp_map.Contains(*iter)) {
      n_filtered++;
      if (scalar_map->AddToMap(*iter)) n_kept++;
    }
  }

  if (n_filtered != galaxy.size())
    std::cout << "Stomp::AngularCorrelation::FindPixelAutoCorrelation - " <<
      "WARNING: " << galaxy.size() - n_filtered << "/" << galaxy.size() <<
      " objects not within input Map.\n";

  if (n_filtered != n_kept)
    std::cout << "Stomp::AngularCorrelation::FindPixelAutoCorrelation - " <<
      "WARNING: Failed to place " << n_filtered - n_kept << "/" <<
      n_filtered << " filtered objects into ScalarMap.\n";

  FindPixelAutoCorrelation(*scalar_map);

  delete scalar_map;
}

void AngularCorrelation::FindPixelAutoCorrelation(ScalarMap& scalar_map) {
  for (ThetaIterator iter=Begin(scalar_map.Resolution());
       iter!=End(scalar_map.Resolution());++iter) {
    std::cout << "Stomp::AngularCorrelation::FindPixelAutoCorrelation - \n";
    if (scalar_map.NRegion() > 0) {
      std::cout << "\tAuto-correlating with regions at " <<
	scalar_map.Resolution() << "...\n";
      scalar_map.AutoCorrelateWithRegions(iter);
    } else {
      scalar_map.AutoCorrelate(iter);
    }
  }

  for (uint32_t resolution=scalar_map.Resolution()/2;
       resolution>=min_resolution_;resolution/=2) {
    ScalarMap* sub_scalar_map =
      new ScalarMap(scalar_map,resolution);
    if (scalar_map.NRegion() > 0) sub_scalar_map->InitializeRegions(scalar_map);
    for (ThetaIterator iter=Begin(resolution);iter!=End(resolution);++iter) {
      if (scalar_map.NRegion() > 0) {
	std::cout << "\tAuto-correlating with regions at " <<
	  sub_scalar_map->Resolution() << "...\n";
	sub_scalar_map->AutoCorrelateWithRegions(iter);
      } else {
	std::cout << "\tAuto-correlating at " <<
	  sub_scalar_map->Resolution() << "...\n";
	sub_scalar_map->AutoCorrelate(iter);
      }
    }

    delete sub_scalar_map;
  }
}

void AngularCorrelation::FindPixelCrossCorrelation(Map& stomp_map_a,
		           Map& stomp_map_b,
						   WAngularVector& galaxy_a,
						   WAngularVector& galaxy_b,
						   bool use_weighted_randoms) {

  std::cout << "Stomp::AngularCorrelation::FindPixelCrossCorrelation - " <<
    "Initialing ScalarMaps at " << max_resolution_ << "...\n";
  ScalarMap* scalar_map_a = new ScalarMap(stomp_map_a, max_resolution_,
	    ScalarMap::DensityField, 0.0000001, false, use_weighted_randoms);
  ScalarMap* scalar_map_b = new ScalarMap(stomp_map_b, max_resolution_,
	    ScalarMap::DensityField, 0.0000001, false, use_weighted_randoms);

  if (stomp_map_a.NRegion() > 0) {
    std::cout << "Stomp::AngularCorrelation::FindPixelCrossCorrelation - " <<
      "Intializing regions...\n";
    scalar_map_a->InitializeRegions(stomp_map_a);
    scalar_map_b->InitializeRegions(stomp_map_b);
  }

  uint32_t n_filtered = 0;
  uint32_t n_kept = 0;
  for (WAngularIterator iter=galaxy_a.begin();iter!=galaxy_a.end();++iter) {
    if (stomp_map_a.Contains(*iter)) {
      n_filtered++;
//      if (use_weighted_randoms){
//      	Pixel tmp_pix = Pixel(*iter, max_resolution_);
//      	WeightedAngularCoordinate tmp_wang = WeightedAngularCoordinate(
//      			iter->UnitSphereX(), iter->UnitSphereY(), iter->UnitSphereZ(),
//      			iter->Weight() / stomp_map_a.FindAverageWeight(tmp_pix));
//      	if (scalar_map_a->AddToMap(tmp_wang)) n_kept++;
//      } else {
//
//      }
      if (scalar_map_a->AddToMap(*iter)) n_kept++;
    }
  }

  if (n_filtered != galaxy_a.size())
    std::cout << "Stomp::AngularCorrelation::FindPixelCrossCorrelation - " <<
      "WARNING: " << galaxy_a.size() - n_filtered <<
      "/" << galaxy_a.size() << " objects not within input Map.\n";
  if (n_filtered != n_kept)
    std::cout << "Stomp::AngularCorrelation::FindPixelCrossCorrelation - " <<
      "WARNING: Failed to place " << n_filtered - n_kept <<
      "/" << n_filtered << " filtered objects into ScalarMap.\n";

  n_filtered = 0;
  n_kept = 0;
  for (WAngularIterator iter=galaxy_b.begin();iter!=galaxy_b.end();++iter) {
    if (stomp_map_b.Contains(*iter)) {
      n_filtered++;
      if (scalar_map_b->AddToMap(*iter)) n_kept++;
    }
  }

  if (n_filtered != galaxy_b.size())
    std::cout << "Stomp::AngularCorrelation::FindPixelCrossCorrelation - " <<
      "WARNING: " << galaxy_b.size() - n_filtered << "/" << galaxy_b.size() <<
      " objects not within input Map.\n";
  if (n_filtered != n_kept)
    std::cout << "Stomp::AngularCorrelation::FindPixelCrossCorrelation - " <<
      "WARNING: Failed to place " << n_filtered - n_kept <<
      "/" << n_filtered << " filtered objects into ScalarMap.\n";

  FindPixelCrossCorrelation(*scalar_map_a, *scalar_map_b);

  delete scalar_map_a;
  delete scalar_map_b;
}

void AngularCorrelation::FindPixelCrossCorrelation(ScalarMap& map_a,
						   ScalarMap& map_b) {
  if (map_a.Resolution() != map_b.Resolution()) {
    std::cout << "Stomp::AngularCorrelation::FindPixelCrossCorrelation - " <<
      "Incompatible density map resolutions.  Exiting!\n";
    exit(1);
  }

  for (ThetaIterator iter=Begin(map_a.Resolution());
       iter!=End(map_a.Resolution());++iter) {
    if (map_a.NRegion() > 0) {
      map_a.CrossCorrelateWithRegions(map_b, iter);
    } else {
      map_a.CrossCorrelate(map_b, iter);
    }
  }

  for (uint32_t resolution=map_a.Resolution()/2;
       resolution>=min_resolution_;resolution/=2) {
    ScalarMap* sub_map_a = new ScalarMap(map_a, resolution);
    ScalarMap* sub_map_b = new ScalarMap(map_b, resolution);

    if (map_a.NRegion() > 0) {
      sub_map_a->InitializeRegions(map_a);
      sub_map_b->InitializeRegions(map_a);
    }

    std::cout << "Stomp::AngularCorrelation::FindPixelCrossCorrelation - \n";
    for (ThetaIterator iter=Begin(resolution);iter!=End(resolution);++iter) {
      if (map_a.NRegion() > 0) {
	std::cout << "\tCross-correlating with regions at " <<
	  sub_map_a->Resolution() << "...\n";
	sub_map_a->CrossCorrelateWithRegions(*sub_map_b, iter);
      } else {
	std::cout << "\tCross-correlating at " <<
	  sub_map_a->Resolution() << "...\n";
	sub_map_a->CrossCorrelate(*sub_map_b, iter);
      }
    }
    delete sub_map_a;
    delete sub_map_b;
  }
}

void AngularCorrelation::FindPairAutoCorrelation(Map& stomp_map,
						 WAngularVector& galaxy,
						 uint8_t random_iterations,
						 bool use_weighted_randoms) {
  int16_t tree_resolution = min_resolution_;
  if (regionation_resolution_ > min_resolution_)
    tree_resolution = regionation_resolution_;

  TreeMap* galaxy_tree = new TreeMap(tree_resolution, 200);

  uint32_t n_kept = 0;
  uint32_t n_fail = 0;
  for (WAngularIterator iter=galaxy.begin();iter!=galaxy.end();++iter) {
    if (stomp_map.Contains(*iter)) {
      n_kept++;
      if (!galaxy_tree->AddPoint(*iter)) {
	std::cout << "Stomp::AngularCorrelation::FindPairAutoCorrelation - " <<
	  "Failed to add point: " << iter->Lambda() << ", " <<
	  iter->Eta() << "\n";
	n_fail++;
      }
    }
  }
  std::cout << "Stomp::AngularCorrelation::FindPairAutoCorrelation - " <<
    n_kept - n_fail << "/" << galaxy.size() << " objects added to tree;" <<
    n_fail << " failed adds...\n";


  if (stomp_map.NRegion() > 0) {
    if (!galaxy_tree->InitializeRegions(stomp_map)) {
      std::cout << "Stomp::AngularCorrelation::FindPairAutoCorrelation - " <<
	"Failed to initialize regions on TreeMap  Exiting.\n";
      exit(2);
    }
  }

  // Galaxy-galaxy
  std::cout << "Stomp::AngularCorrelation::FindPairAutoCorrelation - \n";
  std::cout << "\tGalaxy-galaxy pairs...\n";
  for (ThetaIterator iter=theta_pair_begin_;iter!=theta_pair_end_;++iter) {
    if (stomp_map.NRegion() > 0) {
      galaxy_tree->FindWeightedPairsWithRegions(galaxy, *iter);
    } else {
      galaxy_tree->FindWeightedPairs(galaxy, *iter);
    }
    iter->MoveWeightToGalGal();
  }

  // Done with the galaxy-based tree, so we can delete that memory.
  delete galaxy_tree;

  // Before we start on the random iterations, we'll zero out the data fields
  // for those counts.
  for (ThetaIterator iter=theta_pair_begin_;iter!=theta_pair_end_;++iter) {
    iter->ResetGalRand();
    iter->ResetRandGal();
    iter->ResetRandRand();
  }

  std::cout << "Stomp::AngularCorrelation::FindPairAutoCorrelation - \n";
  for (uint8_t rand_iter=0;rand_iter<random_iterations;rand_iter++) {
    std::cout << "\tRandom iteration " <<
      static_cast<int>(rand_iter) << "...\n";

    // Generate set of random points based on the input galaxy file and map.
    WAngularVector random_galaxy;
    stomp_map.GenerateRandomPoints(random_galaxy, galaxy, use_weighted_randoms);

    // Create the TreeMap from those random points.
    TreeMap* random_tree = new TreeMap(tree_resolution, 200);

    for (WAngularIterator iter=random_galaxy.begin();
	 iter!=random_galaxy.end();++iter) {
      if (!random_tree->AddPoint(*iter)) {
	std::cout << "Stomp::AngularCorrelation::FindPairAutoCorrelation - " <<
	  "Failed to add point: " << iter->Lambda() << ", " <<
	  iter->Eta() << "\n";
      }
    }

    if (stomp_map.NRegion() > 0) {
      if (!random_tree->InitializeRegions(stomp_map)) {
	std::cout << "Stomp::AngularCorrelation::FindPairAutoCorrelation - " <<
	  "Failed to initialize regions on TreeMap  Exiting.\n";
	exit(2);
      }
    }

    // Galaxy-Random -- there's a symmetry here, so the results go in GalRand
    // and RandGal.
    for (ThetaIterator iter=theta_pair_begin_;iter!=theta_pair_end_;++iter) {
      if (stomp_map.NRegion() > 0) {
	random_tree->FindWeightedPairsWithRegions(galaxy, *iter);
      } else {
	random_tree->FindWeightedPairs(galaxy, *iter);
      }
      iter->MoveWeightToGalRand(true);
    }

    // Random-Random
    for (ThetaIterator iter=theta_pair_begin_;iter!=theta_pair_end_;++iter) {
      if (stomp_map.NRegion() > 0) {
	random_tree->FindWeightedPairsWithRegions(random_galaxy, *iter);
      } else {
	random_tree->FindWeightedPairs(random_galaxy, *iter);
      }
      iter->MoveWeightToRandRand();
    }

    delete random_tree;
  }

  // Finally, we rescale our random pair counts to normalize them to the
  // number of input objects.
  for (ThetaIterator iter=theta_pair_begin_;iter!=theta_pair_end_;++iter) {
    iter->RescaleGalRand(1.0*random_iterations);
    iter->RescaleRandGal(1.0*random_iterations);
    iter->RescaleRandRand(1.0*random_iterations);
  }
}

void AngularCorrelation::FindPairCrossCorrelation(Map& stomp_map_a,
		          Map& stomp_map_b,
						  WAngularVector& galaxy_a,
						  WAngularVector& galaxy_b,
						  uint8_t random_iterations,
						  bool use_weighted_randoms) {
  int16_t tree_resolution = min_resolution_;
  if (regionation_resolution_ > min_resolution_)
    tree_resolution = regionation_resolution_;

  TreeMap* galaxy_tree_a = new TreeMap(tree_resolution, 200);

  uint32_t n_kept = 0;
  uint32_t n_fail = 0;
  for (WAngularIterator iter=galaxy_a.begin();iter!=galaxy_a.end();++iter) {
    if (stomp_map_a.Contains(*iter)) {
      n_kept++;
      if (!galaxy_tree_a->AddPoint(*iter)) {
	std::cout << "Stomp::AngularCorrelation::FindPairCrossCorrelation - " <<
	  "Failed to add point: " << iter->Lambda() << ", " <<
	  iter->Eta() << "\n";
	n_fail++;
      }
    }
  }

  std::cout << "Stomp::AngularCorrelation::FindPairCrossCorrelation - " <<
    n_kept - n_fail << "/" << galaxy_a.size() <<
    " objects added to tree;" << n_fail << " failed adds...\n";

  if (stomp_map_a.NRegion() > 0) {
    if (!galaxy_tree_a->InitializeRegions(stomp_map_a)) {
      std::cout << "Stomp::AngularCorrelation::FindPairCrossCorrelation - " <<
	"Failed to initialize regions on TreeMap  Exiting.\n";
      exit(2);
    }
  }

  // Galaxy-galaxy
  std::cout << "Stomp::AngularCorrelation::FindPairCrossCorrelation - \n";
  std::cout << "\tGalaxy-galaxy pairs...\n";
  for (ThetaIterator iter=theta_pair_begin_;iter!=theta_pair_end_;++iter) {
    if (stomp_map_a.NRegion() > 0) {
      galaxy_tree_a->FindWeightedPairsWithRegions(galaxy_b, *iter);
    } else {
      galaxy_tree_a->FindWeightedPairs(galaxy_b, *iter);
    }
    // If the number of random iterations is 0, then we're doing a
    // WeightedCrossCorrelation instead of a cross-correlation between 2
    // population densities.  In that case, we want the ratio between the
    // WeightedPairs and Pairs, so we keep the values in the Weight and
    // Counter fields.
    if (random_iterations > 0) iter->MoveWeightToGalGal();
  }

  // Before we start on the random iterations, we'll zero out the data fields
  // for those counts.
  for (ThetaIterator iter=theta_pair_begin_;iter!=theta_pair_end_;++iter) {
    iter->ResetGalRand();
    iter->ResetRandGal();
    iter->ResetRandRand();
  }

  std::cout << "Stomp::AngularCorrelation::FindPairCrossCorrelation - \n";
  for (uint8_t rand_iter=0;rand_iter<random_iterations;rand_iter++) {
    std::cout << "\tRandom iteration " <<
      static_cast<int>(rand_iter) << "...\n";
    WAngularVector random_galaxy_a;
    stomp_map_a.GenerateRandomPoints(random_galaxy_a, galaxy_a,
    		                             use_weighted_randoms);

    WAngularVector random_galaxy_b;
    stomp_map_b.GenerateRandomPoints(random_galaxy_b, galaxy_b,
    		                             use_weighted_randoms);

    // Galaxy-Random
    for (ThetaIterator iter=theta_pair_begin_;iter!=theta_pair_end_;++iter) {
      if (stomp_map_a.NRegion() > 0) {
	galaxy_tree_a->FindWeightedPairsWithRegions(random_galaxy_b, *iter);
      } else {
	galaxy_tree_a->FindWeightedPairs(random_galaxy_b, *iter);
      }
      iter->MoveWeightToGalRand();
    }

    TreeMap* random_tree_a = new TreeMap(tree_resolution, 200);

    for (WAngularIterator iter=random_galaxy_a.begin();
	 iter!=random_galaxy_a.end();++iter) {
      if (!random_tree_a->AddPoint(*iter)) {
	std::cout << "Stomp::AngularCorrelation::FindPairCrossCorrelation - " <<
	  "Failed to add point: " << iter->Lambda() << ", " <<
	  iter->Eta() << "\n";
      }
    }

    if (stomp_map_a.NRegion() > 0) {
      if (!random_tree_a->InitializeRegions(stomp_map_a)) {
	std::cout << "Stomp::AngularCorrelation::FindPairCrossCorrelation - " <<
	  "Failed to initialize regions on TreeMap  Exiting.\n";
	exit(2);
      }
    }

    // Random-Galaxy
    for (ThetaIterator iter=theta_pair_begin_;iter!=theta_pair_end_;++iter) {
      if (stomp_map_a.NRegion() > 0) {
	random_tree_a->FindWeightedPairsWithRegions(galaxy_b, *iter);
      } else {
	random_tree_a->FindWeightedPairs(galaxy_b, *iter);
      }
      iter->MoveWeightToRandGal();
    }

    // Random-Random
    for (ThetaIterator iter=theta_pair_begin_;iter!=theta_pair_end_;++iter) {
      if (stomp_map_a.NRegion() > 0) {
	random_tree_a->FindWeightedPairsWithRegions(random_galaxy_b, *iter);
      } else {
	random_tree_a->FindWeightedPairs(random_galaxy_b, *iter);
      }
      iter->MoveWeightToRandRand();
    }

    delete random_tree_a;
  }

  delete galaxy_tree_a;

  // Finally, we rescale our random pair counts to normalize them to the
  // number of input objects.
  for (ThetaIterator iter=theta_pair_begin_;iter!=theta_pair_end_;++iter) {
    iter->RescaleGalRand(1.0*random_iterations);
    iter->RescaleRandGal(1.0*random_iterations);
    iter->RescaleRandRand(1.0*random_iterations);
  }
}

bool AngularCorrelation::Write(const std::string& output_file_name) {
  bool wrote_file = false;

  std::ofstream output_file(output_file_name.c_str());

  if (output_file.is_open()) {
    wrote_file = true;

    for (ThetaIterator iter=Begin();iter!=End();++iter) {
      if (iter->NRegion()) {
	output_file << std::setprecision(6) << iter->Theta() << " " <<
	  iter->MeanWtheta()  << " " << iter->MeanWthetaError() << "\n";
      } else {
	if (iter->Resolution() == 0) {
	  output_file << std::setprecision(6) << iter->Theta() << " " <<
	    iter->Wtheta()  << " " << iter->GalGal() << " " <<
	    iter->GalRand() << " " << iter->RandGal() << " " <<
	    iter->RandRand() << "\n";
	} else {
	  output_file << std::setprecision(6) << iter->Theta() << " " <<
	    iter->Wtheta()  << " " << iter->PixelWtheta() << " " <<
	    iter->PixelWeight() << "\n";
	}
      }
    }

    output_file.close();
  }

  return wrote_file;
}

void AngularCorrelation::UseOnlyPixels() {
  AssignBinResolutions();
  theta_pixel_begin_ = thetabin_.begin();
  theta_pixel_end_ = thetabin_.end();

  theta_pair_begin_ = thetabin_.begin();
  theta_pair_end_ = thetabin_.begin();
}

void AngularCorrelation::UseOnlyPairs() {
  theta_pixel_begin_ = thetabin_.end();
  theta_pixel_end_ = thetabin_.end();

  theta_pair_begin_ = thetabin_.begin();
  theta_pair_end_ = thetabin_.end();
  for (ThetaIterator iter=thetabin_.begin();iter!=thetabin_.end();++iter) {
    iter->SetResolution(0);
  }
  manual_resolution_break_ = true;
  
}

double AngularCorrelation::ThetaMin(uint32_t resolution) {
  double theta_min = -1.0;
  if ((resolution < HPixResolution) ||
      (resolution > MaxPixelResolution) ||
      (resolution % 2 != 0)) {
    if (resolution == 0) {
      theta_min = theta_pair_begin_->ThetaMin();
    } else {
      theta_min = theta_min_;
    }
  } else {
    AngularBin theta;
    theta.SetResolution(resolution);
    ThetaPair iter = equal_range(theta_pixel_begin_,thetabin_.end(),
				 theta,AngularBin::ReverseResolutionOrder);
    if (iter.first != iter.second) {
      theta_min = iter.first->ThetaMin();
    }
  }

  return theta_min;
}

double AngularCorrelation::ThetaMax(uint32_t resolution) {
  double theta_max = -1.0;
  if ((resolution < HPixResolution) ||
      (resolution > MaxPixelResolution) ||
      (resolution % 2 != 0)) {
    if (resolution == 0) {
      theta_max = theta_pair_end_->ThetaMin();
    } else {
      theta_max = theta_max_;
    }
  } else {
    AngularBin theta;
    theta.SetResolution(resolution);
    ThetaPair iter = equal_range(theta_pixel_begin_,thetabin_.end(),
				 theta,AngularBin::ReverseResolutionOrder);
    if (iter.first != iter.second) {
      --iter.second;
      theta_max = iter.second->ThetaMax();
    }
  }

  return theta_max;
}

double AngularCorrelation::Sin2ThetaMin(uint32_t resolution) {
  double sin2theta_min = -1.0;
  if ((resolution < HPixResolution) ||
      (resolution > MaxPixelResolution) ||
      (resolution % 2 != 0)) {
    if (resolution == 0) {
      sin2theta_min = theta_pair_begin_->Sin2ThetaMin();
    } else {
      sin2theta_min = sin2theta_min_;
    }
  } else {
    AngularBin theta;
    theta.SetResolution(resolution);
    ThetaPair iter = equal_range(theta_pixel_begin_,thetabin_.end(),
				 theta,AngularBin::ReverseResolutionOrder);
    if (iter.first != iter.second) {
      sin2theta_min = iter.first->Sin2ThetaMin();
    }
  }

  return sin2theta_min;
}

double AngularCorrelation::Sin2ThetaMax(uint32_t resolution) {
  double sin2theta_max = -1.0;
  if ((resolution < HPixResolution) ||
      (resolution > MaxPixelResolution) ||
      (resolution % 2 != 0)) {
    if (resolution == 0) {
      sin2theta_max = theta_pair_end_->Sin2ThetaMin();
    } else {
      sin2theta_max = sin2theta_max_;
    }
  } else {
    AngularBin theta;
    theta.SetResolution(resolution);
    ThetaPair iter = equal_range(theta_pixel_begin_,thetabin_.end(),
				 theta,AngularBin::ReverseResolutionOrder);
    if (iter.first != iter.second) {
      --iter.second;
      sin2theta_max = iter.second->Sin2ThetaMax();
    }
  }

  return sin2theta_max;
}

ThetaIterator AngularCorrelation::Begin(uint32_t resolution) {
  if ((resolution < HPixResolution) ||
      (resolution > MaxPixelResolution) ||
      (resolution % 2 != 0)) {
    if (resolution == 0) {
      return theta_pair_begin_;
    } else {
      return thetabin_.begin();
    }
  } else {
    AngularBin theta;
    theta.SetResolution(resolution);
    ThetaPair iter = equal_range(theta_pixel_begin_, thetabin_.end(),
				 theta, AngularBin::ReverseResolutionOrder);
    return iter.first;
  }
}

ThetaIterator AngularCorrelation::End(uint32_t resolution) {
  if ((resolution < HPixResolution) ||
      (resolution > MaxPixelResolution) ||
      (resolution % 2 != 0)) {
    if (resolution == 0) {
      return theta_pair_end_;
    } else {
      return thetabin_.end();
    }
  } else {
    AngularBin theta;
    theta.SetResolution(resolution);
    ThetaPair iter = equal_range(theta_pixel_begin_,thetabin_.end(),
				 theta,AngularBin::ReverseResolutionOrder);
    return iter.second;
  }
}

ThetaIterator AngularCorrelation::Find(ThetaIterator begin,
				       ThetaIterator end,
				       double sin2theta) {
  ThetaIterator top = --end;
  ThetaIterator bottom = begin;
  ThetaIterator iter;

  if ((sin2theta < bottom->Sin2ThetaMin()) ||
      (sin2theta > top->Sin2ThetaMax())) {
    iter = ++end;
  } else {
    ++top;
    --bottom;
    while (top-bottom > 1) {
      iter = bottom + (top - bottom)/2;
      if (sin2theta < iter->Sin2ThetaMin()) {
        top = iter;
      } else {
        bottom = iter;
      }
    }
    iter = bottom;
  }

  return iter;
}

ThetaIterator AngularCorrelation::BinIterator(uint8_t bin_idx) {
  ThetaIterator iter = thetabin_.begin();

  for (uint8_t i=0;i<bin_idx;++i) {
    if (iter != thetabin_.end()) ++iter;
  }

  return iter;
}

uint32_t AngularCorrelation::NBins() {
  return thetabin_.size();
}

uint32_t AngularCorrelation::MinResolution() {
  return min_resolution_;
}

uint32_t AngularCorrelation::MaxResolution() {
  return max_resolution_;
}

double AngularCorrelation::Covariance(uint8_t bin_idx_a, uint8_t bin_idx_b) {
  double covariance = 0.0;

  ThetaIterator theta_a = BinIterator(bin_idx_a);
  ThetaIterator theta_b = BinIterator(bin_idx_b);

  if ((theta_a->NRegion() == theta_b->NRegion()) &&
      (theta_a->NRegion() > 0)) {
    // We have a valid number of regions and both bins were calculated with
    // the same number of regions, so we calculate the jack-knife covariance.
    uint16_t n_region = theta_a->NRegion();
    double mean_wtheta_a = theta_a->MeanWtheta();
    double mean_wtheta_b = theta_b->MeanWtheta();

    for (uint16_t region_iter=0;region_iter<n_region;region_iter++) {
      covariance +=
	(theta_a->Wtheta(region_iter) - mean_wtheta_a)*
	(theta_b->Wtheta(region_iter) - mean_wtheta_b);
    }

    covariance *= (n_region - 1.0)*(n_region - 1.0)/(1.0*n_region*n_region);
  } else {
    // If the above doesn't hold, then we're reduced to using Poisson errors.
    // In this case, we only have non-zero elements on the diagonal of the
    // covariance matrix; all others are zero by definition.
    if (bin_idx_a == bin_idx_b) {
      covariance = theta_a->WthetaError()*theta_a->WthetaError();
    }
  }

  return covariance;
}

bool AngularCorrelation::WriteCovariance(const std::string& output_file_name) {
  bool wrote_file = false;

  std::ofstream output_file(output_file_name.c_str());

  if (output_file.is_open()) {
    wrote_file = true;

    for (uint8_t theta_idx_a=0;theta_idx_a<thetabin_.size();theta_idx_a++) {
      for (uint8_t theta_idx_b=0;theta_idx_b<thetabin_.size();theta_idx_b++) {
	output_file << std::setprecision(6) <<
	  thetabin_[theta_idx_a].Theta() << " " <<
	  thetabin_[theta_idx_b].Theta() << " " <<
	  Covariance(theta_idx_a, theta_idx_b) << "\n";
      }
    }

    output_file.close();
  }

  return wrote_file;
}

} // end namespace Stomp
