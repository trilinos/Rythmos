//
// @HEADER
// ***********************************************************************
// 
//                           Rythmos Package
//                 Copyright (2005) Sandia Corporation
// 
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
// 
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//  
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//  
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
// Questions? Contact Michael A. Heroux (maherou@sandia.gov) 
// 
// ***********************************************************************
// @HEADER

#ifndef Rythmos_NONLINEAR_MODEL_H
#define Rythmos_NONLINEAR_MODEL_H

#include "Thyra_VectorBase.hpp"
#include "InOutArgs.hpp"

namespace Rythmos {

//-----------------------------------------------------------------------------
// Class         : NonlinearModel
// Purpose       : Define interface to nonlinear models
// Special Notes :
// Creator       : Todd Coffey, SNL
// Creation Date : 05/20/05
//-----------------------------------------------------------------------------
template<class Scalar>
class NonlinearModel
{
  public:
    
    // Destructor
    ~NonlinearModel();

    // Constructor
    NonlinearModel();

    virtual int evalModel(const InArgs<Scalar> &inargs, OutArgs<Scalar> &outargs)=0;

    virtual Teuchos::RefCountPtr<Thyra::VectorBase<Scalar> > &get_vector()=0;

  protected:

//    InArgs<Scalar> inargs_;
//    OutArgs<Scalar> outargs_;

};

} //namespace Rythmos 

#endif // Rythmos_NONLINEAR_MODEL_H
