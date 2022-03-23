#include <stdint.h>
#include <iostream>
#include <math.h>
#include <string>
#include <gflags/gflags.h>
#include "stomp_core.h"
#include "stomp_angular_coordinate.h"
#include "stomp_angular_bin.h"
#include "stomp_angular_correlation.h"
#include "stomp_pixel.h"
#include "stomp_map.h"
#include "stomp_scalar_map.h"

void ScalarMapBasicTests() {
  // Now we start testing the density map functions.  First we make a density
  // map at resolution 128 using our previous Stomp::Map (recall that it was
  // created at resolution 256).  There will be more pixels in the density
  // map, but we should find that their areas are equal.
  std::cout << "\n";
  std::cout << "*****************************\n";
  std::cout << "*** ScalarMap Basic Tests ***\n";
  std::cout << "*****************************\n";
  double theta = 3.0;
  Stomp::AngularCoordinate ang(60.0, 0.0, Stomp::AngularCoordinate::Survey);
  Stomp::Pixel tmp_pix(ang, 256);
  Stomp::PixelVector annulus_pix;
  tmp_pix.WithinRadius(theta, annulus_pix);
  Stomp::Map* stomp_map = new Stomp::Map(annulus_pix);
  Stomp::ScalarMap* scalar_map =
    new Stomp::ScalarMap(*stomp_map, 128, Stomp::ScalarMap::DensityField);

  std::cout << "\t" << scalar_map->Size() << " pixels in the scalar map.\n";
  std::cout << "\t" << stomp_map->Size() << " pixels in the source map.\n";
  std::cout << "\t" << scalar_map->Area() <<
    " sq. degrees in the density map.\n";
  std::cout << "\t" << stomp_map->Area() <<
    " sq. degrees in the source map.\n";

  // Now initialize a version of the map from a vector of ScalarPixels
  // generated from annulus_pix.
  std::cout << "Initializing second map built from ScalarPixels...\n";
  Stomp::ScalarVector scalar_pix;
  scalar_pix.reserve(annulus_pix.size());

  for (Stomp::PixelIterator iter=annulus_pix.begin();
       iter != annulus_pix.end();++iter) {
    scalar_pix.push_back(Stomp::ScalarPixel(iter->PixelX(), iter->PixelY(),
                                            iter->Resolution(), 1.0));
  }

  Stomp::ScalarMap* second_map =
      new Stomp::ScalarMap(scalar_pix, Stomp::ScalarMap::DensityField);
  std::cout << "\t" << second_map->Size() << " pixels (" <<
      scalar_map->Size() << " in original map, " << annulus_pix.size() <<
      " annulus pixels)\n";
  std::cout << "\t" << second_map->Area() << " sq. degrees (" <<
      scalar_map->Area() << ")\n";

  // Now, let's add those random points to the current map.  Since they were
  // both created with the same source map, they should all find a home.
  std::cout << "\tAttempting to add random points to density map\n";
  uint32_t n_random = 100000;
  uint32_t n_found = 0;
  uint32_t n_found_second = 0;
  Stomp::AngularVector rand_ang;
  stomp_map->GenerateRandomPoints(rand_ang, n_random);
  for (Stomp::AngularIterator iter=rand_ang.begin();
       iter!=rand_ang.end();++iter) {
    if (scalar_map->AddToMap(*iter)) n_found++;
    if (second_map->AddToMap(*iter)) n_found_second++;
  }

  if (n_random != n_found)
    std::cout << "Failed to add all random points to the density map.\n";
  if (n_random != n_found_second)
    std::cout << "Failed to add all random points to the 2nd density map.\n";

  std::cout << "\t\tPut " << n_found << "/" << rand_ang.size() <<
    " points in map.\n";
  std::cout << "\t\t\t" << scalar_map->MeanIntensity() <<
    " points/sq. degree.\n";

  std::cout << "\t\tPut " << n_found_second << "/" << rand_ang.size() <<
    " points in second map.\n";
  std::cout << "\t\t\t" << second_map->MeanIntensity() <<
    " points/sq. degree.\n";
}

void ScalarMapLocalTests() {
  // Alright, now let's test the code's ability to find the area and density
  // contained within an annulus projected on the survey area.  This is the
  // forerunner to the code that'll eventually do the correlation function
  // measurements.
  std::cout << "\n";
  std::cout << "*****************************\n";
  std::cout << "*** ScalarMap Local Tests ***\n";
  std::cout << "*****************************\n";
  double theta = 3.0;
  Stomp::AngularCoordinate ang(60.0, 0.0, Stomp::AngularCoordinate::Survey);
  Stomp::Pixel tmp_pix(ang, 256);
  Stomp::PixelVector annulus_pix;
  tmp_pix.WithinRadius(theta, annulus_pix);
  Stomp::Map* stomp_map = new Stomp::Map(annulus_pix);
  Stomp::ScalarMap* scalar_map =
    new Stomp::ScalarMap(*stomp_map, 128, Stomp::ScalarMap::DensityField);

  uint32_t n_random = 10000;
  uint32_t n_found = 0;
  Stomp::AngularVector rand_ang;
  stomp_map->GenerateRandomPoints(rand_ang, n_random);
  for (Stomp::AngularIterator iter=rand_ang.begin();iter!=rand_ang.end();++iter)
    if (scalar_map->AddToMap(*iter, 2.0)) n_found++;
  if (n_random != n_found)
    std::cout << "Failed to add all random points to the density map.\n";

  std::cout << "\t1 degree circle around map origin:\n";
  std::cout << "\t\tLocal Area:" << scalar_map->FindLocalArea(ang,1.0) <<
    " sq. degrees.\n";
  std::cout << "\t\t\tLocal Intensity: " <<
    scalar_map->FindLocalIntensity(ang,1.0) << " objects/sq. degree.\n";
  std::cout << "\t\t\tLocal Density: " <<
    scalar_map->FindLocalDensity(ang,1.0) << " objects/sq. degree.\n";
  std::cout << "\t\t\tLocal Point Density: " <<
    scalar_map->FindLocalPointDensity(ang,1.0) << " objects/sq. degree.\n";

  ang.SetSurveyCoordinates(62.0,2.0);
  std::cout << "\t1 degree circle around nearby map origin:\n";
  std::cout << "\t\tLocal Area:" << scalar_map->FindLocalArea(ang,1.0) <<
    " sq. degrees.\n";
  std::cout << "\t\t\tLocal Intensity: " <<
    scalar_map->FindLocalIntensity(ang,1.0) << " objects/sq. degree.\n";
  std::cout << "\t\t\tLocal Density: " <<
    scalar_map->FindLocalDensity(ang,1.0) << " objects/sq. degree.\n";
  std::cout << "\t\t\tLocal Point Density: " <<
    scalar_map->FindLocalPointDensity(ang,1.0) << " objects/sq. degree.\n";

  ang.SetSurveyCoordinates(0.0,0.0);
  std::cout << "\t1 degree circle around faraway map origin:\n";
  std::cout << "\t\tLocal Area:" << scalar_map->FindLocalArea(ang,1.0) <<
    " sq. degrees.\n";
  std::cout << "\t\t\tLocal Intensity: " <<
    scalar_map->FindLocalIntensity(ang,1.0) << " objects/sq. degree.\n";
  std::cout << "\t\t\tLocal Density: " <<
    scalar_map->FindLocalDensity(ang,1.0) << " objects/sq. degree.\n";
  std::cout << "\t\t\tLocal Point Density: " <<
    scalar_map->FindLocalPointDensity(ang,1.0) << " objects/sq. degree.\n";
}

void ScalarMapResamplingTests() {
  std::cout << "\n";
  std::cout << "**********************************\n";
  std::cout << "*** ScalarMap Resampling Tests ***\n";
  std::cout << "**********************************\n";
  double theta = 3.0;
  Stomp::AngularCoordinate ang(60.0, 0.0, Stomp::AngularCoordinate::Survey);
  Stomp::Pixel tmp_pix(ang, 256);
  Stomp::PixelVector annulus_pix;
  tmp_pix.WithinRadius(theta, annulus_pix);
  Stomp::Map* stomp_map = new Stomp::Map(annulus_pix);
  Stomp::ScalarMap* scalar_map =
    new Stomp::ScalarMap(*stomp_map, 128, Stomp::ScalarMap::DensityField);

  uint32_t n_random = 10000;
  uint32_t n_found = 0;
  Stomp::AngularVector rand_ang;
  stomp_map->GenerateRandomPoints(rand_ang, n_random);
  for (Stomp::AngularIterator iter=rand_ang.begin();iter!=rand_ang.end();++iter)
    if (scalar_map->AddToMap(*iter, 2.0)) n_found++;
  if (n_random != n_found)
    std::cout << "Failed to add all random points to the density map.\n";

  for (uint32_t resolution=scalar_map->Resolution()/2;
       resolution>=Stomp::HPixResolution;resolution /= 2) {
    Stomp::ScalarMap* sub_scalar_map =
        new Stomp::ScalarMap(*scalar_map,resolution);
    double total_intensity = 0.0, total_area = 0.0;;
    for (Stomp::ScalarIterator iter=sub_scalar_map->Begin();
         iter!=sub_scalar_map->End();++iter) {
      total_intensity += iter->Intensity();
      total_area += iter->Weight()*iter->Area();
    }

    std::cout << "\t\t" << resolution << ": Stored object total = " <<
      sub_scalar_map->Intensity() << " (" << sub_scalar_map->Area() << ")\n";
    std::cout << "\t\t    Calculated = " << total_intensity << " (" <<
      total_area << ")\n";
    std::cout << "\t\t    Should be " << scalar_map->Intensity() <<
      " (" << scalar_map->Area() << ")\n";
    std::cout << "\t\t\tMean pixel intensity: " <<
      sub_scalar_map->MeanIntensity() << " (" <<
      scalar_map->MeanIntensity() << ")\n";
    delete sub_scalar_map;
  }

  std::cout << "\tResampling Tests (post-overdensity translation):\n";

  scalar_map->ConvertToOverDensity();

  for (uint32_t resolution=scalar_map->Resolution()/2;
       resolution>=Stomp::HPixResolution;resolution /= 2) {
    Stomp::ScalarMap* sub_scalar_map =
      new Stomp::ScalarMap(*scalar_map,resolution);
    double total_intensity = 0.0, total_area = 0.0;;
    for (Stomp::ScalarIterator iter=sub_scalar_map->Begin();
         iter!=sub_scalar_map->End();++iter) {
      total_intensity += iter->Intensity();
      total_area += iter->Weight()*iter->Area();
    }

    std::cout << "\t\t" << resolution << ": Stored object total = " <<
      sub_scalar_map->Intensity() << " (" << sub_scalar_map->Area() << ")\n";
    std::cout << "\t\t    Calculated = " << total_intensity << " (" <<
      total_area << ")\n";
    std::cout << "\t\t    Should be " << scalar_map->Intensity() <<
      "(" << scalar_map->Area() << ")\n";
    std::cout << "\t\t\tMean pixel density: " <<
      sub_scalar_map->MeanIntensity() << " (" <<
      scalar_map->MeanIntensity() << ")\n";
    delete sub_scalar_map;
  }
}

void ScalarMapRegionTests() {
  std::cout << "\n";
  std::cout << "******************************\n";
  std::cout << "*** ScalarMap Region Tests ***\n";
  std::cout << "******************************\n";
  double theta = 3.0;
  Stomp::AngularCoordinate ang(60.0, 0.0, Stomp::AngularCoordinate::Survey);
  Stomp::Pixel tmp_pix(ang, 256);
  Stomp::PixelVector annulus_pix;
  tmp_pix.WithinRadius(theta, annulus_pix);
  Stomp::Map* stomp_map = new Stomp::Map(annulus_pix);
  Stomp::ScalarMap* scalar_map =
    new Stomp::ScalarMap(*stomp_map, 128, Stomp::ScalarMap::DensityField);

  // Another important step is checking to make sure that we can break
  // the map up into roughly equal chunks for jack-knife error calculations.
  // This first call probably won't work very well since we used a fairly
  // coarse resolution when we initialized this map.
  std::cout << "\tTrying to regionate the density map into 10 pieces...\n";
  scalar_map->InitializeRegions(10);

  // A better result should be found as we increase the resolution for the
  // region map.
  std::cout << "\tNow doing it with finer resolution...\n";
  Stomp::ScalarMap* hires_scalar_map =
    new Stomp::ScalarMap(*stomp_map, 128, Stomp::ScalarMap::DensityField);

  hires_scalar_map->InitializeRegions(10);
  delete hires_scalar_map;

  // Of course, we're limited in this direction by the maximum resolution of
  // our map.  These two things are separated so that we can do several maps
  // of the same data at different resolution (which speeds up the
  // auto-correlation measurement), but use the same regionated map (which is
  // necessary if we want our jack-knife errors to be meaningful.
  std::cout << "\tNow doing it with full resolution...\n";
  hires_scalar_map =
    new Stomp::ScalarMap(*stomp_map, 128, Stomp::ScalarMap::DensityField);
  hires_scalar_map->InitializeRegions(10);
  delete hires_scalar_map;
}

void ScalarMapAutoCorrelationTests() {
  // Ok, finally, we get to try out the auto-correlation code.  It's really
  // just this easy to invoke.
  std::cout << "\n";
  std::cout << "***************************************\n";
  std::cout << "*** ScalarMap AutoCorrelation Tests ***\n";
  std::cout << "***************************************\n";
  double theta = 3.0;
  Stomp::AngularCoordinate ang(60.0, 0.0, Stomp::AngularCoordinate::Survey);
  Stomp::Pixel tmp_pix(ang, 256);
  Stomp::PixelVector annulus_pix;
  tmp_pix.WithinRadius(theta, annulus_pix);
  Stomp::Map* stomp_map = new Stomp::Map(annulus_pix);
  Stomp::ScalarMap* scalar_map =
    new Stomp::ScalarMap(*stomp_map, 128, Stomp::ScalarMap::DensityField);

  uint32_t n_random = 100000;
  uint32_t n_found = 0;
  Stomp::AngularVector rand_ang;
  stomp_map->GenerateRandomPoints(rand_ang, n_random);
  for (Stomp::AngularIterator iter=rand_ang.begin();iter!=rand_ang.end();++iter)
    if (scalar_map->AddToMap(*iter)) n_found++;
  if (n_random != n_found)
    std::cout << "Failed to add all random points to the density map.\n";

  double theta_min = 0.01;
  double theta_max = 10.0;
  Stomp::AngularCorrelation *wtheta =
    new Stomp::AngularCorrelation(theta_min, theta_max, 6.0);

  scalar_map->AutoCorrelate(*wtheta);

  // Alrighty, now for our grand finale, let's lay out the code that we'd
  // use to do the auto-correlation for all scales, using our current
  // density map as the highest resolution map.
  for (uint32_t resolution=scalar_map->Resolution()/2;
       resolution>=wtheta->MinResolution();resolution/=2) {
    Stomp::ScalarMap* sub_scalar_map =
        new Stomp::ScalarMap(*scalar_map,resolution);
    std::cout << "\t" << sub_scalar_map->Resolution() <<
        ": Original Map Density: " << scalar_map->Density() <<
        ": New Map Density: " << sub_scalar_map->Density() << "\n";
    sub_scalar_map->AutoCorrelate(*wtheta);

    delete sub_scalar_map;
  }

  for (Stomp::ThetaIterator iter=wtheta->Begin(scalar_map->Resolution());
       iter!=wtheta->End(wtheta->MinResolution());++iter)
    std::cout << "\tw(" << iter->Theta() << ", " << iter->Resolution() <<
        ") = " << iter->Wtheta() << " +- " <<
        iter->PoissonNoise(scalar_map->Density(),
                           scalar_map->Area()) << "\n";
}

void ScalarMapCrossCorrelationTests() {
  std::cout << "\n";
  std::cout << "****************************************\n";
  std::cout << "*** ScalarMap CrossCorrelation Tests ***\n";
  std::cout << "****************************************\n";
  double theta = 3.0;
  Stomp::AngularCoordinate ang(60.0, 0.0, Stomp::AngularCoordinate::Survey);
  Stomp::Pixel tmp_pix(ang, 256);
  Stomp::PixelVector annulus_pix;
  tmp_pix.WithinRadius(theta, annulus_pix);
  uint32_t scalar_resolution = 512;
  Stomp::Map* stomp_map = new Stomp::Map(annulus_pix);
  Stomp::ScalarMap* scalar_map_a =
      new Stomp::ScalarMap(*stomp_map, scalar_resolution,
                           Stomp::ScalarMap::DensityField);
  Stomp::ScalarMap* scalar_map_b =
      new Stomp::ScalarMap(*stomp_map, scalar_resolution,
                           Stomp::ScalarMap::DensityField);

  uint32_t n_random = 100000;
  uint32_t n_found_a = 0;
  uint32_t n_found_b = 0;
  Stomp::AngularVector rand_ang;
  stomp_map->GenerateRandomPoints(rand_ang, n_random);
  for (Stomp::AngularIterator iter=rand_ang.begin();
       iter!=rand_ang.end();++iter) {
    if (scalar_map_a->AddToMap(*iter)) n_found_a++;
    if (scalar_map_b->AddToMap(*iter)) n_found_b++;
  }
  if (n_random != n_found_a)
    std::cout << "Failed to add all random points to the density map.\n";
  if (n_random != n_found_b)
    std::cout << "Failed to add all random points to the density map.\n";

  double theta_min = 0.01;
  double theta_max = 10.0;
  Stomp::AngularCorrelation *wtheta_auto =
    new Stomp::AngularCorrelation(theta_min, theta_max, 6.0);
  Stomp::AngularCorrelation *wtheta_cross =
    new Stomp::AngularCorrelation(theta_min, theta_max, 6.0);

  scalar_map_a->AutoCorrelate(*wtheta_auto);
  scalar_map_a->CrossCorrelate(*scalar_map_b, *wtheta_cross);

  // Alrighty, now for our grand finale, let's lay out the code that we'd
  // use to do the auto-correlation for all scales, using our current
  // density map as the highest resolution map.
  for (uint32_t resolution=scalar_map_a->Resolution()/2;
       resolution>=wtheta_auto->MinResolution();resolution/=2) {
    Stomp::ScalarMap* sub_scalar_map_a =
        new Stomp::ScalarMap(*scalar_map_a, resolution);
    Stomp::ScalarMap* sub_scalar_map_b =
        new Stomp::ScalarMap(*scalar_map_b, resolution);
    std::cout << "\t" << sub_scalar_map_a->Resolution() <<
        ": Original Map Density: " << scalar_map_a->Density() <<
        ": New Map Density: " << sub_scalar_map_a->Density() << "\n";
    sub_scalar_map_a->AutoCorrelate(*wtheta_auto);
    sub_scalar_map_a->CrossCorrelate(*sub_scalar_map_b, *wtheta_cross);

    delete sub_scalar_map_a;
    delete sub_scalar_map_b;
  }

  for (uint8_t i=0;i<wtheta_auto->NBins();i++) {
    Stomp::ThetaIterator auto_iter = wtheta_auto->BinIterator(i);
    Stomp::ThetaIterator cross_iter = wtheta_cross->BinIterator(i);

    if (auto_iter->Resolution() <= scalar_map_a->Resolution() &&
        auto_iter->Resolution() >= Stomp::HPixResolution) {
      std::cout << "\tw(" << auto_iter->Theta() << ", " <<
          auto_iter->Resolution() << ", " <<
          cross_iter->Resolution() << ") = " <<
          cross_iter->Wtheta() << " (" << auto_iter->Wtheta() << ", +-" <<
          auto_iter->PoissonNoise(scalar_map_a->Density(),
                                  scalar_map_a->Area()) << ")\n";
    }
  }
}

// Define our command line flags
DEFINE_bool(all_scalar_map_tests, false, "Run all class unit tests.");
DEFINE_bool(scalar_map_basic_tests, false, "Run ScalarMap basic tests");
DEFINE_bool(scalar_map_local_tests, false, "Run ScalarMap local tests");
DEFINE_bool(scalar_map_resampling_tests, false,
            "Run ScalarMap resampling tests");
DEFINE_bool(scalar_map_region_tests, false, "Run ScalarMap region tests");
DEFINE_bool(scalar_map_autocorrelation_tests, false,
            "Run ScalarMap auto-correlation tests");
DEFINE_bool(scalar_map_crosscorrelation_tests, false,
            "Run ScalarMap cross-correlation tests");

void ScalarMapUnitTests(bool run_all_tests) {
  void ScalarMapBasicTests();
  void ScalarMapLocalTests();
  void ScalarMapResamplingTests();
  void ScalarMapRegionTests();
  void ScalarMapAutoCorrelationTests();
  void ScalarMapCrossCorrelationTests();

  if (run_all_tests) FLAGS_all_scalar_map_tests = true;

  // Check the basic routines for generating a Stomp::ScalarMap from an input
  // Stomp::Map.
  if (FLAGS_all_scalar_map_tests || FLAGS_scalar_map_basic_tests)
    ScalarMapBasicTests();

  // Check the StomScalarMap methods for finding the area and density of the
  // map within a given pixel.
  if (FLAGS_all_scalar_map_tests || FLAGS_scalar_map_local_tests)
    ScalarMapLocalTests();

  // Check the Stomp::ScalarMap methods for creating new, coarser resolution
  // Stomp::ScalarMaps from an initial high-resolution version.
  if (FLAGS_all_scalar_map_tests || FLAGS_scalar_map_resampling_tests)
    ScalarMapResamplingTests();

  // Check the routines for splitting up the area of a Stomp::ScalarMap into
  // roughly equal-area regions.
  if (FLAGS_all_scalar_map_tests || FLAGS_scalar_map_region_tests)
    ScalarMapRegionTests();

  // Check the auto-correlation methods in the Stomp::ScalarMap class.
  if (FLAGS_all_scalar_map_tests || FLAGS_scalar_map_autocorrelation_tests)
    ScalarMapAutoCorrelationTests();

  // Check the cross-correlation methods in the Stomp::ScalarMap class.
  if (FLAGS_all_scalar_map_tests || FLAGS_scalar_map_crosscorrelation_tests)
    ScalarMapCrossCorrelationTests();
}
