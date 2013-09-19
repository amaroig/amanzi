/*
This is the flow component of the Amanzi code. 

Copyright 2010-2012 held jointly by LANS/LANL, LBNL, and PNNL. 
Amanzi is released under the three-clause BSD License. 
The terms of use and "as is" disclaimer for this license are 
provided in the top-level COPYRIGHT file.

Authors: Konstantin Lipnikov (version 2) (lipnikov@lanl.gov)
*/

#ifndef AMANZI_MATRIX_MFD_HH_
#define AMANZI_MATRIX_MFD_HH_

#include <strings.h>

#include "Epetra_Map.h"
#include "Epetra_Operator.h"
#include "Epetra_MultiVector.h"
#include "Epetra_SerialDenseVector.h"
#include "Epetra_CrsMatrix.h"
#include "Epetra_FECrsMatrix.h"

#include "Preconditioner.hh"
#include "ml_MultiLevelPreconditioner.h"

#include "Ifpack.h" 
// note that if trilinos is compiled with hypre support, then
// including Ifpack.h results in the definition of HAVE_HYPRE

#include "Teuchos_RCP.hpp"
#include "Teuchos_SerialDenseMatrix.hpp"
#include "Teuchos_LAPACK.hpp"

#include "Mesh.hh"
#include "Point.hh"
#include "DenseMatrix.hh"
#include "boundary_function.hh"

#include "Flow_State.hh"
#include "Flow_typedefs.hh"
#include "RelativePermeability.hh"


namespace Amanzi {
namespace AmanziFlow {

class Matrix_MFD {
 public:
  Matrix_MFD() {};
  Matrix_MFD(Teuchos::RCP<Flow_State> FS_, Teuchos::RCP<const Epetra_Map> map_);
  ~Matrix_MFD();

  // main methods
  void CreateMFDmassMatrices(int mfd3d_method, std::vector<WhetStone::Tensor>& K);
  void CreateMFDrhsVectors();
  void ApplyBoundaryConditions(std::vector<int>& bc_model, std::vector<bc_tuple>& bc_values);

  void CreateMFDstiffnessMatrices();
  virtual void CreateMFDstiffnessMatrices(RelativePermeability& rel_perm);
  virtual void SymbolicAssembleGlobalMatrices(const Epetra_Map& super_map);
  virtual void AssembleGlobalMatrices();
  virtual void AssembleSchurComplement(std::vector<int>& bc_model, std::vector<bc_tuple>& bc_values);

  double ComputeResidual(const Epetra_Vector& solution, Epetra_Vector& residual);
  double ComputeNegativeResidual(const Epetra_Vector& solution, Epetra_Vector& residual);

  int ReduceGlobalSystem2LambdaSystem(Epetra_Vector& u);

  void DeriveDarcyMassFlux(const Epetra_Vector& solution, 
                           const Epetra_Import& face_importer, 
                           Epetra_Vector& darcy_mass_flux);

  void InitPreconditioner(const std::string& prec_name, const Teuchos::ParameterList& prec_list);
  virtual void UpdatePreconditioner() { preconditioner_->Update(Sff_); } 
  void DestroyPreconditioner() { preconditioner_->Destroy(); }

  // required methods
  virtual int Apply(const Epetra_MultiVector& X, Epetra_MultiVector& Y) const;
  virtual int ApplyInverse(const Epetra_MultiVector& X, Epetra_MultiVector& Y) const;

  // control methods
  void SetSymmetryProperty(bool flag_symmetry) { flag_symmetry_ = flag_symmetry; }
  void AddActionProperty(int action) { actions_ |= action; }
  bool CheckActionProperty(int action) { return actions_ & action; }

  // development methods
  void CreateMFDmassMatrices_ScaledStability(int method, double factor, std::vector<WhetStone::Tensor>& K);
  int PopulatePreconditioner(Matrix_MFD& matrix);
  void RescaleMFDstiffnessMatrices(const Epetra_Vector& old_scale, const Epetra_Vector& new_scale);

  // access methods
  std::vector<Teuchos::SerialDenseMatrix<int, double> >& Aff_cells() { return Aff_cells_; }
  std::vector<Epetra_SerialDenseVector>& Acf_cells() { return Acf_cells_; }
  std::vector<Epetra_SerialDenseVector>& Afc_cells() { return Afc_cells_; }
  std::vector<double>& Acc_cells() { return Acc_cells_; }
  std::vector<Epetra_SerialDenseVector>& Ff_cells() { return Ff_cells_; }
  std::vector<double>& Fc_cells() { return Fc_cells_; }
  Teuchos::RCP<Epetra_Vector>& rhs() { return rhs_; }
  Teuchos::RCP<Epetra_Vector>& rhs_faces() { return rhs_faces_; }
  Teuchos::RCP<Epetra_Vector>& rhs_cells() { return rhs_cells_; }

  Teuchos::RCP<Epetra_FECrsMatrix>& Aff() { return Aff_; }
  Teuchos::RCP<Epetra_FECrsMatrix>& Sff() { return Sff_; }
  Teuchos::RCP<Epetra_Vector>& Acc() { return Acc_; }
  Teuchos::RCP<Epetra_CrsMatrix>& Acf() { return Acf_; }
  Teuchos::RCP<Epetra_CrsMatrix>& Afc() { return Afc_; }

  int nokay() { return nokay_; }
  int npassed() { return npassed_; }

 protected:
  Teuchos::RCP<Flow_State> FS_;
  Teuchos::RCP<const AmanziMesh::Mesh> mesh_;
  Teuchos::RCP<const Epetra_Map> map_;

  bool flag_symmetry_;
  int actions_;  // applly, apply inverse, or both

  std::vector<WhetStone::DenseMatrix> Mff_cells_;
  std::vector<Teuchos::SerialDenseMatrix<int, double> > Aff_cells_;
  std::vector<Epetra_SerialDenseVector> Acf_cells_, Afc_cells_;
  std::vector<double> Acc_cells_;  // duplication may be useful later

  std::vector<Epetra_SerialDenseVector> Ff_cells_;
  std::vector<double> Fc_cells_;

  Teuchos::RCP<Epetra_Vector> Acc_;
  Teuchos::RCP<Epetra_CrsMatrix> Acf_; 
  Teuchos::RCP<Epetra_CrsMatrix> Afc_;  // We generate transpose of this matrix block. 
  Teuchos::RCP<Epetra_FECrsMatrix> Aff_;
  Teuchos::RCP<Epetra_FECrsMatrix> Sff_;  // Schur complement
  Teuchos::RCP<AmanziPreconditioners::Preconditioner> preconditioner_;

  Teuchos::RCP<Epetra_Vector> rhs_;
  Teuchos::RCP<Epetra_Vector> rhs_cells_;
  Teuchos::RCP<Epetra_Vector> rhs_faces_;

  int nokay_, npassed_;  // performance of algorithms generating mass matrices 

 private:
  void operator=(const Matrix_MFD& matrix);
};

}  // namespace AmanziFlow
}  // namespace Amanzi

#endif
