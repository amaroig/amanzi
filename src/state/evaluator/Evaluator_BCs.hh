/*
  Copyright 2010-201x held jointly by participating institutions.
  Amanzi is released under the three-clause BSD License.
  The terms of use and "as is" disclaimer for this license are
  provided in the top-level COPYRIGHT file.

  Authors:
      Ethan Coon
*/

//! Evaluates a function to provide Dirichlet data on faces.

#ifndef AMANZI_EVALUATOR_BCS_DIRICHLET_HH_
#define AMANZI_EVALUATOR_BCS_DIRICHLET_HH_

#include "BCs_Factory.hh"
#include "Evaluator_Factory.hh"
#include "EvaluatorSecondary.hh"
#include "BoundaryFunctionFactory.hh"

namespace Amanzi {

class Evaluator_BCs : public EvaluatorSecondary {
 public:
  explicit Evaluator_BCs(Teuchos::ParameterList& plist);

  Evaluator_BCs(const Evaluator_BCs& other) = default;
  virtual Teuchos::RCP<Evaluator> Clone() const override
  {
    return Teuchos::rcp(new Evaluator_BCs(*this));
  }

  virtual bool IsDifferentiableWRT(const State& S, const Key& wrt_key,
                                   const Key& wrt_tag) const override final
  {
    return false;
  }

  virtual void EnsureCompatibility(State& S) override;
  // virtual void EnsureCompatibleDerivative(State &S,
  //         const Key& wrt_key, const Key& wrt_tag) override;

  virtual void Update_(State& S) override;
  virtual void UpdateDerivative_(State& S, const Key& wrt_key,
                                 const Key& wrt_tag) override final
  {
    AMANZI_ASSERT(false); // never called
  }

 protected:
  std::vector<int> bc_types_;

 private:
  static Utils::RegisteredFactory<Evaluator, Evaluator_BCs> fac_;
};


} // namespace Amanzi

#endif
