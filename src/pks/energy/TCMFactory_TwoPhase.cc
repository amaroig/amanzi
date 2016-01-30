/*
  Energy

  Copyright 2010-201x held jointly by LANS/LANL, LBNL, and PNNL. 
  Amanzi is released under the three-clause BSD License. 
  The terms of use and "as is" disclaimer for this license are 
  provided in the top-level COPYRIGHT file.

  Author: Ethan Coon

  Self-registering factory for thermal conductivity models.
*/

#include <string>
#include "TCMFactory_TwoPhase.hh"

namespace Amanzi {
namespace Energy {

/* ******************************************************************
* method for instantiating implementations
****************************************************************** */
Teuchos::RCP<TCM_TwoPhase> TCMFactory_TwoPhase::CreateTCM(Teuchos::ParameterList& plist)
{
  std::string tc_typename = plist.get<std::string>("thermal conductivity type");
  return Teuchos::rcp(CreateInstance(tc_typename, plist));
};

}  // namespace Energy
}  // namespace Amanzi

