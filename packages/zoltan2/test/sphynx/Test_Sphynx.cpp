// @HEADER
//
// ***********************************************************************
//
//   Zoltan2: A package of combinatorial algorithms for scientific computing
//                  Copyright 2012 Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact Seher Acer        (sacer@sandia.gov)
//                    Erik Boman        (egboman@sandia.gov)
//                    Siva Rajamanickam (srajama@sandia.gov)
//                    Karen Devine      (kddevin@sandia.gov)
//
// ***********************************************************************
//
// @HEADER
#include <Zoltan2_SphynxProblem.hpp>
#include <Zoltan2_XpetraCrsMatrixAdapter.hpp>
#include <Zoltan2_XpetraCrsGraphAdapter.hpp>
#include <Zoltan2_XpetraMultiVectorAdapter.hpp>
#include <Zoltan2_TestHelpers.hpp>
#include <iostream>
#include <limits>
#include <Teuchos_ParameterList.hpp>
#include <Teuchos_RCP.hpp>
#include <Teuchos_FancyOStream.hpp>
#include <Teuchos_CommandLineProcessor.hpp>
#include <Tpetra_CrsMatrix.hpp>
#include <Tpetra_Vector.hpp>
#include <MatrixMarket_Tpetra.hpp>

using Teuchos::RCP;

/////////////////////////////////////////////////////////////////////////////
// This program is a modified version of partitioning1.cpp (Karen Devine, 2011) 
// which can be found in zoltan2/test/core/partition/. 
// This version demonstrates use of Sphynx to partition a Tpetra matrix
// (read from a MatrixMarket file or generated by Galeri::Xpetra).
// Usage:
//     Zoltan2_Sphynx.exe [--inputFile=filename] [--outputFile=outfile] [--verbose]
//           [--x=#] [--y=#] [--z=#] [--matrix={Laplace1D,Laplace2D,Laplace3D}
//           [--normalized] [--generalized] [--polynomial]
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Eventually want to use Teuchos unit tests to vary z2TestLO and
// GO.  For now, we set them at compile time based on whether Tpetra
// is built with explicit instantiation on.  (in Zoltan2_TestHelpers.hpp)

typedef zlno_t z2TestLO;
typedef zgno_t z2TestGO;
typedef zscalar_t z2TestScalar;

typedef Tpetra::CrsMatrix<z2TestScalar, z2TestLO, z2TestGO> SparseMatrix;
typedef Tpetra::CrsGraph<z2TestLO, z2TestGO> SparseGraph;
typedef Tpetra::Vector<z2TestScalar, z2TestLO, z2TestGO> VectorT;
typedef VectorT::node_type Node;

typedef Zoltan2::XpetraCrsMatrixAdapter<SparseMatrix> SparseMatrixAdapter;
typedef Zoltan2::XpetraCrsGraphAdapter<SparseGraph> SparseGraphAdapter;
typedef Zoltan2::XpetraMultiVectorAdapter<VectorT> MultiVectorAdapter;


// Integer vector
typedef Tpetra::Vector<int, z2TestLO, z2TestGO> IntVector;
typedef Zoltan2::XpetraMultiVectorAdapter<IntVector> IntVectorAdapter;

#define epsilon 0.00000001
#define NNZ_IDX 1

/////////////////////////////////////////////////////////////////////////////
int main(int narg, char** arg)
{
  std::string inputFile = "";        // Matrix Market or Zoltan file to read
  std::string outputFile = "";       // Matrix Market or Zoltan file to write
  std::string inputPath = testDataFilePath;  // Directory with input file
  bool verbose = false;              // Verbosity of output
  bool distributeInput = true;
  bool haveFailure = false;
  int nVwgts = 0;
  int testReturn = 0;

  // Sphynx-related parameters
  bool isNormalized = false;
  bool isGeneralized = false;
  std::string precType = "jacobi";
  std::string initialGuess = "random";
  bool useFullOrtho = true;

  ////// Establish session.
  Tpetra::ScopeGuard tscope(&narg, &arg);
  RCP<const Teuchos::Comm<int> > comm = Tpetra::getDefaultComm();
  int me = comm->getRank();
  int commsize = comm->getSize();

  // Read run-time options.
  Teuchos::CommandLineProcessor cmdp (false, false);
  cmdp.setOption("inputPath", &inputPath,
                 "Path to the MatrixMarket or Zoltan file to be read; "
                 "if not specified, a default path will be used.");
  cmdp.setOption("inputFile", &inputFile,
                 "Name of the Matrix Market or Zoltan file to read; "
                 "if not specified, a matrix will be generated by MueLu.");
  cmdp.setOption("outputFile", &outputFile,
                 "Name of the Matrix Market sparse matrix file to write, "
                 "echoing the input/generated matrix.");
  cmdp.setOption("vertexWeights", &nVwgts,
                 "Number of weights to generate for each vertex");
  cmdp.setOption("verbose", "quiet", &verbose,
                 "Print messages and results.");
  cmdp.setOption("distribute", "no-distribute", &distributeInput,
                "indicate whether or not to distribute "
                "input across the communicator");
  // Sphynx-related parameters:
  cmdp.setOption("normalized", "combinatorial", &isNormalized,
		 "indicate whether or not to use a normalized Laplacian.");
  cmdp.setOption("generalized", "non-generalized", &isGeneralized,
		 "indicate whether or not to use a generalized Laplacian.");
  cmdp.setOption("precond", &precType,
		 "indicate which preconditioner to use [muelu|jacobi|polynomial].");
  cmdp.setOption("initialGuess", &initialGuess,
                 "initial guess for LOBPCG");
  cmdp.setOption("useFullOrtho", "partialOrtho", &useFullOrtho,
                 "use full orthogonalization.");

  //////////////////////////////////
  // Even with cmdp option "true", I get errors for having these
  //   arguments on the command line.  (On redsky build)
  // KDDKDD Should just be warnings, right?  Code should still work with these
  // KDDKDD params in the create-a-matrix file.  Better to have them where
  // KDDKDD they are used.
  int xdim=10;
  int ydim=10;
  int zdim=10;
  std::string matrixType("Laplace3D");

  cmdp.setOption("x", &xdim,
                "number of gridpoints in X dimension for "
                "mesh used to generate matrix.");
  cmdp.setOption("y", &ydim,
                "number of gridpoints in Y dimension for "
                "mesh used to generate matrix.");
  cmdp.setOption("z", &zdim,
                "number of gridpoints in Z dimension for "
                "mesh used to generate matrix.");
  cmdp.setOption("matrix", &matrixType,
                "Matrix type: Laplace1D, Laplace2D, or Laplace3D");
  //////////////////////////////////

  cmdp.parse(narg, arg);

  RCP<UserInputForTests> uinput;

  if (inputFile != "")   // Input file specified; read a matrix
    uinput = rcp(new UserInputForTests(inputPath, inputFile, comm,
                                       true, distributeInput));

  else                  // Let MueLu generate a default matrix
    uinput = rcp(new UserInputForTests(xdim, ydim, zdim, string(""), comm,
                                       true, distributeInput));

  RCP<SparseMatrix> origMatrix = uinput->getUITpetraCrsMatrix();

  if (origMatrix->getGlobalNumRows() < 40) {
    Teuchos::FancyOStream out(Teuchos::rcp(&std::cout,false));
    origMatrix->describe(out, Teuchos::VERB_EXTREME);
  }


  if (outputFile != "") {
    // Just a sanity check.
    Tpetra::MatrixMarket::Writer<SparseMatrix>::writeSparseFile(outputFile,
                                                origMatrix, verbose);
  }

  if (me == 0)
    std::cout << "NumRows     = " << origMatrix->getGlobalNumRows() << std::endl
         << "NumNonzeros = " << origMatrix->getGlobalNumEntries() << std::endl
         << "NumProcs = " << comm->getSize() << std::endl
         << "NumLocalRows (rank 0) = " << origMatrix->getLocalNumRows() << std::endl;

  ////// Create a vector to use with the matrix.
  RCP<VectorT> origVector, origProd;
  origProd   = Tpetra::createVector<z2TestScalar,z2TestLO,z2TestGO>(
                                    origMatrix->getRangeMap());
  origVector = Tpetra::createVector<z2TestScalar,z2TestLO,z2TestGO>(
                                    origMatrix->getDomainMap());
  origVector->randomize();

  ////// Specify the Sphynx parameters
  Teuchos::RCP<Teuchos::ParameterList> params(new Teuchos::ParameterList);
  params->set("num_global_parts", commsize);
  params->set("sphynx_skip_preprocessing", true);   // Preprocessing has not been implemented yet.
  params->set("sphynx_preconditioner_type", precType);
  params->set("sphynx_verbosity", verbose ? 1 : 0);
  params->set("sphynx_initial_guess", initialGuess);
  params->set("sphynx_use_full_ortho", useFullOrtho);
  std::string problemType = "combinatorial";
  if(isNormalized)
    problemType = "normalized";
  else if(isGeneralized)
    problemType = "generalized";
  params->set("sphynx_problem_type", problemType); // Type of the eigenvalue problem. 

  ////// Create an input adapter for the graph of the Tpetra matrix.
  Teuchos::RCP<SparseGraphAdapter> adapter = Teuchos::rcp( new SparseGraphAdapter(origMatrix->getCrsGraph(), nVwgts));

  ////// Add weights, if requested.
  ////// Generate some artificial weights.
  ////// Maybe this code should go into UserInputForTests.

  zscalar_t *vwgts = NULL;
  if (nVwgts) {
    // Test vertex weights with stride nVwgts.
    size_t nrows = origMatrix->getLocalNumRows();
    if (nrows) {
      vwgts = new zscalar_t[nVwgts * nrows];
      for (size_t i = 0; i < nrows; i++) {
        size_t idx = i * nVwgts;
        vwgts[idx] = zscalar_t(origMatrix->getRowMap()->getGlobalElement(i));
        for (int j = 1; j < nVwgts; j++) vwgts[idx+j] = 1.;
      }
      for (int j = 0; j < nVwgts; j++) {
        if (j != NNZ_IDX) adapter->setVertexWeights(&vwgts[j], nVwgts, j);
        else              adapter->setVertexWeightIsDegree(NNZ_IDX);
      }
    }
  }

  ////// Create and solve partitioning problem
  Zoltan2::SphynxProblem<SparseGraphAdapter> problem(adapter, params);

  try {
    if (me == 0) std::cout << "Calling solve() " << std::endl;

    problem.solve();

    if (me == 0) std::cout << "Done solve() " << std::endl;
  }
  catch (std::runtime_error &e) {
    delete [] vwgts;
    std::cout << "Runtime exception returned from solve(): " << e.what();
    if (!strncmp(e.what(), "BUILD ERROR", 11)) {
      // Catching build errors as exceptions is OK in the tests
      std::cout << " PASS" << std::endl;
      return 0;
    }
    else {
      // All other runtime_errors are failures
      std::cout << " FAIL" << std::endl;
      return -1;
    }
  }
  catch (std::logic_error &e) {
    delete [] vwgts;
    std::cout << "Logic exception returned from solve(): " << e.what()
         << " FAIL" << std::endl;
    return -1;
  }
  catch (std::bad_alloc &e) {
    delete [] vwgts;
    std::cout << "Bad_alloc exception returned from solve(): " << e.what()
         << " FAIL" << std::endl;
    return -1;
  }
  catch (std::exception &e) {
    delete [] vwgts;
    std::cout << "Unknown exception returned from solve(). " << e.what()
         << " FAIL" << std::endl;
    return -1;
  }

  ///// Basic metric checking of the partitioning solution
  ///// Not ordinarily done in application code; just doing it for testing here.
  size_t checkNparts = comm->getSize();

  size_t  checkLength = origMatrix->getLocalNumRows();
  const SparseGraphAdapter::part_t *checkParts = problem.getSolution().getPartListView();

  // Check for load balance
  size_t *countPerPart = new size_t[checkNparts];
  size_t *globalCountPerPart = new size_t[checkNparts];
  zscalar_t *wtPerPart = (nVwgts ? new zscalar_t[checkNparts*nVwgts] : NULL);
  zscalar_t *globalWtPerPart = (nVwgts ? new zscalar_t[checkNparts*nVwgts] : NULL);
  for (size_t i = 0; i < checkNparts; i++) countPerPart[i] = 0;
  for (size_t i = 0; i < checkNparts * nVwgts; i++) wtPerPart[i] = 0.;

  for (size_t i = 0; i < checkLength; i++) {
    if (size_t(checkParts[i]) >= checkNparts)
      std::cout << "Invalid Part " << checkParts[i] << ": FAIL" << std::endl;
    countPerPart[checkParts[i]]++;
    for (int j = 0; j < nVwgts; j++) {
      if (j != NNZ_IDX)
        wtPerPart[checkParts[i]*nVwgts+j] += vwgts[i*nVwgts+j];
      else
        wtPerPart[checkParts[i]*nVwgts+j] += origMatrix->getNumEntriesInLocalRow(i);
    }
  }
  Teuchos::reduceAll<int, size_t>(*comm, Teuchos::REDUCE_SUM, checkNparts,
                                  countPerPart, globalCountPerPart);
  Teuchos::reduceAll<int, zscalar_t>(*comm, Teuchos::REDUCE_SUM,
                                    checkNparts*nVwgts,
                                    wtPerPart, globalWtPerPart);

  size_t min = std::numeric_limits<std::size_t>::max();
  size_t max = 0;
  size_t sum = 0;
  size_t minrank = 0, maxrank = 0;
  for (size_t i = 0; i < checkNparts; i++) {
    if (globalCountPerPart[i] < min) {min = globalCountPerPart[i]; minrank = i;}
    if (globalCountPerPart[i] > max) {max = globalCountPerPart[i]; maxrank = i;}
    sum += globalCountPerPart[i];
  }

  if (me == 0) {
    float avg = (float) sum / (float) checkNparts;
    std::cout << "Minimum count:  " << min << " on rank " << minrank << std::endl;
    std::cout << "Maximum count:  " << max << " on rank " << maxrank << std::endl;
    std::cout << "Average count:  " << avg << std::endl;
    std::cout << "Total count:    " << sum
         << (sum != origMatrix->getGlobalNumRows()
                 ? "Work was lost; FAIL"
                 : " ")
         << std::endl;
    std::cout << "Imbalance:     " << max / avg << std::endl;
    if (nVwgts) {
      std::vector<zscalar_t> minwt(nVwgts, std::numeric_limits<zscalar_t>::max());
      std::vector<zscalar_t> maxwt(nVwgts, 0.);
      std::vector<zscalar_t> sumwt(nVwgts, 0.);
      for (size_t i = 0; i < checkNparts; i++) {
        for (int j = 0; j < nVwgts; j++) {
          size_t idx = i*nVwgts+j;
          if (globalWtPerPart[idx] < minwt[j]) minwt[j] = globalWtPerPart[idx];
          if (globalWtPerPart[idx] > maxwt[j]) maxwt[j] = globalWtPerPart[idx];
          sumwt[j] += globalWtPerPart[idx];
        }
      }
      for (int j = 0; j < nVwgts; j++) {
        float avgwt = (float) sumwt[j] / (float) checkNparts;
        std::cout << std::endl;
        std::cout << "Minimum weight[" << j << "]:  " << minwt[j] << std::endl;
        std::cout << "Maximum weight[" << j << "]:  " << maxwt[j] << std::endl;
        std::cout << "Average weight[" << j << "]:  " << avgwt << std::endl;
        std::cout << "Imbalance:       " << maxwt[j] / avgwt << std::endl;
      }
    }
  }

  delete [] countPerPart;
  delete [] wtPerPart;
  delete [] globalCountPerPart;
  delete [] globalWtPerPart;
  delete [] vwgts;

  ////// Redistribute matrix and vector into new matrix and vector.
  if (me == 0) std::cout << "Redistributing matrix..." << std::endl;
  SparseMatrix *redistribMatrix;
  SparseMatrixAdapter adapterMatrix(origMatrix);
  adapterMatrix.applyPartitioningSolution(*origMatrix, redistribMatrix,
                                          problem.getSolution());
  if (redistribMatrix->getGlobalNumRows() < 40) {
    Teuchos::FancyOStream out(Teuchos::rcp(&std::cout,false));
    redistribMatrix->describe(out, Teuchos::VERB_EXTREME);
  }

  if (me == 0) std::cout << "Redistributing vectors..." << std::endl;
  VectorT *redistribVector;
  MultiVectorAdapter adapterVector(origVector); //, weights, weightStrides);
  adapterVector.applyPartitioningSolution(*origVector, redistribVector,
                                          problem.getSolution());

  RCP<VectorT> redistribProd;
  redistribProd = Tpetra::createVector<z2TestScalar,z2TestLO,z2TestGO>(
                                       redistribMatrix->getRangeMap());

  // Test redistributing an integer vector with the same solution.
  // This test is mostly to make sure compilation always works.
  RCP<IntVector> origIntVec;
  IntVector *redistIntVec;
  origIntVec = Tpetra::createVector<int,z2TestLO,z2TestGO>(
                                        origMatrix->getRangeMap());
  for (size_t i = 0; i < origIntVec->getLocalLength(); i++)
    origIntVec->replaceLocalValue(i, me);

  IntVectorAdapter int_vec_adapter(origIntVec);
  int_vec_adapter.applyPartitioningSolution(*origIntVec, redistIntVec,
                                             problem.getSolution());
  int origIntNorm = origIntVec->norm1();
  int redistIntNorm = redistIntVec->norm1();
  if (me == 0) std::cout << "IntegerVectorTest:  " << origIntNorm << " == "
                         << redistIntNorm << " ?";
  if (origIntNorm != redistIntNorm) {
    if (me == 0) std::cout << " FAIL" << std::endl;
    haveFailure = true;
  }
  else if (me == 0) std::cout << " OK" << std::endl;
  delete redistIntVec;

  ////// Verify that redistribution is "correct"; perform matvec with
  ////// original and redistributed matrices/vectors and compare norms.

  if (me == 0) std::cout << "Matvec original..." << std::endl;
  origMatrix->apply(*origVector, *origProd);
  z2TestScalar origNorm = origProd->norm2();
  if (me == 0)
    std::cout << "Norm of Original matvec prod:       " << origNorm << std::endl;

  if (me == 0) std::cout << "Matvec redistributed..." << std::endl;
  redistribMatrix->apply(*redistribVector, *redistribProd);
  z2TestScalar redistribNorm = redistribProd->norm2();
  if (me == 0)
    std::cout << "Norm of Redistributed matvec prod:  " << redistribNorm << std::endl;

  if (redistribNorm > origNorm+epsilon || redistribNorm < origNorm-epsilon) {
    testReturn = 1;
    haveFailure = true;
  }

  delete redistribVector;
  delete redistribMatrix;

  if (me == 0) {
    if (testReturn) {
      std::cout << "Mat-Vec product changed; FAIL" << std::endl;
      haveFailure = true;
    }
    if (!haveFailure)
      std::cout << "PASS" << std::endl;
  }
 
  return testReturn;
}
