/*
  Flow PK

  Copyright 2010-201x held jointly by LANS/LANL, LBNL, and PNNL. 
  Amanzi is released under the three-clause BSD License. 
  The terms of use and "as is" disclaimer for this license are 
  provided in the top-level COPYRIGHT file.

  Author: Konstantin Lipnikov (lipnikov@lanl.gov)
*/

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

// TPLs
#include "Epetra_MpiComm.h"
#include "Epetra_SerialComm.h"
#include "Teuchos_RCP.hpp"
#include "Teuchos_ParameterList.hpp"
#include "Teuchos_XMLParameterListHelpers.hpp"
#include "UnitTest++.h"

// Amanzi
#include "LeastSquare.hh"
#include "Mesh.hh"
#include "MeshFactory.hh"

// Flow
#include "Richards_PK.hh"

#include "Analytic01.hh"
#include "Richards_SteadyState.hh"

using namespace Amanzi;
using namespace Amanzi::AmanziMesh;
using namespace Amanzi::AmanziGeometry;
using namespace Amanzi::Flow;

void RunTestConvergence(std::string input_xml) {
  Epetra_MpiComm* comm = new Epetra_MpiComm(MPI_COMM_WORLD);
  int MyPID = comm->MyPID();
  if (MyPID == 0) std::cout <<"\nSteady-state Richards: convergence analysis: " << input_xml << std::endl;

  std::string xmlFileName = input_xml;
  Teuchos::RCP<Teuchos::ParameterList> plist = Teuchos::getParametersFromXmlFile(xmlFileName);

  // convergence estimate
  std::vector<double> h, p_error, v_error;

  for (int n = 40; n < 161; n*=2) {
    Teuchos::ParameterList region_list = plist->get<Teuchos::ParameterList>("Regions");
    GeometricModelPtr gm = new GeometricModel(3, region_list, comm);
    
    FrameworkPreference pref;
    pref.clear();
    pref.push_back(MSTK);
    pref.push_back(STKMESH);

    MeshFactory meshfactory(comm);
    meshfactory.preference(pref);
    Teuchos::RCP<const Mesh> mesh = meshfactory(0.0, 0.0, -10.0, 1.0, 1.0, 0.0, 1, 1, n, gm);

    /* create and populate flow state */
    Amanzi::VerboseObject::hide_line_prefix = false;

    Teuchos::ParameterList state_list = plist->get<Teuchos::ParameterList>("State");
    Teuchos::RCP<State> S = Teuchos::rcp(new State(state_list));
    S->RegisterDomainMesh(Teuchos::rcp_const_cast<Mesh>(mesh));

    /* create Richards process kernel */
    Teuchos::RCP<TreeVector> soln = Teuchos::rcp(new TreeVector());
    Richards_PK* RPK = new Richards_PK(plist, "Flow", S, soln);
    RPK->Setup();
    S->Setup();
    S->InitializeFields();
    S->InitializeEvaluators();

    RPK->Initialize();
    S->CheckAllFieldsInitialized();

    // solve the problem
    TI_Specs ti_specs;
    ti_specs.T0 = 0.0;
    ti_specs.dT0 = 1.0;
    ti_specs.T1 = 1.0e+4;
    ti_specs.max_itrs = 1000;

    AdvanceToSteadyState(S, *RPK, ti_specs, soln);
    RPK->CommitStep(0.0, 1.0);  // dummy times

    double pressure_err, flux_err, div_err;  // error checks
    const Epetra_MultiVector& p = *S->GetFieldData("pressure")->ViewComponent("cell");
    const Epetra_MultiVector& flux = *S->GetFieldData("darcy_flux")->ViewComponent("face", true);

    pressure_err = CalculatePressureCellError(mesh, p);
    flux_err = CalculateDarcyFluxError(mesh, flux);
    div_err = CalculateDarcyDivergenceError(mesh, flux);

    if (n == 80) CHECK(pressure_err < 5.0e-2 && flux_err < 5.0e-2);
    int num_nonlinear_steps = ti_specs.num_itrs;
    printf("n=%3d itrs=%4d  L2_pressure_err=%7.3e  l2_flux_err=%7.3e  L2_div_err=%7.3e\n",
        n, num_nonlinear_steps, pressure_err, flux_err, div_err);

    delete RPK;

    h.push_back(10.0 / n);
    p_error.push_back(pressure_err);
    v_error.push_back(flux_err);
  }

  /* convergence rates */
  double p_rate = Amanzi::Utils::bestLSfit(h, p_error);
  double v_rate = Amanzi::Utils::bestLSfit(h, v_error);
  printf("convergence rates: %23.2f %22.2f\n", p_rate, v_rate);

  CHECK_CLOSE(1.9, p_rate, 0.2);
  CHECK(v_rate > 1.8);

  delete comm;
}

/* *****************************************************************
* Run with various discretization methods
* **************************************************************** */
TEST(FLOW_RICHARDS_CONVERGENCE_MFD) {
  RunTestConvergence("test/flow_richards_convergence.xml");
}

TEST(FLOW_RICHARDS_CONVERGENCE_NLFV) {
  RunTestConvergence("test/flow_richards_convergence_nlfv.xml");
}

