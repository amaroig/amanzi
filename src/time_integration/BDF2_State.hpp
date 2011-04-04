#ifndef _BDF2_STATE_HPP_
#define _BDF2_STATE_HPP_

#include "dbc.hh"
#include "BDF2_SolutionHistory.hpp"
#include <limits>

namespace BDF2 {

  struct State {

  public:

    State()
    {
      seq = -1;
      usable_pc = false;
      mitr = 5;
      ntol = 0.1;
      
      pcfun_calls = 0;
      updpc_calls = 0;
      updpc_failed = 0;
      retried_bce = 0;
      failed_bce = 0; 
      rejected_steps = 0;
      hmin = std::numeric_limits<double>::max();
      hmax = std::numeric_limits<double>::min();

      verbose = false;
    }

    ~State()
    {
      if (! uhist) delete uhist;
      
    }

    void init_solution_history(SolutionHistory* sh) 
    {
      ASSERT(sh);
      uhist = sh;
    }

    
    int       seq;          // number of steps taken
    double    hlast;        // last step size
    double    hpc;          // step size built into the current preconditioner
    bool      usable_pc;    // whether the current preconditioner is usable
    int       freeze_count; // don't increase step size for this number of steps
    int       mitr;         // maximum number of nonlinear iterations
    double    ntol;         // nonlinear solver error tolerance (relative to 1)
    SolutionHistory* uhist; // solution history structure

    // performance counters
    int    pcfun_calls;    // number of calls to PCFUN
    int    updpc_calls;    // number of calls to UPDPC
    int    updpc_failed;   // number of UPDPC calls returning an error
    int    retried_bce;    // number of retried BCE steps
    int    failed_bce;     // number of completely failed BCE steps
    int    rejected_steps; // number of steps rejected on error tolerance
    double hmin;           // minimum step size used on a successful step
    double hmax;           // maximum step size used on a successful step

    // Diagnostics
    bool   verbose;
   
  };

}

#endif // _BDF2_STATE_HPP_
