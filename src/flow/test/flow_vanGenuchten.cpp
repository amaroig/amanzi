#include <iostream>
#include "UnitTest++.h"

#include "WRM_vanGenuchten.hpp"
#include "math.h"

TEST(vanGenuchten) {
  using namespace Amanzi::AmanziFlow;

  double m = 0.5;
  double alpha = 0.01;
  double sr = 0.4;
  double p_atm = 1.0e+5;
  std::string krel_function("Mualem");
  double pc0 = 500.0;

  WRM_vanGenuchten vG("test", m, alpha, sr, krel_function, pc0);
 
  // check k_relative for p = 2*p_atm
  double pc = -p_atm;
  CHECK_EQUAL(vG.k_relative(pc), 1.0);
  
  // check k_relative for p = 0
  pc = p_atm;
  double se = pow(1.0 + pow(alpha * pc, 1.0 / (1.0-m)),-m);
  CHECK_CLOSE(vG.k_relative(pc), 
              sqrt(se) * pow(1.0 - pow(1.0 - pow(se, 1.0/m), m), 2.0), 1e-15);
  
  // check saturation for p = 2*p_atm
  pc = -p_atm;
  CHECK_EQUAL(vG.saturation(pc), 1.0);
  
  // check saturation for p = 0  
  pc = p_atm;
  CHECK_CLOSE(vG.saturation(pc),
	      pow(1.0 + pow(alpha * pc, 1.0/ (1.0-m)), -m) * (1.0-sr) + sr, 1e-15);

  // check derivative of saturation(pc) at p = 2*p_atm.
  pc = -p_atm;
  CHECK_EQUAL(vG.dSdPc(pc), 0.0);

  // check derivative of saturation(p) at p = 0.
  pc = p_atm;
  CHECK_CLOSE(vG.dSdPc(pc), 
              (1.0-sr) * m * pow(1.0 + pow(alpha*pc, 1.0/(1.0-m)), -m-1.0)
               * (-alpha) * pow(alpha*pc, m/(1.0-m)) / (1.0-m), 1e-15);

  // check smoothing at p = 0.995 * p_atm
  pc = 0.005 * p_atm;
  CHECK_CLOSE(vG.k_relative(pc), 1.67003e-4, 1e-9);
}


